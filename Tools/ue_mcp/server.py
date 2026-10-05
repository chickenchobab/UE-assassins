"""
ue-editor MCP server.

Bridges Claude Code and a running Unreal Editor through the Python remote execution protocol shipped with the engine
(Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py).
The work happens inside the editor (editor_ops.py -> unreal.AssassinsMigrationLibrary); this process forwards the
tool calls and reads the results back from files, so large outputs never go through the remote execution socket.

Only the standard library is used: the Python bundled with the engine is enough.

Usage
    python server.py                         MCP server over stdio
    python server.py tools                   list the tools
    python server.py wait [seconds]          wait until the editor answers
    python server.py call <tool> [json]      call a tool from the command line

Editor requirements
    - AssassinsEditor module built (it hosts unreal.AssassinsMigrationLibrary)
    - PythonScriptPlugin enabled, Project Settings > Python > Enable Remote Execution
"""

import json
import os
import socket
import sys
import time
import traceback
import uuid

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.abspath(os.path.join(TOOLS_DIR, "..", ".."))
PROJECT_NAME = "Assassins"
ENGINE_DIR = os.environ.get("UE_ENGINE_DIR", r"C:\Program Files\Epic Games\UE_5.7\Engine")
RESULT_DIR = os.path.join(PROJECT_DIR, "Saved", "ue_mcp")

SERVER_NAME = "ue-editor"
SERVER_VERSION = "0.1.0"

sys.path.insert(0, os.path.join(ENGINE_DIR, "Plugins", "Experimental", "PythonScriptPlugin", "Content", "Python"))
import remote_execution as rex  # noqa: E402  pylint: disable=wrong-import-position


def _log(message):
    sys.stderr.write("[{0}] {1}\n".format(SERVER_NAME, message))
    sys.stderr.flush()


def _receive_message(self, expected_type):
    """
    Replacement for _RemoteExecutionCommandConnection._receive_message.
    The stock version stops reading as soon as a recv returns less than its buffer size, which truncates large
    responses. This one keeps reading until the payload parses as JSON.
    """
    data = b""
    while True:
        part = self._command_channel_socket.recv(rex.DEFAULT_RECEIVE_BUFFER_SIZE)
        if not part:
            break
        data += part
        try:
            json.loads(data.decode("utf-8"))
        except ValueError:
            continue
        break

    if data:
        message = rex._RemoteExecutionMessage(None, None)  # pylint: disable=protected-access
        if message.from_json_bytes(data) and message.passes_receive_filter(self._node_id) and message.type_ == expected_type:
            return message
    raise RuntimeError("Remote party failed to send a valid response!")


rex._RemoteExecutionCommandConnection._receive_message = _receive_message  # pylint: disable=protected-access


class EditorNotFound(RuntimeError):
    pass


class EditorConnection(object):
    """Lazily discovers the editor of this project and keeps one command connection open."""

    def __init__(self):
        self._remote = None
        self.node = None

    @staticmethod
    def _free_port():
        # Every client hosts its own TCP command socket, a fixed port would clash with a second client.
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            probe.bind(("127.0.0.1", 0))
            return probe.getsockname()[1]

    def _ensure_started(self):
        if self._remote is None:
            config = rex.RemoteExecutionConfig()
            config.command_endpoint = ("127.0.0.1", self._free_port())
            self._remote = rex.RemoteExecution(config)
            self._remote.start()

    def _remote_nodes(self):
        nodes = self._remote.remote_nodes
        return nodes() if callable(nodes) else nodes

    def find_node(self, timeout):
        self._ensure_started()
        deadline = time.time() + timeout
        while True:
            nodes = self._remote_nodes()
            for node in nodes:
                if node.get("project_name", "").lower() == PROJECT_NAME.lower():
                    return node
            if time.time() >= deadline:
                return None
            time.sleep(0.25)

    def connect(self, timeout=5.0):
        if self._remote is not None and self._remote.has_command_connection():
            return
        node = self.find_node(timeout)
        if node is None:
            raise EditorNotFound(
                "No Unreal Editor running project '{0}' answered. Start the editor and make sure "
                "Project Settings > Python > Enable Remote Execution is on.".format(PROJECT_NAME))
        self._remote.open_command_connection(node["node_id"])
        self.node = node

    def reset(self):
        if self._remote is not None:
            try:
                self._remote.stop()
            except Exception:  # pylint: disable=broad-except
                pass
        self._remote = None
        self.node = None

    def run(self, code):
        for attempt in range(2):
            try:
                self.connect()
                return self._remote.run_command(code, unattended=True, exec_mode=rex.MODE_EXEC_FILE)
            except EditorNotFound:
                raise
            except Exception:  # pylint: disable=broad-except
                self.reset()
                if attempt == 1:
                    raise
        return None


CONNECTION = EditorConnection()


def call_editor(function_name, arguments):
    """Runs editor_ops.<function_name>(**arguments) inside the editor and returns its result."""
    os.makedirs(RESULT_DIR, exist_ok=True)
    result_path = os.path.join(RESULT_DIR, "{0}.json".format(uuid.uuid4().hex))
    code = "\n".join([
        "import sys, importlib",
        "_tools_dir = {0!r}".format(TOOLS_DIR),
        "if _tools_dir not in sys.path: sys.path.insert(0, _tools_dir)",
        "import editor_ops",
        "importlib.reload(editor_ops)",
        "editor_ops._run({0!r}, {1!r}, {2!r})".format(function_name, json.dumps(arguments), result_path),
    ])

    response = CONNECTION.run(code)
    if not os.path.exists(result_path):
        output = "\n".join(entry.get("output", "") for entry in (response or {}).get("output", []))
        raise RuntimeError("The editor did not produce a result.\n{0}\n{1}".format((response or {}).get("result", ""), output[-4000:]))

    with open(result_path, encoding="utf-8") as f:
        result = json.load(f)
    os.remove(result_path)

    if not result.get("ok"):
        raise RuntimeError("{0}\n{1}".format(result.get("error"), result.get("traceback", "")))
    return result["result"]


###############################################################################
# Tools

def tool_ue_status(_arguments):
    try:
        CONNECTION.connect(timeout=3.0)
    except EditorNotFound as error:
        return {"connected": False, "reason": str(error)}
    status = call_editor("status", {})
    status.update({"connected": True, "node": CONNECTION.node})
    return status


def _editor_tool(function_name):
    return lambda arguments: call_editor(function_name, arguments)


TOOLS = {
    "ue_status": {
        "handler": tool_ue_status,
        "description": "Checks whether the Unreal Editor of the Assassins project is reachable and the migration library is loaded.",
        "schema": {"type": "object", "properties": {}},
    },
    "list_abilities": {
        "handler": _editor_tool("list_abilities"),
        "description": "Lists every gameplay ability blueprint with its parent, children and external referencers. "
                       "Writes Migration/GA/_index.json.",
        "schema": {"type": "object", "properties": {}},
    },
    "export_ability": {
        "handler": _editor_tool("export_abilities"),
        "description": "Writes the baseline of blueprints to Migration/<subdir>/<Name>/: listing.txt (graphs, readable), "
                       "graph.t3d (graphs, full fidelity), variables.json, defaults.json (effective class defaults), info.json. "
                       "An existing baseline is kept unless overwrite_baseline is true. Leave asset_paths empty to export all abilities.",
        "schema": {
            "type": "object",
            "properties": {
                "asset_paths": {"type": "array", "items": {"type": "string"},
                                "description": "Object paths such as /Game/Characters/Abilities/Foundation/GA_UnitTargeted.GA_UnitTargeted"},
                "overwrite_baseline": {"type": "boolean", "default": False},
                "subdir": {"type": "string", "default": "GA",
                           "description": "Folder under Migration/. Use \"Deps\" for the blueprints the abilities depend on."},
            },
        },
    },
    "dump_defaults": {
        "handler": _editor_tool("dump_defaults"),
        "description": "Returns the effective class default values of a class, native (/Script/Assassins.GA_X) or blueprint (/Game/..../GA_X.GA_X_C).",
        "schema": {"type": "object", "properties": {"class_path": {"type": "string"}}, "required": ["class_path"]},
    },
    "compile_blueprint": {
        "handler": _editor_tool("compile_blueprint"),
        "description": "Compiles a blueprint and returns the errors and warnings.",
        "schema": {"type": "object", "properties": {"asset_path": {"type": "string"}}, "required": ["asset_path"]},
    },
    "migrate_ability": {
        "handler": _editor_tool("migrate_ability"),
        "description": "Turns a gameplay ability blueprint into a data-only child of a native class: removes graphs/variables/interfaces, "
                       "reparents to native_class, restores the baseline defaults by name, compiles, saves, and diffs the defaults "
                       "against the baseline. Requires a baseline (export_ability). Nothing is saved on failure. save=false is a dry run.",
        "schema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "native_class": {"type": "string", "description": "e.g. /Script/Assassins.GA_UnitTargeted"},
                "save": {"type": "boolean", "default": True},
                "renames": {"type": "object", "additionalProperties": {"type": "string"},
                            "description": "Baseline property name -> native property name. Names with spaces are mapped to "
                                           "PascalCase automatically (\"Montage to Play\" -> \"MontageToPlay\")."},
                "subdir": {"type": "string", "default": "GA", "description": "Folder under Migration/ holding the baseline."},
            },
            "required": ["asset_path", "native_class"],
        },
    },
    "snapshot": {
        "handler": _editor_tool("snapshot"),
        "description": "Records blueprints, all their descendants (effective class defaults, compile status) and the status of the "
                       "blueprints referencing them to Migration/<subdir>/_snapshot_<name>.json. instance_classes "
                       "({\"/Game/X/DA_Y.DA_Y_C\": [\"PropA\"]}) also records property values of the assets of those classes. "
                       "Take one before changing a native parent and one after, then compare them.",
        "schema": {
            "type": "object",
            "properties": {
                "asset_paths": {"type": "array", "items": {"type": "string"}},
                "name": {"type": "string", "description": "e.g. before / after"},
                "instance_classes": {"type": "object", "additionalProperties": {"type": "array", "items": {"type": "string"}}},
                "subdir": {"type": "string", "default": "Deps"},
            },
            "required": ["asset_paths", "name"],
        },
    },
    "diff_snapshots": {
        "handler": _editor_tool("diff_snapshots"),
        "description": "Compares two snapshots (parents, compile status, removed variables, default values, instance values). "
                       "Writes Migration/<subdir>/_diff_<before>_<after>.json.",
        "schema": {
            "type": "object",
            "properties": {
                "before": {"type": "string", "default": "before"},
                "after": {"type": "string", "default": "after"},
                "subdir": {"type": "string", "default": "Deps"},
            },
        },
    },
    "rebase_blueprint": {
        "handler": _editor_tool("rebase_blueprint"),
        "description": "Moves blueprint members to a native parent while keeping the blueprint logic: removes remove_graphs, reparents to "
                       "new_parent_class (optional), turns custom events the native parent declares as BlueprintImplementableEvent into "
                       "overrides (links kept), compiles. The compiler drops variables the native parent declares with the same name and "
                       "property type and points their references to the native ones. Saves only when save is true and there are no errors.",
        "schema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "new_parent_class": {"type": "string", "default": ""},
                "remove_graphs": {"type": "array", "items": {"type": "string"}},
                "save": {"type": "boolean", "default": False},
            },
            "required": ["asset_path"],
        },
    },
    "replace_variable": {
        "handler": _editor_tool("replace_variable"),
        "description": "Points the references to old_name (in the blueprint and the loaded blueprints depending on it) to new_name and "
                       "removes the variable old_name. Fixes the X_0 variables the compiler creates when it can't match a blueprint "
                       "variable with the native one of the same name. Saves only when save is true and there are no errors.",
        "schema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "old_name": {"type": "string"},
                "new_name": {"type": "string"},
                "save": {"type": "boolean", "default": False},
            },
            "required": ["asset_path", "old_name", "new_name"],
        },
    },
    "fix_orphaned_pins": {
        "handler": _editor_tool("fix_orphaned_pins"),
        "description": "Moves the links of orphaned pins to the live pin of the same name on the same node when the schema accepts the "
                       "connection, removes the orphans left without links, compiles. Use it after a variable's type became one of its "
                       "parent classes (\"In use pin X no longer exists on node\"). Saves only when save is true and there are no errors.",
        "schema": {
            "type": "object",
            "properties": {"asset_path": {"type": "string"}, "save": {"type": "boolean", "default": False}},
            "required": ["asset_path"],
        },
    },
    "replace_pin_default_object": {
        "handler": _editor_tool("replace_pin_default_object"),
        "description": "Points the pin default values set to old_object_path (e.g. the class of an IsA node) to new_object_path, then "
                       "compiles. Saves only when save is true and there are no errors.",
        "schema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "old_object_path": {"type": "string"},
                "new_object_path": {"type": "string"},
                "save": {"type": "boolean", "default": False},
            },
            "required": ["asset_path", "old_object_path", "new_object_path"],
        },
    },
    "set_object_property": {
        "handler": _editor_tool("set_object_property"),
        "description": "Sets a property of an object (asset or subobject path) from its text form, with the editor's change notifications. "
                       "Reaches properties Python can't, e.g. of struct types not exposed to Blueprint. Returns the value before and after.",
        "schema": {
            "type": "object",
            "properties": {
                "object_path": {"type": "string"},
                "property_name": {"type": "string", "description": "Native name, e.g. ComponentList"},
                "value": {"type": "string"},
                "save": {"type": "boolean", "default": False},
            },
            "required": ["object_path", "property_name", "value"],
        },
    },
    "compile_blueprints": {
        "handler": _editor_tool("compile_blueprints"),
        "description": "Compiles blueprints and returns the ones with errors or warnings.",
        "schema": {"type": "object", "properties": {"asset_paths": {"type": "array", "items": {"type": "string"}}}, "required": ["asset_paths"]},
    },
    "save_assets": {
        "handler": _editor_tool("save_assets"),
        "description": "Saves loaded assets.",
        "schema": {"type": "object", "properties": {"asset_paths": {"type": "array", "items": {"type": "string"}}}, "required": ["asset_paths"]},
    },
    "run_python": {
        "handler": _editor_tool("run_python"),
        "description": "Runs Python inside the editor. `unreal`, `lib` (AssassinsMigrationLibrary) and `json` are available; "
                       "assign to `result` to return a value. Printed output is returned too.",
        "schema": {"type": "object", "properties": {"code": {"type": "string"}}, "required": ["code"]},
    },
}


def call_tool(name, arguments):
    if name not in TOOLS:
        raise KeyError("Unknown tool '{0}'".format(name))
    return TOOLS[name]["handler"](arguments or {})


###############################################################################
# MCP over stdio (newline delimited JSON-RPC 2.0)

def _send(message):
    sys.stdout.buffer.write((json.dumps(message, ensure_ascii=False) + "\n").encode("utf-8"))
    sys.stdout.buffer.flush()


def _handle(message):
    method = message.get("method")
    params = message.get("params") or {}

    if method == "initialize":
        return {
            "protocolVersion": params.get("protocolVersion", "2025-06-18"),
            "capabilities": {"tools": {"listChanged": False}},
            "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
            "instructions": "Drives the Unreal Editor of the Assassins project for the blueprint to native gameplay ability migration. "
                            "Call ue_status first; the editor must be running.",
        }

    if method == "ping":
        return {}

    if method == "tools/list":
        return {"tools": [{"name": name, "description": tool["description"], "inputSchema": tool["schema"]}
                          for name, tool in TOOLS.items()]}

    if method == "tools/call":
        try:
            result = call_tool(params.get("name"), params.get("arguments"))
            return {"content": [{"type": "text", "text": json.dumps(result, ensure_ascii=False, indent=1)}], "isError": False}
        except Exception as error:  # pylint: disable=broad-except
            _log(traceback.format_exc())
            return {"content": [{"type": "text", "text": "{0}: {1}".format(type(error).__name__, error)}], "isError": True}

    raise LookupError(method)


def serve():
    _log("started, project dir {0}".format(PROJECT_DIR))
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            break
        line = line.strip()
        if not line:
            continue

        try:
            message = json.loads(line.decode("utf-8"))
        except ValueError:
            _send({"jsonrpc": "2.0", "id": None, "error": {"code": -32700, "message": "Parse error"}})
            continue

        # Notifications (initialized, cancelled, ...) need no answer.
        if "id" not in message:
            continue

        try:
            _send({"jsonrpc": "2.0", "id": message["id"], "result": _handle(message)})
        except LookupError as error:
            _send({"jsonrpc": "2.0", "id": message["id"], "error": {"code": -32601, "message": "Method not found: {0}".format(error)}})
        except Exception as error:  # pylint: disable=broad-except
            _log(traceback.format_exc())
            _send({"jsonrpc": "2.0", "id": message["id"], "error": {"code": -32603, "message": str(error)}})

    CONNECTION.reset()


def main(argv):
    if len(argv) <= 1:
        serve()
        return 0

    command = argv[1]
    try:
        if command == "tools":
            for name, tool in TOOLS.items():
                print("{0}: {1}".format(name, tool["description"]))
            return 0

        if command == "wait":
            timeout = float(argv[2]) if len(argv) > 2 else 300.0
            node = CONNECTION.find_node(timeout)
            print(json.dumps({"found": node is not None, "node": node}, ensure_ascii=False, indent=1))
            return 0 if node else 1

        if command == "call":
            arguments = json.loads(argv[3]) if len(argv) > 3 else {}
            print(json.dumps(call_tool(argv[2], arguments), ensure_ascii=False, indent=1))
            return 0
    finally:
        CONNECTION.reset()

    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
