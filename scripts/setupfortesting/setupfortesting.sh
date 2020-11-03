cd "${0%/*}"
export CXX=g++-7
export CC=gcc-7
#./powerpolicy.sh  performance
./disablehyperthreading.sh
./turboboost.sh on
