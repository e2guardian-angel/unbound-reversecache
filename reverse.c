/**
 *
 * This is a module that stores a reversed IP => hostname lookup for every
 * DNS response received by unbound. This is to make it easier for the squid
 * proxy to identify websites by IP when configured for transparent proxy mode.
 */

#include "../../config.h"
#include "../../util/module.h"
#include "../../services/cache/dns.h"
#include "../../sldns/parseutil.h"
#include "../../sldns/rrdef.h"
#include "../dynlibmod.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "hiredis.h"

/* Declare the EXPORT macro that expands to exporting the symbol for DLLs when
 * compiling for Windows. All procedures marked with EXPORT in this example are
 * called directly by the dynlib module and must be present for the module to
 * load correctly. */
#ifdef HAVE_WINDOWS_H
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define MIN_TTL 90

redisContext *c;

/* Forward declare a callback, implemented at the bottom of this file */
int reply_callback(struct query_info* qinfo,
    struct module_qstate* qstate, struct reply_info* rep, int rcode,
    struct edns_data* edns, struct edns_option** opt_list_out,
    struct comm_reply* repinfo, struct regional* region,
    struct timeval* start_time, int id, void* callback);

/* Init is called when the module is first loaded. It should be used to set up
 * the environment for this module and do any other initialisation required. */
EXPORT int init(struct module_env* env, int id) {
    struct dynlibmod_env* de = (struct dynlibmod_env*) env->modinfo[id];
    de->inplace_cb_register_wrapped(&reply_callback,
                                    inplace_cb_reply,
                                    NULL, env, id);
    struct dynlibmod_env* local_env = env->modinfo[id];
    local_env->dyn_env = NULL;

    const char* redis_host = getenv("REDIS_HOST");
    uint16_t redis_port = (uint16_t)atoi(getenv("REDIS_PORT"));
    const char* redis_pass = getenv("REDIS_PASS");

    c = redisConnect(redis_host, redis_port);

    while (c != NULL && c->err) {
      log_err("Error connecting to redis: %sn", c->errstr);
      sleep(1);
      c = redisConnect(redis_host, redis_port);
    }
    log_info("Connected to redis instance");

    if (redis_pass) {
        redisReply *reply;
        reply = redisCommand(c, "AUTH %s", redis_pass);
        log_info("Auth result: %s", reply->str);
        freeReplyObject(reply);
    }

    //log_info("Connect to redis instance %s:%s", redis_host, redis_port);

    return 1;
}

/* Deinit is run as the program is shutting down. It should be used to clean up
 * the environment and any left over data. */
EXPORT void deinit(struct module_env* env, int id) {
    struct dynlibmod_env* de = (struct dynlibmod_env*) env->modinfo[id];
    de->inplace_cb_delete_wrapped(env, inplace_cb_reply, id);
    if (de->dyn_env != NULL) free(de->dyn_env);
    redisFree(c);
}


char* parse_host_name(uint8_t* dname, unsigned long len) {
    char* host_name = (char*) malloc(len);
    char* h_current = host_name;
    uint8_t* d_current = dname;
    uint8_t* d_end = (uint8_t*)(dname + len);
    while (d_current < (dname + len)) {
        uint8_t plen = *((uint8_t*)d_current);
        // Prevent buffer overflow;
        if ((d_current + plen) > d_end) {
            break;
        }
        d_current += 1;
        // copy this part to the host name
        strncpy(h_current, (char*)d_current, plen);
        h_current += plen;
        d_current += plen;
        h_current[0] = '.';
        h_current += 1;
    }
    if (h_current - host_name > 1) {
        h_current -= 2;
    }
    // eliminate the trailing '.'
    if (h_current[0] != '.') {
        h_current += 1;
    }

    h_current[0] = 0;
    return host_name;
}

/* Parse reply and cache */
void parse_dns_reply(struct module_qstate* qstate) {
    struct dns_msg* ret = qstate->return_msg;
    struct reply_info* r = ret->rep;
    //struct query_info* qinfo = &(ret->qinfo);
    if (r) {
        for(int i = 0; i < r->rrset_count; i++) {
            
            struct ub_packed_rrset_key* rr = r->rrsets[i];
            struct packed_rrset_key rk = rr->rk;
            struct packed_rrset_data* d = (struct packed_rrset_data*) (rr->entry.data);

            for (int j = 0; j < (d->count + d->rrsig_count); j++) {
                uint8_t* data = (uint8_t*)(d->rr_data[j]);
                time_t ttl = d->ttl;
                if (ttl < MIN_TTL) {
                    ttl = MIN_TTL;
                }
                uint16_t length = ntohs(*((uint16_t*)data));
                
                char* host_name = parse_host_name(rk.dname, rk.dname_len);

                int type = ntohs(rk.type);

                redisReply *reply = 0;
                if (type == LDNS_RR_TYPE_AAAA && length == 16) {
                    char* ip6_str = malloc(46);
                    inet_ntop(AF_INET6, (in_addr_t*)(data + 2), ip6_str, 46);
                    reply = redisCommand(c, "SET %s %s EX %lu", ip6_str, host_name, ttl);
                } else if (type == LDNS_RR_TYPE_CNAME) {
                    char* cname = parse_host_name(data + 2, length);
                    reply = redisCommand(c, "SET %s %s EX %lu", cname, host_name, ttl);
                    free(cname);
                } else if (type == LDNS_RR_TYPE_A && length == 4) {
                    char* ip4_str = malloc(16);
                    inet_ntop(AF_INET, (in_addr_t*)(data + 2), ip4_str, 16);
                    reply = redisCommand(c, "SET %s %s EX %lu", ip4_str, host_name, ttl);
                }
                if (reply) {
                    freeReplyObject(reply);
                }
                
                free(host_name);
            }
        }
    }
}

/* Operate is called every time a query passes by this module. The event can be
 * used to determine which direction in the module chain it came from. */
EXPORT void operate(struct module_qstate* qstate, enum module_ev event,
                    int id, struct outbound_entry* entry) {
    if (event == module_event_new || event == module_event_pass) {
        qstate->ext_state[id] = module_wait_module;
    } else if (event == module_event_moddone) {
        parse_dns_reply(qstate);
        qstate->ext_state[id] = module_finished;
    } else {
        qstate->ext_state[id] = module_error;
    }
}

/* Inform super is called when a query is completed or errors out, but only if
 * a sub-query has been registered to it by this module. Look at
 * mesh_attach_sub in services/mesh.h to see how this is done. */
EXPORT void inform_super(struct module_qstate* qstate, int id,
                         struct module_qstate* super) {
}

/* Clear is called once a query is complete and the response has been sent
 * back. It is used to clear up any per-query allocations. */
EXPORT void clear(struct module_qstate* qstate, int id) {
    struct dynlibmod_env* env = qstate->env->modinfo[id];
    if (env->dyn_env != NULL) {
        free(env->dyn_env);
        env->dyn_env = NULL;
    }
}

/* Get mem is called when Unbound is printing performance information. This
 * only happens explicitly and is only used to show memory usage to the user. */
EXPORT size_t get_mem(struct module_env* env, int id) {
    return 0;
}

/* The callback that was forward declared earlier. It is registered in the init
 * procedure to run when a query is being replied to. */
int reply_callback(struct query_info* qinfo,
    struct module_qstate* qstate, struct reply_info* rep, int rcode,
    struct edns_data* edns, struct edns_option** opt_list_out,
    struct comm_reply* repinfo, struct regional* region,
    struct timeval* start_time, int id, void* callback) {
    return 0;
}
