#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$repo_dir"

out_dir=${AFL_OUT:-}
if [[ -z "$out_dir" ]]; then
    out_dir=$(find fuzz/output -maxdepth 1 -type d -name 'decode-*' -printf '%T@ %p\n' 2>/dev/null \
        | sort -nr | awk 'NR == 1 { print $2 }')
fi

if [[ -z "$out_dir" || ! -d "$out_dir/.pids" ]]; then
    echo "no AFL run found; set AFL_OUT or start fuzzers first" >&2
    exit 1
fi

for pid_file in "$out_dir"/.pids/*.pid; do
    [[ -e "$pid_file" ]] || continue
    name=$(basename "$pid_file" .pid)
    pid=$(cat "$pid_file")
    if kill -0 "$pid" 2>/dev/null; then
        kill "$pid"
        echo "sent TERM to $name pid $pid"
    else
        echo "$name pid $pid is not running"
    fi
done
