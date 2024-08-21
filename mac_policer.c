/*
 * mac_policer.c - skeleton vpp engine plug-in
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

#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <mac_policer/mac_policer.h>

#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vpp/app/version.h>
#include <stdbool.h>

#include <mac_policer/mac_policer.api_enum.h>
#include <mac_policer/mac_policer.api_types.h>

#define REPLY_MSG_ID_BASE mmp->msg_id_base
#include <vlibapi/api_helper_macros.h>

mac_policer_main_t mac_policer_main;

/* Action function shared between message handler and debug CLI */

void add_mac_policer(mac_policer_main_t * mmp, u64 mac, u64 current_limit);

int mac_policer_enable_disable (mac_policer_main_t * mmp, int enable_disable)
{
  int rv = 0;
  
  vnet_sw_interface_t *swif;

  pool_foreach (swif, mmp->vnet_main->interface_main.sw_interfaces)
  {
    if (swif->type == VNET_SW_INTERFACE_TYPE_HARDWARE){
      vnet_feature_enable_disable ("device-input", "mac_policer_inbound", swif->sw_if_index, enable_disable, 0, 0);
      // vnet_feature_enable_disable ("interface-output", "mac_policer_outbound", swif->sw_if_index, enable_disable, 0, 0);
    }
  }

  return rv;
}

void add_mac_policer(mac_policer_main_t * mmp, u64 mac, u64 current_limit)
{
  double period = 1000;
  double internal_cir_bytes_per_period;
  u32 max;
  u32 scale_shift;
  u32 scale_amount;
  u8 is_add = 1;

  mac_policer policer; 
  mac_policer *walk_policer; 

  policer.mac = mac;
  policer.current_limit = current_limit >> 3;

  internal_cir_bytes_per_period = (double) (policer.current_limit) / period;
  /* maximize the resolution */ 
  #define MAX_RATE_SHIFT 10
  max = policer.current_limit;
  max = MAX (max, (u64) internal_cir_bytes_per_period << MAX_RATE_SHIFT);
  scale_shift = __builtin_clz (max);
  scale_amount = 1 << scale_shift;
  policer.scale = scale_shift;
  policer.current_limit = policer.current_limit << scale_shift;
  policer.cir_tokens_per_period = (u64)(internal_cir_bytes_per_period * ((double) scale_amount));

  if(policer.cir_tokens_per_period < 1){
    policer.cir_tokens_per_period = 1;
  }

  vec_foreach(walk_policer, mmp->policer)
  { 
    if(walk_policer->mac == mac){
      walk_policer->scale=policer.scale;
      walk_policer->current_limit=policer.current_limit;
      walk_policer->cir_tokens_per_period=policer.cir_tokens_per_period;
      is_add = 0;
      break;
    }
  }


  if(is_add){
    policer.last_update_time = vec_new(u32, vec_len(vlib_worker_threads));
    vec_set(policer.last_update_time, 0);
    vec_add1(mmp->policer, policer);
  }
}

static clib_error_t * 
mac_policer_enable_disable_command_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
  mac_policer_main_t * mmp = &mac_policer_main;

  int enable_disable = 1;
  int rv;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "disable"))
        enable_disable = 0;
      else
        break;
  }

  rv = mac_policer_enable_disable (mmp, enable_disable);

  switch(rv)
    {
  case 0:
    break;

  default:
    return clib_error_return (0, "mac_policer_enable_disable returned %d",
                              rv);
    }
  return 0;
}

static clib_error_t * 
mac_policer_add_command_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
  mac_policer_main_t * mmp = &mac_policer_main;

  u64 mac = 0;
  u64 limit = 0;

  /*
  mac_policer enable-disable
  mac_policer add mac b4de3113cdeb limit 100000
  */

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "mac %llx", &mac))
        ;
      else if (unformat (input, "limit %llu", &limit))
        ;
      else
        break;
  }

  clib_warning("map_policer added/modified mac %llx limit %llu", mac, limit);
  add_mac_policer(mmp, mac, limit);

  return 0;
}

/* *INDENT-OFF* */
VLIB_CLI_COMMAND (mac_policer_enable_disable_command, static) =
{
  .path = "mac_policer enable-disable",
  .short_help =
  "mac_policer enable-disable [disable]",
  .function = mac_policer_enable_disable_command_fn,
};

VLIB_CLI_COMMAND (mac_policer_add_command, static) =
{
  .path = "mac_policer add",
  .short_help =
  "mac_policer add mac <mac> limit <limit-in-bits>",
  .function = mac_policer_add_command_fn,
};
/* *INDENT-ON* */

/* API message handler */
static void vl_api_mac_policer_enable_disable_t_handler
(vl_api_mac_policer_enable_disable_t * mp)
{
  vl_api_mac_policer_enable_disable_reply_t * rmp;
  mac_policer_main_t * mmp = &mac_policer_main;
  int rv;

  rv = mac_policer_enable_disable (mmp, (int) (mp->enable_disable));

  REPLY_MACRO(VL_API_MAC_POLICER_ENABLE_DISABLE_REPLY);
}

/* API definitions */
#include <mac_policer/mac_policer.api.c>

static clib_error_t * mac_policer_init (vlib_main_t * vm)
{
  mac_policer_main_t * mmp = &mac_policer_main;
  clib_error_t * error = 0;

  mmp->vlib_main = vm;
  mmp->vnet_main = vnet_get_main();

  /* Add our API messages to the global name_crc hash table */
  mmp->msg_id_base = setup_message_id_table ();

  return error;
}

VLIB_INIT_FUNCTION (mac_policer_init);

/* *INDENT-OFF* */
VNET_FEATURE_INIT (mac_policer_inbound, static) =
{
  .arc_name = "device-input",
  .node_name = "mac_policer_inbound",
  .runs_before = VNET_FEATURES ("ethernet-input"),
};
/* *INDENT-ON */

/* *INDENT-OFF* */
VNET_FEATURE_INIT (mac_policer_outbound, static) =
{
  .arc_name = "interface-output",
  .node_name = "mac_policer_outbound",
  // .runs_before = VNET_FEATURES ("ethernet-output"),
};
/* *INDENT-ON */

/* *INDENT-OFF* */
VLIB_PLUGIN_REGISTER () =
{
  .version = VPP_BUILD_VER,
  .description = "mac_policer is a policer based on mac for GIBIRIX",
};
/* *INDENT-ON* */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
