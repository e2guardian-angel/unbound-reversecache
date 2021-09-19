#!/usr/bin/python3
import os
import redis
import time

redisHost = 'redis' if not ('REDIS_SERVICE_HOST' in os.environ.keys()) else os.environ['REDIS_SERVICE_HOST']
redisPort = 6379 if not ('REDIS_SERVICE_PORT' in os.environ.keys()) else os.environ['REDIS_SERVICE_PORT']
redisPass = '' if not ('REDIS_PASS' in os.environ.keys()) else os.environ['REDIS_PASS']
retryDelay = 3 if not ('REDIS_RETRY_DELAY' in os.environ.keys()) else int(os.environ['REDIS_RETRY_DELAY'])

reverseCache = redis.Redis(host=redisHost, port=redisPort, db=0, password=redisPass)

# Wait for redis to come up
while (!reverseCache.ping()):
    time.sleep(retryDelay)

# Add permanent safesearch entries
reverseCache.set('213.180.193.56', 'yandex.com')
reverseCache.set('40.89.244.237', 'duckduckgo.com')
reverseCache.set('216.239.38.120', 'google.com')
reverseCache.set('216.239.38.119', 'youtube.com')
reverseCache.set('104.18.21.183', 'pixabay.com')
