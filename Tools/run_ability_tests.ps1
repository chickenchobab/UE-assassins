<#
Runs the ability automation tests(Source/AssassinsTests) in an editor process of its own and keeps the report.

  .\Tools\run_ability_tests.ps1                                   # everything under Assassins.Abilities
  .\Tools\run_ability_tests.ps1 -Filter Assassins.Abilities.Smoke # one group; several joined with +

Close the editor first: the tests open their own map and play session. The report goes to
Saved\Automation\Reports\<time>, the log to Saved\Logs\AbilityTests-<time>.log. Exits with 0 when every test passed,
1 when one failed, 2 when no report came out.
#>
param(
	[string]$Filter = "Assassins.Abilities",
	[string]$EngineDir = "C:\Program Files\Epic Games\UE_5.7\Engine"
)

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectRoot "Assassins.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor-Cmd.exe"
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$Report = Join-Path $ProjectRoot "Saved\Automation\Reports\$Stamp"
$Log = Join-Path $ProjectRoot "Saved\Logs\AbilityTests-$Stamp.log"

# In the background the editor slows down to a few frames a second, below what the automation waits for before it
# starts. Off for this process only; the user's settings stay as they are.
$NoThrottle = "-ini:EditorSettings:[/Script/UnrealEd.EditorPerformanceSettings]:bThrottleCPUWhenNotForeground=False"

# -nocef: the editor's embedded web browser can assert in this process(CEFWebBrowserWindowRHIHelper) and the tests
# don't need it.
& $Editor $Project "-ExecCmds=Automation RunTests $Filter; Quit" "-TestExit=Automation Test Queue Empty" "-ReportExportPath=$Report" $NoThrottle -unattended -nopause -nosplash -nocef "-abslog=$Log"

Write-Host "Report: $Report"
Write-Host "Log: $Log"

$Index = Join-Path $Report "index.json"
if (-not (Test-Path $Index))
{
	Write-Host "No report came out: see the log."
	exit 2
}

$Result = Get-Content $Index -Raw | ConvertFrom-Json
Write-Host ("Succeeded {0}, with warnings {1}, failed {2}, not run {3}" -f $Result.succeeded, $Result.succeededWithWarnings, $Result.failed, $Result.notRun)
foreach ($Test in $Result.tests)
{
	if ($Test.state -ne "Success")
	{
		Write-Host ("  {0}: {1}" -f $Test.state, $Test.fullTestPath)
	}
}

if ($Result.failed -gt 0) { exit 1 }
exit 0
