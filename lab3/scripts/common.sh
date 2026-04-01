#!/usr/bin/env bash

readonly LAB4_DEFAULT_LIMIT=2147483647
readonly LAB4_DEFAULT_SEED=42

readonly LAB4_BUILD_FROM=100000
readonly LAB4_BUILD_TO=1500000
readonly LAB4_BUILD_STEP=100000
readonly LAB4_BUILD_COPIES=5

readonly LAB4_DIJKSTRA_FROM=2500
readonly LAB4_DIJKSTRA_TO=17500
readonly LAB4_DIJKSTRA_STEP=2500
readonly LAB4_DIJKSTRA_COPIES=3

normalize_path() {
  local path="${1//\\//}"
  if [[ "$path" =~ ^([A-Za-z]):(/.*)?$ ]]; then
    if command -v wslpath >/dev/null 2>&1; then
      wslpath -a "$path"
      return 0
    fi
    local drive="${BASH_REMATCH[1],,}"
    local rest="${BASH_REMATCH[2]}"
    printf '/%s%s\n' "$drive" "${rest:-}"
    return 0
  fi
  printf '%s\n' "$path"
}

native_path() {
  local path="$1"
  if command -v wslpath >/dev/null 2>&1; then
    wslpath -w "$path"
    return 0
  fi
  printf '%s\n' "$path"
}

die() {
  echo "$*" >&2
  exit 1
}

resolve_lab_root() {
  local script_dir="$1"
  (cd -- "$script_dir/.." && pwd -P)
}

guard_project_relative_path() {
  local lab_root="$1"
  local raw_path="$2"
  local kind="$3"
  local current_dir
  local normalized_raw
  current_dir="$(pwd -P)"
  normalized_raw="${raw_path//\\//}"

  case "$normalized_raw" in
    [A-Za-z]:/*|/*)
      return 0
      ;;
  esac

  if [[ ( "$current_dir" == "$lab_root" || "$current_dir" == "$lab_root/"* ) &&
        "$normalized_raw" == ./* ]]; then
    die "ambiguous $kind path '$raw_path' while already inside '$lab_root'; use '${normalized_raw#./}' instead"
  fi
}

resolve_binary() {
  local lab_root="$1"
  local name="$2"
  local plain_path="$lab_root/build/bin/$name"
  local exe_path="$plain_path.exe"

  if [[ -x "$plain_path" ]]; then
    printf '%s\n' "$plain_path"
    return 0
  fi
  if [[ -x "$exe_path" ]]; then
    printf '%s\n' "$exe_path"
    return 0
  fi

  die "missing binary '$name'; build the project first with: make -C \"$lab_root\" all"
}

resolve_python_bin() {
  if command -v python3 >/dev/null 2>&1; then
    printf '%s\n' python3
    return 0
  fi
  if command -v python >/dev/null 2>&1; then
    printf '%s\n' python
    return 0
  fi

  die "python not found; install python3 with matplotlib first"
}
