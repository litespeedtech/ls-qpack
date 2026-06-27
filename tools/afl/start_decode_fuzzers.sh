#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$repo_dir"

: "${AFL_TMPDIR:?AFL_TMPDIR must be set to a writable tmpfs directory}"
: "${AFL_JOBS:?AFL_JOBS must be set to the number of afl-fuzz instances to run}"

if ! [[ "$AFL_JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo "AFL_JOBS must be a positive integer" >&2
    exit 1
fi

if ! command -v afl-fuzz >/dev/null 2>&1; then
    echo "afl-fuzz not found in PATH" >&2
    exit 1
fi

target=${AFL_TARGET:-"$repo_dir/build-afl/bin/fuzz-decode"}
if [[ ! -x "$target" ]]; then
    echo "target is not executable: $target" >&2
    echo "run tools/afl/build_decode_target.sh first or set AFL_TARGET" >&2
    exit 1
fi

stage_root=${AFL_STAGE_DIR:-"$AFL_TMPDIR/ls-qpack-afl/decode"}
if [[ ! -d "$stage_root/a/inputs" || ! -d "$stage_root/b/inputs" || ! -d "$stage_root/d/inputs" ]]; then
    stage_root=$(tools/afl/stage_decode_inputs.sh)
fi

timestamp=$(date +%Y%m%d-%H%M%S)
out_dir=${AFL_OUT:-"$repo_dir/fuzz/output/decode-$timestamp"}
pid_dir="$out_dir/.pids"
log_dir="$out_dir/.logs"
mkdir -p "$pid_dir" "$log_dir"

campaigns=(a b d)
schedules=(explore fast coe lin quad exploit rare)
host=$(hostname -s 2>/dev/null || hostname)
cpu_count=$(nproc)

export AFL_FINAL_SYNC=${AFL_FINAL_SYNC:-1}
export AFL_IMPORT_FIRST=${AFL_IMPORT_FIRST:-1}
export AFL_TESTCACHE_SIZE=${AFL_TESTCACHE_SIZE:-200}
export AFL_IGNORE_SEED_PROBLEMS=${AFL_IGNORE_SEED_PROBLEMS:-1}

echo "AFL output: $out_dir"
echo "AFL stage:  $stage_root"
echo "Target:     $target"
echo "Jobs:       $AFL_JOBS"
if [[ "${AFL_MAIN_FOREGROUND:-0}" == 1 ]]; then
    echo "Main mode:  foreground TUI"
fi

main_name=
main_log=
main_pid_file=
main_cmd=()
main_env_vars=()

stop_background_fuzzers()
{
    local pid_file pid name

    [[ -d "$pid_dir" ]] || return 0
    for pid_file in "$pid_dir"/*.pid; do
        [[ -e "$pid_file" ]] || continue
        name=$(basename "$pid_file" .pid)
        pid=$(cat "$pid_file")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            echo "sent TERM to $name pid $pid"
        fi
    done
}

if [[ "${AFL_DRY_RUN:-0}" != 1 && "${AFL_MAIN_FOREGROUND:-0}" == 1 ]]; then
    trap 'stop_background_fuzzers; exit 130' INT
    trap 'stop_background_fuzzers; exit 143' TERM
fi

for ((i = 0; i < AFL_JOBS; ++i)); do
    campaign=${campaigns[$((i % ${#campaigns[@]}))]}
    seed_dir="$stage_root/$campaign/inputs"
    preamble="$stage_root/$campaign/preamble"
    schedule=${schedules[$((i % ${#schedules[@]}))]}
    cpu=$((i % cpu_count))

    risked=100
    table_size=256
    if [[ "$campaign" == "b" ]]; then
        table_size=4096
    fi

    if (( i == 0 )); then
        role=(-M "main-$host")
        name="main-$host"
    else
        role=(-S "decode-$campaign-$i")
        name="decode-$campaign-$i"
    fi

    afl_opts=(-i "$seed_dir" -o "$out_dir" "${role[@]}" -b "$cpu" -a binary -m 0 -t 1000+ -p "$schedule")

    case $((i % 10)) in
    1) afl_opts+=(-L 0) ;;
    2) afl_opts+=(-Z) ;;
    3|4|5|6) export AFL_DISABLE_TRIM=1 ;;
    *) unset AFL_DISABLE_TRIM ;;
    esac

    case $((i % 5)) in
    0|1) afl_opts+=(-P explore) ;;
    2) afl_opts+=(-P exploit) ;;
    esac

    target_args=(-i "$preamble" -s "$risked" -t "$table_size")
    if (( i > 0 && i % 2 == 1 )); then
        target_args+=(-H)
    fi

    cmd=(afl-fuzz "${afl_opts[@]}" -- "$target" "${target_args[@]}")
    log="$log_dir/$name.log"
    pid_file="$pid_dir/$name.pid"

    env_vars=(
        AFL_FINAL_SYNC="$AFL_FINAL_SYNC"
        AFL_IMPORT_FIRST="$AFL_IMPORT_FIRST"
        AFL_TESTCACHE_SIZE="$AFL_TESTCACHE_SIZE"
        AFL_IGNORE_SEED_PROBLEMS="$AFL_IGNORE_SEED_PROBLEMS"
    )
    if [[ -n "${AFL_DISABLE_TRIM:-}" ]]; then
        env_vars+=(AFL_DISABLE_TRIM="$AFL_DISABLE_TRIM")
    fi

    if [[ "${AFL_DRY_RUN:-0}" == 1 ]]; then
        printf '%q ' "${cmd[@]}"
        printf '\n'
    elif (( i == 0 )) && [[ "${AFL_MAIN_FOREGROUND:-0}" == 1 ]]; then
        main_name=$name
        main_log=$log
        main_pid_file=$pid_file
        main_cmd=("${cmd[@]}")
        main_env_vars=("${env_vars[@]}")
    else
        nohup env "${env_vars[@]}" "${cmd[@]}" >"$log" 2>&1 &
        echo "$!" >"$pid_file"
        echo "started $name pid $(cat "$pid_file") cpu $cpu campaign $campaign log $log"
    fi
done

if [[ "${AFL_DRY_RUN:-0}" != 1 && "${AFL_MAIN_FOREGROUND:-0}" == 1 ]]; then
    echo "starting $main_name in foreground"
    echo "log/pid files are not used for foreground main: $main_log $main_pid_file"
    set +e
    env "${main_env_vars[@]}" "${main_cmd[@]}"
    main_status=$?
    set -e
    stop_background_fuzzers
    exit "$main_status"
fi
