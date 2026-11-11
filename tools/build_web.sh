#!/usr/bin/env bash
# Builds the web page (build-web/SSSVRecompiled.html, .js, .wasm, .data) from the USA 1.0 ROM, on Linux with emsdk
# active (N64Recomp-browser's README, Using it in a recomp, says what else: naga, spirv-opt, ccache), clang, ld.lld,
# make, Ninja and Python 3:
#
#   bash tools/build_web.sh <USA ROM> [--prepare]
#   bash tools/build_web.sh --patches          step 1 alone: no ROM, and only git and CMake
#
# 1. The submodules the page needs and their patches, from clean whenever dependency-patches/, lib/N64Recomp-browser
#    or web/patches moved since the last time (build-web-prep/web-patched records it): apply.cmake can't take a
#    checkout from one version of its patches to another. First the game's own, dependency-patches/ (manifest.json),
#    as tools/apply_dependency_patches.py applies them for the desktop build; then N64Recomp-browser's web support
#    (lib/N64Recomp-browser/patches/apply.cmake, project by project), but for the files where the two change the same
#    lines: the web's change to those (web/patches/files.txt) is web/patches/<project>.patch instead, made on top of
#    the game's from the N64Recomp-browser commit web/patches/files.txt names, with what the web has changed in them
#    since merged in by apply.cmake --since (only a change that meets the game's own lines stops the build, for
#    another merge by hand). A file marked own there keeps the game's code alone: the web's change to it is left out,
#    then and later.
# 2. N64Recomp and RSPRecomp, built natively from the patched lib/N64ModernRuntime/N64Recomp: the page needs the code
#    the web-patched N64Recomp writes, the patches library's included (CMakeLists.txt's SSSV_RECOMP_TOOL).
# 3. The recompiled game (sssv.us.toml) and the audio microcode (aspMain.toml), from the ROM, in build-web-gen, as
#    the desktop build makes them in the checkout.
# 4. The page, with emcmake, in build-web (CMakeLists.txt's EMSCRIPTEN parts): the patches library is built with clang
#    and ld.lld and recompiled there too. --prepare stops before it.
# --patches stops after step 1. N64Recomp-browser's CI runs it with its own checkout as lib/N64Recomp-browser and the
# submodules checked out, so a web change there that meets this port's own lines in a file left out is red where it is
# made: merge it into the patch by hand then.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
[[ $# -ge 1 ]] || { echo "usage: $0 <USA ROM> [--prepare] | --patches" >&2; exit 2; }
patches_only=0
[[ "$1" == --patches ]] && patches_only=1
[[ $patches_only == 1 ]] || rom="$(realpath "$1")"
prepare_only=0
[[ "${2:-}" == --prepare ]] && prepare_only=1
cd "$root"
jobs="$(nproc)"
browser=lib/N64Recomp-browser
prep=build-web-prep
gen="$root/build-web-gen"
mkdir -p $prep

step() { printf '\n== %s\n' "$*"; }

# --- 1. Submodules and patches ---
step "Submodules"
[[ -f lib/rt64/CMakeLists.txt && -f lib/N64ModernRuntime/N64Recomp/CMakeLists.txt && -f lib/RecompFrontend/CMakeLists.txt &&
   -f lib/RecompFrontend/recompui/lib/RmlUi/CMakeLists.txt && -f $browser/patches/apply.cmake ]] ||
    git submodule update --init --recursive lib/rt64 lib/N64ModernRuntime lib/RecompFrontend $browser
[[ $patches_only == 1 || -f SSSVRecompSyms/sssv.us.syms.toml ]] || git submodule update --init SSSVRecompSyms

step "Patches"
# <name> <checkout> [files left to web/patches/<name>.patch, merged from the commit files.txt names, or left out]
shared() { awk -v p="$1" '!/^#/ && $1 == p { printf "%s ", $2 }' web/patches/files.txt; }
merged() { awk -v p="$1" '!/^#/ && $1 == p && $3 != "own" { printf "%s ", $2 }' web/patches/files.txt; }
merged_from="$(sed -n 's/^# merged from N64Recomp-browser //p' web/patches/files.txt)"
web_patches="rt64 lib/rt64 $(shared rt64)
plume lib/rt64/src/contrib/plume $(shared plume)
hlslpp lib/rt64/src/contrib/hlslpp $(shared hlslpp)
N64ModernRuntime lib/N64ModernRuntime $(shared N64ModernRuntime)
N64Recomp lib/N64ModernRuntime/N64Recomp $(shared N64Recomp)
RecompFrontend lib/RecompFrontend $(shared RecompFrontend)"
# The game's own patches: <checkout> <patch>, from dependency-patches/manifest.json.
own_patches="$(python3 -c '
import json, sys
for item in json.load(open("dependency-patches/manifest.json")):
    print(item["path"], "dependency-patches/" + item["patch"])')"
stamp="$(git -C $browser rev-parse HEAD) $( (cat dependency-patches/* web/patches/*.patch web/patches/files.txt;
    echo "$web_patches") | sha1sum | cut -c1-40)"
# A checkout reset since (rt64 clean) is patched again whatever the stamp says.
if [[ "$(cat $prep/web-patched 2> /dev/null)" != "$stamp" ]] || git -C lib/rt64 diff --quiet; then
    for s in lib/rt64 lib/N64ModernRuntime lib/RecompFrontend; do
        git -C $s submodule foreach --recursive -q 'git reset -q --hard && git clean -fdq'
        git -C $s reset -q --hard && git -C $s clean -fdq
        rm -f "$(git -C $s rev-parse --path-format=absolute --git-dir)"/n64recomp-browser-*
    done
    while read -r checkout patch; do
        git -C "$checkout" apply --whitespace=nowarn "$root/$patch"
    done <<< "$own_patches"
    while read -r name checkout skip; do
        cmake -P $browser/patches/apply.cmake "$name" "$checkout" $skip
        [[ ! -f web/patches/$name.patch ]] || git -C "$checkout" apply --whitespace=nowarn "$root/web/patches/$name.patch"
        since="$(merged "$name")"
        [[ -z "$since" ]] || cmake -P $browser/patches/apply.cmake "$name" "$checkout" --since "$merged_from" $since
    done <<< "$web_patches"
    echo "$stamp" > $prep/web-patched
fi

[[ $patches_only == 1 ]] && exit 0

# --- 2. N64Recomp and RSPRecomp ---
step "N64Recomp"
tools=$prep/tools
[[ -f $tools/build.ninja ]] || cmake -S lib/N64ModernRuntime/N64Recomp -B $tools -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > /dev/null
ninja -C $tools N64RecompCLI RSPRecomp > /dev/null
recomp_tool="$root/$tools/N64Recomp"
[[ -x "$recomp_tool" ]] || recomp_tool="$root/$tools/N64RecompCLI"

# --- 3. The recompiled game and the audio microcode ---
step "Recompiled code"
inputs="$( (cat "$rom" sssv.us.toml aspMain.toml SSSVRecompSyms/sssv.us.syms.toml "$recomp_tool" $tools/RSPRecomp) |
    sha1sum | cut -c1-40)"
if [[ "$(cat $prep/recompiled-stamp 2> /dev/null)" != "$inputs" || ! -f "$gen/RecompiledFuncs/funcs.h" ||
      ! -f "$gen/rsp/aspMain.cpp" ]]; then
    rm -rf "$gen"
    mkdir -p "$gen/rsp"
    # The configs read their paths from the folder they are in.
    cp sssv.us.toml aspMain.toml "$gen/"
    ln -s "$rom" "$gen/sssv.us.z64"
    ln -s "$root/SSSVRecompSyms" "$gen/SSSVRecompSyms"
    (cd "$gen" && "$recomp_tool" sssv.us.toml && "$root/$tools/RSPRecomp" aspMain.toml) > $prep/n64recomp.log 2>&1 ||
        { tail -20 $prep/n64recomp.log; exit 1; }
    echo "$inputs" > $prep/recompiled-stamp
fi
[[ $prepare_only == 1 ]] && exit 0

# --- 4. The page ---
step "Page"
embuilder build sdl2-mt freetype-legacysjlj > /dev/null 2>&1
[[ -f build-web/build.ninja ]] || emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DGAME_GENERATED_DIR="$gen" -DSSSV_RECOMP_TOOL="$recomp_tool" -DRT64_NAGA_EXECUTABLE="$(command -v naga)" \
    -DPATCHES_C_COMPILER="$(command -v clang)" -DPATCHES_LD="$(command -v ld.lld)" \
    -DPython3_EXECUTABLE="$(command -v python3)" > $prep/configure.log 2>&1 ||
    { tail -30 $prep/configure.log; rm -f build-web/build.ninja; exit 1; }
cmake --build build-web --target SSSVRecompiled -j"$jobs"
ls -l build-web/SSSVRecompiled.{html,js,wasm,data}
