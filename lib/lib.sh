#!/usr/bin/env bash

sec_to_min() {
	local sec=${1}
	local min=$(( sec / 60 ))
	local frac_10=$(( (sec - min * 60) * 10 / 60 ))
	local frac_100=$(( (sec - min * 60) * 100 / 60 ))

	if (( frac_10 != 0 )); then
		unset frac_10
	fi

	echo "${min}.${frac_10}${frac_100}"
}

check_file() {
	local msg="${1}"
	local file="${2}"

	if [[ ! -f "${file}" ]]; then
		echo "${script_name}: ERROR: ${msg} not found: '${file}'" >&2
		exit 1
	fi
}

check_program() {
	local prog="${1}"
	local path="${2}"

	if ! test -x "$(command -v "${path}")"; then
		echo "${script_name}: ERROR: Please install '${prog}'." >&2
		exit 1
	fi
}

