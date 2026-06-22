#!/usr/bin/env bash

set -euo pipefail

venv_dir="${DSNOTE_CANARY_VENV:-${DSNOTE_PREFIX:-${HOME}/.local}/share/dsnote/venv}"
python_bin="${PYTHON:-python3}"
force_repair=false
check_model=""

usage() {
    cat <<'EOF'
Usage: install_canary_runtime.sh [--venv PATH] [--python PYTHON] [--repair] [--check-model PATH]

Creates or updates the Python venv used by dsnote's Canary STT engine.
The default venv path matches a user-prefix install:
  ~/.local/share/dsnote/venv

Options:
  --check-model PATH  Also restore a local Canary .nemo file on CPU. This is
                      slower, but catches import-time dependency corruption.

Environment:
  DSNOTE_PREFIX       Install prefix used to derive share/dsnote/venv.
  DSNOTE_CANARY_VENV  Explicit venv path.
  PYTHON              Python executable used to create the venv.
EOF
}

require_value() {
    if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "missing value for $1" >&2
        usage >&2
        exit 2
    fi
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --venv)
            require_value "$@"
            venv_dir="$2"
            shift 2
            ;;
        --python)
            require_value "$@"
            python_bin="$2"
            shift 2
            ;;
        --repair)
            force_repair=true
            shift
            ;;
        --check-model)
            require_value "$@"
            check_model="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [ -n "${check_model}" ] && [ ! -f "${check_model}" ]; then
    echo "model file not found: ${check_model}" >&2
    exit 2
fi

packages=(
    "numpy==2.3.5"
    "torch==2.9.1"
    "triton==3.5.1"
    "cuda-python==13.1.1"
    "huggingface-hub==0.36.0"
    "fsspec==2024.12.0"
    "pooch==1.8.2"
)

repair_packages=(
    "torch==2.9.1"
    "numpy==2.3.5"
    "triton==3.5.1"
    "fsspec==2024.12.0"
    "pooch==1.8.2"
    "sympy==1.14.0"
    "pyparsing==3.3.2"
    "matplotlib==3.11.0"
    "idna==3.18"
    "certifi==2026.6.17"
    "click==8.4.1"
    "pyarrow==22.0.0"
    "stack-data==0.6.3"
    "pure-eval==0.2.3"
)

venv_python="${venv_dir}/bin/python"

create_venv() {
    if [ ! -x "${venv_python}" ]; then
        mkdir -p "$(dirname "${venv_dir}")"
        "${python_bin}" -m venv "${venv_dir}"
    fi
}

nemo_runtime_installed() {
    "${venv_python}" - <<'PY'
import importlib.metadata as metadata
import sys

try:
    version = metadata.version("nemo-toolkit")
except metadata.PackageNotFoundError:
    sys.exit(1)

sys.exit(0 if version == "2.6.0" else 1)
PY
}

install_runtime() {
    "${venv_python}" -m ensurepip --upgrade
    "${venv_python}" -m pip install --upgrade \
        "pip>=26.0.1" \
        "setuptools==80.9.0" \
        "wheel"
    if ! nemo_runtime_installed; then
        "${venv_python}" -m pip install "nemo-toolkit[asr]==2.6.0"
    fi
    "${venv_python}" -m pip install "${packages[@]}"
}

repair_runtime_payloads() {
    "${venv_python}" -m pip install --force-reinstall --no-deps \
        "${repair_packages[@]}"
}

verify_runtime() {
    "${venv_python}" - <<'PY'
import importlib.metadata as metadata

import cuda.bindings
import cuda.core
import librosa.filters
import nemo.collections.asr
import pooch.core
import torch

print("torch", metadata.version("torch"))
print("nemo_toolkit", metadata.version("nemo_toolkit"))
print("cuda-python", metadata.version("cuda-python"))
PY
    "${venv_python}" -m pip check
}

verify_model_restore() {
    if [ -z "${check_model}" ]; then
        return 0
    fi
    "${venv_python}" - "${check_model}" <<'PY'
import gc
import sys

import torch
from nemo.collections.asr.models import ASRModel

model_path = sys.argv[1]
model = ASRModel.restore_from(model_path, map_location=torch.device("cpu"))
model.eval()
print("restored_model", model.__class__.__name__)
del model
gc.collect()
PY
}

create_venv
install_runtime

if "${force_repair}"; then
    repair_runtime_payloads
fi

if ! verify_runtime; then
    echo "Canary runtime import check failed; repairing package payloads." >&2
    repair_runtime_payloads
    verify_runtime
fi

verify_model_restore

echo "Canary runtime ready: ${venv_dir}"
