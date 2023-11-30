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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "cell_info.h"
#include "memutil.h"
#include "log.h"
#include "target.h"
#include "qm_conn.h"
#include "cellm_mgr.h"

struct cell_info_packed_buffer *serialized_report = NULL;
struct cell_info_report *report = NULL;

char cell_mqtt_topic[256];

static uint32_t cell_request_id = 0;

static int
cell_send_report(char *topic, struct cell_info_packed_buffer *pb)
{
#ifndef ARCH_X86
    qm_response_t res;
    bool ret = false;
#endif

    if (!topic)
    {
        LOGE("%s: topic NULL", __func__);
        return -1;
    }

    if (!pb)
    {
        LOGE("%s: pb NULL", __func__);
        return -1;
    }

    if (!pb->buf)
    {
        LOGE("%s: pb->buf NULL", __func__);
        return -1;
    }

    LOGI("%s: msg len: %zu, topic: %s",
         __func__, pb->len, topic);

#ifndef ARCH_X86
    ret = qm_conn_send_direct(QM_REQ_COMPRESS_IF_CFG, topic,
                              pb->buf, pb->len, &res);
    if (!ret)
    {
        LOGE("error sending mqtt with topic %s", topic);
        return -1;
    }
#endif

    return 0;
}

/**
 * @brief Set the common header
 */
int
cell_set_common_header(struct cell_info_report *report)
{
    bool ret;
    struct cell_common_header common_header;
    cellm_mgr_t *mgr = cellm_get_mgr();

    if (!report) return -1;

    LOGI("%s: if_name[%s], node_id[%s], location_id[%s], imei[%s], imsi[%s], iccid[%s], modem_info[%s]",
         __func__, mgr->modem_info->header.if_name, mgr->node_id, mgr->location_id, mgr->modem_info->header.imei,
         mgr->modem_info->header.imsi, mgr->modem_info->header.iccid, mgr->modem_info->header.modem_info);
    MEMZERO(common_header);
    common_header.request_id = cell_request_id++;
    STRSCPY(common_header.if_name, mgr->modem_info->header.if_name);
    STRSCPY(common_header.node_id, mgr->node_id);
    STRSCPY(common_header.location_id, mgr->location_id);
    STRSCPY(common_header.imei, mgr->modem_info->header.imei);
    STRSCPY(common_header.imsi, mgr->modem_info->header.imsi);
    STRSCPY(common_header.iccid, mgr->modem_info->header.iccid);
    STRSCPY(common_header.modem_info, mgr->modem_info->header.modem_info);
    // reported_at is set in cell_info_set_common_header
    ret = cell_info_set_common_header(&common_header, report);
    if (!ret) return -1;

    return 0;
}

/**
 * @brief Set the cell_net_info in the report
 */
int
cell_set_net_info(struct cell_info_report *report)
{
    bool ret;
    struct cell_net_info net_info;
    cellm_mgr_t *mgr = cellm_get_mgr();

    net_info.net_status = mgr->modem_info->cell_net_info.net_status;
    net_info.mcc = mgr->modem_info->cell_net_info.mcc;
    net_info.mnc = mgr->modem_info->cell_net_info.mnc;
    net_info.tac = mgr->modem_info->cell_net_info.tac;
    STRSCPY(net_info.service_provider, mgr->modem_info->cell_net_info.service_provider);
    net_info.sim_type = mgr->modem_info->cell_net_info.sim_type;
    net_info.sim_status = mgr->modem_info->cell_net_info.sim_status;
    net_info.active_sim_slot = mgr->modem_info->cell_net_info.active_sim_slot;
    net_info.rssi = mgr->modem_info->cell_net_info.rssi;
    net_info.ber = mgr->modem_info->cell_net_info.ber;
    net_info.endc = mgr->modem_info->cell_net_info.endc;
    net_info.mode = mgr->modem_info->cell_net_info.mode;

    ret = cell_info_set_net_info(&net_info, report);
    if (!ret) return -1;

    return 0;
}

/**
 * @brief Set cell_data_usage in the report
 */
int
cell_set_data_usage(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_data_usage data_usage;

    LOGI("%s: rx_bytes[%lu], tx_bytes[%lu]", __func__,
         mgr->modem_info->cell_data_usage.rx_bytes,
         mgr->modem_info->cell_data_usage.tx_bytes);
    data_usage.rx_bytes = mgr->modem_info->cell_data_usage.rx_bytes;
    data_usage.tx_bytes = mgr->modem_info->cell_data_usage.tx_bytes;
    ret = cell_info_set_data_usage(&data_usage, report);
    if (!ret) return -1;

    return 0;

}

/**
 * @brief Set serving cell info
 */
int
cell_set_serving_cell(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct lte_serving_cell_info *srv_cell;

    srv_cell = &mgr->modem_info->cell_srv_cell;

    LOGI("%s: state[%d], fdd_tdd_mode[%d], cellid[%d] pcid[%d], uarfcn[%d], "
         "earfcn[%d], band[%d], ul_bandwidth[%d] dl_bandwidth[%d], tac[%d], rsrp[%d], "
         "rsrq[%d], rssi[%d], sinr[%d], srxlev[%d] endc[%d]",
         __func__, srv_cell->state, srv_cell->fdd_tdd_mode, srv_cell->cellid,
         srv_cell->pcid, srv_cell->uarfcn, srv_cell->earfcn, srv_cell->band,
         srv_cell->ul_bandwidth, srv_cell->dl_bandwidth, srv_cell->tac, srv_cell->rsrp,
         srv_cell->rsrq, srv_cell->rssi, srv_cell->sinr, srv_cell->srxlev, srv_cell->endc);

    ret = cell_info_set_serving_cell(srv_cell, report);
    if (!ret) return -1;

    return 0;
}

/**
 * @brief Set intra neighbor cell info in the report
 */
int
cell_set_neigh_cell_info(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_net_neighbor_cell_info *neigh_cell;
    int i;

    for (i = 0; i < MAX_CELL_COUNT; i++)
    {
        neigh_cell = &mgr->modem_info->cell_neigh_cell_info[i];
        ret = cell_info_add_neigh_cell(neigh_cell, report);
        if (!ret) return -1;
    }

    return 0;
}


/**
 * @brief Set carrier aggregation info to the report
 */
int
cell_set_pca_info(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_net_pca_info *pca_info;

    pca_info  = &mgr->modem_info->cell_pca_info;
    LOGI("%s: lcc[%d], freq[%d], bandwidth[%d], pcell_state[%d], pcid[%d], rsrp[%d], rsrq[%d], rssi[%d], sinr[%d]",
         __func__, pca_info->lcc, pca_info->freq, pca_info->bandwidth, pca_info->pcell_state, pca_info->pcid, pca_info->rsrp, pca_info->rsrq,
         pca_info->rssi, pca_info->sinr);
    ret = cell_info_set_pca(pca_info, report);
    if (!ret) return -1;

    return 0;
}

/**
 * @brief Set LTE secondary aggregation info
 */
int
cell_set_lte_sca_info(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_net_lte_sca_info *lte_sca_info;
    size_t i;

    for (i = 0; i < mgr->modem_info->n_lte_sca_cells; i++)
    {
        lte_sca_info = &mgr->modem_info->cell_lte_sca_info[i];
        if (lte_sca_info == NULL) return -1;
        LOGI("%s: lcc[%d], freq[%d], bandwidth[%d], scell_state[%d], pcid[%d], rsrp[%d], rsrq[%d], rssi[%d], sinr[%d]",
             __func__, lte_sca_info->lcc, lte_sca_info->freq, lte_sca_info->bandwidth, lte_sca_info->scell_state,
             lte_sca_info->pcid, lte_sca_info->rsrp, lte_sca_info->rsrq, lte_sca_info->rssi, lte_sca_info->sinr);
        ret = cell_info_add_lte_sca(lte_sca_info, report);
        if (!ret) return -1;
    }

    return 0;
}

/**
 * @brief Set dynamic pdp context parameters info
 */
int
cell_set_pdp_ctx_info(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_pdp_ctx_dynamic_params_info *pdp_ctx;
    size_t i;

    for (i = 0; i < mgr->modem_info->n_pdp_cells; i++)
    {
        pdp_ctx = &mgr->modem_info->cell_pdp_ctx_info[i];
        if (pdp_ctx == NULL) return -1;
        ret = cell_info_add_pdp_ctx(pdp_ctx, report);
        if (!ret) return -1;
    }

    return 0;

}

/**
 * @brief Set nr5g sa serving cell
 */
int cell_set_nr5g_sa_serving_cell(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_nr5g_cell_info *srv_cell_info;

    srv_cell_info = &mgr->modem_info->nr5g_sa_srv_cell;
    LOGI("%s: state[%d], fdd_tdd_mode[%d], mcc[%d], mnc[%d], cellid[%d], pcid[%d], tac[%d], arfcn[%d], band[%d], "
         "ul_bandwidth[%d], dl_bandwidth[%d], rsrp[%d], rsrq[%d], sinr[%d], scs[%d], srxlev[%d], layers[%d], mcs[%d], "
         "modulation[%d]", __func__,
         srv_cell_info->state, srv_cell_info->fdd_tdd_mode, srv_cell_info->mcc, srv_cell_info->mnc,
         srv_cell_info->cellid, srv_cell_info->pcid, srv_cell_info->tac, srv_cell_info->arfcn, srv_cell_info->band,
         srv_cell_info->ul_bandwidth, srv_cell_info->dl_bandwidth, srv_cell_info->rsrp, srv_cell_info->rsrq,
         srv_cell_info->sinr, srv_cell_info->scs, srv_cell_info->srxlev, srv_cell_info->layers, srv_cell_info->mcs,
         srv_cell_info->modulation);
    ret = cell_info_set_nr5g_sa_serving_cell(srv_cell_info, report);
    if (!ret) return -1;

    return 0;
}

/**
 * @brief Set nr5g nsa serving cell
 */
int cell_set_nr5g_nsa_serving_cell(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_nr5g_cell_info *srv_cell_info;

    srv_cell_info = &mgr->modem_info->nr5g_nsa_srv_cell;
    LOGI("%s: state[%d], fdd_tdd_mode[%d], mcc[%d], mnc[%d], cellid[%d], pcid[%d], tac[%d], arfcn[%d], band[%d], "
         "ul_bandwidth[%d], dl_bandwidth[%d], rsrp[%d], rsrq[%d], sinr[%d], scs[%d], srxlev[%d], layers[%d], mcs[%d], "
         "modulation[%d]", __func__,
         srv_cell_info->state, srv_cell_info->fdd_tdd_mode, srv_cell_info->mcc, srv_cell_info->mnc,
         srv_cell_info->cellid, srv_cell_info->pcid, srv_cell_info->tac, srv_cell_info->arfcn, srv_cell_info->band,
         srv_cell_info->ul_bandwidth, srv_cell_info->dl_bandwidth, srv_cell_info->rsrp, srv_cell_info->rsrq,
         srv_cell_info->sinr, srv_cell_info->scs, srv_cell_info->srxlev, srv_cell_info->layers, srv_cell_info->mcs,
         srv_cell_info->modulation);
    ret = cell_info_set_nr5g_nsa_serving_cell(srv_cell_info, report);
    if (!ret) return -1;

    return 0;
}

/**
 * @brief Set nrg secondary aggregation info
 */
int
cell_set_nrg_sca_info(struct cell_info_report *report)
{
    bool ret;
    cellm_mgr_t *mgr = cellm_get_mgr();
    struct cell_nr5g_cell_info *nrg_sca_info;
    size_t i;

    for (i = 0; i < mgr->modem_info->n_nrg_sca_cells; i++)
    {
        nrg_sca_info = &mgr->modem_info->cell_nrg_sca_info[i];
        if (nrg_sca_info == NULL) return -1;
        ret = cell_info_add_nrg_sca(nrg_sca_info, report);
        if (!ret) return -1;
    }

    return 0;
}

/**
 * @brief set a full report
 */
int
cell_set_report(void)
{
    int res;
    cellm_mgr_t *mgr = cellm_get_mgr();


    report = cell_info_allocate_report(mgr->modem_info->n_neigh_cells, mgr->modem_info->n_lte_sca_cells,
                                       mgr->modem_info->n_pdp_cells, mgr->modem_info->n_nrg_sca_cells);
    if (!report)
    {
        LOGE("report calloc failed");
        return -1;
    }

    /* Set the common header */
    res = cell_set_common_header(report);
    if (res)
    {
        LOGE("Failed to set common header");
        return res;
    }

    /* Add the cell net info */
    res = cell_set_net_info(report);
    if (res)
    {
        LOGE("Failed to set net info");
        return res;
    }

    /* Add the cell data usage */
    res = cell_set_data_usage(report);
    if (res)
    {
        LOGE("Failed to set data usage");
        return res;
    }

    /* Add the serving cell info */
    res = cell_set_serving_cell(report);
    if (res)
    {
        LOGE("Failed to set serving cell");
        return res;
    }

    /* Add neighbor cell info */
    res = cell_set_neigh_cell_info(report);
    if (res)
    {
        LOGE("Failed to set neighbor cell");
        return res;
    }

    /* Add primary carrier aggregation info */
    res = cell_set_pca_info(report);
    if (res) {
        LOGE("Failed to set carrier aggregation info");
        return res;
    }

    /* Add lte secondary aggregation info */
    res = cell_set_lte_sca_info(report);
    if (res)
    {
        LOGE("Failed to set lte sca info");
        return res;
    }

    /* Add pdp context dynamic parameters info */
    res = cell_set_pdp_ctx_info(report);
    if (res)
    {
        LOGE("Failed to set pdp context info");
        return res;
    }

    /* Add nr5g_sa serving cell info */
    res = cell_set_nr5g_sa_serving_cell(report);
    if (res)
    {
        LOGE("Failed to set nr5g_sa srv cell info");
        return res;
    }

    /* Add nr5g_nsa serving cell info */
    res = cell_set_nr5g_nsa_serving_cell(report);
    if (res)
    {
        LOGE("Failed to set nr5g_nsa srv cell info");
        return res;
    }

    /* Add nrg secondary aggregation info */
    res = cell_set_nrg_sca_info(report);
    if (res)
    {
        LOGE("Failed to set nrg sca info");
        return res;
    }

    return 0;

}

void
cell_mqtt_cleanup(void)
{
    cell_info_free_report(report);
    cell_info_free_packed_buffer(serialized_report);
    return;
}

/**
 * @brief serialize a full report
 */
int
cell_serialize_report(void)
{
    int res;
    cellm_mgr_t *mgr = cellm_get_mgr();

    res = cell_set_report();
    if (res) return res;

    serialized_report = serialize_cell_info(report);
    if (!serialized_report) return -1;


    if (mgr->topic[0])
    {
        res = cell_send_report(mgr->topic, serialized_report);
        LOGD("%s: AWLAN topic[%s]", __func__, mgr->topic);
    }
    else
    {
        LOGE("%s: AWLAN topic: not set, mqtt report not sent", __func__);
        return -1;
    }
    return res;
}

int
cellm_build_mqtt_report(time_t now)
{
    int res;

    LOGI("%s()", __func__);
    res = osn_cell_read_modem();
    if (res < 0)
    {
        LOGW("%s: osn_cell_read_modem() failed", __func__);
    }

    res = cell_serialize_report();
    if (res)
    {
        LOGW("%s: cell_serialize_report: failed", __func__);
    }

    cell_mqtt_cleanup();

    return res;
}
