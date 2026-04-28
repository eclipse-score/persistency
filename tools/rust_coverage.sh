#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Generate Rust coverage for this repository without Ferrocene's broken symbol-report package.

Usage:
  bazel run //:rust_coverage -- [options]

Options:
  --targets <labels>            Comma-separated Bazel test labels.
                                Default: //src/rust/rust_kvs:tests
  --out-dir <path>              Output directory for reports.
                                Default: $(bazel info bazel-bin)/coverage/rust-tests
  --min-line-coverage <p>       Minimum line coverage percentage (0-100).
  --bazel-config <name>         Bazel config to pass through to nested Bazel commands.
                                Repeat to pass multiple configs.
                                Defaults: per-x86_64-linux, ferrocene-coverage
  --help                        Show this help.

Environment:
  FERROCENE_LLVM_BIN            Directory containing matching llvm-profdata and llvm-cov.
USAGE
}

TARGETS_CSV="//src/rust/rust_kvs:tests"
OUT_DIR=""
MIN_LINE_COVERAGE=""
TARGET_TRIPLE="x86_64-unknown-linux-gnu"
IGNORE_FILENAME_REGEX='^(external/|.*/\.cache/ferrocene-src[^/]*/)'
BAZEL_CONFIGS=("per-x86_64-linux" "ferrocene-coverage")

while [[ $# -gt 0 ]]; do
  case "$1" in
    --targets)
      TARGETS_CSV="$2"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="$2"
      shift 2
      ;;
    --min-line-coverage)
      MIN_LINE_COVERAGE="$2"
      shift 2
      ;;
    --bazel-config)
      BAZEL_CONFIGS+=("$2")
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

workspace="${BUILD_WORKSPACE_DIRECTORY:-$(pwd)}"
cd "${workspace}"

BAZEL_FLAGS=(--lockfile_mode=error)
for cfg in "${BAZEL_CONFIGS[@]}"; do
  [[ -n "${cfg}" ]] || continue
  BAZEL_FLAGS+=("--config=${cfg}")
done

mapfile -t targets < <(printf '%s\n' "${TARGETS_CSV}" | tr ',' '\n' | sed '/^$/d')
if [[ ${#targets[@]} -eq 0 ]]; then
  echo "No Rust test targets configured." >&2
  exit 1
fi

bazel_bin="$(bazel info bazel-bin)"
bazel_testlogs="$(bazel info bazel-testlogs)"
if [[ -z "${OUT_DIR}" ]]; then
  OUT_DIR="${bazel_bin}/coverage/rust-tests"
fi

find_test_binary() {
  local label="$1"
  local pkg="${label#//}"
  local name="${label##*:}"
  pkg="${pkg%%:*}"
  local search_root="${bazel_bin}/${pkg}"
  local fallback=""

  while IFS= read -r path; do
    [[ -n "${path}" ]] || continue
    if [[ -z "${fallback}" ]]; then
      fallback="${path}"
    fi
    case "${path}" in
      */test-*/"${name}")
        echo "${path}"
        return 0
        ;;
    esac
  done < <(find "${search_root}" -type f -perm -u+x -name "${name}" 2>/dev/null | sort)

  if [[ -n "${fallback}" ]]; then
    echo "${fallback}"
    return 0
  fi

  return 1
}

pick_llvm_bin() {
  local sample_profraw="$1"
  local probe
  probe="$(mktemp)"
  local candidates=()

  if [[ -n "${FERROCENE_LLVM_BIN:-}" ]]; then
    candidates+=("${FERROCENE_LLVM_BIN}")
  fi

  candidates+=(
    "${workspace}/../ferrocene_builder/build/x86_64-unknown-linux-gnu/stage2/lib/rustlib/${TARGET_TRIPLE}/bin"
    "${workspace}/../ferrocene_builder/build/x86_64-unknown-linux-gnu/stage3/lib/rustlib/${TARGET_TRIPLE}/bin"
    "/home/${USER}/sources/ferrocene_builder/build/x86_64-unknown-linux-gnu/stage2/lib/rustlib/${TARGET_TRIPLE}/bin"
    "/home/${USER}/sources/ferrocene_builder/build/x86_64-unknown-linux-gnu/stage3/lib/rustlib/${TARGET_TRIPLE}/bin"
    "/usr/lib/llvm-18/bin"
  )

  for dir in "${candidates[@]}"; do
    [[ -d "${dir}" ]] || continue
    if [[ ! -x "${dir}/llvm-profdata" || ! -x "${dir}/llvm-cov" ]]; then
      continue
    fi
    if "${dir}/llvm-profdata" merge -sparse "${sample_profraw}" -o "${probe}" >/dev/null 2>&1; then
      rm -f "${probe}"
      echo "${dir}"
      return 0
    fi
  done

  rm -f "${probe}"
  return 1
}

parse_line_totals() {
  local summary_json="$1"
  python3 - "${summary_json}" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as fh:
    data = json.load(fh)

totals = data["data"][0]["totals"]["lines"]
print(f"{totals['percent']:.2f} {totals['covered']} {totals['count']}")
PY
}

mkdir -p "${OUT_DIR}"

echo "Running Rust tests with coverage instrumentation..."
bazel test "${BAZEL_FLAGS[@]}" --nocache_test_results --test_output=errors "${targets[@]}"

sample_profraw=""
for label in "${targets[@]}"; do
  pkg="${label#//}"
  pkg="${pkg%%:*}"
  name="${label##*:}"
  profraw_dir="${bazel_testlogs}/${pkg}/${name}/test.outputs"
  shopt -s nullglob
  profraw_files=("${profraw_dir}"/*.profraw)
  shopt -u nullglob
  if [[ ${#profraw_files[@]} -gt 0 ]]; then
    sample_profraw="${profraw_files[0]}"
    break
  fi
done

if [[ -z "${sample_profraw}" ]]; then
  echo "No .profraw files were generated under ${bazel_testlogs}." >&2
  exit 1
fi

if ! llvm_bin="$(pick_llvm_bin "${sample_profraw}")"; then
  echo "Could not find a compatible llvm-profdata/llvm-cov pair for ${sample_profraw}." >&2
  echo "Set FERROCENE_LLVM_BIN to a matching Ferrocene LLVM bin directory." >&2
  exit 1
fi

llvm_profdata="${llvm_bin}/llvm-profdata"
llvm_cov="${llvm_bin}/llvm-cov"
llvm_tool_env=(env LLVM_PROFILE_FILE=/dev/null)

echo "Using LLVM tools from ${llvm_bin}"

failures=()
total_covered=0
total_lines=0
parsed_targets=0

for label in "${targets[@]}"; do
  pkg="${label#//}"
  pkg="${pkg%%:*}"
  name="${label##*:}"
  profraw_dir="${bazel_testlogs}/${pkg}/${name}/test.outputs"

  shopt -s nullglob
  profraw_files=("${profraw_dir}"/*.profraw)
  shopt -u nullglob
  if [[ ${#profraw_files[@]} -eq 0 ]]; then
    echo "Skipping ${label}: no .profraw files in ${profraw_dir}" >&2
    failures+=("${label} (no .profraw files)")
    continue
  fi

  if ! bin_path="$(find_test_binary "${label}")"; then
    echo "Skipping ${label}: could not find test binary under ${bazel_bin}/${pkg}" >&2
    failures+=("${label} (test binary not found)")
    continue
  fi

  safe_label="${label//\//_}"
  safe_label="${safe_label//:/_}"
  safe_label="${safe_label//@/_}"
  report_dir="${OUT_DIR}/${safe_label}"
  html_dir="${report_dir}/html"
  profdata_path="${report_dir}/coverage.profdata"
  summary_json="${report_dir}/summary.json"
  mkdir -p "${html_dir}"

  "${llvm_tool_env[@]}" "${llvm_profdata}" merge -sparse "${profraw_files[@]}" -o "${profdata_path}"
  "${llvm_tool_env[@]}" "${llvm_cov}" export \
    --summary-only \
    --ignore-filename-regex="${IGNORE_FILENAME_REGEX}" \
    --instr-profile="${profdata_path}" \
    "${bin_path}" > "${summary_json}"
  "${llvm_tool_env[@]}" "${llvm_cov}" show \
    --format=html \
    --output-dir="${html_dir}" \
    --ignore-filename-regex="${IGNORE_FILENAME_REGEX}" \
    --instr-profile="${profdata_path}" \
    "${bin_path}" >/dev/null

  read -r line_pct line_cov_lines line_total_lines < <(parse_line_totals "${summary_json}")
  echo "Line coverage for ${label}: ${line_pct}% (${line_cov_lines}/${line_total_lines} lines)"
  echo "Report for ${label}: ${html_dir}/index.html"

  total_covered=$((total_covered + line_cov_lines))
  total_lines=$((total_lines + line_total_lines))
  parsed_targets=$((parsed_targets + 1))

  if [[ -n "${MIN_LINE_COVERAGE}" ]]; then
    if ! python3 - "${line_pct}" "${MIN_LINE_COVERAGE}" <<'PY'
import sys
sys.exit(0 if float(sys.argv[1]) >= float(sys.argv[2]) else 1)
PY
    then
      failures+=("${label} (${line_pct}% line coverage)")
    fi
  fi
done

echo "---"
if [[ ${total_lines} -gt 0 ]]; then
  overall_pct="$(python3 - "${total_covered}" "${total_lines}" <<'PY'
import sys
covered = int(sys.argv[1])
total = int(sys.argv[2])
print(f"{(covered / total) * 100:.2f}")
PY
)"
  echo "Overall line coverage: ${overall_pct}% (${total_covered}/${total_lines} lines across ${parsed_targets} targets)"
else
  echo "Overall line coverage: n/a"
fi
echo "---"

if [[ ${#failures[@]} -gt 0 ]]; then
  echo "Coverage gate failed:" >&2
  printf '  %s\n' "${failures[@]}" >&2
  exit 1
fi
