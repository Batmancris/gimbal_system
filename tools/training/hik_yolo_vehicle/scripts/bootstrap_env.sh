#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
VENV_DIR="${YOLO_VENV_DIR:-$PROJECT_DIR/.venv_yolo_train}"

log() {
  printf '[BOOTSTRAP] %s\n' "$1" >&2
}

pick_python() {
  if [[ -n "${PYTHON_BIN:-}" ]] && command -v "${PYTHON_BIN}" >/dev/null 2>&1; then
    printf '%s\n' "${PYTHON_BIN}"
    return 0
  fi

  local candidate
  for candidate in python3.10 python3; do
    if command -v "$candidate" >/dev/null 2>&1; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  return 1
}

ensure_venv() {
  local system_python="$1"
  local venv_python="$VENV_DIR/bin/python"

  if [[ ! -x "$venv_python" ]]; then
    log "Creating virtual environment at $VENV_DIR"
    if ! "$system_python" -m venv "$VENV_DIR"; then
      log "Failed to create venv. Install python3-venv on Ubuntu and rerun."
      exit 1
    fi
  fi

  printf '%s\n' "$venv_python"
}

ensure_packages() {
  local python_exe="$1"

  log "Upgrading pip tooling"
  "$python_exe" -m pip install --upgrade pip setuptools wheel >&2

  if ! "$python_exe" -c "import torch; raise SystemExit(0 if torch.cuda.is_available() else 1)" >/dev/null 2>&1; then
    log "Installing CUDA-enabled PyTorch"
    "$python_exe" -m pip install --upgrade torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121 >&2
  else
    log "CUDA-enabled PyTorch already available"
  fi

  if ! "$python_exe" -c "import ultralytics, yaml" >/dev/null 2>&1; then
    log "Installing training requirements"
    "$python_exe" -m pip install --upgrade -r "$PROJECT_DIR/requirements-train.txt" >&2
  else
    log "Training requirements already available"
  fi

  log "Running environment self-check"
  "$python_exe" -c "import torch, ultralytics, yaml; assert torch.cuda.is_available(); print(f'[BOOTSTRAP] torch={torch.__version__} cuda={torch.version.cuda} gpu={torch.cuda.get_device_name(0)}', flush=True)" >&2
}

main() {
  local system_python
  system_python="$(pick_python)" || {
    log "No suitable Python interpreter found. Install python3.10 or set PYTHON_BIN."
    exit 1
  }

  local python_exe
  python_exe="$(ensure_venv "$system_python")"
  ensure_packages "$python_exe"
  printf '%s\n' "$python_exe"
}

main "$@"
