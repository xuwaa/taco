export TACO_INCLUDE_DIR=/home/xwb/taco/transpose/taco/include
export TACO_LIBRARY_DIR=/home/xwb/taco/transpose/taco/build/lib
rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make


