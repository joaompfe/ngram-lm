#!/bin/bash
set -e -u -x

PROJECT_DIR=/ngram_lm
CMAKE_BUILD_DIR="${PROJECT_DIR}"/build/cmake/release
PY_SRC_DIR="${PROJECT_DIR}"/src/python
WHEELS_HOUSE="${PROJECT_DIR}"/build/wheelhouse
REPAIRED_WHEELS_HOUSE="${PROJECT_DIR}"/build/repairedwheelhouse
PLAT=manylinux2010_x86_64

function repair_wheel {
    wheel="$1"
    if ! auditwheel show "$wheel"; then
        echo "Skipping non-platform wheel $wheel"
    else
        auditwheel repair "$wheel" --plat "$PLAT" -w "${REPAIRED_WHEELS_HOUSE}"
    fi
}

# Build ngram_lm shared lib
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_DEPENDS_USE_COMPILER=FALSE -G "CodeBlocks - Unix Makefiles" . -B "${CMAKE_BUILD_DIR}"
cmake --build "${CMAKE_BUILD_DIR}"  --target ngram_lm -- -j 6

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"${CMAKE_BUILD_DIR}/src/c"
export NGRAM_LM_DIR="${PROJECT_DIR}"

rm -rf "${WHEELS_HOUSE}"
rm -rf "${REPAIRED_WHEELS_HOUSE}"

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    "${PYBIN}/pip" install -r "${PY_SRC_DIR}/dev-requirements.txt"
    "${PYBIN}/pip" wheel "${PY_SRC_DIR}" --no-deps -w "${WHEELS_HOUSE}"
done

# Bundle external shared libraries into the wheels
for whl in "${WHEELS_HOUSE}"/*.whl; do
    repair_wheel "$whl"
done

# Install packages and test
# for PYBIN in /opt/python/*/bin/; do
#     "${PYBIN}/pip" install python-manylinux-demo --no-index -f "${PROJECT_DIR}"/build/python/wheelhouse
#     (cd "$HOME"; "${PYBIN}/nosetests" pymanylinuxdemo)
# done