
/*
 * mac_policer.h - skeleton vpp engine plug-in header file
 *
 * Copyright (c) <current-year> <your-organization>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __included_mac_policer_h__
#define __included_mac_policer_h__

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vnet/ethernet/ethernet.h>

#include <vppinfra/hash.h>
#include <vppinfra/error.h>

#define MAC_POLICER_NOT_DEBUG 1

#ifndef MAX
#define MAX(x,y)            (((x)>(y))?(x):(y))
#endif

typedef struct {
    CLIB_CACHE_LINE_ALIGN_MARK (cacheline0);
    u64 mac;
    u32 cir_tokens_per_period;    // # of tokens for each period
    u32 scale;            // power-of-2 shift amount for lower rates
    u64 current_limit;
    u64 current_bucket[2];       // MOD
    u32 *last_update_time; //need to adapt to core/worker count //vector
} mac_policer;

typedef struct {
    /* API message ID base */
    u16 msg_id_base;

    /* on/off switch for the periodic function */
    u8 periodic_timer_enabled;
    /* Node index, non-zero if the periodic process has been created */
    u32 periodic_node_index;

    /* convenience */
    vlib_main_t * vlib_main;
    vnet_main_t * vnet_main;
    ethernet_main_t * ethernet_main;
    mac_policer *policer;
} mac_policer_main_t;

extern mac_policer_main_t mac_policer_main;

extern vlib_node_registration_t mac_policer_node;
extern vlib_node_registration_t mac_policer_periodic_node;

/* Periodic function events */
#define MAC_POLICER_EVENT1 1
#define MAC_POLICER_EVENT2 2
#define MAC_POLICER_EVENT_PERIODIC_ENABLE_DISABLE 3

void mac_policer_create_periodic_process (mac_policer_main_t *);

#endif /* __included_mac_policer_h__ */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */

