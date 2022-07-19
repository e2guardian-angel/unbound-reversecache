FROM alpine:3.12.1 as builder
ARG UNBOUND_VERSION=1.16.1

RUN apk update && \
apk add build-base \
	ca-certificates \
	bind-tools \
	expat-dev \
	git \
	libevent-dev \
	hiredis \
	hiredis-dev \
	openssl-dev \
	rsyslog \
	swig \
	wget


WORKDIR /opt

COPY reverse.c /opt

RUN wget "https://www.nlnetlabs.nl/downloads/unbound/unbound-${UNBOUND_VERSION}.tar.gz" && \
        tar zxvf unbound*.tar.gz && \
	cd $(find . -type d -name 'unbound*') && \
	./configure --with-dynlibmodule --prefix=/opt && \
	make && \
	make install && \
        adduser -u 48 -H -D unbound && \
	chown -R unbound: /opt/etc/unbound/ && \
	cd /opt/unbound-${UNBOUND_VERSION}/dynlibmod/examples && \
	mv /opt/reverse.c . && \
	gcc -I/usr/include/hiredis -I../.. -shared -Wall -Werror -fpic  -o reverse.so reverse.c -lhiredis && \
	cp reverse.so /opt/lib/ && \
	cd /opt && \
	rm -Rf /opt/unbound

FROM alpine:3.12.1
MAINTAINER Justin Schwartzbeck <justinmschw@gmail.com>

RUN adduser -u 48 -H -D unbound

COPY --from=builder /opt /opt
COPY --from=builder /usr/lib /usr/lib

RUN apk add bind-tools \
    hiredis \
	jq \
	rsyslog && \
	rm -rf /var/cache/apk/*


WORKDIR /opt/etc/unbound

RUN mkdir conf
COPY unbound-safe.conf ./conf
COPY unbound-unsafe.conf ./conf
COPY unbound-fwd.conf.tmpl ./conf

RUN chown -R unbound /opt/etc/unbound

COPY initsafe.py /
COPY entrypoint.sh /

EXPOSE 53

# Ready! Once in a Bash shell you can do 'unbound' then 'dig +noall +answer @127.0.0.1' to see the output of the
# Hello World Python module:
# root@nnn:/usr/local/etc/unbound#: unbound
# root@nnn:/usr/local/etc/unbound#: dig +noall +answer @127.0.0.1
# helloworld.  300 IN A 127.0.0.1
USER unbound
ENTRYPOINT ["/bin/sh", "/entrypoint.sh"]
