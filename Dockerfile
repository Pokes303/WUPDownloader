# build wut
FROM devkitpro/devkitppc:20220128 AS final

ENV DEBIAN_FRONTEND=noninteractive
RUN mkdir -p /usr/share/man/man1 /usr/share/man/man2 && \
 apt-get update && \
 apt-get -y install --no-install-recommends wget tar autoconf automake libtool openjdk-11-jre && rm -rf /var/lib/apt/lists/*

ENV PATH=$DEVKITPPC/bin:$PATH

WORKDIR /
RUN git clone https://github.com/devkitPro/wut
WORKDIR /wut
RUN git checkout cd6b4fb45d054d53af92bc0b3685e8bd9f01445d && make -j$(nproc) && make install
ENV WUT_ROOT=$DEVKITPRO/wut

# build SDL2
WORKDIR /
RUN git clone -b wiiu-2.0.9 --single-branch https://github.com/yawut/SDL
WORKDIR /SDL
RUN mkdir build
WORKDIR /SDL/build
RUN /opt/devkitpro/portlibs/wiiu/bin/powerpc-eabi-cmake .. -DCMAKE_INSTALL_PREFIX=$DEVKITPRO/portlibs/wiiu && \
 make -j$(nproc) && make install

ARG openssl_ver=1.1.1n

RUN wget https://www.openssl.org/source/openssl-$openssl_ver.tar.gz && mkdir /openssl && tar xf openssl-$openssl_ver.tar.gz -C /openssl --strip-components=1
WORKDIR /openssl

RUN echo 'diff --git a/Configurations/10-main.conf b/Configurations/10-main.conf\n\
index 61c6689..efe686a 100644\n\
--- a/Configurations/10-main.conf\n\
+++ b/Configurations/10-main.conf\n\
@@ -627,6 +627,27 @@ my %targets = (\n\
         shared_extension => ".so",\n\
     },\n\
 \n\
+### Wii U target\n\
+    "wiiu" => {\n\
+        inherit_from     => [ "BASE_unix" ],\n\
+        CC               => "$ENV{DEVKITPPC}/bin/powerpc-eabi-gcc",\n\
+        CXX              => "$ENV{DEVKITPPC}/bin/powerpc-eabi-g++",\n\
+        AR               => "$ENV{DEVKITPPC}/bin/powerpc-eabi-ar",\n\
+        CFLAGS           => picker(default => "-Wall",\n\
+                                   debug   => "-O0 -g",\n\
+                                   release => "--Ofast -pipe"),\n\
+        CXXFLAGS         => picker(default => "-Wall",\n\
+                                   debug   => "-O0 -g",\n\
+                                   release => "--Ofast -pipe"),\n\
+        LDFLAGS          => "-L$ENV{DEVKITPRO}/wut/lib",\n\
+        cflags           => add("-mcpu=750 -meabi -mhard-float -ffunction-sections -fdata-sections"),\n\
+        cxxflags         => add("-std=c++11"),\n\
+        lib_cppflags     => "-DOPENSSL_USE_NODELETE -DB_ENDIAN -DNO_SYS_UN_H -DNO_SYSLOG -D__WIIU__ -D__WUT__ -I$ENV{DEVKITPRO}/wut/include",\n\
+        ex_libs          => add("-lwut -lm"),\n\
+        bn_ops           => "BN_LLONG RC4_CHAR",\n\
+        asm_arch         => '"'"'ppc32'"'"',\n\
+    },\n\
+\n ####\n #### Variety of LINUX:-)\n ####\n\
diff --git a/crypto/rand/rand_unix.c b/crypto/rand/rand_unix.c\n\
index 0f45251..d303e8e 100644\n\
--- a/crypto/rand/rand_unix.c\n\
+++ b/crypto/rand/rand_unix.c\n\
@@ -202,6 +202,41 @@ void rand_pool_keep_random_devices_open(int keep)\n\
 {\n\
 }\n\
 \n\
+# elif defined(__WIIU__)\n\
+\n\
+#include <coreinit/time.h>\n\
+\n\
+size_t rand_pool_acquire_entropy(RAND_POOL *pool)\n\
+{\n\
+    int i;\n\
+    size_t bytes_needed;\n\
+    unsigned char v;\n\
+\n\
+    bytes_needed = rand_pool_bytes_needed(pool, 4 /*entropy_factor*/);\n\
+\n\
+    for (i = 0; i < bytes_needed; i++) {\n\
+        srand(OSGetSystemTick());\n\
+        v = rand() & 0xff;\n\
+\n\
+        rand_pool_add(pool, &v, sizeof(v), 2);\n\
+    }\n\
+\n\
+    return rand_pool_entropy_available(pool);\n\
+}\n\
+\n\
+int rand_pool_init(void)\n\
+{\n\
+    return 1;\n\
+}\n\
+\n\
+void rand_pool_cleanup(void)\n\
+{\n\
+}\n\
+\n\
+void rand_pool_keep_random_devices_open(int keep)\n\
+{\n\
+}\n\
+\n # else\n\
 \n #  if defined(OPENSSL_RAND_SEED_EGD) && \\\n\
diff --git a/crypto/uid.c b/crypto/uid.c\n\
index a9eae36..4a81d98 100644\n\
--- a/crypto/uid.c\n\
+++ b/crypto/uid.c\n\
@@ -10,7 +10,7 @@\n #include <openssl/crypto.h>\n #include <openssl/opensslconf.h>\n\
 \n\
-#if defined(OPENSSL_SYS_WIN32) || defined(OPENSSL_SYS_VXWORKS) || defined(OPENSSL_SYS_UEFI)\n\
+#if defined(OPENSSL_SYS_WIN32) || defined(OPENSSL_SYS_VXWORKS) || defined(OPENSSL_SYS_UEFI) || defined(__WIIU__)\n\
 \n\
 int OPENSSL_issetugid(void)\n\
 {\
' >> wiiu.patch && git apply wiiu.patch && \
 ./Configure wiiu \
  no-threads no-shared no-asm no-ui-console no-unit-test no-tests no-buildtest-c++ no-external-tests no-autoload-config \
  --with-rand-seed=none -static --prefix=$DEVKITPRO/portlibs/wiiu --openssldir=openssldir && \
 make build_generated && make libssl.a libcrypto.a -j$(nproc)

# build curl
WORKDIR /
ARG curl_ver=7.83.0

RUN wget https://curl.se/download/curl-$curl_ver.tar.gz && mkdir /curl && tar xf curl-$curl_ver.tar.gz -C /curl --strip-components=1
WORKDIR /curl

RUN autoreconf -fi && ./configure \
--prefix=$DEVKITPRO/portlibs/wiiu/ \
--host=powerpc-eabi \
--enable-static \
--disable-threaded-resolver \
--disable-pthreads \
--with-ssl=$DEVKITPRO/portlibs/wiiu/ \
--disable-ipv6 \
--disable-unix-sockets \
--disable-socketpair \
--disable-ntlm-wb \
CFLAGS="-mcpu=750 -meabi -mhard-float -O3 -ffunction-sections -fdata-sections" \
CXXFLAGS="-mcpu=750 -meabi -mhard-float -O3 -ffunction-sections -fdata-sections" \
CPPFLAGS="-D__WIIU__ -D__WUT__ -I$DEVKITPRO/wut/include" \
LDFLAGS="-L$DEVKITPRO/wut/lib" \
LIBS="-lwut -lm" \
CC=$DEVKITPPC/bin/powerpc-eabi-gcc \
AR=$DEVKITPPC/bin/powerpc-eabi-ar \
RANLIB=$DEVKITPPC/bin/powerpc-eabi-ranlib \
PKG_CONFIG=$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-pkg-config

WORKDIR /curl/lib
RUN make -j$(nproc) install
WORKDIR /curl/include
RUN make -j$(nproc) install

# build libiosuhax
WORKDIR /
RUN git clone --recursive https://github.com/Crementif/libiosuhax
WORKDIR /libiosuhax
RUN make -j$(nproc) && make install
WORKDIR /

# build libromfs
WORKDIR /
RUN git clone --recursive https://github.com/yawut/libromfs-wiiu
WORKDIR /libromfs-wiiu
RUN make -j$(nproc) && make install

# build NUSspli
WORKDIR /
RUN mkdir /nuspacker
WORKDIR /nuspacker
RUN wget https://github.com/Maschell/nuspacker/raw/master/NUSPacker.jar

WORKDIR /project
