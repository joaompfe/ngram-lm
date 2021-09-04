#!/bin/sh

PROJECT_DIR=.
REPAIRED_WHEELS_HOUSE="${PROJECT_DIR}"/build/repairedwheelhouse
printf "PyPI username: "
read -r USERNAME
stty -echo
printf "Password: "
read -r PASSWORD
stty echo

### ATTENTION: Uploading to PyPI. Not PyPI test
for WHEEL in "${REPAIRED_WHEELS_HOUSE}"/*.whl; do
    twine upload \
        -u "${USERNAME}" -p "${PASSWORD}" \
        "${WHEEL}" # --repository testpypi
done
