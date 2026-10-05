#!/bin/sh
# Builds manual.pdf; the program version is read from set(GRT_VERSION ...) in CMakeLists.txt.
cd "$(dirname "$0")" || exit 1
v=$(sed -n 's/^set(GRT_VERSION "\(.*\)").*/\1/p' ../../CMakeLists.txt)
for i in 1 2; do pdflatex -interaction=nonstopmode "\\def\\programversion{${v:-0.3}}\\input{manual.tex}" >/dev/null || exit 1; done
