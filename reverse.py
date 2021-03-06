import os
'''
 calc.py: Response packet logger

 Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
                     Marek Vavrusa  (xvavru00 AT stud.fit.vutbr.cz)

 This software is open source.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
 
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
 
    * Neither the name of the organization nor the names of its
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
'''

def bytes_to_int(bytes):
    result = 0
    for b in bytes:
        result = result * 256 + int(b)
    return result

def logDnsMsg(qstate):
    """Logs response"""

    r  = qstate.return_msg.rep
    q  = qstate.return_msg.qinfo

    if (r):
        print("Reply:")
        for i in range(0, r.rrset_count):
            rr = r.rrsets[i]

            rk = rr.rk

            if (rk.type_str == 'A'):
                d = rr.entry.data
                for j in range(0, d.count+d.rrsig_count):
                    data = d.rr_data[j]
                    ttl = d.rr_ttl[j]
                    length = bytes_to_int(data[:2])
                    if (length == 4):
                        ip = data[2:]
                        ipString = '{}.{}.{}.{}'.format(*ip)
                        print("Name: " + rk.dname_str + ", Address: " + ipString + ", TTL=%d" % ttl)

            d = rr.entry.data

def init(id, cfg):
   log_info("pythonmod: init called, module id is %d port: %d script: %s" % (id, cfg.port, cfg.python_script))
   return True

def deinit(id):
   log_info("pythonmod: deinit called, module id is %d" % id)
   return True

def inform_super(id, qstate, superqstate, qdata):
   return True

def operate(id, event, qstate, qdata):
   log_info("pythonmod: operate called, id: %d, event:%s" % (id, strmodulevent(event)))
  
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

