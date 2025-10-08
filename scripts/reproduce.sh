#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULT_CSV="${ROOT_DIR}/results/results_cleaned_forPAPER.csv"
PLOT_SCRIPT="${ROOT_DIR}/scripts/plot_results.py"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release

REPATH_EXE="${BUILD_DIR}/250919repath"
STAT_EXE="${BUILD_DIR}/250604statisticlog"

if [[ -x "${REPATH_EXE}" ]]; then
    "${REPATH_EXE}"
elif [[ -x "${REPATH_EXE}.exe" ]]; then
    "${REPATH_EXE}.exe"
else
    echo "Repath executable not found" >&2
    exit 1
fi

if [[ -x "${STAT_EXE}" ]]; then
    "${STAT_EXE}"
elif [[ -x "${STAT_EXE}.exe" ]]; then
    "${STAT_EXE}.exe"
else
    echo "Statistic executable not found" >&2
    exit 1
fi

python "${PLOT_SCRIPT}" --input "${RESULT_CSV}" --output "${ROOT_DIR}/docs/figs"
