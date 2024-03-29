#!/bin/sh
UNBOUND_CONF_DIR=/opt/etc/unbound/conf
UNBOUND_CONF_FORWARDER=${UNBOUND_CONF_DIR}/unbound-fwd.conf
UNBOUND_CONF_SAFE=${UNBOUND_CONF_DIR}/unbound-safe.conf
UNBOUND_CONF_UNSAFE=${UNBOUND_CONF_DIR}/unbound-unsafe.conf

if [ "${FORWARDER}" = "true" ]; then
	cp $UNBOUND_CONF_FORWARDER.tmpl $UNBOUND_CONF_FORWARDER
	sed -i "s~DNS_REVERSE_SVC_IP~$DNS_REVERSE_SERVICE_HOST~g" $UNBOUND_CONF_FORWARDER
    export UNBOUND_CONF=${UNBOUND_CONF_FORWARDER}
else
    if [ "${SAFESEARCH}" = "yes" ]; then
        export UNBOUND_CONF=${UNBOUND_CONF_SAFE}
    else
        export UNBOUND_CONF=${UNBOUND_CONF_UNSAFE}
    fi
fi

/opt/sbin/unbound -d -c ${UNBOUND_CONF}
