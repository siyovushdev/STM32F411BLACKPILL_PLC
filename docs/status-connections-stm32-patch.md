# STM32 status connections patch

Goal: include PLC graph input references (`inA`, `inB`) in the existing `GET_NODES_SNAPSHOT` response so ESP32/Web can build real topology lines.

Current state:

- `PlcNode` already has `inA` and `inB` in `friendly_plc_core/include/friendly_plc/plc_types.h`.
- `send_nodes_snapshot()` in `Core/Src/plc_link_ext.c` sends 64 bytes per node.
- Bytes `+56` and `+60` are currently reserved and always zero.

Required change:

## 1. Add include

In `Core/Src/plc_link_ext.c` add:

```c
#include "friendly_plc/plc.h"
```

This gives access to `g_activeGraph` and `g_activeGraphValid`.

## 2. Replace reserved fields in `send_nodes_snapshot()`

Current code near the end of each 64-byte node record:

```c
put_i32(&p[48], ns.acc);
put_u32(&p[52], bool_flag(ns.prevClk));
put_u32(&p[56], 0u);
put_u32(&p[60], 0u);
```

Replace with:

```c
int16_t in_a = -1;
int16_t in_b = -1;

if (g_activeGraphValid && node_index < g_activeGraph.nodeCount) {
    in_a = g_activeGraph.nodes[node_index].inA;
    in_b = g_activeGraph.nodes[node_index].inB;
}

put_i32(&p[48], ns.acc);
put_u32(&p[52], bool_flag(ns.prevClk));
put_u32(&p[56], (uint32_t)(int32_t)in_a);
put_u32(&p[60], (uint32_t)(int32_t)in_b);
```

## Resulting node record layout

```text
+0   u16 index
+2   u16 id
+4   u32 type
+8   u32 flags
+12  u32 out_bool
+16  i32 out_int
+20  f32 out_float
+24  u32 force_enabled
+28  u32 force_bool
+32  u32 force_left_ms
+36  u32 runtime_flags
+40  u32 ton_accum_ms
+44  u32 toff_left_ms
+48  i32 acc
+52  u32 prev_clk
+56  i32 inA
+60  i32 inB
```

This is backward compatible with the existing 64-byte snapshot frame size.

Expected result for example graph:

```text
node[0] DIGITAL_IN   inA=-1 inB=-1
node[1] TON          inA=0  inB=-1
node[2] DIGITAL_OUT  inA=1  inB=-1
```

Then ESP32 can expose:

```json
"connections": [
  { "fromNode": 0, "fromPort": "out", "toNode": 1, "toPort": "inA" },
  { "fromNode": 1, "fromPort": "out", "toNode": 2, "toPort": "inA" }
]
```
