ARG VERSION=latest
FROM connectedhomeip/chip-build:${VERSION}

# Setup Ameba
ARG AMEBA_DIR=/opt/ameba
RUN set -x \
    && apt-get update \
    && mkdir ${AMEBA_DIR} \
    && cd ${AMEBA_DIR} \
    && git clone --progress -b cmake_build https://github.com/pankore/ambd_sdk_with_chip_non_NDA.git \
    && cd ambd_sdk_with_chip_non_NDA \
    && git reset --hard ae7ff59 \
    && git submodule update --depth 1 --init --progress \
    && : # last line

ENV AMEBA_PATH=${AMEBA_DIR}/ambd_sdk_with_chip_non_NDA
