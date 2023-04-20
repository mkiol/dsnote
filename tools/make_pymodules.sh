#!/usr/bin/bash

py_dir="$1"
py_ver="$2"
out_file="$3"

if [ -f "${out_file}" ]; then
    echo "python modules already exist"
else
    rm -Rf "${py_dir}" \
    && pip3 install \
        transformers==4.28.1 \
        torch==2.0.0 \
        --user \
    && mkdir -p "${py_dir}" \
    && mv "$HOME/.local/lib/python${py_ver}" "${py_dir}/" \
    && cd "${py_dir}" \
    && tar cf - python${py_ver}/ | xz -z - > "${out_file}" \
    && echo "python modules created"
fi
