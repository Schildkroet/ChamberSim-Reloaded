#!/bin/sh
# Builds manual.pdf; the program version is read from set(CSR_VERSION ...) in CMakeLists.txt.
cd "$(dirname "$0")" || exit 1
v=$(sed -n 's/^set(CSR_VERSION "\(.*\)").*/\1/p' ../../CMakeLists.txt)
for i in 1 2; do
    if ! pdflatex -interaction=nonstopmode "\\def\\programversion{${v:-0.3}}\\input{manual.tex}" >/dev/null; then
        # the errors ("! ..." with the lines after them), else the end of the log
        echo "pdflatex failed (run $i):" >&2
        if grep -q '^!' manual.log 2>/dev/null; then grep -A 6 '^!' manual.log >&2; else tail -n 40 manual.log >&2; fi
        exit 1
    fi
done
