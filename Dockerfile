# This Dockerfile builds Unbound --with-pythonmodule support and includes a simple Hello World style Python
# module to demonstrate the --with-pythonmodule functionality.
# See: https://unbound.net/
FROM alpine:3.12.1 as builder
ARG UNBOUND_VERSION=1.9.4
ARG PYTHONPATH=/usr/lib/python3.8

RUN apk update && \
apk add build-base \
	ca-certificates \
	bind-tools \
	expat-dev \
	libevent-dev \
	python3 \
	python3-dev \
	openssl-dev \
	py3-distutils-extra \
	py3-redis \
	rsyslog \
	swig \
	wget


WORKDIR /opt
RUN wget "https://www.nlnetlabs.nl/downloads/unbound/unbound-${UNBOUND_VERSION}.tar.gz" && \
	tar zxvf unbound*.tar.gz && \
	cd $(find . -type d -name 'unbound*') && \
	ln -s /usr/bin/python3 /usr/bin/python && \
	./configure --with-pyunbound --with-libevent --with-pythonmodule --prefix=/opt && \
	make && \
	make install && \
        adduser -H -D unbound && \
	chown -R unbound: /opt/etc/unbound/ && \
	cd /opt && \
	rm -Rf /opt/unbound*

FROM alpine:3.12.1
MAINTAINER Justin Schwartzbeck <justinmschw@gmail.com>

RUN adduser -H -D unbound

COPY --from=builder /opt /opt
COPY --from=builder /usr/lib /usr/lib

RUN apk add bind-tools \
        libevent \
        python3 \
	py3-distutils-extra \
	py3-redis \
	jq \
	rsyslog && \
	rm -rf /var/cache/apk/*


WORKDIR /opt/etc/unbound
RUN mv unbound.conf unbound.conf.org

COPY unbound.conf .

COPY reverse.py .

COPY entrypoint.sh /

EXPOSE 53

# Ready! Once in a Bash shell you can do 'unbound' then 'dig +noall +answer @127.0.0.1' to see the output of the
# Hello World Python module:
# root@nnn:/usr/local/etc/unbound#: unbound
# root@nnn:/usr/local/etc/unbound#: dig +noall +answer @127.0.0.1
# helloworld.  300 IN A 127.0.0.1
ENTRYPOINT ["/bin/sh", "/entrypoint.sh"]
