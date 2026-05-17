#include "plc_link_ext.h"
#include "plc_link.h"

#include "plc_diag.h"
#include "plc_graph_loader.h"
#include "plc_link_uart.h"
#include "plc_persist.h"
#include "plc_persist_service.h"
#include "plc_platform.h"
#include "friendly_plc/plc_types.h"
#include "friendly_plc/plc_snapshot.h"
#include "friendly_plc/plc_memory.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define EXT_STATUS_VERSION 1u
#define EXT_STATUS_WEB_VERSION 2u
#define EXT_NODE_VERSION 1u
#define EXT_NODES_SNAPSHOT_VERSION 1u
#define EXT_MEM_VERSION 1u

#define PLC_HW_DI_TOTAL 1u
#define PLC_HW_DO_TOTAL 1u
#define PLC_HW_AI_TOTAL 0u
#define PLC_HW_AO_TOTAL 0u
#define PLC_HW_PWM_TOTAL 0u
#define PLC_HW_HSC_TOTAL 0u
#define PLC_HW_ENCODER_TOTAL 0u

#define EXT_STATUS_WEB_VERSION 2u
#define EXT_NODES_SNAPSHOT_VERSION 1u

#define U16_LE(p) ((uint16_t)((uint16_t)(p)[0] | ((uint16_t)(p)[1] << 8u)))

static void put_u16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8u) & 0xFFu);
}

static void put_u32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8u) & 0xFFu);
    p[2] = (uint8_t)((v >> 16u) & 0xFFu);
    p[3] = (uint8_t)((v >> 24u) & 0xFFu);
}

static void put_i32(uint8_t* p, int32_t v)
{
    put_u32(p, (uint32_t)v);
}

static void put_f32(uint8_t* p, float v)
{
    uint32_t bits = 0u;
    memcpy(&bits, &v, sizeof(bits));
    put_u32(p, bits);
}

/* If your plc_link_ext.c already has get_u16/get_u32, do not duplicate them. */
static uint16_t ext_get_u16(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static uint32_t ext_get_u32(const uint8_t* p)
{
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) |
           ((uint32_t)p[3] << 24u);
}

static float ext_get_f32(const uint8_t* p)
{
    uint32_t bits = ext_get_u32(p);
    float v = 0.0f;
    memcpy(&v, &bits, sizeof(v));
    return v;
}

static uint32_t bool_flag(bool v)
{
    return v ? 1u : 0u;
}

static bool send_status_ext(uint16_t seq)
{
    PlcRuntimeSnapshot rs;
    PlcGraphLoaderStatus gs;
    PlcLinkUartStats us;
    PlcPersistStatus ps;
    PlcPersistServiceStatus pss;
    PlcDiagStatus ds;

    memset(&rs, 0, sizeof(rs));
    memset(&gs, 0, sizeof(gs));
    memset(&us, 0, sizeof(us));
    memset(&ps, 0, sizeof(ps));
    memset(&pss, 0, sizeof(pss));
    memset(&ds, 0, sizeof(ds));

    (void)plc_snapshot_get_runtime(&rs);
    plc_graph_loader_get_status(&gs);
    plc_link_uart_get_stats(&us);
    plc_persist_get_status(&ps);
    plc_persist_service_get_status(&pss);
    plc_diag_get_status(&ds);

    uint8_t body[224];
    memset(body, 0, sizeof(body));

    uint32_t runtime_flags = 0u;
    if (rs.activeGraphValid) { runtime_flags |= 0x01u; }
    if (rs.running) { runtime_flags |= 0x02u; }
    if (rs.safeOrFaulted) { runtime_flags |= 0x04u; }

    uint32_t graph_flags = 0u;
    if (gs.upload_active) { graph_flags |= 0x01u; }
    if (gs.image_ready) { graph_flags |= 0x02u; }

    uint32_t persist_flags = 0u;
    if (ps.has_valid_image) { persist_flags |= 0x01u; }
    if (pss.initialized) { persist_flags |= 0x02u; }
    if (pss.busy) { persist_flags |= 0x04u; }
    if (pss.pending) { persist_flags |= 0x08u; }
    if (pss.copying) { persist_flags |= 0x10u; }

    /* Header */
    put_u32(&body[0], 0x31545853u); /* 'STX1' little-endian marker */
    put_u32(&body[4], EXT_STATUS_VERSION);
    put_u32(&body[8], plc_platform_now_ms());

    /* Runtime snapshot */
    put_u32(&body[12], runtime_flags);
    put_u32(&body[16], rs.cycleMs);
    put_u32(&body[20], (uint32_t)rs.nodeCount);
    put_u32(&body[24], rs.cycleCounter);
    put_u32(&body[28], rs.lastCycleUs);
    put_u32(&body[32], rs.maxCycleUs);
    put_u32(&body[36], rs.faultCounter);
    put_u32(&body[40], (uint32_t)rs.runtimeLastFault);

    /* Graph loader */
    put_u32(&body[44], gs.active_version);
    put_u32(&body[48], gs.upload_size);
    put_u32(&body[52], gs.uploaded_bytes);
    put_u32(&body[56], gs.expected_crc32);
    put_u32(&body[60], graph_flags);
    put_u32(&body[64], gs.last_error);

    /* UART */
    put_u32(&body[68], us.rx_frames_ok);
    put_u32(&body[72], us.rx_crc_errors);
    put_u32(&body[76], us.rx_size_errors);
    put_u32(&body[80], us.rx_overflows);
    put_u32(&body[84], us.rx_bytes);
    put_u32(&body[88], us.tx_frames);
    put_u32(&body[92], us.tx_errors);
    put_u32(&body[96], us.uart_errors);

    /* Persist A/B storage */
    put_u32(&body[100], persist_flags);
    put_u32(&body[104], (uint32_t)ps.last_result);
    put_u32(&body[108], (uint32_t)ps.active_slot);
    put_u32(&body[112], ps.active_sequence);
    put_u32(&body[116], ps.active_version);
    put_u32(&body[120], ps.active_size);
    put_u32(&body[124], ps.active_crc32);

    /* Async persist service */
    put_u32(&body[128], pss.request_count);
    put_u32(&body[132], pss.success_count);
    put_u32(&body[136], pss.fail_count);
    put_u32(&body[140], pss.rejected_count);
    put_u32(&body[144], pss.last_version);
    put_u32(&body[148], pss.last_size);
    put_u32(&body[152], pss.last_crc32);
    put_u32(&body[156], pss.last_duration_ms);
    put_u32(&body[160], (uint32_t)pss.last_result);

    /* RTOS diagnostics */
    put_u32(&body[164], ds.heap_total_bytes);
    put_u32(&body[168], ds.heap_free_bytes);
    put_u32(&body[172], ds.heap_min_ever_free_bytes);
    put_u32(&body[176], ds.task_registered_flags);

    put_u32(&body[180], ds.scan_stack_free_words);
    put_u32(&body[184], ds.scan_stack_used_words);
    put_u32(&body[188], ds.scan_stack_size_words);

    put_u32(&body[192], ds.link_rx_stack_free_words);
    put_u32(&body[196], ds.link_rx_stack_used_words);
    put_u32(&body[200], ds.link_rx_stack_size_words);

    put_u32(&body[204], ds.link_tx_stack_free_words);
    put_u32(&body[208], ds.link_tx_stack_used_words);
    put_u32(&body[212], ds.link_tx_stack_size_words);

    put_u32(&body[216], ds.persist_stack_free_words);
    put_u32(&body[220], ds.persist_stack_used_words);

    return plc_link_send_response(PLC_LINK_RSP_STATUS_EXT, seq, body, sizeof(body));
}

static bool send_status_web_v2(uint16_t seq)
{
    PlcRuntimeSnapshot rs;
    PlcGraphLoaderStatus gs;
    PlcLinkUartStats us;
    PlcPersistStatus ps;
    PlcPersistServiceStatus pss;
    PlcDiagStatus ds;

    memset(&rs, 0, sizeof(rs));
    memset(&gs, 0, sizeof(gs));
    memset(&us, 0, sizeof(us));
    memset(&ps, 0, sizeof(ps));
    memset(&pss, 0, sizeof(pss));
    memset(&ds, 0, sizeof(ds));

    (void)plc_snapshot_get_runtime(&rs);
    plc_graph_loader_get_status(&gs);
    plc_link_uart_get_stats(&us);
    plc_persist_get_status(&ps);
    plc_persist_service_get_status(&pss);
    plc_diag_get_status(&ds);

    uint8_t body[168];
    memset(body, 0, sizeof(body));

    uint32_t run_state = 0u;
    if (rs.safeOrFaulted) {
        run_state = 3u; /* FAULT */
    } else if (rs.running) {
        run_state = 1u; /* RUN */
    } else {
        run_state = 0u; /* STOP */
    }

    uint32_t capabilities = 0u;
    capabilities |= (1u << 0); /* NODE_SNAPSHOT later */
    capabilities |= (1u << 1); /* IO_SUMMARY later */

    uint32_t memory_usage_x100 = 0u;
    if (ds.heap_total_bytes > 0u) {
        uint32_t used = ds.heap_total_bytes - ds.heap_free_bytes;
        memory_usage_x100 = (used * 10000u) / ds.heap_total_bytes;
    }

    /*
     * WEB2 payload layout, little-endian:
     *
     *  0  u32 magic 'WEB2'
     *  4  u32 version
     *  8  u32 now_ms
     * 12  u32 capabilities
     * 16  u32 run_state: 0 STOP, 1 RUN, 2 SAFE, 3 FAULT
     *
     * Runtime:
     * 20  u32 cycle_ms
     * 24  u32 node_count
     * 28  u32 cycle_counter
     * 32  u32 last_cycle_us
     * 36  u32 max_cycle_us
     * 40  u32 fault_counter
     * 44  u32 runtime_last_fault
     *
     * Graph:
     * 48  u32 active_graph_version
     * 52  u32 active_graph_size
     * 56  u32 active_graph_crc32
     * 60  u32 uploaded_bytes
     * 64  u32 graph_last_error
     *
     * UART:
     * 68  u32 rx_ok
     * 72  u32 tx_frames
     * 76  u32 crc_errors
     * 80  u32 size_errors
     * 84  u32 rx_overflows
     * 88  u32 tx_errors
     * 92  u32 uart_errors
     *
     * Memory:
     * 96  u32 heap_total
     * 100 u32 heap_free
     * 104 u32 heap_min
     * 108 u32 memory_usage_x100
     *
     * Persist:
     * 112 u32 persist_active_version
     * 116 u32 persist_active_size
     * 120 u32 persist_active_crc32
     * 124 u32 persist_last_result
     *
    * Performance:
    * 128 u32 scan_avg_us
    * 132 u32 scan_max_us
    * 136 u32 work_avg_us
    * 140 u32 work_max_us
    * 144 u32 cycle_real_avg_us
    * 148 u32 cycle_real_max_us
    * 152 u32 scan_limit_ms
    * 156 u32 cpu_load_x100
    * 160 u32 scan_long_steps
    * 164..167 reserved
     */

    put_u32(&body[0],  0x32424557u); /* 'WEB2' */
    put_u32(&body[4],  EXT_STATUS_WEB_VERSION);
    put_u32(&body[8],  plc_platform_now_ms());
    put_u32(&body[12], capabilities);
    put_u32(&body[16], run_state);

    put_u32(&body[20], rs.cycleMs);
    put_u32(&body[24], (uint32_t)rs.nodeCount);
    put_u32(&body[28], rs.cycleCounter);
    put_u32(&body[32], rs.lastCycleUs);
    put_u32(&body[36], rs.maxCycleUs);
    put_u32(&body[40], rs.faultCounter);
    put_u32(&body[44], (uint32_t)rs.runtimeLastFault);

    put_u32(&body[48], gs.active_version);
    put_u32(&body[52], gs.upload_size);
    put_u32(&body[56], gs.expected_crc32);
    put_u32(&body[60], gs.uploaded_bytes);
    put_u32(&body[64], gs.last_error);

    put_u32(&body[68], us.rx_frames_ok);
    put_u32(&body[72], us.tx_frames);
    put_u32(&body[76], us.rx_crc_errors);
    put_u32(&body[80], us.rx_size_errors);
    put_u32(&body[84], us.rx_overflows);
    put_u32(&body[88], us.tx_errors);
    put_u32(&body[92], us.uart_errors);

    put_u32(&body[96],  ds.heap_total_bytes);
    put_u32(&body[100], ds.heap_free_bytes);
    put_u32(&body[104], ds.heap_min_ever_free_bytes);
    put_u32(&body[108], memory_usage_x100);

    put_u32(&body[112], ps.active_version);
    put_u32(&body[116], ps.active_size);
    put_u32(&body[120], ps.active_crc32);
    put_u32(&body[124], (uint32_t)ps.last_result);
    put_u32(&body[128], ds.scan_avg_us);
    put_u32(&body[132], ds.scan_max_us);
    put_u32(&body[136], ds.work_avg_us);
    put_u32(&body[140], ds.work_max_us);
    put_u32(&body[144], ds.cycle_real_avg_us);
    put_u32(&body[148], ds.cycle_real_max_us);
    put_u32(&body[152], ds.scan_limit_ms);
    put_u32(&body[156], ds.cpu_load_x100);
    put_u32(&body[160], ds.scan_long_steps);

    return plc_link_send_response(
            PLC_LINK_RSP_STATUS_WEB_V2,
            seq,
            body,
            sizeof(body)
    );
}

static bool send_io_summary(uint16_t seq)
{
    PlcRuntimeSnapshot rs;
    memset(&rs, 0, sizeof(rs));
    (void)plc_snapshot_get_runtime(&rs);

    uint16_t di_used = 0u;
    uint16_t do_used = 0u;
    uint16_t ai_used = 0u;
    uint16_t ao_used = 0u;
    uint16_t pwm_used = 0u;
    uint16_t hsc_used = 0u;
    uint16_t encoder_used = 0u;

    for (uint16_t i = 0u; i < rs.nodeCount; i++) {
        PlcNodeSnapshot ns;
        memset(&ns, 0, sizeof(ns));

        if (!plc_snapshot_get_node(i, &ns)) {
            continue;
        }

        switch ((PlcNodeType)ns.type) {
            case PLC_NODE_DIGITAL_IN:
                di_used++;
                break;

            case PLC_NODE_DIGITAL_OUT:
            case PLC_NODE_SAFE_OUTPUT:
                do_used++;
                break;

            case PLC_NODE_AI_IN:
                ai_used++;
                break;

            case PLC_NODE_AO:
                ao_used++;
                break;

            case PLC_NODE_HSC_IN:
                hsc_used++;
                break;

            case PLC_NODE_ENCODER_IN:
                encoder_used++;
                break;

            default:
                break;
        }
    }

    uint8_t body[36];
    memset(body, 0, sizeof(body));

    put_u32(&body[0], 0x31534F49u); /* 'IOS1' */
    put_u32(&body[4], 1u);

    put_u16(&body[8],  di_used);
    put_u16(&body[10], PLC_HW_DI_TOTAL);

    put_u16(&body[12], do_used);
    put_u16(&body[14], PLC_HW_DO_TOTAL);

    put_u16(&body[16], ai_used);
    put_u16(&body[18], PLC_HW_AI_TOTAL);

    put_u16(&body[20], ao_used);
    put_u16(&body[22], PLC_HW_AO_TOTAL);

    put_u16(&body[24], pwm_used);
    put_u16(&body[26], PLC_HW_PWM_TOTAL);

    put_u16(&body[28], hsc_used);
    put_u16(&body[30], PLC_HW_HSC_TOTAL);

    put_u16(&body[32], encoder_used);
    put_u16(&body[34], PLC_HW_ENCODER_TOTAL);

    return plc_link_send_response(
            PLC_LINK_RSP_IO_SUMMARY,
            seq,
            body,
            sizeof(body)
    );
}

static bool send_nodes_snapshot(uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    /*
     * Request body:
     * 0..1 u16 chunk_index
     *
     * Response body layout, little-endian:
     *
     *  0  u32 magic 'NSN1'
     *  4  u32 version
     *  8  u16 total_nodes
     * 10  u16 chunk_index
     * 12  u16 chunk_count
     * 14  u16 nodes_in_chunk
     *
     * Node record, 40 bytes each:
     *  +0  u16 index
     *  +2  u16 id
     *  +4  u32 type
     *  +8  u32 flags
     *  +12 u32 out_bool
     *  +16 i32 out_int
     *  +20 f32 out_float
     *  +24 u32 force_enabled
     *  +28 u32 force_bool
     *  +32 u32 force_left_ms
     *  +36 u32 runtime_flags
     */

    if (body == NULL || body_len != 2u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    const uint16_t requested_chunk = ext_get_u16(&body[0]);

    PlcRuntimeSnapshot rs;
    memset(&rs, 0, sizeof(rs));
    (void)plc_snapshot_get_runtime(&rs);

    const uint16_t total_nodes = rs.nodeCount;

    /*
     * 16 header + 8 nodes * 40 bytes = 336 bytes payload.
     * If PLC_LINK_MAX_BODY_SIZE is lower in your project, reduce this to 4.
     */
    const uint16_t nodes_per_chunk = 8u;
    const uint16_t chunk_count =
            (total_nodes == 0u)
            ? 1u
            : (uint16_t)((total_nodes + nodes_per_chunk - 1u) / nodes_per_chunk);

    if (requested_chunk >= chunk_count) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_INDEX, requested_chunk);
    }

    const uint16_t start_index = (uint16_t)(requested_chunk * nodes_per_chunk);
    uint16_t nodes_in_chunk = 0u;

    if (total_nodes > start_index) {
        uint16_t remaining = (uint16_t)(total_nodes - start_index);
        nodes_in_chunk = (remaining > nodes_per_chunk) ? nodes_per_chunk : remaining;
    }

    uint8_t rsp[16u + (8u * 64u)];
    memset(rsp, 0, sizeof(rsp));

    put_u32(&rsp[0], 0x314E534Eu); /* 'NSN1' */
    put_u32(&rsp[4], EXT_NODES_SNAPSHOT_VERSION);
    put_u16(&rsp[8], total_nodes);
    put_u16(&rsp[10], requested_chunk);
    put_u16(&rsp[12], chunk_count);
    put_u16(&rsp[14], nodes_in_chunk);

    for (uint16_t i = 0u; i < nodes_in_chunk; i++) {
        const uint16_t node_index = (uint16_t)(start_index + i);

        PlcNodeSnapshot ns;
        memset(&ns, 0, sizeof(ns));

        if (!plc_snapshot_get_node(node_index, &ns)) {
            continue;
        }

        uint8_t* p = &rsp[16u + ((uint16_t)i * 64u)];

        uint32_t runtime_flags = 0u;

        if (ns.tonActive) {
            runtime_flags |= (1u << 0);
        }

        if (ns.toffHolding) {
            runtime_flags |= (1u << 1);
        }

        if (ns.srQ) {
            runtime_flags |= (1u << 2);
        }

        if (ns.trigPrev) {
            runtime_flags |= (1u << 3);
        }

        if (ns.pidInited) {
            runtime_flags |= (1u << 4);
        }

        if (ns.filterInited) {
            runtime_flags |= (1u << 5);
        }

        if (ns.rampInited) {
            runtime_flags |= (1u << 6);
        }

        if (ns.aoZeroHold) {
            runtime_flags |= (1u << 7);
        }

        put_u16(&p[0], ns.index);
        put_u16(&p[2], ns.id);
        put_u32(&p[4], (uint32_t)ns.type);
        put_u32(&p[8], ns.flags);
        put_u32(&p[12], bool_flag(ns.outB));
        put_i32(&p[16], ns.outI);
        put_f32(&p[20], ns.outF);
        put_u32(&p[24], bool_flag(ns.forceEnabled));
        put_u32(&p[28], bool_flag(ns.forceBool));
        put_u32(&p[32], ns.forceLeftMs);
        put_u32(&p[36], runtime_flags);
        put_u32(&p[40], ns.tonAccumMs);
        put_u32(&p[44], ns.toffLeftMs);
        put_i32(&p[48], ns.acc);
        put_u32(&p[52], bool_flag(ns.prevClk));
        put_u32(&p[56], 0u);
        put_u32(&p[60], 0u);
    }

    const uint16_t rsp_len = (uint16_t)(16u + ((uint16_t)nodes_in_chunk * 64u));

    return plc_link_send_response(
            PLC_LINK_RSP_NODES_SNAPSHOT,
            seq,
            rsp,
            rsp_len
    );
}

static bool send_node(uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    if (body == NULL || body_len != 2u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    uint16_t index = U16_LE(body);
    PlcNodeSnapshot ns;
    memset(&ns, 0, sizeof(ns));

    if (!plc_snapshot_get_node(index, &ns)) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_INDEX, index);
    }

    uint8_t rsp[128];
    memset(rsp, 0, sizeof(rsp));

    /* Header */
    put_u32(&rsp[0], 0x31444F4Eu); /* 'NOD1' little-endian marker */
    put_u32(&rsp[4], EXT_NODE_VERSION);

    put_u16(&rsp[8], ns.index);
    put_u16(&rsp[10], ns.id);
    put_u32(&rsp[12], (uint32_t)ns.type);
    put_u32(&rsp[16], ns.flags);

    put_u32(&rsp[20], bool_flag(ns.outB));
    put_i32(&rsp[24], ns.outI);
    put_f32(&rsp[28], ns.outF);

    put_u32(&rsp[32], bool_flag(ns.forceEnabled));
    put_u32(&rsp[36], bool_flag(ns.forceBool));
    put_u32(&rsp[40], ns.forceLeftMs);

    put_u32(&rsp[44], bool_flag(ns.tonActive));
    put_u32(&rsp[48], ns.tonAccumMs);

    put_u32(&rsp[52], bool_flag(ns.toffHolding));
    put_u32(&rsp[56], ns.toffLeftMs);

    put_u32(&rsp[60], bool_flag(ns.srQ));
    put_u32(&rsp[64], bool_flag(ns.trigPrev));

    put_i32(&rsp[68], ns.acc);
    put_u32(&rsp[72], bool_flag(ns.prevClk));

    put_u32(&rsp[76], bool_flag(ns.pidInited));
    put_f32(&rsp[80], ns.pidI);
    put_f32(&rsp[84], ns.pidPrevMeas);

    put_u32(&rsp[88], bool_flag(ns.filterInited));
    put_f32(&rsp[92], ns.filterPrev);

    put_u32(&rsp[96], bool_flag(ns.rampInited));
    put_f32(&rsp[100], ns.rampPrev);

    put_u32(&rsp[104], bool_flag(ns.aoZeroHold));

    return plc_link_send_response(PLC_LINK_RSP_NODE, seq, rsp, 108u);
}






static bool send_ext_ack(uint16_t seq, uint8_t ack_for)
{
    uint8_t body[1] = { ack_for };
    return plc_link_send_response(PLC_LINK_RSP_ACK, seq, body, sizeof(body));
}

static bool handle_force_output(uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    if (body == NULL || body_len != 10u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    uint16_t node_index = ext_get_u16(&body[0]);
    uint32_t value_raw = ext_get_u32(&body[2]);
    uint32_t hold_ms = ext_get_u32(&body[6]);

    bool value = (value_raw != 0u);

    if (!plc_force_output(node_index, value, hold_ms)) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_GRAPH_LOADER, node_index);
    }

    return send_ext_ack(seq, PLC_LINK_CMD_FORCE_OUTPUT);
}

static bool handle_release_output(uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    if (body == NULL || body_len != 2u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    uint16_t node_index = ext_get_u16(&body[0]);

    if (!plc_release_output(node_index)) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_GRAPH_LOADER, node_index);
    }

    return send_ext_ack(seq, PLC_LINK_CMD_RELEASE_OUTPUT);
}

static uint8_t mem_error_to_link_error(PlcMemoryResult r)
{
    switch (r) {
        case PLC_MEM_OK:
            return PLC_LINK_ERR_OK;

        case PLC_MEM_ERR_BAD_TYPE:
            return PLC_LINK_ERR_BAD_MEM_TYPE;

        case PLC_MEM_ERR_BAD_INDEX:
        case PLC_MEM_ERR_BAD_RANGE:
            return PLC_LINK_ERR_BAD_MEM_RANGE;

        case PLC_MEM_ERR_BAD_ARG:
        default:
            return PLC_LINK_ERR_BAD_MEM_WRITE;
    }
}

static uint16_t mem_element_size(PlcMemoryType type)
{
    switch (type) {
        case PLC_MEM_TYPE_BOOL:
            return 1u;

        case PLC_MEM_TYPE_INT:
            return 4u;

        case PLC_MEM_TYPE_REAL:
            return 4u;

        default:
            return 0u;
    }
}

static bool handle_mem_info(uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    (void)body;

    if (body_len != 0u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    PlcMemoryInfo info;
    memset(&info, 0, sizeof(info));
    plc_mem_get_info(&info);

    uint8_t rsp[32];
    memset(rsp, 0, sizeof(rsp));

    put_u32(&rsp[0], 0x314D454Du); /* 'MEM1' */
    put_u32(&rsp[4], EXT_MEM_VERSION);

    put_u16(&rsp[8], info.bool_count);
    put_u16(&rsp[10], info.int_count);
    put_u16(&rsp[12], info.real_count);

    put_u16(&rsp[14], info.bool_size);
    put_u16(&rsp[16], info.int_size);
    put_u16(&rsp[18], info.real_size);

    return plc_link_send_response(PLC_LINK_RSP_MEM_INFO, seq, rsp, sizeof(rsp));
}

static bool handle_mem_read(uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    if (body == NULL || body_len != 8u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    PlcMemoryType type = (PlcMemoryType)body[0];
    uint16_t index = ext_get_u16(&body[2]);
    uint16_t count = ext_get_u16(&body[4]);

    uint16_t elem_size = mem_element_size(type);
    if (elem_size == 0u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_MEM_TYPE, (uint32_t)type);
    }

    PlcMemoryResult vr = plc_mem_validate_range(type, index, count);
    if (vr != PLC_MEM_OK) {
        return plc_link_send_error_response(seq, mem_error_to_link_error(vr), ((uint32_t)index << 16u) | count);
    }

    uint32_t data_bytes = (uint32_t)count * (uint32_t)elem_size;
    if (data_bytes > (PLC_LINK_MAX_BODY_SIZE - 16u)) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_MEM_RANGE, data_bytes);
    }

    uint8_t rsp[PLC_LINK_MAX_BODY_SIZE];
    memset(rsp, 0, sizeof(rsp));

    put_u32(&rsp[0], 0x3152444Du); /* 'MDR1' */
    put_u32(&rsp[4], EXT_MEM_VERSION);

    rsp[8] = (uint8_t)type;
    rsp[9] = 0u;
    put_u16(&rsp[10], index);
    put_u16(&rsp[12], count);
    put_u16(&rsp[14], elem_size);

    uint8_t* data = &rsp[16];

    switch (type) {
        case PLC_MEM_TYPE_BOOL:
            for (uint16_t i = 0u; i < count; i++) {
                data[i] = plc_mem_get_bool(index + i) ? 1u : 0u;
            }
            break;

        case PLC_MEM_TYPE_INT:
            for (uint16_t i = 0u; i < count; i++) {
                put_i32(&data[i * 4u], plc_mem_get_int(index + i));
            }
            break;

        case PLC_MEM_TYPE_REAL:
            for (uint16_t i = 0u; i < count; i++) {
                put_f32(&data[i * 4u], plc_mem_get_real(index + i));
            }
            break;

        default:
            return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_MEM_TYPE, (uint32_t)type);
    }

    return plc_link_send_response(PLC_LINK_RSP_MEM_READ, seq, rsp, (uint16_t)(16u + data_bytes));
}

static bool handle_mem_write(uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    if (body == NULL || body_len < 8u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    PlcMemoryType type = (PlcMemoryType)body[0];
    uint16_t index = ext_get_u16(&body[2]);
    uint16_t count = ext_get_u16(&body[4]);

    uint16_t elem_size = mem_element_size(type);
    if (elem_size == 0u) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_MEM_TYPE, (uint32_t)type);
    }

    uint32_t expected_len = 8u + ((uint32_t)count * (uint32_t)elem_size);
    if (body_len != expected_len) {
        return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
    }

    PlcMemoryResult vr = plc_mem_validate_range(type, index, count);
    if (vr != PLC_MEM_OK) {
        return plc_link_send_error_response(seq, mem_error_to_link_error(vr), ((uint32_t)index << 16u) | count);
    }

    const uint8_t* data = &body[8];

    switch (type) {
        case PLC_MEM_TYPE_BOOL:
            for (uint16_t i = 0u; i < count; i++) {
                plc_mem_set_bool(index + i, data[i] != 0u);
            }
            break;

        case PLC_MEM_TYPE_INT:
            for (uint16_t i = 0u; i < count; i++) {
                plc_mem_set_int(index + i, (int32_t)ext_get_u32(&data[i * 4u]));
            }
            break;

        case PLC_MEM_TYPE_REAL:
            for (uint16_t i = 0u; i < count; i++) {
                plc_mem_set_real(index + i, ext_get_f32(&data[i * 4u]));
            }
            break;

        default:
            return plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_MEM_TYPE, (uint32_t)type);
    }

    return send_ext_ack(seq, PLC_LINK_CMD_MEM_WRITE);
}



bool plc_link_ext_handle(PlcLinkCommand cmd,
                         uint16_t seq,
                         const uint8_t* body,
                         uint16_t body_len)
{
    switch (cmd) {
        case PLC_LINK_CMD_GET_STATUS_EXT:
            if (body_len != 0u) {
                (void)plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
                return true;
            }
            (void)send_status_ext(seq);
            return true;

        case PLC_LINK_CMD_GET_STATUS_WEB_V2:
            if (body_len != 0u) {
                (void)plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
                return true;
            }
            (void)send_status_web_v2(seq);
            return true;

        case PLC_LINK_CMD_GET_IO_SUMMARY:
            if (body_len != 0u) {
                (void)plc_link_send_error_response(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
                return true;
            }
            (void)send_io_summary(seq);
            return true;

        case PLC_LINK_CMD_GET_NODE:
            (void)send_node(seq, body, body_len);
            return true;

        case PLC_LINK_CMD_GET_NODES_SNAPSHOT:
            (void)send_nodes_snapshot(seq, body, body_len);
            return true;

        case PLC_LINK_CMD_FORCE_OUTPUT:
            (void)handle_force_output(seq, body, body_len);
            return true;

        case PLC_LINK_CMD_RELEASE_OUTPUT:
            (void)handle_release_output(seq, body, body_len);
            return true;

        case PLC_LINK_CMD_MEM_INFO:
            (void)handle_mem_info(seq, body, body_len);
            return true;

        case PLC_LINK_CMD_MEM_READ:
            (void)handle_mem_read(seq, body, body_len);
            return true;

        case PLC_LINK_CMD_MEM_WRITE:
            (void)handle_mem_write(seq, body, body_len);
            return true;

        default:
            return false;
    }
}


