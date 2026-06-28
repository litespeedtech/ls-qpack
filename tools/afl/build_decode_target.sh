#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$repo_dir"

build_dir=${AFL_BUILD_DIR:-build-afl}
compiler=${AFL_CC:-$(command -v afl-clang-fast || true)}
build_jobs=${AFL_BUILD_JOBS:-$(nproc)}

if [[ -z "$compiler" ]]; then
    echo "afl-clang-fast not found; set AFL_CC to the AFL compiler" >&2
    exit 1
fi

if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
    cmake -S . -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER="$compiler" \
        -DCMAKE_C_FLAGS="${CFLAGS:--fsanitize=address}" \
        -DLSQPACK_BIN=ON
fi

cmake --build "$build_dir" --target fuzz-decode -j "$build_jobs"

echo "$repo_dir/$build_dir/bin/fuzz-decode"
