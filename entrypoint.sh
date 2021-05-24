#!/bin/sh

export REDIS_HOST="redis"
export REDIS_PORT=6379

extract_value () {
    echo "${1}" | jq -r .${2}
}

if [ -f "${GUARDIAN_CONF}" ]; then
    CONFIG="$(cat $GUARDIAN_CONF)"
    REDIS_CONF=$(extract_value "${CONFIG}" redisConfig)
    export REDIS_HOST=$(extract_value "${REDIS_CONF}" host)
    export REDIS_PORT=$(extract_value "${REDIS_CONF}" port)
fi

/opt/sbin/unbound -d -c /opt/etc/unbound/unbound.conf
