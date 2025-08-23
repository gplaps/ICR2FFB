#! /bin/bash
echo "clang-tidy check"
clangTidyBin=clang-tidy
checks=clang-diagnostic-*,clang-analyzer-*
toolchainRoot=/usr/bin/
systemInclude1=${toolchainRoot}../x86_64-w64-mingw32/include/c++/v1
systemInclude2=${toolchainRoot}../x86_64-w64-mingw32/include

files=$(find ../src -name \*.cpp)
currDir=$(dirname -- $(readlink -fn -- "$0"))
cd "${currDir}/../build"
for item in $files ; do
  echo ${item}
  echo "${item}:" >> ../scripts/tidy_err.log
  cmd_invokation="${toolchainRoot}${clangTidyBin} --config-file=${currDir}/../.clang-tidy --extra-arg=--target=x86_64-w64-windows-gnu --extra-arg-before=--driver-mode=g++ --extra-arg-before=-isystem${systemInclude1} --extra-arg-before=-isystem${systemInclude2} --extra-arg=-resource-dir=${toolchainRoot}../lib/clang/19 $item"
  ${cmd_invokation} >> ../scripts/tidy_log.log 2>> ../scripts/tidy_err.log
done
cd ${currDir}
echo ''
echo "Done"
