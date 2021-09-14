#!/bin/sh
GUARDIAN_CONF=/opt/guardian/guardian.json

export REDIS_HOST="redis"
export REDIS_PORT=6379
UNBOUND_CONF_DIR=/opt/etc/unbound/conf
UNBOUND_CONF_FORWARDER=${UNBOUND_CONF_DIR}/unbound-fwd.conf
UNBOUND_CONF_SAFE=${UNBOUND_CONF_DIR}/unbound-safe.conf
UNBOUND_CONF_UNSAFE=${UNBOUND_CONF_DIR}/unbound-unsafe.conf

extract_value () {
    echo "${1}" | jq -r .${2}
}

if [ -f "${GUARDIAN_CONF}" ]; then
    CONFIG="$(cat $GUARDIAN_CONF)"
    REDIS_CONF=$(extract_value "${CONFIG}" redisConfig)
    export REDIS_HOST=$(extract_value "${REDIS_CONF}" host)
    export REDIS_PORT=$(extract_value "${REDIS_CONF}" port)
    SAFESEARCH=$(extract_value "${CONFIG}" safeSearchEnforced)
    if [ "${FORWARDER}" = "true" ]; then
	DNS_IP=$(extract_value "${CONFIG}" dnsIP)
	sed -i "s~DNS_REVERSE_SVC_IP~$DNS_REVERSE_SERVICE_HOST~g" $UNBOUND_CONF_FORWARDER
        export UNBOUND_CONF=${UNBOUND_CONF_FORWARDER}
    else
	if [ "${SAFESEARCH}" = "true" ]; then
            export UNBOUND_CONF=${UNBOUND_CONF_SAFE}
        else
            export UNBOUND_CONF=${UNBOUND_CONF_UNSAFE}
        fi
    fi
fi

# Add permanent entries for safe search sites
/usr/bin/python3 /initsafe.py

/opt/sbin/unbound -d -c ${UNBOUND_CONF}
