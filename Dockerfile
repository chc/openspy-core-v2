FROM ubuntu:16.04 AS build
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get -y dist-upgrade
RUN apt-get install -y build-essential cmake openssl libcurl4-openssl-dev libssl-dev curl

RUN mkdir /root/fs-out

WORKDIR /root/

RUN curl -L http://www.digip.org/jansson/releases/jansson-2.11.tar.gz > jansson.tar.gz
RUN mkdir jansson-src
RUN tar -xvzf jansson.tar.gz --strip 1 -C jansson-src
WORKDIR jansson-src
RUN ./configure CFLAGS="-fPIC" --disable-shared
RUN make install DESTDIR=/root/fs-out

WORKDIR /root/
RUN curl -L https://www.openssl.org/source/openssl-1.0.2p.tar.gz > openssl.tar.gz
RUN mkdir openssl-src
RUN tar -xvzf openssl.tar.gz --strip 1 -C openssl-src
WORKDIR openssl-src
RUN ./Configure linux-x86_64 --prefix=/usr/local enable-ssl3 enable-ssl2 enable-shared -fPIC
RUN make install INSTALL_PREFIX=/root/fs-out

WORKDIR /root/
RUN curl -L https://curl.haxx.se/download/curl-7.61.1.tar.gz > curl.tar.gz
RUN mkdir curl-src
RUN tar -xvzf curl.tar.gz --strip 1 -C curl-src
WORKDIR curl-src
RUN ./configure --without-ssl --disable-shared
RUN make
RUN make DESTDIR=/root/fs-out install

WORKDIR /root/
RUN curl -L https://codeload.github.com/redis/hiredis/tar.gz/v1.1.0 > hiredis.tar.gz
RUN mkdir hiredis-src hiredis-bin
RUN tar -xvzf hiredis.tar.gz --strip 1 -C hiredis-src
WORKDIR hiredis-bin
RUN cmake -DCMAKE_BUILD_TYPE="Release" ../hiredis-src
RUN make
RUN make DESTDIR=/root/fs-out install

WORKDIR /root/
RUN curl -L https://codeload.github.com/alanxz/rabbitmq-c/tar.gz/v0.9.0 > rabbitmq.tar.gz
RUN mkdir rabbitmq-src rabbitmq-bin
RUN tar -xvzf rabbitmq.tar.gz --strip 1 -C rabbitmq-src
WORKDIR rabbitmq-bin
RUN cmake -DCMAKE_BUILD_TYPE="Release" -DCMAKE_INSTALL_INCLUDEDIR="include/rabbitmq-c" -DENABLE_SSL_SUPPORT="OFF"  ../rabbitmq-src
RUN make
RUN make DESTDIR=/root/fs-out install

WORKDIR /root/
RUN curl -L http://github.com/zeux/pugixml/releases/download/v1.9/pugixml-1.9.tar.gz > pugixml.tar.gz
RUN mkdir pugixml-src pugixml-bin
RUN tar -xvzf pugixml.tar.gz --strip 1 -C pugixml-src
WORKDIR pugixml-bin
RUN cmake -DCMAKE_BUILD_TYPE="Release" -DCMAKE_CXX_FLAGS="-fPIC" ../pugixml-src
RUN make
RUN make DESTDIR=/root/fs-out install

COPY code /root/code
COPY cmake /root/cmake
RUN mkdir /root/os-bin /root/os-make
WORKDIR /root/os-make
run cmake -DCMAKE_PREFIX_PATH="/root/fs-out/usr/local" -DCMAKE_BINARY_DIR="/root/os-bin" -DCMAKE_CXX_FLAGS="-I/root/fs-out/usr/local/include -I/root/fs-out/usr/local/include/rabbitmq-c -L/root/fs-out/usr/local/lib -lz -L/root/fs-out/usr/local/lib/x86_64-linux-gnu/ -pthread"  -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE="Release" ../code
run make
RUN mkdir -p /root/fs-out/opt/openspy/bin /root/fs-out/opt/openspy/lib
RUN mv /root/os-make/bin/* /root/fs-out/opt/openspy/bin
RUN mv /root/os-make/lib/* /root/fs-out/opt/openspy/lib
COPY docker-support/openspy.xml /root/fs-out/opt/openspy/openspy.xml
COPY docker-support/x509.pem /root/fs-out/opt/openspy/x509.pem
COPY docker-support/rsa_priv.pem /root/fs-out/opt/openspy/rsa_priv.pem
COPY docker-support/password_key.pem /root/fs-out/opt/openspy/password_key.pem
COPY docker-support/tos_battlefield.txt /root/fs-out/opt/openspy/tos_battlefield.txt
COPY docker-support/run.sh /root/fs-out/opt/openspy/run.sh
RUN chmod a+x /root/fs-out/opt/openspy/run.sh

WORKDIR /root/fs-out
RUN tar -cvzf /fs-out.tar.gz *
#RUN cp -rv /root/fs-out/* /


FROM ubuntu:16.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get -y dist-upgrade
RUN apt-get install -y libjemalloc-dev
COPY --from=build /fs-out.tar.gz /root/fs.tar.gz
RUN tar -xvzf /root/fs.tar.gz -C /
RUN echo /opt/openspy/lib >> /etc/ld.so.conf.d/openspy.conf
RUN echo /usr/local/lib/x86_64-linux-gnu  >> /etc/ld.so.conf.d/usr_libs.conf
RUN echo LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.1 >> /etc/environment
RUN /sbin/ldconfig
WORKDIR /opt/openspy
CMD ["/bin/bash", "/opt/openspy/run.sh"]