FROM ghcr.io/wiiu-env/devkitppc:20231112

ENV DEBIAN_FRONTEND=noninteractive \
 PATH=$DEVKITPPC/bin:$PATH \
 WUT_ROOT=$DEVKITPRO/wut \
 CURL_VER=8.6.0 \
 NGHTTP2_VER=1.60.0

WORKDIR /

# Install apt-fast
RUN mkdir -p /usr/share/man/man1 /usr/share/man/man2 && \
 /bin/bash -c "$(curl -sL https://raw.githubusercontent.com/V10lator/NUSspli/master/apt-fast/install.sh)"

# Upgrade the systen
RUN apt-fast -y --no-install-recommends update && \
 apt-fast -y --no-install-recommends upgrade

# Install the requirements to package the homebrew
RUN apt-fast -y install --no-install-recommends autoconf automake libtool openjdk-11-jre-headless python3-pycurl && \
 apt-fast clean

# Install nghttp2 for HTTP/2 support (WUT don't include this)
RUN curl -LO https://github.com/nghttp2/nghttp2/releases/download/v1.60.0/nghttp2-$NGHTTP2_VER.tar.xz && \
  mkdir nghttp2 && \
  tar xf nghttp2-$NGHTTP2_VER.tar.xz -C nghttp2/ --strip-components=1 && \
  rm -f nghttp2-$NGHTTP2_VER.tar.xz && \
  cd nghttp2 && \
  autoreconf -fi && \
  automake && \
  autoconf && \
  ./configure  \
--enable-lib-only \
--prefix=$DEVKITPRO/portlibs/wiiu/ \
--enable-static \
--disable-threads \
--host=powerpc-eabi \
CFLAGS="-mcpu=750 -meabi -mhard-float -Ofast -ffunction-sections -fdata-sections" \
CXXFLAGS="-mcpu=750 -meabi -mhard-float -Ofast -ffunction-sections -fdata-sections" \
CPPFLAGS="-D__WIIU__ -D__WUT__ -I$DEVKITPRO/wut/include" \
LDFLAGS="-L$DEVKITPRO/wut/lib" \
LIBS="-lwut -lm" \
CC=$DEVKITPPC/bin/powerpc-eabi-gcc \
AR=$DEVKITPPC/bin/powerpc-eabi-ar \
RANLIB=$DEVKITPPC/bin/powerpc-eabi-ranlib \
PKG_CONFIG=$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-pkg-config && \
  make -j$(nproc) && \
  make -j$(nproc) install && \
  cd .. && \
  rm -rf nghttp2

# Install libCURL since WUT doesn't ship with the latest version
RUN curl -LO https://curl.se/download/curl-$CURL_VER.tar.xz && \
 mkdir /curl && \
 tar xJf curl-$CURL_VER.tar.xz -C /curl --strip-components=1 && \
 rm -f curl-$CURL_VER.tar.xz && \
 cd curl && \
 autoreconf -fi && ./configure \
--prefix=$DEVKITPRO/portlibs/wiiu/ \
--host=powerpc-eabi \
--enable-static \
--disable-threaded-resolver \
--disable-pthreads \
--with-mbedtls=$DEVKITPRO/portlibs/wiiu/ \
--disable-ipv6 \
--disable-unix-sockets \
--disable-socketpair \
--disable-ntlm-wb \
--with-nghttp2=$DEVKITPRO/portlibs/wiiu/ \
--without-libpsl \
CFLAGS="-mcpu=750 -meabi -mhard-float -Ofast -ffunction-sections -fdata-sections" \
CXXFLAGS="-mcpu=750 -meabi -mhard-float -Ofast -ffunction-sections -fdata-sections" \
CPPFLAGS="-D__WIIU__ -D__WUT__ -I$DEVKITPRO/wut/include" \
LDFLAGS="-L$DEVKITPRO/wut/lib" \
LIBS="-lwut -lm" \
CC=$DEVKITPPC/bin/powerpc-eabi-gcc \
AR=$DEVKITPPC/bin/powerpc-eabi-ar \
RANLIB=$DEVKITPPC/bin/powerpc-eabi-ranlib \
PKG_CONFIG=$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-pkg-config && \
 cd lib && \
 make -j$(nproc) install && \
 cd ../include && \
 make -j$(nproc) install && \
 cd ../.. && \
 rm -rf curl

COPY --from=ghcr.io/wiiu-env/libmocha:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/librpxloader:20230621 /artifacts $DEVKITPRO

RUN git config --global --add safe.directory /project && \
  git config --global --add safe.directory /project/SDL_FontCache && \
  git config --global --add safe.directory /project/zlib

WORKDIR /project
