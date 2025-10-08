param(
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"
$resultCsv = Join-Path $rootDir "results/results_cleaned_forPAPER.csv"
$plotScript = Join-Path $rootDir "scripts/plot_results.py"

cmake -S $rootDir -B $buildDir -DCMAKE_BUILD_TYPE=$BuildType
cmake --build $buildDir --config $BuildType

$repathExe = Join-Path $buildDir "250919repath"
if (Test-Path ("$repathExe.exe")) {
    $repathExe = "$repathExe.exe"
}

$statExe = Join-Path $buildDir "250604statisticlog"
if (Test-Path ("$statExe.exe")) {
    $statExe = "$statExe.exe"
}

& $repathExe
& $statExe

python $plotScript --input $resultCsv --output (Join-Path $rootDir "docs/figs")
