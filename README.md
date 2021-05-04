# pythonunbound
Note: this is still under development
This is based off of NLnetLabs/pythonunbound.

This is an unbound dns server that is modified to use redis as a reverse dns cache. This is being created as part of the e2guardian-angel project to facilitate transparent proxying for applications that don't support proxy configurations.

Usage:
```
*** start redis here ***
docker run -d -p 53:53 jusschwa/unbound-reversecache-pi
```