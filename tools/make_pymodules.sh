#!/usr/bin/bash

py_dir="$1"
py_ver="$2"
out_file="$3"
xz_path="$4"
py_source_dir="$HOME/.local/lib/python${py_ver}"

if [ -f "${out_file}" ]; then
    echo "python modules already exist"
else
    rm -Rf "${py_dir}" \
    && rm -Rf "${py_source_dir}" \
    && pip3 install --user \
        transformers==4.28.1 \
        tokenizers==0.13.3 \
        sympy==1.11.1 \
        numpy==1.24.3 \
        charset-normalizer==3.1.0 \
        torch==2.0.0 \
        accelerate==0.18.0 \
    && mkdir -p "${py_dir}" \
    && mv "$HOME/.local/lib/python${py_ver}" "${py_dir}/" \
    && cd "${py_dir}" \
    && tar cf - python${py_ver}/ | "${xz_path}" -z -T 0 - > "${out_file}" \
    && echo "python modules created"
fi
