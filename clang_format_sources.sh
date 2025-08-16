#! /bin/bash
echo "Applying clang-format"

clangFormatBin=clang-format

echo "Sources"
files=$(find ./src -name \*.cpp)
for item in $files ; do
  ${clangFormatBin} -i $item
  printf '.'
done
echo ''
echo "Headers"
files=$(find ./src -name \*.h)
for item in $files ; do
  ${clangFormatBin} -i $item
  printf '.'
done
echo ''
echo ''
echo "Done"
