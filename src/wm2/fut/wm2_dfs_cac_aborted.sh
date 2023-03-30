#!/bin/sh

# Copyright (c) 2015, Plume Design Inc. All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    3. Neither the name of the Plume Design Inc. nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# FUT environment loading
# shellcheck disable=SC1091
source /tmp/fut-base/shell/config/default_shell.sh
[ -e "/tmp/fut-base/fut_set_env.sh" ] && source /tmp/fut-base/fut_set_env.sh
source "${FUT_TOPDIR}/shell/lib/unit_lib.sh"
[ -e "${PLATFORM_OVERRIDE_FILE}" ] && source "${PLATFORM_OVERRIDE_FILE}" || raise "${PLATFORM_OVERRIDE_FILE}" -ofm
[ -e "${MODEL_OVERRIDE_FILE}" ] && source "${MODEL_OVERRIDE_FILE}" || raise "${MODEL_OVERRIDE_FILE}" -ofm

manager_setup_file="wm2/wm2_setup.sh"
# Wait for channel to change, not necessarily become usable (CAC for DFS)
channel_change_timeout=60

usage()
{
cat << usage_string
wm2/wm2_dfs_cac_aborted.sh [-h] arguments
Testcase info:
    Setup:
        - If channels provided in testcase config are not both in the 'nop_finished' or
        'available' state, the script would find alternative channels and execute
        with new channels. Actual used channels would be reported inside log_title.
    Problem statement (example):
        - Start on DFS channel_a and wait for CAC to complete before channel is usable
        - Radar is detected while channel_a CAC is in progress (cac = 1-10 min)
        - Driver should switch to channel_b immediately, and not wait for CAC to finish
    Script tests the following:
      - CAC must be aborted on channel_a, if channel change is requested while CAC is in progress.
      - Correct transition to "cac_started" on channel_a
      - Correct transition to "nop_finished" on channel_a after transition to channel_b
    Simplified test steps (example):
        - Ensure <CHANNEL_A> and <CHANNEL_B> are allowed
        - Verify if <CHANNEL_A> is in nop_finished state
        - Switch <CHANNEL_A> to new channel if not in "nop_finished" state
        - Verify if <CHANNEL_B> is in nop_finished state
        - Switch <CHANNEL_B> to new channel if not in "nop_finished" state
        - Configure radio, create VIF and apply <CHANNEL_A>
        - Verify if <CHANNEL_A> is applied
        - Verify if <CHANNEL_A> has started CAC
        - Change to <CHANNEL_B> while CAC is in progress
        - Verify if <CHANNEL_B> is applied
        - Verify if <CHANNEL_A> has stopped CAC and entered NOP_FINISHED
        - Verify if <CHANNEL_B> has started CAC
Arguments:
    -h  show this help message
    (if_name)          : Wifi_Radio_Config::if_name        : (string)(required)
    (vif_if_name)      : Wifi_VIF_Config::if_name          : (string)(required)
    (vif_radio_idx)    : Wifi_VIF_Config::vif_radio_idx    : (int)(required)
    (ssid)             : Wifi_VIF_Config::ssid             : (string)(required)
    (channel_a)        : Wifi_Radio_Config::channel        : (int)(required)
    (channel_b)        : Wifi_Radio_Config::channel        : (int)(required)
    (ht_mode)          : Wifi_Radio_Config::ht_mode        : (string)(required)
    (hw_mode)          : Wifi_Radio_Config::hw_mode        : (string)(required)
    (mode)             : Wifi_VIF_Config::mode             : (string)(required)
    (channel_mode)     : Wifi_Radio_Config::channel_mode   : (string)(required)
    (enabled)          : Wifi_Radio_Config::enabled        : (string)(required)
    (wifi_security_type) : 'wpa' if wpa fields are used or 'legacy' if security fields are used: (string)(required)

Wifi Security arguments(choose one or the other):
    If 'wifi_security_type' == 'wpa' (preferred)
    (wpa)              : Wifi_VIF_Config::wpa              : (string)(required)
    (wpa_key_mgmt)     : Wifi_VIF_Config::wpa_key_mgmt     : (string)(required)
    (wpa_psks)         : Wifi_VIF_Config::wpa_psks         : (string)(required)
    (wpa_oftags)       : Wifi_VIF_Config::wpa_oftags       : (string)(required)
                    (OR)
    If 'wifi_security_type' == 'legacy' (deprecated)
    (security)         : Wifi_VIF_Config::security         : (string)(required)
Testcase procedure:
    - On DEVICE: Run: ./${manager_setup_file} (see ${manager_setup_file} -h)
                 Run: ./wm2/wm2_dfs_cac_aborted.sh -if_name <IF_NAME> -vif_if_name <VIF_IF_NAME> -vif_radio_idx <VIF-RADIO-IDX> -ssid <SSID> -channel_a <CHANNEL_A> -channel_b <CHANNEL_B> -ht_mode <HT_MODE> -hw_mode <HW_MODE> -mode <MODE> -channel_mode <CHANNEL_MODE> -enabled <ENABLED> -wifi_security_type <WIFI_SECURITY_TYPE> -wpa <WPA> -wpa_key_mgmt <WPA_KEY_MGMT> -wpa_psks <WPA_PSKS> -wpa_oftags <WPA_OFTAGS>
                             (OR)
                 Run: ./wm2/wm2_dfs_cac_aborted.sh -if_name <IF_NAME> -vif_if_name <VIF_IF_NAME> -vif_radio_idx <VIF-RADIO-IDX> -ssid <SSID> -channel_a <CHANNEL_A> -channel_b <CHANNEL_B> -ht_mode <HT_MODE> -hw_mode <HW_MODE> -mode <MODE> -channel_mode <CHANNEL_MODE> -enabled <ENABLED> -wifi_security_type <WIFI_SECURITY_TYPE> -security <SECURITY>
Script usage example:
    ./wm2/wm2_dfs_cac_aborted.sh -if_name wifi2 -vif_if_name home-ap-u50 -vif_radio_idx 2 -ssid FUTssid -channel_a 120 -channel_b 104 -ht_mode HT20 -hw_mode 11ac -mode ap -wifi_security_type wpa -wpa "true" -wpa_key_mgmt "wpa-psk" -wpa_psks '["map",[["key","FutTestPSK"]]]' -wpa_oftags '["map",[["key","home--1"]]]'
    ./wm2/wm2_dfs_cac_aborted.sh -if_name wifi2 -vif_if_name home-ap-u50 -vif_radio_idx 2 -ssid FUTssid -channel_a 120 -channel_b 104 -ht_mode HT20 -hw_mode 11ac -mode ap -channel_mode manual -enabled true -wifi_security_type legacy -security '["map",[["encryption","WPA-PSK"],["key","FutTestPSK"]]]'
usage_string
}
if [ -n "${1}" ]; then
    case "${1}" in
        help | \
        --help | \
        -h)
            usage && exit 1
            ;;
        *)
            ;;
    esac
fi

NARGS=26
[ $# -lt ${NARGS} ] && usage && raise "Requires at least '${NARGS}' input argument(s)" -l "wm2/wm2_dfs_cac_aborted.sh" -arg

trap '
    fut_info_dump_line
    print_tables Wifi_Radio_Config Wifi_Radio_State
    check_restore_ovsdb_server
    fut_info_dump_line
' EXIT SIGINT SIGTERM

# Parsing arguments passed to the script.
while [ -n "$1" ]; do
    option=$1
    shift
    case "$option" in
        -ht_mode | \
        -mode | \
        -vif_if_name | \
        -vif_radio_idx | \
        -hw_mode | \
        -ssid | \
        -channel_mode | \
        -enabled)
            radio_vif_args="${radio_vif_args} -${option#?} ${1}"
            shift
            ;;
        -if_name)
            if_name=${1}
            radio_vif_args="${radio_vif_args} -${option#?} ${if_name}"
            shift
            ;;
        -channel_a)
            channel_a=${1}
            radio_vif_args="${radio_vif_args} -channel ${channel_a}"
            shift
            ;;
        -channel_b)
            channel_b=${1}
            shift
            ;;
        -wifi_security_type)
            wifi_security_type=${1}
            shift
            ;;
        -wpa | \
        -wpa_key_mgmt | \
        -wpa_psks | \
        -wpa_oftags)
            [ "${wifi_security_type}" != "wpa" ] && raise "FAIL: Incorrect combination of WPA and legacy wifi security type provided" -l "wm2/wm2_dfs_cac_aborted.sh" -arg
            radio_vif_args="${radio_vif_args} -${option#?} ${1}"
            shift
            ;;
        -security)
            [ "${wifi_security_type}" != "legacy" ] && raise "FAIL: Incorrect combination of WPA and legacy wifi security type provided" -l "wm2/wm2_dfs_cac_aborted.sh" -arg
            radio_vif_args="${radio_vif_args} -${option#?} ${1}"
            shift
            ;;
        *)
            raise "FAIL: Wrong option provided: $option" -l "wm2/wm2_dfs_cac_aborted.sh" -arg
            ;;
    esac
done

# Performs sanity check(channels allowed on the radio) and verifies configured
# channel is in nop_finished state for the test. If not, switch to new channel.
channel_a=$(verify_channel_is_in_nop_finished "$if_name" "$channel_a" "$channel_b")
channel_b=$(verify_channel_is_in_nop_finished "$if_name" "$channel_b" "$channel_a")

log_title "wm2/wm2_dfs_cac_aborted.sh: WM2 test - DFC CAC Aborted - Using: '${channel_a}'->'${channel_b}'"

# Testcase:
# Configure radio, create VIF and apply channel
# This needs to be done simultaneously for the driver to bring up an active AP
# Function only checks if the channel is set in Wifi_Radio_State, not if it is
# available for immediate use, so CAC could be in progress. This is desired.
log "wm2/wm2_dfs_cac_aborted.sh: Configuring Wifi_Radio_Config, creating interface in Wifi_VIF_Config."
log "wm2/wm2_dfs_cac_aborted.sh: Waiting for ${channel_change_timeout}s for settings {channel:$channel_a}"
create_radio_vif_interface \
    ${radio_vif_args} \
    -timeout ${channel_change_timeout} \
    -disable_cac &&
        log "wm2/wm2_dfs_cac_aborted.sh: create_radio_vif_interface {$if_name, $channel_a} - Success" ||
        raise "FAIL: create_radio_vif_interface {$if_name, $channel_a} - Interface not created" -l "wm2/wm2_dfs_cac_aborted.sh" -tc

wait_ovsdb_entry Wifi_Radio_State -w if_name "$if_name" -is channel "$channel_a" &&
    log "wm2/wm2_dfs_cac_aborted.sh: wait_ovsdb_entry - Wifi_Radio_Config reflected to Wifi_Radio_State::channel is $channel_a - Success" ||
    raise "FAIL: wait_ovsdb_entry - Failed to reflect Wifi_Radio_Config to Wifi_Radio_State::channel is not $channel_a" -l "wm2/wm2_dfs_cac_aborted.sh" -tc

wait_for_function_response 0 "check_is_cac_started $channel_a $if_name" &&
    log "wm2/wm2_dfs_cac_aborted.sh: wait_for_function_response - channel $channel_a - CAC STARTED - Success" ||
    raise "FAIL: wait_for_function_response - channel $channel_a - CAC NOT STARTED" -l "wm2/wm2_dfs_cac_aborted.sh" -tc

log "wm2/wm2_dfs_cac_aborted.sh: Do not wait for CAC to finish, changing channel to $channel_b"
update_ovsdb_entry Wifi_Radio_Config -w if_name "$if_name" -u channel "$channel_b" &&
    log "wm2/wm2_dfs_cac_aborted.sh: update_ovsdb_entry - Wifi_Radio_Config::channel is $channel_b - Success" ||
    raise "FAIL: update_ovsdb_entry - Failed to update Wifi_Radio_Config::channel is not $channel_b" -l "wm2/wm2_dfs_cac_aborted.sh" -tc

wait_ovsdb_entry Wifi_Radio_State -w if_name "$if_name" -is channel "$channel_b" &&
    log "wm2/wm2_dfs_cac_aborted.sh: wait_ovsdb_entry - Wifi_Radio_Config reflected to Wifi_Radio_State::channel is $channel_b - Success" ||
    raise "FAIL: wait_ovsdb_entry - Failed to reflect Wifi_Radio_Config to Wifi_Radio_State::channel is not $channel_b" -l "wm2/wm2_dfs_cac_aborted.sh" -tc

wait_for_function_response 0 "check_is_nop_finished $channel_a $if_name" &&
    log "wm2/wm2_dfs_cac_aborted.sh: wait_for_function_response - channel $channel_a - NOP FINISHED - Success" ||
    raise "FAIL: wait_for_function_response - channel $channel_a - NOP NOT FINISHED" -l "wm2/wm2_dfs_cac_aborted.sh" -tc

wait_for_function_response 0 "check_is_cac_started $channel_b $if_name" &&
    log "wm2/wm2_dfs_cac_aborted.sh: wait_for_function_response - channel $channel_b - CAC STARTED - Success" ||
    raise "FAIL: wait_for_function_response - channel $channel_b - CAC NOT STARTED" -l "wm2/wm2_dfs_cac_aborted.sh" -tc

pass
