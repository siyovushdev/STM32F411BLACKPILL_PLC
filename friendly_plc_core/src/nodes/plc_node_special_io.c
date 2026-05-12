#include "plc_node_special_io.h"

#include "friendly_plc/plc_port.h"

void plc_node_exec_hsc_in(PlcNode* self)
{
    int32_t value = plc_port_read_hsc((uint16_t)self->paramInt);

    self->out.i = value;
    self->out.f = (float)value;
    self->out.b = (value != 0);
}

void plc_node_exec_encoder_in(PlcNode* self)
{
    int32_t value = plc_port_read_encoder((uint16_t)self->paramInt);

    self->out.i = value;
    self->out.f = (float)value;
    self->out.b = (value != 0);
}