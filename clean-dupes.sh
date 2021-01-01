#!/usr/bin/env bash

usage() {
	local old_xtrace
	old_xtrace="$(shopt -po xtrace || :)"
	set +o xtrace
	echo "${script_name} - Move duplicate files to a backup directory." >&2
	echo "Usage: ${script_name} [flags] src-directory [src-directory]..." >&2
	echo "Option flags:" >&2
	echo "  -b --backup-dir - Backup directory. Default: '${backup_dir}'." >&2
	echo "  -d --dupes-list - Dupes list file. Default: '${dupes_list}'." >&2
	echo "  -m --moves-list - Moves list file. Default: '${moves_list}'." >&2
	echo "  -k --keep-pos   - Moves list keep position {1, 2, last}.  Default: '${keep_pos}'." >&2
	echo "  -h --help       - Show this help and exit." >&2
	echo "  -v --verbose    - Verbose execution." >&2
	echo "  -g --debug      - Extra verbose execution." >&2
	echo "Option steps:" >&2
	echo "  -1 --gen-dupes  - Find duplicate files and generate a dupes list.  Does not move files.  Default: '${gen_dupes}'." >&2
	echo "  -2 --gen-moves  - Generate a moves list from a dupes list.  Does not move files.  Default: '${gen_moves}'." >&2
	echo "  -3 --move-files - Move files in moves list to the backup directory.  Default: '${move_files}'." >&2
	echo "Send bug reports to: Geoff Levand <geoff@infradead.org>." >&2
	eval "${old_xtrace}"
}

process_opts() {
	local short_opts="b:d:m:k:hvg123"
	local long_opts="backup-dir:,dupes-list:,moves-list:,keep-pos:,\
help,verbose,debug,\
gen-dupes,gen-moves,move-files"

	local opts
	opts=$(getopt --options ${short_opts} --long ${long_opts} -n "${script_name}" -- "$@")

	eval set -- "${opts}"

	while : ; do
		#Secho "${FUNCNAME[0]}: @${1}@ @${2}@"
		case "${1}" in
		-b | --backup-dir)
			backup_dir="${2}"
			shift 2
			;;
		-d | --dupes-list)
			dupes_list="${2}"
			shift 2
			;;
		-m | --moves-list)
			moves_list="${2}"
			shift 2
			;;
		-k | --keep-pos)
			keep_pos="${2}"
			shift 2
			;;
		-h | --help)
			usage=1
			shift
			;;
		-v | --verbose)
			verbose=1
			shift
			;;
		-g | --debug)
			verbose=1
			debug=1
			set -x
			shift
			;;
		-1 | --gen-dupes)
			gen_dupes=1
			shift
			;;
		-2 | --gen-moves)
			gen_moves=1
			shift
			;;
		-3 | --move-files)
			move_files=1
			shift
			;;
		--)
			shift
			while [[ ${1} ]] ; do
				#echo "fill src_dirs: @${1}@"
				src_dirs+=("${1}")
				shift
			done
			break
			;;
		*)
			echo "${script_name}: ERROR: Internal opts: '${*}'" >&2
			exit 1
			;;
		esac
	done
}

on_exit() {
	local result=${1}
	local sec=${SECONDS}

	set +x
	echo "${script_name}: Done: ${result}, ${sec} sec ($(sec_to_min "${sec}") min)." >&2
}

check_find_dupes() {
	if ! test -x "$(command -v "${find_dupes}")"; then
		echo "${script_name}: ERROR: Please install find-dupes program." >&2
		exit 1
	fi
}

check_pos() {
	local kp=${1}

	case "${kp}" in
	1 | 2 | last)
		;;
	*)
		echo "${script_name}: ERROR: Bad --keep-pos '${kp}'." >&2
		exit 1
		;;
	esac
}

check_src_dirs() {
	local sd=("${@}")

	#echo "${FUNCNAME[0]}: src dirs: @${@}@" >&2
	#echo "${FUNCNAME[0]}: count: @${#sd[@]}@" >&2

	if [[ ${#sd[@]} -eq 0 ]]; then
		echo "${script_name}: ERROR: No source directories given." >&2
		usage
		exit 1
	fi

	for ((i = 0; i < ${#sd[@]}; i++)); do
		if [[ -d "${sd[i]}" ]]; then
			[[ ${verbose} ]] && echo "${FUNCNAME[0]}: [$((i + 1))] '${sd[i]}' OK." >&2
		else
			echo "${script_name}: ERROR: Bad source directory: [$((i + 1))] '${sd[i]}'." >&2
			usage
			exit 1
		fi
	done
	[[ ${verbose} ]] && echo "" >&2
	return 0
}

check_backup() {
	local bd=${1}

	if [[ -e ${bd} ]]; then
		echo "${script_name}: ERROR: Backup directory exists: '${bd}'." >&2
		exit 1
	fi
}

process_group() {
	local group=("${@}")
	local count="${#group[@]}"
	#local debug=1

	if [[ ${count} -eq 0 ]]; then
		echo "${FUNCNAME[0]}: ERROR: Empty group." >&2
		exit 1
	fi

	local i
	if [[ "${keep_pos}" == "last" ]]; then
		for ((i = 0; i < count - 1; i++)); do
			[[ ${debug} ]] && echo "${FUNCNAME[0]}: Move (last) [$((i + 1))] '${group[i]}'" >&2
			echo "[$((i + 1))] ${group[i]}" >> "${moves_list}"
		done
		[[ ${debug} ]] && echo "${FUNCNAME[0]}: Keep (last) [$((i + 1))] '${group[i]}'" >&2
		echo "# [$((i + 1))] ${group[i]}" >> "${moves_list}"
	else
		for ((i = 0; i < count; i++)); do
			if [[ $((i + 1)) -ne ${keep_pos} ]]; then
				[[ ${debug} ]] && echo "${FUNCNAME[0]}: Move (${keep_pos}) [$((i + 1))] '${group[i]}'" >&2
				echo "[$((i + 1))] ${group[i]}" >> "${moves_list}"
			else
				[[ ${debug} ]] && echo "${FUNCNAME[0]}: Keep (${keep_pos}) [$((i + 1))] '${group[i]}'" >&2
				echo "# [$((i + 1))] ${group[i]}" >> "${moves_list}"
			fi
		done
	fi
	echo "" >> "${moves_list}"
}

generate_moves() {
	declare -a group
	local line_in
	local line_no=0
	#local debug=1

	echo "${script_name}: INFO: Generating moves list..." >&2

	echo "# Generated moves, keep-pos = '${keep_pos}'" > "${moves_list}"
	echo "# $(date '+%a %d %b %Y %r %Z')" >> "${moves_list}"

	while read -r line_in; do
		((line_no += 1))

		if [[ "${line_in:0:1}" == "#" ]]; then
			[[ ${debug} ]] && echo "${FUNCNAME[0]}:${line_no}: Skip '${line_in}'" >&2
			echo "${line_in}" >> "${moves_list}"
			continue
		fi

		if [[ ! ${line_in} ]]; then
			if [[ ${#group[@]} -eq 0 ]]; then
				[[ ${debug} ]] && echo "${FUNCNAME[0]}:${line_no}: Empty group." >&2
				echo '' >> "${moves_list}"
			else
				[[ ${debug} ]] && echo "${FUNCNAME[0]}:${line_no}: Have group (${#group[@]})." >&2
				process_group "${group[@]}"
				group=()
			fi
			continue
		fi

		local regex="^\[([[:digit:]]+)\] ([[:print:]]+)$"

		if [[ ! "${line_in}" =~ ${regex} ]]; then
			echo "${FUNCNAME[0]}:${line_no}: ERROR: Match failed: '${line_in}'" >&2
			exit 1
		fi

		local pos
		local file
		
		pos="${BASH_REMATCH[1]}"
		file="${BASH_REMATCH[2]}"

		[[ ${debug} ]] && echo "${FUNCNAME[0]}:${line_no}: [${pos}] = '${file}'" >&2

		group+=("${file}")

	done < "${dupes_list}"

	if [[ ${line_in} ]]; then
		process_group "${group[@]}"
	fi
}

move_files() {
	local line_in
	local line_no=0
	#local debug=1

	while read -r line_in; do
		(( line_no = line_no + 1 ))

		if [[ "${line_in:0:1}" == "#" || ! ${line_in} ]]; then
			[[ ${debug} ]] && echo "${FUNCNAME[0]}:${line_no}: Skip '${line_in}'" >&2
			continue
		fi

		local regex="^\[([[:digit:]]+)\] ([[:print:]]+)$"

		if [[ ! "${line_in}" =~ ${regex} ]]; then
			echo "${FUNCNAME[0]}:${line_no}: ERROR: Match failed: '${line_in}'" >&2
			exit 1
		fi

		local pos
		local file
		
		pos="${BASH_REMATCH[1]}"
		file="${BASH_REMATCH[2]}"
		file="$(realpath "${file}")"

		[[ ${debug} ]] && echo "${FUNCNAME[0]}:${line_no}: Move [${pos}] = '${file}'" >&2

		if [[ ! -e ${file} ]]; then
			echo "${FUNCNAME[0]}:${line_no}: ERROR: No Such file: '${line_in}'" >&2
			exit 1
		fi

		local dest_dir
		dest_dir="${backup_dir}${file%/*}"
		mkdir -p "${dest_dir}"
		mv --verbose "${file}" "${dest_dir}/"

	done < "${moves_list}"
}

#===============================================================================
export PS4='\[\e[0;33m\]+ ${BASH_SOURCE##*/}:${LINENO}:(${FUNCNAME[0]:-"?"}):\[\e[0m\] '
script_name="${0##*/}"

SCRIPTS_TOP=${SCRIPTS_TOP:-"$(cd "${BASH_SOURCE%/*}" && pwd)"}
SECONDS=0

source "${SCRIPTS_TOP}/lib/lib.sh"

trap "on_exit 'Failed.'" EXIT
set -e
set -o pipefail

start_time="$(date +%Y.%m.%d-%H.%M.%S)"

find_dupes="${find_dupes:-find-dupes}"

declare -a src_dirs

process_opts "${@}"

backup_dir="${backup_dir:-/tmp/${script_name%.sh}-${start_time}}"
backup_dir="$(realpath "${backup_dir}")"

dupes_list="${dupes_list:-${backup_dir}/dupes.lst}"
dupes_list="$(realpath --canonicalize-missing "${dupes_list}")"

keep_pos=${keep_pos:-1}
moves_list="${moves_list:-${dupes_list%/*}/moves-keep-${keep_pos}.lst}"
moves_list="$(realpath --canonicalize-missing "${moves_list}")"

if [[ ! ${gen_dupes} && ! ${gen_moves} && ! ${move_files} ]]; then
	gen_dupes=1
	gen_moves=1
fi

if [[ ${usage} ]]; then
	usage
	trap - EXIT
	exit 0
fi

if [[ ${gen_dupes} ]]; then
	check_program "find-dupes" "${find_dupes}"
	check_backup "${backup_dir}"
	check_src_dirs "${src_dirs[@]}"

	mkdir -p "${backup_dir}"
	"${find_dupes}" --verbose --file-list --list-dir="${backup_dir}" "${src_dirs[@]}"

	if [[ "${backup_dir}/dupes.lst" != "${dupes_list}" ]]; then
		mv -f "${backup_dir}/dupes.lst" "${dupes_list}"
	fi

	if [[ ${verbose} ]]; then
		old_xtrace="$(shopt -po xtrace || :)"
		set +o xtrace
		echo '---| dupes.lst |----------------' >&2
		cat "${dupes_list}" >&2
		echo '--------------------------------' >&2
		eval "${old_xtrace}"
	fi
fi

if [[ ${gen_moves} ]]; then
	check_pos "${keep_pos}"
	check_file '--dupes-list' "${dupes_list}"
	generate_moves

	if [[ ${verbose} ]]; then
		old_xtrace="$(shopt -po xtrace || :)"
		set +o xtrace
		echo '---| moves.lst |----------------' >&2
		cat "${moves_list}" >&2
		echo '--------------------------------' >&2
		eval "${old_xtrace}"
	fi
fi

if [[ ${move_files} ]]; then
	check_file '--moves-list' "${moves_list}"
	move_files
	echo "${script_name}: Duplicate files moved to '${backup_dir}'."
	trap $'on_exit "Success"' EXIT
	exit 0
fi

	echo "${script_name}: File lists generated in '${moves_list%/*}'."
	trap $'on_exit "Success"' EXIT
exit 0
