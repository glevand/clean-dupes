# clean-dupes

Search source directories for duplicate files and optionally move duplicates to a backup directory.

## Discussion

Cleaning duplicate files with the clean-dupes programs is done in a four step process allowing for complete user control of the process.

The first step is to take an inventory of candidate files contained in user specified source directories and output a list of duplicate files, the dupes list.  This can be done using either the `clean-dupes.sh --gen-dupes` command, or using the `find-dupes` helper program directly.

The second step in the process is to generate a moves list, a list of source files to be moved.  The `clean-dupes.sh --gen-moves` command can be used to automatically generate a moves list from a dupes list.  It will comment out lines based on the `--keep-pos` flag.  A moves list can also be created manually, or by manually editing a moves list that was generated by `clean-dupes.sh`.

Once a satisfactory moves list is ready the third step is to move the source files to a backup directory with the `clean-dupes.sh --move-files` command.  All data will be preserved in the move operation.

The final step is to delete unneeded files in the backup directory.

## Usage clean-dupes.sh

```
clean-dupes.sh - Move duplicate files to a backup directory.
Usage: clean-dupes.sh [flags] src-directory [src-directory]...
Option flags:
  -b --backup-dir - Backup directory. Default: '/tmp/clean-dupes'.
  -d --dupes-list - Dupes list file. Default: '/tmp/clean-dupes/dupes.lst'.
  -m --moves-list - Moves list file. Default: '/tmp/clean-dupes/moves-keep-1.lst'.
  -k --keep-pos   - Moves list keep position {1, 2, last}.  Default: '1'.
  -h --help       - Show this help and exit.
  -v --verbose    - Verbose execution.
  -g --debug      - Extra verbose execution.
Option steps:
  -1 --gen-dupes  - Find duplicate files and generate a dupes list.  Does not move files.  Default: '1'.
  -2 --gen-moves  - Generate a moves list from a dupes list.  Does not move files.  Default: '1'.
  -3 --move-files - Move files in moves list to the backup directory.  Default: ''.
```

## Usage find-dupes

```
find-dupes - Search directories and generate lists of unique and duplicate files found.
Usage: find-dupes [flags] src-directory [src-directory]...
Option flags:
  -l --list-dir   - Output lists to this directory. Default: '/tmp/find-dupes-lists'.
  -f --file-list  - Generate a list of all files found.
  -j --jobs       - Number of jobs to run in parallel. Default: '8'.
  -h --help       - Show this help and exit.
  -v --verbose    - Verbose execution.
  -V --version    - Display the program version number.
```
## Typical Moves List

As output by `clean-dupes.sh --keep-pos=last --gen-dupes --gen-moves`.

```
# Generated moves, keep-pos = 'last'
# find-dupes (clean-dupes) 20210101.ge7c22c17
# Fri 01 Jan 2021 12:00:00 AM PST

[1] /music/Led Zeppelin/Led Zeppelin II-DISK1/07-Ramble On.flac
# [2] /music/Led Zeppelin/Led Zeppelin II - Disc1/07-Ramble On.flac

[1] /music/Led Zeppelin/Led Zeppelin II-DISK1/09-Bring It On Home.flac
# [2] /music/Led Zeppelin/Led Zeppelin II - Disc1/09-Bring It On Home.flac

[1] /music/Led Zeppelin/Led Zeppelin II-DISK1/01-Whole Lotta Love.flac
# [2] /music/Led Zeppelin/Led Zeppelin II - Disc1/01-Whole Lotta Love.flac

[1] /music/Led Zeppelin/Led Zeppelin II-DISK1/04-Thank You.flac
# [2] /music/Led Zeppelin/Led Zeppelin II - Disc1/04-Thank You.flac

```

## License

All files in the [clean-dupes project](https://github.com/glevand/clean-dupes), unless otherwise noted, are covered by an [MIT Plus License](https://github.com/glevand/clean-dupes/blob/master/mit-plus-license.txt).  The text of the license describes what usage is allowed.