#!/usr/bin/bash

name="$1"
prefix="$2"
out="$3"
shift 3
locales="$@"

exec 1> "${out}"

echo "<RCC><qresource prefix=\"${prefix}\">"
for locale in ${locales}; do
    echo "<file>${name}-${locale}.qm</file>"
done
echo "</qresource></RCC>"
