/*
Copyright (c) 2015, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef INET_BRIDGE_H_INCLUDED
#define INET_BRIDGE_H_INCLUDED

/*
 * ===========================================================================
 * Bridge driver interface
 * ===========================================================================
 */

#include "inet.h"
#include "inet_base.h"
#include "osn_bridge.h"

typedef struct inet_bridge inet_bridge_t;

struct inet_bridge
{
    /* Subclass inet_base_t */
    union
    {
        inet_t inet;
        inet_base_t base;
    };

    char                in_br_ifname[C_IFNAME_LEN]; /* Interface name */
    bool                in_br_add;                  /* add/del bridge */
    struct ev_debounce  in_br_debounce;             /* Reconfiguration debounce timer */
    ds_tree_t           in_br_port_list;
};

inet_t *inet_bridge_new(const char *ifname);
bool inet_bridge_dtor(inet_t *super);
extern bool inet_bridge_commit(inet_t *super);

#endif /* INET_BRIDGE_H_INCLUDED */
