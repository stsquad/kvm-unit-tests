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

Usage: $0 [-g group] [-a accel] [-h] [-v]

    -g: Only execute tests in the given group
    -a: Force acceleration mode (tcg/kvm)
    -h: Output this help text
    -v: Enables verbose mode

Set the environment variable QEMU=/path/to/qemu-system-ARCH to
specify the appropriate qemu binary for ARCH-run.

EOF
}

RUNTIME_arch_run="./$TEST_DIR/run"
source scripts/runtime.bash

while getopts "g:a:hv" opt; do
    case $opt in
        g)
            only_group=$OPTARG
            ;;
        a)
            force_accel=$OPTARG
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
for_each_unittest $config run
