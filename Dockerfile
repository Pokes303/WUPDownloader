# build wut
FROM wiiuenv/devkitppc:20221228 AS final

ENV DEBIAN_FRONTEND=noninteractive \
 PATH=$DEVKITPPC/bin:$PATH \
 WUT_ROOT=$DEVKITPRO/wut
WORKDIR /
COPY --from=wiiuenv/libmocha:20220919112600f3c45c /artifacts $DEVKITPRO
COPY --from=wiiuenv/librpxloader:20220903141341abbf92 /artifacts $DEVKITPRO

RUN mkdir -p /usr/share/man/man1 /usr/share/man/man2 && \
 /bin/bash -c "$(curl -sL https://raw.githubusercontent.com/V10lator/NUSspli/master/apt-fast/install.sh)" && \
 apt-fast -y --no-install-recommends update && \
 apt-fast -y --no-install-recommends upgrade && \
 apt-fast -y install --no-install-recommends autoconf automake libtool openjdk-11-jre-headless && \
 apt-fast clean && \
 git clone --recursive https://github.com/yawut/libromfs-wiiu --single-branch && \
 cd libromfs-wiiu && \
 make -j$(nproc) && \
 make install

WORKDIR /project
