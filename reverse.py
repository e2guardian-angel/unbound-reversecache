import os
import redis

MIN_TTL = 90
'''
Python module to store a reverse DNS lookup cache
'''
redisHost = 'redis' if not ('REDIS_HOST' in os.environ.keys()) else os.environ['REDIS_HOST']
redisPort = 6379 if not ('REDIS_PORT' in os.environ.keys()) else os.environ['REDIS_PORT']

reverseCache = redis.Redis(host=redisHost, port=redisPort, db=0)

def bytes_to_int(bytes):
    result = 0
    for b in bytes:
        result = result * 256 + int(b)
    return result

def parse_cname(bytes):
    i = 0
    parts = []
    while(i < len(bytes)):
        lpart = bytes_to_int(bytes[i:i+1])
        parts.append(bytes[i+1:i+1+lpart].decode('utf-8'))
        i = i + lpart + 1
    while '' in parts:
        parts.remove('')
    return '.'.join(parts)

def logDnsMsg(qstate):
    """Logs response"""

    r  = qstate.return_msg.rep
    q  = qstate.return_msg.qinfo

    if (r):
        for i in range(0, r.rrset_count):
            rr = r.rrsets[i]
            rk = rr.rk
            d = rr.entry.data
            for j in range(0, d.count+d.rrsig_count):
                data = d.rr_data[j]
                ttl = d.rr_ttl[j]
                length = bytes_to_int(data[:2])
                if (rk.type_str == 'CNAME'):
                    hostName = rk.dname_str
                    cname = parse_cname(data[2:])
                    if hostName[-1] == '.':
                        hostName = hostName[:-1]
                    reverseCache.set(cname, hostName, ex=ttl)
                if (rk.type_str == 'A' and length == 4):
                    ip = data[2:]
                    ipString = "{}.{}.{}.{}".format(*ip)
                    hostName = rk.dname_str
                    if hostName[-1] == '.':
                        hostName = hostName[:-1]
                    reverseCache.set(ipString, hostName, ex=ttl)

def init(id, cfg):
   log_info("pythonmod: init called, module id is %d port: %d script: %s" % (id, cfg.port, cfg.python_script))
   # TODO: Put this in a config file
   return True

def deinit(id):
   return True

def inform_super(id, qstate, superqstate, qdata):
   return True

def operate(id, event, qstate, qdata):
   if (event == MODULE_EVENT_NEW) or (event == MODULE_EVENT_PASS):
      #Pass on the new event to the iterator
      qstate.ext_state[id] = MODULE_WAIT_MODULE 
      return True

   if event == MODULE_EVENT_MODDONE:
      #Iterator finished, show response (if any)

      if (qstate.return_msg):
          logDnsMsg(qstate)

      qstate.ext_state[id] = MODULE_FINISHED 
      return True

   qstate.ext_state[id] = MODULE_ERROR
   return True

