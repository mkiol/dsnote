#!/usr/bin/bash

in_dir="$1"
out_dir="$2"
out_file="$3"
xz_path="$4"

if [ -f "${out_file}" ]; then
    echo "rhvoicedata module already exists"
else
    rm -Rf "${out_dir}" \
    && mkdir -p "${out_dir}/rhvoicedata" \
    && cp -r "${in_dir}/languages" "${out_dir}/rhvoicedata" \
    && rm "${out_dir}/rhvoicedata/languages/CMakeLists.txt" \
    && cd "${out_dir}" \
    && tar cf - rhvoicedata/ | "${xz_path}" -z -T 0 - > "${out_file}" \
    && echo "rhvoicedata module created"
fi
