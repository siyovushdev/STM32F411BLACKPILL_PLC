# Friendly PLC Core

Portable PLC runtime engine written in pure C11.

Targets:
- STM32
- Linux
- ESP32
- RTOS / bare-metal systems

Architecture:
- graph-based execution
- deterministic scan cycle
- portable HAL layer
- hot graph swap
- runtime forcing
- software timers
- analog processing
- PID control
- HSC / Encoder support

---

# Features

## Digital IO
- DI
- DO
- debounce filtering

## Analog IO
- AI
- AO
- scaling
- filtering
- ramp limiting

## Logic
- AND
- OR
- NOT
- SR latch

## Timers
- TON
- TOFF
- TP
- R_TRIG
- F_TRIG

## Math
- ADD
- SCALE
- LIMIT
- generic MATH_OP

## Memory
- MEM_BOOL
- MEM_INT
- MEM_REAL

## Motion-related IO
- HSC input
- Encoder input

## Runtime
- hot graph activation
- runtime validation
- force outputs
- safe mode
- watchdog support

---

# Build

## Linux

```bash
cmake -B cmake-build-debug-system
cmake --build cmake-build-debug-system
```

## Run tests

```bash
ctest --test-dir cmake-build-debug-system --output-on-failure
```

---

# Project Structure

```text
include/friendly_plc/
    plc.h
    plc_port.h
    plc_error.h
    plc_runtime.h
    ...

src/
    plc.c
    plc_nodes.c
    plc_validate.c
    plc_runtime.c
    plc_node_exec.c

src/nodes/
    plc_node_logic.c
    plc_node_timer.c
    plc_node_math.c
    ...

ports/linux/
    plc_port_linux.c

tests/
    test_basic.c
    test_timer_nodes.c
    ...
```

---

# Runtime Flow

```text
upload graph
    ↓
validate graph
    ↓
activate graph
    ↓
periodic plc_tick()
    ↓
refresh inputs
    ↓
execute graph
    ↓
write outputs
```

---

# Example

```c
PlcGraph g;

plc_upload_graph(&g);

plc_request_activate_graph();

while (1) {
    plc_tick(now_ms);
}
```