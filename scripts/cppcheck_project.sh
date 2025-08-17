#! /bin/bash
# https://unix.stackexchange.com/a/725492
# %pattern* strips all after pattern, #*pattern strips all before pattern, doubling ## or %% selects last match
currFolder=${PWD##*/}
fullPath=${PWD}
pathWithoutCurrFolder="${fullPath%/${currFolder}*}"
projectFolder="${pathWithoutCurrFolder##*/}"
filter=*/${projectFolder}/src/*
directories=$(find  ../src -type d)
includes= # -I arguments are ignored when a compile_commands.json is imported and cppcheck as of 2.17 does not resolve response files in compile commands, so includes are not available
for dir in ${directories} ; do
    includes+=" -I""$(pwd -P)/"$dir
done
cppcheck --project=../build/compile_commands.json --output-file=cppcheck_result_colorcoded.txt --checkers-report=cppcheck_report.txt --include=cppcheck_include.hpp --platform=win64 --std=c++11 --std=c11 --inline-suppr --enable=all --check-level=exhaustive --inconclusive --max-ctu-depth=4 --suppressions-list=cppcheck_suppressions.txt --file-filter=${filter}
# ${includes}
cat cppcheck_result_colorcoded.txt | ansi2txt > cppcheck_result.txt
rm cppcheck_result_colorcoded.txt
echo 'Result in cppcheck_result.txt'
