# this is a basic shell for the c projects in this course.
# it should have build-essential and gcc included implicitly
# thanks to stdenv (see nixpkgs manual for more info)
#
# source: https://ryantm.github.io/nixpkgs/stdenv/stdenv/#sec-stdenv-phases
# The standard environment provides the following packages:
#
#     The GNU C Compiler, configured with C and C++ support.
#     GNU coreutils (contains a few dozen standard Unix commands).
#     GNU findutils (contains find).
#     GNU diffutils (contains diff, cmp).
#     GNU sed.
#     GNU grep.
#     GNU awk.
#     GNU tar.
#     gzip, bzip2 and xz.
#     GNU Make.
#     Bash. This is the shell used for all builders in the Nix Packages collection. Not using /bin/sh removes a large source of portability problems.
#     The patch command.
#
# On Linux, stdenv also includes the patchelf utility.
#
{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  buildInputs = [
    # these are for the man pages for linux and glibc
    pkgs.man-pages
    pkgs.man-pages-posix

    pkgs.valgrind # tool for checking memory leaks

    pkgs.xxd # tool for looking at binary files
  ];
}
