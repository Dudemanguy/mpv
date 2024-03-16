#!/bin/sh
set -e

gitclone="git clone --depth=1 --recursive --shallow-submodules"
rm -rf subprojects
mkdir -p subprojects

# build libglvnd for libplacebo/glad
# not actually a subproject but just throw it in the directory
$gitclone https://gitlab.freedesktop.org/glvnd/libglvnd subprojects/libglvnd
meson setup subprojects/libglvnd/build
meson install -C subprojects/libglvnd/build

# libplacebo on openBSD is too old; use a subproject
$gitclone https://code.videolan.org/videolan/libplacebo.git subprojects/libplacebo

meson setup build \
    -Dlibmpv=true \
    -Dlua=enabled \
    -Dopenal=enabled \
    -Dpulse=enabled \
    -Dtests=true \
    -Dvulkan=enabled \
    -Ddvdnav=enabled \
    -Dcdda=enabled

meson compile -C build
./build/mpv -v --no-config
