#!/bin/bash

verbose="no"

if [ ! -f config.mak ]; then
    echo "run ./configure && make first. See ./configure -h"
    exit 1
fi
source config.mak
source scripts/functions.bash

function usage()
{
cat <<EOF

Usage: $0 [-g group] [-h] [-v] [-- QEMU options]

    -g: Only execute tests in the given group
    -h: Output this help text
    -v: Enables verbose mode

Set the environment variable QEMU=/path/to/qemu-system-ARCH to
specify the appropriate qemu binary for ARCH-run.

All options specified after -- are passed on to QEMU.

EOF
}

RUNTIME_arch_run="./$TEST_DIR/run"
source scripts/runtime.bash

while getopts "g:hv" opt; do

    case $opt in
        g)
            only_group=$OPTARG
            ;;
        h)
            usage
            exit
            ;;
        v)
            verbose="yes"
            ;;
        *)
            exit 1
            ;;
    esac
done

# Any options left for QEMU?
shift $((OPTIND-1))
if [ "$#" -gt  0 ]; then
    extra_opts="$@"
fi

RUNTIME_log_stderr () { cat >> test.log; }
RUNTIME_log_stdout () {
    if [ "$PRETTY_PRINT_STACKS" = "yes" ]; then
        ./scripts/pretty_print_stacks.py $1 >> test.log
    else
        cat >> test.log
    fi
}


config=$TEST_DIR/unittests.cfg
rm -f test.log
printf "BUILD_HEAD=$(cat build-head)\n\n" > test.log
for_each_unittest $config run "$extra_opts"
