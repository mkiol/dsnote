#!/usr/bin/bash

out_dir="$1"
out_file="$2"
xz_path="$3"

if [ -f "${out_file}" ]; then
    echo "python module already exists"
else
    rm -Rf "${out_dir}" \
    && pip3 install --user \
        transformers==4.28.1 \
        tokenizers==0.13.3 \
        sympy==1.11.1 \
        numpy==1.24.3 \
        charset-normalizer==3.1.0 \
        torch==2.0.0 \
        accelerate==0.18.0 \
        setuptools==67.6.1 \
    && mkdir -p "${out_dir}" \
    && mv --no-target-directory "$(find "$HOME/.local/lib" -name "python*" | head -n 1)" "${out_dir}/python" \
    && strip -s $(find "${out_dir}" -type f -name "*.so*" | sed 's/.*/&/' | tr '\n' ' ') \
    && cd "${out_dir}" \
    && tar cf - python/ | "${xz_path}" -z -T 0 - > "${out_file}" \
    && echo "python module created"
fi
