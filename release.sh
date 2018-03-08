#!/bin/sh

make clean
sync
make

TARGET_DIR="release"

if [ -d "$TARGET_DIR" ]; then
	rm -rf ${TARGET_DIR}
fi

# ------- base folder -------
destDir=${TARGET_DIR}/include/fpnn/base

mkdir -p $destDir
cp -f src/base/*.h $destDir

# ------- proto folder -------
destDir=${TARGET_DIR}/include/fpnn/proto

mkdir -p $destDir
cp -f src/proto/*.h $destDir
cp -rf src/proto/msgpack $destDir
cp -rf src/proto/rapidjson $destDir

# ------- core folder -------
destDir=${TARGET_DIR}/include/fpnn/core

mkdir -p $destDir
cp -f src/core/*.h $destDir
cp -rf src/core/micro-ecc $destDir
rm -f $destDir/micro-ecc/*.c
rm -f $destDir/micro-ecc/*.o

# ------- make libfpnn -------
destDir=${TARGET_DIR}/include/fpnn/
cp -f src/fpnn.h $destDir

destDir=${TARGET_DIR}/lib

mkdir -p $destDir

fpnnLib=libfpnn.a
ar -rcs ${destDir}/$fpnnLib src/base/*.o src/proto/*.o src/core/*.o src/core/micro-ecc/*.o