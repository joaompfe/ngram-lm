#!/bin/bash
docker run --rm -v `pwd`:/ngram_lm quay.io/pypa/manylinux2010_x86_64 \
    /bin/bash /ngram_lm/src/python/docker/build-wheels.sh