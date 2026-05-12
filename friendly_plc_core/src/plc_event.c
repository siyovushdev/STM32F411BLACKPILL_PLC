#include "friendly_plc/plc_event.h"
#include "friendly_plc/plc_port.h"

static PlcEvent s_events[PLC_EVENT_CAPACITY];
static uint16_t s_head = 0;
static uint16_t s_tail = 0;
static uint16_t s_count = 0;

void plc_event_clear(void)
{
    s_head = 0;
    s_tail = 0;
    s_count = 0;
}

bool plc_event_push(PlcEventCode code, int16_t a, int16_t b)
{
    PlcEvent e;
    e.ts_ms = plc_port_now_ms();
    e.code = (uint16_t)code;
    e.a = a;
    e.b = b;

    s_events[s_head] = e;
    s_head = (uint16_t)((s_head + 1u) % PLC_EVENT_CAPACITY);

    if (s_count < PLC_EVENT_CAPACITY) {
        s_count++;
    } else {
        s_tail = (uint16_t)((s_tail + 1u) % PLC_EVENT_CAPACITY);
    }

    return true;
}

bool plc_event_pop(PlcEvent* out)
{
    if (!out || s_count == 0) {
        return false;
    }

    *out = s_events[s_tail];
    s_tail = (uint16_t)((s_tail + 1u) % PLC_EVENT_CAPACITY);
    s_count--;

    return true;
}

uint16_t plc_event_count(void)
{
    return s_count;
}