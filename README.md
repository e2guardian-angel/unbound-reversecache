# pythonunbound
Note: I just started this so it is not developed at all yet.

This is based off of NLnetLabs/pythonunbound.

A simple Dockerfile that builds Unbound --with-pythonmodule support and includes a python module that will store user lookups in a reverse cache in redis. This will be used by a squid external acl to determine what hostname is associated with the destination IP in a client request. This is to solve the problem of squid's dstdomain feature, which will normally attempt a (very unreliable) reverse DNS lookup when a client application is being proxied transparently. The idea being that if the client and squid are using the same DNS server, then the DNS server should be able to tell squid what hostname was associated with the DNS reply to the client's requests. This way squid can better handle acls that act on domains.

# Hello World
```
$ docker run -it nlnetlabs/pythonunbound
root@nnn:/usr/local/etc/unbound#: unbound
root@nnn:/usr/local/etc/unbound#: dig +noall +answer @127.0.0.1
helloworld.  300 IN A 127.0.0.1
```

# Develop and test your own Python module for Unbound
Edit the `helloworld.py` or your own Python module file, reload Unbound then submit DNS queries to the locally running Unbound process to see your module in action. Running Unbound in the foreground with lots of diagnostic output will probably be useful while developing, e.g.:

```
docker run -it nlnetlabs/pythonunbound
root@nnn:/usr/local/etc/unbound#: unbound -dd -vvvv
```

Then in another terminal get the container ID with `docker ps` then connect to it like so:
```
docker exec -it <container id> /bin/bash
root@nnn:/usr/local/etc/unbound#: dig +noall +answer @127.0.0.1
helloworld.  300 IN A 127.0.0.1
```

# Building a different version of Unbound
Rather than use the [stock version](https://cloud.docker.com/u/nlnetlabs/repository/docker/nlnetlabs/pythonunbound) of the image hosted at Docker Hub, you can rebuild the image yourself against any `unbound-X.Y.Z.tar.gz` available at https://nlnetlabs.nl/downloads/unbound/ by specifying the `UNBOUND_VERSION` build argument to the `docker build` command.

For example, to build the image using Unbound 1.8.0:

1. Clone this repository and CD into the clone directory.
2. Build the image: `docker build --bulld-arg UNBOUND_VERSION=1.8.0 .`

END
