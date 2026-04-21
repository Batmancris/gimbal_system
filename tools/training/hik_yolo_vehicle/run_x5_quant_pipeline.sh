#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-/home/demo/tianbot_ws}"
MODEL_PATH="${MODEL_PATH:-$ROOT_DIR/best.onnx}"
CFG_PATH="${1:-${CFG_PATH:-$ROOT_DIR/x5_quant_best_nv12.yaml}}"
VENV_DIR="${VENV_DIR:-$ROOT_DIR/.venv_oe_py310}"
HB_MAPPER_BIN="${HB_MAPPER_BIN:-$VENV_DIR/bin/hb_mapper}"

if [[ ! -f "$MODEL_PATH" ]]; then
  echo "ONNX model not found: $MODEL_PATH"
  exit 1
fi

if [[ ! -f "$CFG_PATH" ]]; then
  echo "Quant config not found: $CFG_PATH"
  exit 1
fi

if [[ ! -x "$HB_MAPPER_BIN" ]]; then
  echo "hb_mapper not found or not executable: $HB_MAPPER_BIN"
  exit 1
fi

export HOME="${HOME:-$ROOT_DIR}"
export MPLCONFIGDIR="${MPLCONFIGDIR:-/tmp/matplotlib-x5}"
export HORIZON_LIB_PATH="${HORIZON_LIB_PATH:-$ROOT_DIR/.horizon}"
export DDK_LIB_PATH="${DDK_LIB_PATH:-$ROOT_DIR/.horizon/ddk}"
export X5_X86_GCC1140_PATH="${X5_X86_GCC1140_PATH:-$ROOT_DIR/.horizon/ddk/x5_x86_64_gcc_11.4.0}"
export HB_DNN_SIM_PLATFORM="${HB_DNN_SIM_PLATFORM:-BAYESE}"
export LD_LIBRARY_PATH="${X5_X86_GCC1140_PATH}/dnn_x86/lib:${LD_LIBRARY_PATH:-}"
export PATH="${VENV_DIR}/bin:${PATH}"

echo "[X5] model=$MODEL_PATH"
echo "[X5] config=$CFG_PATH"
echo "[X5] hb_mapper=$HB_MAPPER_BIN"

echo "[X5] step 1/2: checker"
"$HB_MAPPER_BIN" checker --model-type onnx --model "$MODEL_PATH" --march bayes-e

echo "[X5] step 2/2: makertbin"
"$HB_MAPPER_BIN" makertbin --config "$CFG_PATH" --model-type onnx

echo "[X5] done"
