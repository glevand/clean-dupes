#!/usr/bin/env bash

usage() {
	echo "Usage: ${script_name} build-dir" >&2
}

on_exit() {
	local result=${1}

	local sec="${SECONDS}"

	set +x
	echo "${script_name}: Done: ${result}, ${sec} sec." >&2
}

on_err() {
	local f_name=${1}
	local line_no=${2}
	local err_no=${3}

	echo "${script_name}: ERROR: (${err_no}) at ${f_name}:${line_no}." >&2
	exit "${err_no}"
}

#===============================================================================
export PS4='\[\e[0;33m\]+ ${BASH_SOURCE##*/}:${LINENO}:(${FUNCNAME[0]:-main}):\[\e[0m\] '
script_name="${0##*/}"

SECONDS=0
start_time="$(date +%Y.%m.%d-%H.%M.%S)"

trap "on_exit 'Failed'" EXIT
trap 'on_err ${FUNCNAME[0]:-main} ${LINENO} ${?}' ERR
set -eE
set -o pipefail
set -o nounset

# TESTS_TOP="$(realpath "${BASH_SOURCE%/*}")"

build_dir="${1}"

if [[ "${build_dir}" == '-h' || "${build_dir}" == '--help' ]]; then
        usage
        exit 0
fi

if [[ ! -d "${build_dir}" ]]; then
        echo "${script_name}: ERROR: Bad build-dir: '${build_dir}'" >&2
        exit 1
fi

build_dir="$(realpath "${build_dir}")"
cd "${build_dir}"

{
	echo ''
	echo '==================================================='
	echo "${script_name} (clean-dupes) - ${start_time}"
	echo '==================================================='
}

echo ''
echo "--- find-dupes ---"
"${build_dir}/install/bin/find-dupes" --list-dir="${build_dir}/test-out" /usr/bin

echo ''
echo "--- ls lists ---"
ls -l "${build_dir}/test-out/"

echo ''
echo "--- cat dupes.lst ---"
cat "${build_dir}/test-out/dupes.lst"

echo ''
echo "--- Done ---"

trap "on_exit 'Success'" EXIT
exit 0
