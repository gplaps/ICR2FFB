#! /bin/bash
echo "clang-tidy check"
# from: https://unix.stackexchange.com/a/446200
clangTidyBin=clang-tidy
checks=clang-diagnostic-*,clang-analyzer-*
toolchainRoot=/usr/bin/
systemInclude1=${toolchainRoot}../x86_64-w64-mingw32/include/c++/v1
systemInclude2=${toolchainRoot}../x86_64-w64-mingw32/include

# array of files, assuming at least 1 item exists
files=($(find ../src -name \*.cpp))

currDir=$(dirname -- $(readlink -fn -- "$0"))
cd "${currDir}/../build-clang"

# task
task() { # $1 = idWorker, $2 = asset
  echo "Worker $1: $2"
  echo "$2:" >> ../scripts/tidy_err$1.log
  cmd_invokation="${toolchainRoot}${clangTidyBin} --config-file=${currDir}/../.clang-tidy --extra-arg=--target=x86_64-w64-windows-gnu --extra-arg-before=--driver-mode=g++ --extra-arg-before=-isystem${systemInclude1} --extra-arg-before=-isystem${systemInclude2} --extra-arg=-resource-dir=${toolchainRoot}../lib/clang/20 $2"
  ${cmd_invokation} >> ../scripts/tidy_log$1.log 2>> ../scripts/tidy_err$1.log
}

nVirtualCores=$(nproc --all)
nWorkers=$(( $nVirtualCores * 1 )) # 1 process per core

worker() { # $1 = idWorker
  echo "Worker $1 GO!"
  idAsset=0
  for asset in "${files[@]}"; do
    # split assets among workers (using modulo); each worker will go through
    # the list and select the asset only if it belongs to that worker
    (( idAsset % nWorkers == $1 )) && task $1 "$asset"
    (( idAsset++ ))
  done
  echo "    Worker $1 ALL DONE!"
}

for (( idWorker=0; idWorker<nWorkers; idWorker++ )); do
  # start workers in parallel, use 1 process for each
  worker $idWorker &
done
wait # until all workers are done

cd ${currDir}
echo ''
echo "Done"
