#!/usr/bin/python
import os
import redis

redisHost = 'redis' if not ('REDIS_HOST' in os.environ.keys()) else os.environ['REDIS_HOST']
redisPort = 6379 if not ('REDIS_PORT' in os.environ.keys()) else os.environ['REDIS_PORT']
redisPass = '' if not ('REDIS_PASS' in os.environ.keys()) else os.environ['REDIS_PASS']

reverseCache = redis.Redis(host=redisHost, port=redisPort, db=0, password=redisPass)

# Add permanent safesearch entries
reverseCache.set('213.180.193.56', 'yandex.com')
reverseCache.set('40.89.244.237', 'duckduckgo.com')
reverseCache.set('216.239.38.120', 'google.com')
reverseCache.set('216.239.38.119', 'youtube.com')
reverseCache.set('104.18.21.183', 'pixabay.com')
