/*
 * node.c - skeleton vpp engine plug-in dual-loop node skeleton
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
#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/pg/pg.h>
#include <vppinfra/error.h>
#include <mac_policer/mac_policer.h>

void gibirix_police_ethernet_packet (vlib_main_t * vm, u32 now, vlib_buffer_t * b, u32 *next);
u8 gibirix_police_packet (vlib_main_t *vm, mac_policer * policer, u8 type, u32 packet_length, u32 time);
uword gibirix_mac_policer_node_fn(vlib_main_t * vm, vlib_node_runtime_t * node, vlib_frame_t * frame);


typedef enum 
{
  MAC_POLICER_NEXT_INTERFACE_DROP,
  MAC_POLICER_N_NEXT,
} mac_policer_next_t;

#ifndef CLIB_MARCH_VARIANT

u8 gibirix_police_packet (vlib_main_t *vm, mac_policer * policer, u8 is_from_host, u32 packet_length, u32 time)
{
  u32 n_periods;
  u64 current_tokens;

  packet_length = packet_length << policer->scale;
  n_periods = time - policer->last_update_time[vm->thread_index]; 

  policer->last_update_time[vm->thread_index] = time;

  policer->current_bucket[0] += n_periods * policer->cir_tokens_per_period;
  policer->current_bucket[1] += n_periods * policer->cir_tokens_per_period;

  current_tokens = policer->current_bucket[is_from_host];
  
  if (current_tokens > policer->current_limit)
  {
    current_tokens = policer->current_limit;
  }

  #ifndef MAC_POLICER_NOT_DEBUG
  clib_warning("mac: %llx, limit: %u, current: %u, direction: %u, len: %u", policer->mac, policer->current_limit, current_tokens, is_from_host, packet_length);
  #endif


  if(current_tokens >= packet_length)
  {
    policer->current_bucket[is_from_host] = current_tokens - packet_length;
    return 1;
  }else{ 
    policer->current_bucket[is_from_host] = current_tokens;
    return 0;
  }

}

void gibirix_police_ethernet_packet (vlib_main_t * vm, u32 now, vlib_buffer_t * b, u32 *next)
{
  mac_policer *policer; 
  ethernet_header_t *en = vlib_buffer_get_current (b);
  u32 packet_length = vlib_buffer_length_in_chain (vm, b) * 8; //Bits

  /* ethernet_mac_address_u64() */
  u64 src_mac = ((u64)en->src_address[0] << 40) | ((u64)en->src_address[1] << 32) | ((u64)en->src_address[2] << 24) | ((u64)en->src_address[3] << 16) | ((u64)en->src_address[4] << 8) | en->src_address[5];
  u64 dst_mac = ((u64)en->dst_address[0] << 40) | ((u64)en->dst_address[1] << 32) | ((u64)en->dst_address[2] << 24) | ((u64)en->dst_address[3] << 16) | ((u64)en->dst_address[4] << 8) | en->dst_address[5];

  #ifndef MAC_POLICER_NOT_DEBUG
  clib_warning("mac src: %llx", src_mac);
  clib_warning("mac dst: %llx", dst_mac);
  #endif

  vec_foreach(policer, mac_policer_main.policer)
  { 

    /* prefetch needed */

    /* hand off needed for optimization */

    if(policer->mac == src_mac || policer->mac == dst_mac)
    {
      if(PREDICT_FALSE(gibirix_police_packet (vm, policer, policer->mac == src_mac, packet_length, now) == 0))
      {
          *next = MAC_POLICER_NEXT_INTERFACE_DROP;
          return;
      }

    }
  }
}

uword gibirix_mac_policer_node_fn(vlib_main_t * vm, vlib_node_runtime_t * node, vlib_frame_t * frame)
{
  u32 n_left_from, * from, * to_next;
  mac_policer_next_t next_index;
  u32 now = (u32)(vlib_time_now (vm) * 1000); //1ms

  from = vlib_frame_vector_args (frame);
  n_left_from = frame->n_vectors;
  next_index = node->cached_next_index;

  while (n_left_from > 0)
  {
      u32 n_left_to_next;

      vlib_get_next_frame (vm, node, next_index, to_next, n_left_to_next);

      while (n_left_from >= 4 && n_left_to_next >= 2)
       {
          u32 next0 = MAC_POLICER_NEXT_INTERFACE_DROP;
          u32 next1 = MAC_POLICER_NEXT_INTERFACE_DROP;
          u32 bi0, bi1;
          vlib_buffer_t * b0, * b1;
          
          /* Prefetch next iteration. */
          {
            vlib_buffer_t * p2, * p3;
                  
            p2 = vlib_get_buffer (vm, from[2]);
            p3 = vlib_get_buffer (vm, from[3]);
                  
            vlib_prefetch_buffer_header (p2, LOAD);
            vlib_prefetch_buffer_header (p3, LOAD);

            CLIB_PREFETCH (p2->data, CLIB_CACHE_LINE_BYTES, STORE);
            CLIB_PREFETCH (p3->data, CLIB_CACHE_LINE_BYTES, STORE);
          }

          /* speculatively enqueue b0 and b1 to the current next frame */
          to_next[0] = bi0 = from[0];
          to_next[1] = bi1 = from[1];
          from += 2;
          to_next += 2;
          n_left_from -= 2;
          n_left_to_next -= 2;

          b0 = vlib_get_buffer (vm, bi0);
          b1 = vlib_get_buffer (vm, bi1);

          ASSERT (b0->current_data == 0);
          ASSERT (b1->current_data == 0);

          vnet_feature_next (&next0, b0);
          vnet_feature_next (&next1, b1);

          gibirix_police_ethernet_packet (vm, now, b0, &next0);
          gibirix_police_ethernet_packet (vm, now, b1, &next1);
          
          /* verify speculative enqueues, maybe switch current next frame */
          vlib_validate_buffer_enqueue_x2 (vm, node, next_index,
                                             to_next, n_left_to_next,
                                             bi0, bi1, next0, next1);
      }

    while (n_left_from > 0 && n_left_to_next > 0)
     {
          u32 bi0;
          vlib_buffer_t * b0;
          u32 next0 = MAC_POLICER_NEXT_INTERFACE_DROP;

          /* speculatively enqueue b0 to the current next frame */
          bi0 = from[0];
          to_next[0] = bi0;
          from += 1;
          to_next += 1;
          n_left_from -= 1;
          n_left_to_next -= 1;

          b0 = vlib_get_buffer (vm, bi0);

          /* 
           * Direct from the driver, we should be at offset 0
           * aka at &b0->data[0]
           */
          ASSERT (b0->current_data == 0);
          
          vnet_feature_next (&next0, b0);

          gibirix_police_ethernet_packet (vm, now, b0, &next0);

          /* verify speculative enqueue, maybe switch current next frame */
          vlib_validate_buffer_enqueue_x1 (vm, node, next_index,
             to_next, n_left_to_next,
             bi0, next0);
       }

      vlib_put_next_frame (vm, node, next_index, n_left_to_next);
    }

  return frame->n_vectors;
}

vlib_node_registration_t mac_inbound_policer_node;
vlib_node_registration_t mac_outboun_policer_node;

#endif /* CLIB_MARCH_VARIANT */

#define foreach_mac_policer_error \
_(SWAPPED, "Mac swap packets processed")

typedef enum {
#define _(sym,str) MAC_POLICER_ERROR_##sym,
  foreach_mac_policer_error
#undef _
  MAC_POLICER_N_ERROR,
} mac_policer_error_t;

#ifndef CLIB_MARCH_VARIANT
static char * mac_policer_error_strings[] = 
{
#define _(sym,string) string,
  foreach_mac_policer_error
#undef _
};
#endif /* CLIB_MARCH_VARIANT */


VLIB_NODE_FN (mac_inbound_policer_node) (vlib_main_t * vm,
		  vlib_node_runtime_t * node,
		  vlib_frame_t * frame)
{
  return gibirix_mac_policer_node_fn(vm, node, frame);
}

VLIB_NODE_FN (mac_outbound_policer_node) (vlib_main_t * vm,
      vlib_node_runtime_t * node,
      vlib_frame_t * frame)
{
  return gibirix_mac_policer_node_fn(vm, node, frame);
}

/* *INDENT-OFF* */
#ifndef CLIB_MARCH_VARIANT
VLIB_REGISTER_NODE (mac_inbound_policer_node) = 
{
  .name = "mac_policer_inbound",
  .vector_size = sizeof (u32),
  .type = VLIB_NODE_TYPE_INTERNAL,
  
  .n_errors = ARRAY_LEN(mac_policer_error_strings),
  .error_strings = mac_policer_error_strings,

  .n_next_nodes = MAC_POLICER_N_NEXT,

  /* edit / add dispositions here */
  .next_nodes = {
        [MAC_POLICER_NEXT_INTERFACE_DROP] = "error-drop",
  },
};

VLIB_REGISTER_NODE (mac_outbound_policer_node) = 
{
  .name = "mac_policer_outbound",
  .vector_size = sizeof (u32),
  .type = VLIB_NODE_TYPE_INTERNAL,
  
  .n_errors = ARRAY_LEN(mac_policer_error_strings),
  .error_strings = mac_policer_error_strings,

  .n_next_nodes = MAC_POLICER_N_NEXT,

  /* edit / add dispositions here */
  .next_nodes = {
        [MAC_POLICER_NEXT_INTERFACE_DROP] = "error-drop",
  },
};
#endif /* CLIB_MARCH_VARIANT */
/* *INDENT-ON* */
/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
