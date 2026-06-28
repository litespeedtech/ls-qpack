#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$repo_dir"

: "${AFL_TMPDIR:?AFL_TMPDIR must be set to a writable tmpfs directory}"

stage_root=${AFL_STAGE_DIR:-"$AFL_TMPDIR/ls-qpack-afl/decode"}
campaigns=(a b d)

case "$stage_root" in
    "$AFL_TMPDIR"/*) ;;
    *)
        echo "AFL_STAGE_DIR must be under AFL_TMPDIR: $stage_root" >&2
        exit 1
        ;;
esac

mkdir -p "$stage_root"

for campaign in "${campaigns[@]}"; do
    src_dir="fuzz/decode/$campaign"
    dst_dir="$stage_root/$campaign"
    dst_inputs="$dst_dir/inputs"

    if [[ ! -f "$src_dir/preamble" || ! -d "$src_dir/test-cases" ]]; then
        echo "missing campaign inputs: $src_dir" >&2
        exit 1
    fi

    rm -rf "$dst_dir"
    mkdir -p "$dst_inputs"
    cp "$src_dir/preamble" "$dst_dir/preamble"
    cp "$src_dir"/test-cases/* "$dst_inputs"/
done

echo "$stage_root"
