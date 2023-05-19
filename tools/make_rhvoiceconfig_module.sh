#!/usr/bin/bash

in_dir="$1"
out_dir="$2"
out_file="$3"
xz_path="$4"

if [ -f "${out_file}" ]; then
    echo "rhvoiceconfig module already exists"
else
    rm -Rf "${out_dir}" \
    && mkdir -p "${out_dir}/rhvoiceconfig" \
    && cp -r "${in_dir}/dicts" "${out_dir}/rhvoiceconfig" \
    && cp "${in_dir}/RHVoice.conf" "${out_dir}/rhvoiceconfig" \
    && cd "${out_dir}" \
    && tar cf - rhvoiceconfig/ | "${xz_path}" -z -T 0 - > "${out_file}" \
    && echo "rhvoiceconfig module created"
fi
