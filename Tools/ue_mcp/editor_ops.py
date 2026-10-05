"""
Editor side of the ue-editor MCP server.

This module runs inside Unreal Editor's Python (it is imported by the code server.py sends through remote execution).
Every public function returns a JSON serializable dict. Large outputs are written to files under Migration/,
only small summaries travel back to the MCP server.
"""

import contextlib
import io
import json
import os
import re
import traceback

import unreal

GA_BASE_CLASS = "/Script/GameplayAbilities.GameplayAbility"
BASELINE_FILES = ("info.json", "variables.json", "defaults.json", "listing.txt", "graph.t3d")


def _lib():
    if not hasattr(unreal, "AssassinsMigrationLibrary"):
        raise RuntimeError("unreal.AssassinsMigrationLibrary is missing. Is the AssassinsEditor module built and loaded?")
    return unreal.AssassinsMigrationLibrary


def _project_dir():
    return os.path.abspath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))


def _ga_dir(subdir="GA"):
    return os.path.join(_project_dir(), "Migration", subdir)


def _package_name(asset_path):
    return asset_path.split(".")[0]


def _find_ability_blueprints():
    # Loaded blueprints also bring their SKEL_/REINST_ classes, which aren't assets: keep "/Game/X/GA_Y.GA_Y" only.
    paths = _lib().find_blueprints_derived_from(GA_BASE_CLASS)
    return [path for path in paths if path.rsplit(".", 1)[-1] == _package_name(path).rsplit("/", 1)[-1]]


def _load_blueprint(asset_path):
    asset = unreal.load_asset(asset_path)
    if asset is None or not isinstance(asset, unreal.Blueprint):
        raise ValueError("'{0}' is not a blueprint asset.".format(asset_path))
    return asset


def _loads(text):
    data = json.loads(text)
    if isinstance(data, dict) and data.get("error"):
        raise RuntimeError(data["error"])
    return data


def _write_json(path, data):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, ensure_ascii=False, indent=1, sort_keys=True)
        f.write("\n")


def _write_text(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


# "/Script/CoreUObject.Class'/Script/GameplayAbilities.GameplayAbility'" -> "/Script/GameplayAbilities.GameplayAbility"
_TYPED_PATH = re.compile(r"/Script/[A-Za-z0-9_]+\.[A-Za-z0-9_]+'([^']*)'")
_GRAPH_GUID = re.compile(r",?GraphGuid=[0-9A-Fa-f]+")


def _clean_listing(text):
    return _GRAPH_GUID.sub("", _TYPED_PATH.sub(r"\1", text))


def _short_class_name(class_path):
    # "/Game/X/GA_Y.GA_Y_C" -> "GA_Y", "/Script/Assassins.AssassinsGameplayAbility" -> "AssassinsGameplayAbility"
    name = class_path.rsplit(".", 1)[-1] if class_path else ""
    return name[:-2] if name.endswith("_C") else name


_IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def _native_property_name(key, renames):
    """Blueprint variables may contain spaces("Montage to Play"), native properties can't: "MontageToPlay"."""
    if key in renames:
        return renames[key]
    if _IDENTIFIER.fullmatch(key):
        return key
    return "".join(word[:1].upper() + word[1:] for word in re.split(r"[^A-Za-z0-9_]+", key) if word)


def _rename_properties(defaults, renames):
    renamed = {}
    properties = {}
    for key, entry in defaults.get("properties", {}).items():
        new_key = _native_property_name(key, renames)
        if new_key != key:
            renamed[key] = new_key
        properties[new_key] = entry
    return dict(defaults, properties=properties), renamed


def _diff_defaults(baseline, after):
    before_props = baseline.get("properties", {})
    after_props = after.get("properties", {})
    changed = {}
    for key, entry in before_props.items():
        if key in after_props and after_props[key]["value"] != entry["value"]:
            changed[key] = {"baseline": entry["value"], "after": after_props[key]["value"]}
    return {
        "changed": changed,
        "missing": sorted(key for key in before_props if key not in after_props),
        "added": sorted(key for key in after_props if key not in before_props),
    }


def _is_dirty(blueprint):
    package_name = blueprint.get_outermost().get_name()
    dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    return any(package.get_name() == package_name for package in dirty)


def _discard_changes(blueprint):
    """Reloads the package from disk. Returns whether the in-memory changes are really gone."""
    try:
        result = unreal.EditorLoadingAndSavingUtils.reload_packages(
            [blueprint.get_outermost()], unreal.ReloadPackagesInteractionMode.ASSUME_POSITIVE)
        any_reloaded, message = result if isinstance(result, tuple) else (bool(result), "")
        return {"reloaded": bool(any_reloaded), "message": str(message)}
    except Exception as error:  # pylint: disable=broad-except
        return {"reloaded": False, "message": str(error)}


###############################################################################
# Tools

def status():
    return {
        "engine_version": unreal.SystemLibrary.get_engine_version(),
        "project_dir": _project_dir(),
        "migration_library": hasattr(unreal, "AssassinsMigrationLibrary"),
    }


def list_abilities():
    """Every gameplay ability blueprint with its hierarchy and referencers. Writes Migration/GA/_index.json."""
    lib = _lib()
    paths = _find_ability_blueprints()
    packages = {_package_name(path) for path in paths}

    entries = []
    for path in paths:
        info = _loads(lib.describe_blueprint(_load_blueprint(path)))
        referencers = list(lib.get_referencers(_package_name(path)))
        entries.append({
            "name": info["name"],
            "path": path,
            "generated_class": info["generated_class"],
            "parent_class": info["parent_class"],
            "native_parent_class": info["native_parent_class"],
            "is_data_only": info["is_data_only"],
            "referencers": referencers,
            "external_referencers": [ref for ref in referencers if ref not in packages],
        })

    for entry in entries:
        entry["children"] = sorted(other["name"] for other in entries if other["parent_class"] == entry["generated_class"])

    os.makedirs(_ga_dir(), exist_ok=True)
    index_path = os.path.join(_ga_dir(), "_index.json")
    _write_json(index_path, {"abilities": entries})

    return {
        "count": len(entries),
        "index": index_path,
        "abilities": [
            "{0}  <- {1}  children={2}  external_refs={3}{4}".format(
                entry["name"], _short_class_name(entry["parent_class"]), len(entry["children"]),
                len(entry["external_referencers"]), "  (data only)" if entry["is_data_only"] else "")
            for entry in entries
        ],
    }


def export_ability(asset_path, overwrite_baseline=False, subdir="GA"):
    """Writes the baseline of one blueprint(graphs, variables, effective defaults) to Migration/<subdir>/<Name>/."""
    lib = _lib()
    blueprint = _load_blueprint(asset_path)
    name = blueprint.get_name()
    out_dir = os.path.join(_ga_dir(subdir), name)

    if not overwrite_baseline and os.path.exists(os.path.join(out_dir, "defaults.json")):
        return {"name": name, "dir": out_dir, "skipped": "baseline already exists (pass overwrite_baseline to replace it)"}

    os.makedirs(out_dir, exist_ok=True)

    info = _loads(lib.describe_blueprint(blueprint))
    info["referencers"] = list(lib.get_referencers(_package_name(asset_path)))
    variables = _loads(lib.export_blueprint_variables(blueprint))
    defaults = _loads(lib.dump_class_defaults(info["generated_class"]))

    _write_json(os.path.join(out_dir, "info.json"), info)
    _write_json(os.path.join(out_dir, "variables.json"), variables)
    _write_json(os.path.join(out_dir, "defaults.json"), defaults)
    _write_text(os.path.join(out_dir, "listing.txt"), _clean_listing(lib.export_blueprint_graphs_as_listing(blueprint)))
    _write_text(os.path.join(out_dir, "graph.t3d"), lib.export_blueprint_graphs_as_text(blueprint))

    return {
        "name": name,
        "dir": out_dir,
        "files": {file_name: os.path.getsize(os.path.join(out_dir, file_name)) for file_name in BASELINE_FILES},
        "entry_points": ["{0}:{1}".format(point["kind"], point["name"]) for point in info["entry_points"]],
        "functions": [graph["name"] for graph in info["graphs"] if graph["kind"] != "Ubergraph"],
        "variables": len(variables["variables"]),
        "defaults": len(defaults["properties"]),
    }


def export_abilities(asset_paths=None, overwrite_baseline=False, subdir="GA"):
    """export_ability for several blueprints, all the abilities when asset_paths is empty."""
    if not asset_paths:
        asset_paths = _find_ability_blueprints()

    exported, skipped, failed = [], [], {}
    for asset_path in asset_paths:
        try:
            result = export_ability(asset_path, overwrite_baseline, subdir)
            (skipped if "skipped" in result else exported).append(result["name"])
        except Exception as error:  # pylint: disable=broad-except
            failed[asset_path] = str(error)

    return {"dir": _ga_dir(subdir), "exported": exported, "skipped": skipped, "failed": failed}


def dump_defaults(class_path):
    return _loads(_lib().dump_class_defaults(class_path))


def compile_blueprint(asset_path):
    return _loads(_lib().compile_blueprint(_load_blueprint(asset_path)))


def migrate_ability(asset_path, native_class, save=True, renames=None, subdir="GA"):
    """
    strip -> reparent to native_class -> restore the baseline defaults -> compile -> save -> dump -> diff.
    Baseline keys which aren't identifiers are mapped to PascalCase("Montage to Play" -> "MontageToPlay"),
    renames({"baseline name": "native name"}) overrides that mapping.
    Nothing is saved when something fails, and the in-memory changes are discarded.
    With save=False it is a dry run: the report is produced, then the changes are discarded.
    """
    lib = _lib()
    blueprint = _load_blueprint(asset_path)
    name = blueprint.get_name()
    base_dir = os.path.join(_ga_dir(subdir), name)
    baseline_path = os.path.join(base_dir, "defaults.json")
    if not os.path.exists(baseline_path):
        raise RuntimeError("No baseline for {0}. Run export_ability first.".format(name))

    # Unsaved changes would be migrated(and saved) along, and could not be discarded on failure.
    if _is_dirty(blueprint):
        raise RuntimeError("{0} has unsaved changes. Save or revert it in the editor first.".format(name))

    with open(baseline_path, encoding="utf-8") as f:
        original_baseline = json.load(f)
    baseline, renamed = _rename_properties(original_baseline, renames or {})
    baseline_text = json.dumps(baseline, ensure_ascii=False)
    generated_class = baseline["class"]

    report = {"blueprint": asset_path, "native_class": native_class, "saved": False, "renamed": renamed}
    report["strip"] = _loads(lib.strip_blueprint(blueprint))
    report["reparent"] = _loads(lib.reparent_blueprint(blueprint, native_class))

    ok = report["reparent"].get("num_errors", 0) == 0
    if ok:
        report["apply"] = _loads(lib.apply_class_defaults(blueprint, baseline_text))
        report["compile"] = _loads(lib.compile_blueprint(blueprint))
        ok = report["compile"].get("num_errors", 0) == 0 and not report["apply"]["failed"]

    if ok:
        after = _loads(lib.dump_class_defaults(generated_class))
        report["diff"] = _diff_defaults(baseline, after)
        if save:
            report["saved"] = bool(unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False))
            _write_json(os.path.join(base_dir, "after_defaults.json"), after)

    if not report["saved"]:
        report["discard"] = _discard_changes(blueprint)

    _write_json(os.path.join(base_dir, "migration.json" if save else "migration_dry_run.json"), report)

    apply_report = report.get("apply", {})
    diff = report.get("diff", {})
    return {
        "name": name,
        "ok": ok,
        "saved": report["saved"],
        "discarded": report.get("discard", {}).get("reloaded"),
        "report": os.path.join(base_dir, "migration.json" if save else "migration_dry_run.json"),
        "compile": report.get("compile", report["reparent"]),
        "renamed": renamed,
        "applied": len(apply_report.get("applied", [])),
        "missing": apply_report.get("missing", []),
        "failed": apply_report.get("failed", []),
        "mismatched": apply_report.get("mismatched", []),
        "skipped_edit_const": apply_report.get("skipped_edit_const", []),
        "diff_changed": diff.get("changed", {}),
        "diff_missing": diff.get("missing", []),
    }


def _blueprint_asset_path(package_name):
    """"/Game/X/B_Y" -> "/Game/X/B_Y.B_Y" when the package holds a blueprint, else None."""
    asset_path = "{0}.{1}".format(package_name, package_name.rsplit("/", 1)[-1])
    asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path)
    if not asset_data or not asset_data.is_valid():
        return None
    return asset_path if str(asset_data.asset_class_path.asset_name) == "Blueprint" else None


def _value_text(value):
    # Objects print with their address, which changes every session.
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    return str(value)


_QUOTED_PATH = re.compile(r"'([^']+)'")


def _component_templates(class_path, defaults):
    """
    Property values of the default subobjects(components) of an actor class, keyed by the property holding them.
    A child blueprint only stores what differs from its parent in these, so they have to be compared too when a parent changes.
    """
    cdo = unreal.get_default_object(unreal.load_class(None, class_path))
    if not isinstance(cdo, unreal.Actor):
        return {}

    prefix = cdo.get_path_name() + ":"
    components = {}
    for key, entry in defaults.items():
        match = _QUOTED_PATH.search(entry["value"])
        if not match or not match.group(1).startswith(prefix):
            continue
        template = unreal.find_object(None, match.group(1))
        if template is not None:
            components[key] = _loads(_lib().dump_object_properties(template))["properties"]
    return components


def _find_derived_blueprints(class_path):
    paths = _lib().find_blueprints_derived_from(class_path)
    return [path for path in paths if path.rsplit(".", 1)[-1] == _package_name(path).rsplit("/", 1)[-1]]


def snapshot(asset_paths, name, instance_classes=None, subdir="Deps"):
    """
    Records the given blueprints, all their descendants(effective defaults and compile status) and the
    status of the blueprints referencing them, to Migration/<subdir>/_snapshot_<name>.json.
    instance_classes({class path: [property names]}) also records the values of the assets of these classes.
    Run it before and after changing native parents, then diff_snapshots.
    """
    lib = _lib()

    targets = set(asset_paths)
    for asset_path in asset_paths:
        info = _loads(lib.describe_blueprint(_load_blueprint(asset_path)))
        targets.update(_find_derived_blueprints(info["generated_class"]))

    referencers = set()
    for asset_path in targets:
        for package_name in lib.get_referencers(_package_name(asset_path)):
            referencer = _blueprint_asset_path(package_name)
            if referencer and referencer not in targets:
                referencers.add(referencer)

    blueprints = {}
    for asset_path in sorted(targets):
        info = _loads(lib.describe_blueprint(_load_blueprint(asset_path)))
        defaults = _loads(lib.dump_class_defaults(info["generated_class"]))["properties"]
        blueprints[asset_path] = {
            "parent_class": info["parent_class"],
            "status": info["status"],
            "variables": info["variables"],
            "defaults": defaults,
            "components": _component_templates(info["generated_class"], defaults),
        }

    referencing = {}
    for asset_path in sorted(referencers):
        info = _loads(lib.describe_blueprint(_load_blueprint(asset_path)))
        referencing[asset_path] = {"status": info["status"]}

    instances = {}
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for class_path, property_names in (instance_classes or {}).items():
        package_name, class_name = class_path.split(".")
        for asset_data in registry.get_assets_by_class(unreal.TopLevelAssetPath(package_name, class_name)):
            asset = asset_data.get_asset()
            instances[asset.get_path_name()] = {prop: _value_text(asset.get_editor_property(prop)) for prop in property_names}

    out_dir = _ga_dir(subdir)
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "_snapshot_{0}.json".format(name))
    _write_json(out_path, {"blueprints": blueprints, "referencing": referencing, "instances": instances})

    return {
        "file": out_path,
        "blueprints": len(blueprints),
        "referencing": len(referencing),
        "instances": len(instances),
        "not_up_to_date": sorted(path for path, entry in list(blueprints.items()) + list(referencing.items())
                                 if entry["status"] not in ("UpToDate", "UpToDateWithWarnings")),
    }


def diff_snapshots(before="before", after="after", subdir="Deps"):
    """Compares two snapshots: default values, parents, compile status, removed variables and instance values."""
    out_dir = _ga_dir(subdir)
    with open(os.path.join(out_dir, "_snapshot_{0}.json".format(before)), encoding="utf-8") as f:
        old = json.load(f)
    with open(os.path.join(out_dir, "_snapshot_{0}.json".format(after)), encoding="utf-8") as f:
        new = json.load(f)

    report = {"parent_changed": {}, "status_changed": {}, "variables_removed": {}, "defaults_changed": {},
              "defaults_missing": {}, "instances_changed": {}, "missing_blueprints": []}

    for path, old_entry in old["blueprints"].items():
        new_entry = new["blueprints"].get(path)
        if new_entry is None:
            report["missing_blueprints"].append(path)
            continue
        name = path.rsplit(".", 1)[-1]
        if old_entry["parent_class"] != new_entry["parent_class"]:
            report["parent_changed"][name] = [old_entry["parent_class"], new_entry["parent_class"]]
        if old_entry["status"] != new_entry["status"]:
            report["status_changed"][name] = [old_entry["status"], new_entry["status"]]
        removed = sorted(set(old_entry["variables"]) - set(new_entry["variables"]))
        if removed:
            report["variables_removed"][name] = removed
        changed = {key: [value["value"], new_entry["defaults"][key]["value"]]
                   for key, value in old_entry["defaults"].items()
                   if key in new_entry["defaults"] and new_entry["defaults"][key]["value"] != value["value"]}
        if changed:
            report["defaults_changed"][name] = changed
        # Not dumped anymore, usually because the native property that replaced it isn't editable. Baseline value shown.
        missing = {key: value["value"] for key, value in old_entry["defaults"].items() if key not in new_entry["defaults"]}
        if missing:
            report["defaults_missing"][name] = missing
        for component, old_props in old_entry.get("components", {}).items():
            new_props = new_entry.get("components", {}).get(component)
            if new_props is None:
                report.setdefault("components_missing", {}).setdefault(name, []).append(component)
                continue
            changed = {key: [value["value"], new_props[key]["value"]]
                       for key, value in old_props.items() if key in new_props and new_props[key]["value"] != value["value"]}
            if changed:
                report.setdefault("components_changed", {}).setdefault(name, {})[component] = changed

    for path, old_entry in old["referencing"].items():
        new_entry = new["referencing"].get(path)
        if new_entry and new_entry["status"] != old_entry["status"]:
            report["status_changed"][path.rsplit(".", 1)[-1]] = [old_entry["status"], new_entry["status"]]

    for path, old_values in old["instances"].items():
        new_values = new["instances"].get(path)
        if new_values != old_values:
            report["instances_changed"][path.rsplit(".", 1)[-1]] = [old_values, new_values]

    _write_json(os.path.join(out_dir, "_diff_{0}_{1}.json".format(before, after)), report)
    return report


def rebase_blueprint(asset_path, new_parent_class="", remove_graphs=None, save=False):
    """unreal.AssassinsMigrationLibrary.rebase_blueprint, optionally saving the blueprint when it compiled without errors."""
    blueprint = _load_blueprint(asset_path)
    report = _loads(_lib().rebase_blueprint(blueprint, new_parent_class, remove_graphs or []))
    if save and report.get("num_errors", 0) == 0:
        report["saved"] = bool(unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False))
    return report


def replace_variable(asset_path, old_name, new_name, save=False):
    """unreal.AssassinsMigrationLibrary.replace_variable, optionally saving the blueprint when it compiled without errors."""
    blueprint = _load_blueprint(asset_path)
    report = _loads(_lib().replace_variable(blueprint, old_name, new_name))
    if save and report.get("num_errors", 0) == 0:
        report["saved"] = bool(unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False))
    return report


def fix_orphaned_pins(asset_path, save=False):
    """unreal.AssassinsMigrationLibrary.fix_orphaned_pins, optionally saving the blueprint when it compiled without errors."""
    blueprint = _load_blueprint(asset_path)
    report = _loads(_lib().fix_orphaned_pins(blueprint))
    if save and report.get("num_errors", 0) == 0:
        report["saved"] = bool(unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False))
    return report


def replace_pin_default_object(asset_path, old_object_path, new_object_path, save=False):
    """unreal.AssassinsMigrationLibrary.replace_pin_default_object, optionally saving the blueprint when it compiled without errors."""
    blueprint = _load_blueprint(asset_path)
    report = _loads(_lib().replace_pin_default_object(blueprint, old_object_path, new_object_path))
    if save and report.get("num_errors", 0) == 0:
        report["saved"] = bool(unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False))
    return report


def set_object_property(object_path, property_name, value, save=False):
    """unreal.AssassinsMigrationLibrary.set_object_property on a loaded asset(or a subobject path), optionally saving its package."""
    obj = unreal.load_object(None, object_path)
    if obj is None:
        raise ValueError("Failed to load '{0}'.".format(object_path))
    report = _loads(_lib().set_object_property(obj, property_name, value))
    if save and "error" not in report:
        report["saved"] = bool(unreal.EditorAssetLibrary.save_asset(obj.get_outermost().get_name(), False))
    return report


def compile_blueprints(asset_paths):
    """Compiles the blueprints, returns the ones with errors or warnings and a count of the others."""
    lib = _lib()
    problems, clean = {}, 0
    for asset_path in asset_paths:
        result = _loads(lib.compile_blueprint(_load_blueprint(asset_path)))
        if result.get("num_errors", 0) or result.get("num_warnings", 0):
            problems[asset_path] = {key: result[key] for key in ("status", "num_errors", "num_warnings", "messages")}
        else:
            clean += 1
    return {"clean": clean, "problems": problems}


def save_assets(asset_paths):
    saved, failed = [], []
    for asset_path in asset_paths:
        asset = unreal.load_asset(asset_path)
        (saved if asset and unreal.EditorAssetLibrary.save_loaded_asset(asset, False) else failed).append(asset_path)
    return {"saved": saved, "failed": failed}


def run_python(code):
    """Escape hatch. `unreal`, `lib` and `json` are available; assign to `result` to return a value."""
    namespace = {"unreal": unreal, "lib": _lib(), "json": json}
    stdout = io.StringIO()
    with contextlib.redirect_stdout(stdout):
        exec(code, namespace)  # pylint: disable=exec-used
    result = namespace.get("result")
    try:
        json.dumps(result)
    except TypeError:
        result = str(result)
    return {"stdout": stdout.getvalue()[-20000:], "result": result}


###############################################################################
# Entry point used by server.py

_PUBLIC = {
    "status": status,
    "list_abilities": list_abilities,
    "export_ability": export_ability,
    "export_abilities": export_abilities,
    "dump_defaults": dump_defaults,
    "compile_blueprint": compile_blueprint,
    "migrate_ability": migrate_ability,
    "snapshot": snapshot,
    "diff_snapshots": diff_snapshots,
    "rebase_blueprint": rebase_blueprint,
    "replace_variable": replace_variable,
    "fix_orphaned_pins": fix_orphaned_pins,
    "replace_pin_default_object": replace_pin_default_object,
    "set_object_property": set_object_property,
    "compile_blueprints": compile_blueprints,
    "save_assets": save_assets,
    "run_python": run_python,
}


def _run(function_name, arguments_json, result_path):
    try:
        arguments = json.loads(arguments_json) if arguments_json else {}
        result = {"ok": True, "result": _PUBLIC[function_name](**arguments)}
    except Exception as error:  # pylint: disable=broad-except
        result = {"ok": False, "error": "{0}: {1}".format(type(error).__name__, error), "traceback": traceback.format_exc()}

    with open(result_path, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False)
