#!/usr/bin/bash

#
# Make Fedora RPM package
#
# Usage:
# make_rpm.sh [spec_file_path] [output_dir]
#

spec_file="${1:-"$(realpath "$(dirname "$0")")/dsnote.spec"}"
self_dir="$(realpath "$(dirname "$0")")"
out_dir="${2:-"$self_dir"}"

echo "Using spec file: $spec_file"
echo "Output dir: $out_dir"

if [ ! -f "$spec_file" ]; then
    echo "spec file doesn't exist: $spec_file"
    exit 1
fi
if [ ! -d "$out_dir" ]; then
    echo "output dir doesn't exist: $out_dir"
    exit 1
fi

src_dir="$(realpath "$self_dir/..")"
ver="$(sed -rn 's_.*Version:\s*(.*).*_\1_p' "$spec_file")"
name="$(sed -rn 's_.*Name:\s*(.*).*_\1_p' "$spec_file")"
url="$(sed -rn 's_.*Source0:\s*(.*).*_\1_p' "$spec_file")"
build_requires=$(sed -rn 's_.*BuildRequires:\s*(.*).*_\1_p' "$spec_file" | tr '\n' ' ')
out_dir=$self_dir
rpmbuild_dir="$(realpath "$HOME/rpmbuild")"
src_dir_ver="./$name-$ver"
tarball_filename="$(echo "$url" | sed -rn 's_.*/(.*)_\1_p')"
tarball_path="$out_dir/$tarball_filename"

echo
echo "To install all the packages required for the build:"
echo "dnf install rpmdevtools $build_requires"
echo

if ! command -v rpmbuild 2>&1 >/dev/null
then
    echo "rpmbuild could not be found"
    exit 1
fi

cd "$self_dir" &&
ln -sf --no-target-directory "$src_dir" "$(realpath "$src_dir_ver")" &&
tar \
    --exclude="${src_dir_ver}/appimage" \
    --exclude="${src_dir_ver}/arch" \
    --exclude="${src_dir_ver}/bak" \
    --exclude="${src_dir_ver}/deb" \
    --exclude="${src_dir_ver}/fedora" \
    --exclude="${src_dir_ver}/flatpak" \
    --exclude="${src_dir_ver}/misc" \
    --exclude="${src_dir_ver}/external" \
    --exclude="${src_dir_ver}/dsnote-*" \
    --exclude="${src_dir_ver}/.*" \
    --exclude="${src_dir_ver}/*.user" \
    -hzcvf "$tarball_path" "$src_dir_ver" &&
rm "$src_dir_ver" &&
rm -rf "$rpmbuild_dir" &&
mkdir -p "$rpmbuild_dir"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS} &&
mv "$tarball_path" "$rpmbuild_dir/SOURCES/$tarball_filename" &&
cp "$spec_file" "$rpmbuild_dir/SPECS/$name.spec" &&
QA_RPATHS=$((0x0002)) rpmbuild --define "_topdir $rpmbuild_dir" --clean -ba "$rpmbuild_dir/SPECS/$name.spec" &&
find "$rpmbuild_dir" -type f -name "*.rpm" -exec cp -- "{}" "$out_dir/" \;

