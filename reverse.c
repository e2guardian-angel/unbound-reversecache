/**
 * \file
 *
 * This is an example to show how dynamic libraries can be made to work with
 * unbound. To build a .so file simply run:
 *   gcc -I../.. -shared -Wall -Werror -fpic  -o helloworld.so helloworld.c
 * And to build for windows, first make unbound with the --with-dynlibmod
 * switch, then use this command:
 *   x86_64-w64-mingw32-gcc -m64 -I../.. -shared -Wall -Werror -fpic
 *      -o helloworld.dll helloworld.c -L../.. -l:libunbound.dll.a
 * to cross-compile a 64-bit Windows DLL.  The libunbound.dll.a is produced
 * by the compile step that makes unbound.exe and allows the dynlib dll to
 * access definitions in unbound.exe.
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
#include <arpa/inet.h>

/* Declare the EXPORT macro that expands to exporting the symbol for DLLs when
 * compiling for Windows. All procedures marked with EXPORT in this example are
 * called directly by the dynlib module and must be present for the module to
 * load correctly. */
#ifdef HAVE_WINDOWS_H
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

/* Forward declare a callback, implemented at the bottom of this file */
int reply_callback(struct query_info* qinfo,
    struct module_qstate* qstate, struct reply_info* rep, int rcode,
    struct edns_data* edns, struct edns_option** opt_list_out,
    struct comm_reply* repinfo, struct regional* region,
    struct timeval* start_time, int id, void* callback);

/* Init is called when the module is first loaded. It should be used to set up
 * the environment for this module and do any other initialisation required. */
EXPORT int init(struct module_env* env, int id) {
    log_info("dynlib: hello world from init");
    struct dynlibmod_env* de = (struct dynlibmod_env*) env->modinfo[id];
    de->inplace_cb_register_wrapped(&reply_callback,
                                    inplace_cb_reply,
                                    NULL, env, id);
    struct dynlibmod_env* local_env = env->modinfo[id];
    local_env->dyn_env = NULL;
    return 1;
}

/* Deinit is run as the program is shutting down. It should be used to clean up
 * the environment and any left over data. */
EXPORT void deinit(struct module_env* env, int id) {
    log_info("dynlib: hello world from deinit");
    struct dynlibmod_env* de = (struct dynlibmod_env*) env->modinfo[id];
    de->inplace_cb_delete_wrapped(env, inplace_cb_reply, id);
    if (de->dyn_env != NULL) free(de->dyn_env);
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
    if (h_current > host_name) {
        h_current -= 1;
    }
    h_current[0] = 0;
    return host_name;
}

/* Parse reply and cache */
void parse_dns_reply(struct module_qstate* qstate) {
    struct dns_msg* ret = qstate->return_msg;
    struct reply_info* r = ret->rep;
    struct query_info* qinfo = &(ret->qinfo);
    if (r) {
        for(int i = 0; i < r->rrset_count; i++) {
            
            struct ub_packed_rrset_key* rr = r->rrsets[i];
            struct packed_rrset_key rk = rr->rk;
            struct packed_rrset_data* d = (struct packed_rrset_data*) (rr->entry.data);

            for (int j = 0; j < (d->count + d->rrsig_count); j++) {
                uint8_t* data = (uint8_t*)(d->rr_data[j]);
                time_t ttl = d->ttl;
                uint16_t length = ntohs(*((uint16_t*)data));
                
                char* host_name = parse_host_name(rk.dname, rk.dname_len);

                int type = ntohs(rk.type);

                if (type == LDNS_RR_TYPE_AAAA && length == 16) {
                    log_info("Parsing IPv6 Response...");
                } else if (type == LDNS_RR_TYPE_CNAME) {
                    log_info("Parsing CNAME response...");
                    char* cname = parse_host_name(data + 2, length);
                    log_info("Parsed CNAME: %s ", cname);
                    free(cname);
                } else if (type == LDNS_RR_TYPE_A && length == 4) {
                    log_info("Parsing A response...");
                }
                
                // TODO: parse response based on response type
                log_info("%s (%d %d %d) %d %d %lu %p", host_name, LDNS_RR_TYPE_A, LDNS_RR_TYPE_CNAME, LDNS_RR_TYPE_AAAA, type, length, ttl, &qinfo);
                free(host_name);
            }
        }
    }
}

/* Operate is called every time a query passes by this module. The event can be
 * used to determine which direction in the module chain it came from. */
EXPORT void operate(struct module_qstate* qstate, enum module_ev event,
                    int id, struct outbound_entry* entry) {
    log_info("dynlib: hello world from operate");
    log_info("dynlib: incoming query: %s %s(%d) %s(%d)",
            qstate->qinfo.qname,
            sldns_lookup_by_id(sldns_rr_classes, qstate->qinfo.qclass)->name,
            qstate->qinfo.qclass,
            sldns_rr_descript(qstate->qinfo.qtype)->_name,
            qstate->qinfo.qtype);
    if (event == module_event_new || event == module_event_pass) {
        qstate->ext_state[id] = module_wait_module;
        struct dynlibmod_env* env = qstate->env->modinfo[id];
        if (env->dyn_env == NULL) {
            env->dyn_env = calloc(3, sizeof(int));
            ((int *)env->dyn_env)[0] = 42;
            ((int *)env->dyn_env)[1] = 102;
            ((int *)env->dyn_env)[2] = 192;
        } else {
            log_err("dynlib: already has data!");
            qstate->ext_state[id] = module_error;
        }
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
    log_info("dynlib: hello world from inform_super");
}

/* Clear is called once a query is complete and the response has been sent
 * back. It is used to clear up any per-query allocations. */
EXPORT void clear(struct module_qstate* qstate, int id) {
    log_info("dynlib: hello world from clear");
    struct dynlibmod_env* env = qstate->env->modinfo[id];
    if (env->dyn_env != NULL) {
        free(env->dyn_env);
        env->dyn_env = NULL;
    }
}

/* Get mem is called when Unbound is printing performance information. This
 * only happens explicitly and is only used to show memory usage to the user. */
EXPORT size_t get_mem(struct module_env* env, int id) {
    log_info("dynlib: hello world from get_mem");
    return 0;
}

/* The callback that was forward declared earlier. It is registered in the init
 * procedure to run when a query is being replied to. */
int reply_callback(struct query_info* qinfo,
    struct module_qstate* qstate, struct reply_info* rep, int rcode,
    struct edns_data* edns, struct edns_option** opt_list_out,
    struct comm_reply* repinfo, struct regional* region,
    struct timeval* start_time, int id, void* callback) {
    log_info("dynlib: hello world from callback");
    struct dynlibmod_env* env = qstate->env->modinfo[id];
    if (env->dyn_env != NULL) {
        log_info("dynlib: numbers gotten from query: %d, %d, and %d",
            ((int *)env->dyn_env)[0],
            ((int *)env->dyn_env)[1],
            ((int *)env->dyn_env)[2]);
    }
    return 0;
}
