#!/usr/bin/bash

in_dir="$1"
out_dir="$2"
out_file="$3"
xz_path="$4"

if [ -f "${out_file}" ]; then
    echo "espeakdata module already exists"
else
    rm -Rf "${out_dir}" \
    && mkdir -p "${out_dir}" \
    && cp -r --no-target-directory "${in_dir}" "${out_dir}/espeakdata" \
    && cd "${out_dir}" \
    && tar cf - espeakdata/ | "${xz_path}" -z -T 0 - > "${out_file}" \
    && echo "espeakdata module created"
fi
