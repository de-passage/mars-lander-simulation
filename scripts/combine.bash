#!/usr/bin/env bash
#
# Combine C/C++ files and their dependencies into a single file.
# I use this when working on single-file solutions to online problems (codewars, codingame...)
#
# Usage:
#  combine.bash [-I./path/to/includes] [-o output.cpp] FILE...

unset source_file
declare target_file="result.cpp"
declare -a include_paths=()
declare debug_mode=false

set -e

function err() {
  if [[ -t 2 ]]; then
    echo "$(tput setaf 1)$(tput bold)${*}$(tput sgr0)" >&2
  else
    echo "$@" >&2
  fi
  exit 1
}

function debug() {
  if [[ "$debug_mode" == true ]]; then
    echo "$@" >&2
  fi
}

# Return the path to the file given in argument.
# Goes through the list of include directories to find a match
function find_file_path() {
  local target="$1"

  local target_dir="$(dirname "$target")"
  local target_base="$(basename "$target")"
  debug "Looking for '${target_dir}/${target_base}'"

  if [[ -f "${target_dir}/${target_base}" ]]; then
    debug "Found ${target_dir}/${target_base}"
    echo "${target_dir}/${target_base}"
    return
  else
    for path in "${include_paths[@]}"; do
      local expected_path="${path}/${target_dir}/${target_base}"
      debug "Looking for '${expected_path}'"
      if [[ -f "${expected_path}" ]]; then
        echo "${expected_path}"
        return
      fi
    done
  fi
  exit 1
}

while (( $# > 0 )); do
  case "$1" in
    --output)
      if [[ -z "$2" ]]; then
        err ""
      fi
      target_file="$2"
      shift
      ;;
    --debug)
      debug_mode=true
      ;;
    --*)
      err "Unhandled option '$1'"
      ;;
    -*)
      for (( i = 1; i < ${#1}; i++ )); do
        option="${1:$i:1}"
        case "$option" in
          I)
            if [[ -n "${1:$i+1}" ]]; then
              include_paths+=( "${1:$i+1}" )
              break
            elif [[ -n "$2" ]]; then
              include_paths+=( "$2" )
              shift
              break
            else
              err "Missing parameter for '-${option}'"
            fi
            ;;
          o)
            if [[ -n "${1:$i+1}" ]]; then
              target_file="${1:$i+1}"
              break
            elif [[ -n "$2" ]]; then
              target_file="$2"
              shift
              break
            else
              err "Missing parameter for '-${option}'"
            fi
            ;;
          *) err "Unhandled short form option '-${option}'";;
        esac
      done
      ;;
    *)
      source_file+=("$1")
      ;;
  esac
  shift
done

if (( ${#source_file[@]} == 0 )); then
  err "Need at least one source file"
fi

temp_file="${DEBUG_FILE:-$(mktemp)}"
if [[ "$debug_mode" == "false" ]]; then trap 'rm "$temp_file"' EXIT ; fi

cat "${source_file[@]}" > "$temp_file"

declare -A already_included=()

# Process "" includes
debug "Processing local includes"
while true; do
  includes=$(sed -n '/^\s*#include\s\+"[^"]*"/ { s/\r//g; s/^[^"]*"\([^"]*\)"/\1/p; q }' "$temp_file" )
  debug "includes:'${includes}'"
  if [[ -z "$includes" ]]; then
    break
  fi
  found=false
  while IFS= read -r line; do
    if ! filename="$(find_file_path "$line")" ; then
      continue
    fi
    found=true

    debug "Processing in '${filename}':
    ${line}"

    if [[ -n "${already_included[$filename]}" ]]; then
      debug "Already included: $filename"
      escaped_line="${line//\//\\/}"
      debug sed '/^\s*#include\s\+"'"${escaped_line}"'"/d' -i "$temp_file"''
      sed '/^\s*#include\s\+"'"${escaped_line}"'"/d' -i "$temp_file"
    else
      already_included[$filename]=included
      debug "Splicing in '$filename'"
      escaped_line="${line//\//\\/}"
      debug sed '/^\s*#include\s\+"'"${escaped_line}"'"/ { r'"${filename}"'
      :p;n;bp }' "$temp_file" -i
      sed '/^\s*#include\s\+"'"${escaped_line}"'"/ { r'"${filename}"'
      :p;n;bp }' "$temp_file" -i
    fi
  done <<< "$includes"
  if [[ "$found" == false ]]; then
    err "Could not find file '$includes'"
  fi
done

# Process <> includes, they are supposed to be system headers
# If we find them, we'll include them anyway, if not we'll add them to the list to add at the beginning of the file
debug "Processing system includes"
declare -a system_includes=()
while true; do
  includes=$(sed -n '/^\s*#include\s\+<[^>]*>/ { s/\r//g; s/^[^"]*<\([^>]*\)>/\1/p; q }' "$temp_file")
  debug "includes:$includes"
  if [[ -z "$includes" ]]; then
    break
  fi
  while IFS= read -r line; do
    if ! filename="$(find_file_path "$line")" ; then
      debug "Ignoring missing system file '$line'"
      filename="$line"
      already_included[$filename]=included
      system_includes+=("#include <$line>;")
    fi
    debug "Processing in '${filename}':
    ${line}"

    if [[ -n "${already_included[$filename]}" ]]; then
      debug "Already included: $filename"
      escaped_line="${line//\//\\/}"
      debug sed '/^\s*#include\s\+<'"${escaped_line}"'>/d' -i "$temp_file"''
      sed '/^\s*#include\s\+<'"${escaped_line}"'>/d' -i "$temp_file"''
    else
      already_included[$filename]=included
      debug "Splicing in '$filename'"
      escaped_line="${line//\//\\/}"
      debug sed '/^\s*#include\s\+<'"${escaped_line}"'>/ { r'"${filename}"':p;n;bp }' "$temp_file" -i
      sed '/^\s*#include\s\+<'"${escaped_line}"'>/ { r'"${filename}"'
      :p;n;bp }' "$temp_file" -i
    fi
  done <<< "$includes"
done

# Cleanup, pushing all remaining includes to the top of the file
sed '/^\s*#include\s\+<\([^>]*\)>/d;/^\s*#pragma\s\+once/d' -i "$temp_file"
added_includes=()
for inc in "${system_includes[@]}"; do
  if ! [[ "${inc}" =~ tracy ]]; then
    added_includes+=("${inc}")
  fi
done
sed 's/;\s*/\n/g' <<< "${added_includes[*]}"| sort | uniq > "$target_file"
cat "$temp_file" >> "$target_file"
