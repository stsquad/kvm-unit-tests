# Welcome to kvm-unit-tests

See http://www.linux-kvm.org/page/KVM-unit-tests for a high-level
description of this project, as well as running tests and adding
tests HOWTOs.

# Building the tests

This directory contains sources for a kvm test suite.

To create the test images do:

    ./configure
    make

in this directory. Test images are created in ./<ARCH>/*.flat

## Standalone tests

The tests can be built as standalone
To create and use standalone tests do:

    ./configure
    make standalone
    (send tests/some-test somewhere)
    (go to somewhere)
    ./some-test

'make install' will install all tests in PREFIX/share/kvm-unit-tests/tests,
each as a standalone test.


# Running the tests

Then use the runner script to detect the correct invocation and
invoke the test:

    ./x86-run ./x86/msr.flat
or:

    ./run_tests.sh

to run them all.

To select a specific qemu binary, specify the QEMU=<path>
environment variable:

    QEMU=/tmp/qemu/x86_64-softmmu/qemu-system-x86_64 ./x86-run ./x86/msr.flat

To force the use of TCG:

    ACCEL=tcg ./run_tests.sh

To force failure when KVM is not present:

    ACCEL=kvm ./run_tests.sh

To modify or disable the timeouts (see man timeout(1)):

    TIMEOUT=$DURATION ./run_tests.sh
    TIMEOUT=0 ./run_tests.sh

Any arguments past the end-of-arguments marker (--) is passed on down
to the QEMU invocation. This can of course be combined with the other
modifiers:

    ACCEL=tcg ./run_tests.sh -v -- --accel tcg,thread=multi

# Contributing

## Directory structure

    .:				configure script, top-level Makefile, and run_tests.sh
    ./scripts:		helper scripts for building and running tests
    ./lib:			general architecture neutral services for the tests
    ./lib/<ARCH>:	architecture dependent services for the tests
    ./<ARCH>:		the sources of the tests and the created objects/images

See <ARCH>/README for architecture specific documentation.

## Style

Currently there is a mix of indentation styles so any changes to
existing files should be consistent with the existing style. For new
files:

  - C: please use standard linux-with-tabs
  - Shell: use TABs for indentation

## Patches

Patches are welcome at the KVM mailing list <kvm@vger.kernel.org>.

Please prefix messages with: [kvm-unit-tests PATCH]

You can add the following to .git/config to do this automatically for you:

    [format]
        subjectprefix = kvm-unit-tests PATCH

Additionally it is helpful to have a common order of file types in
patches. Our chosen order attempts to place the more declarative files
before the code files. We also start with common code and finish with
unit test code. git-diff's orderFile feature allows us to specify the
order in a file. The orderFile we use is `scripts/git.difforder`.
Adding the config with `git config diff.orderFile
scripts/git.difforder` enables it.

Please run the kernel's ./scripts/checkpatch.pl on new patches
