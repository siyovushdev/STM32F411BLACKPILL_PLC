cmake_minimum_required(VERSION 3.20)

project(friendly_plc_core C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(friendly_plc_core STATIC
        src/plc_log.c
        src/plc_event.c
        src/plc_safety.c
        src/plc_memory.c
        src/plc_validation.c
        src/plc_runtime.c
        src/plc_nodes.c
        src/plc_metrics.c
        src/plc_error.c
        src/nodes/plc_node_timer.c
        src/nodes/plc_node_logic.c
        src/nodes/plc_node_compare.c
        src/nodes/plc_node_math.c
        src/nodes/plc_node_counter.c
        src/nodes/plc_node_memory.c
        src/nodes/plc_node_filter.c
        src/nodes/plc_node_misc.c
        src/nodes/plc_node_io.c
        src/nodes/plc_node_pid.c
        src/nodes/plc_node_basic.c
        src/nodes/plc_node_special_io.c
        src/nodes/plc_node_exec.c
        src/plc_snapshot.c
        ports/stm32/plc_port_stm32.c
)

target_include_directories(friendly_plc_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(plc_basic_example
        examples/basic_graph/main.c
)

target_link_libraries(plc_basic_example
        PRIVATE
        friendly_plc_core
)

target_compile_definitions(friendly_plc_core
        PUBLIC
        FRIENDLY_PLC_ENABLE_TEST_API=1
)

add_executable(plc_ton_di_do_example
        examples/ton_di_do/main.c
)

target_link_libraries(plc_ton_di_do_example
        PRIVATE
        friendly_plc_core
)

add_executable(plc_ai_compare_do_example
        examples/ai_compare_do/main.c
)

target_link_libraries(plc_ai_compare_do_example
        PRIVATE
        friendly_plc_core
)

add_executable(plc_const_ao_example
        examples/const_ao/main.c
)

target_link_libraries(plc_const_ao_example
        PRIVATE
        friendly_plc_core
)

add_executable(plc_mem_bool_do_example
        examples/mem_bool_do/main.c
)

target_link_libraries(plc_mem_bool_do_example
        PRIVATE
        friendly_plc_core
)

add_executable(plc_validation_error_example
        examples/validation_error/main.c
)

target_link_libraries(plc_validation_error_example
        PRIVATE
        friendly_plc_core
)

add_executable(plc_hsc_compare_do_example
        examples/hsc_compare_do/main.c
)

target_link_libraries(plc_hsc_compare_do_example
        PRIVATE
        friendly_plc_core
)

add_executable(plc_encoder_compare_do_example
        examples/encoder_compare_do/main.c
)

target_link_libraries(plc_encoder_compare_do_example
        PRIVATE
        friendly_plc_core
)

add_executable(plc_force_do_example
        examples/force_do/main.c
)

target_link_libraries(plc_force_do_example PRIVATE friendly_plc_core)

add_test(NAME plc_force_do COMMAND plc_force_do_example)

add_executable(plc_math_op_example
        examples/math_op/main.c
)

target_link_libraries(plc_math_op_example PRIVATE friendly_plc_core)

add_test(NAME plc_math_op COMMAND plc_math_op_example)

add_executable(plc_mem_int_example
        examples/mem_int/main.c
)

target_link_libraries(plc_mem_int_example PRIVATE friendly_plc_core)

add_executable(plc_safe_mode_example
        examples/safe_mode/main.c
)

target_link_libraries(plc_safe_mode_example PRIVATE friendly_plc_core)

add_executable(plc_force_timeout_example
        examples/force_timeout/main.c
)

target_link_libraries(plc_force_timeout_example PRIVATE friendly_plc_core)

add_executable(plc_graph_hot_swap_example
        examples/graph_hot_swap/main.c
)

target_link_libraries(plc_graph_hot_swap_example PRIVATE friendly_plc_core)


# ===== Friendly PLC tests pack 2 =====

add_executable(plc_timer_nodes_example
        examples/timer_nodes/main.c
)
target_link_libraries(plc_timer_nodes_example PRIVATE friendly_plc_core)
add_test(NAME plc_timer_nodes COMMAND plc_timer_nodes_example)

add_executable(plc_pid_basic_example
        examples/pid_basic/main.c
)
target_link_libraries(plc_pid_basic_example PRIVATE friendly_plc_core)
add_test(NAME plc_pid_basic COMMAND plc_pid_basic_example)

add_executable(plc_filter_ramp_example
        examples/filter_ramp/main.c
)
target_link_libraries(plc_filter_ramp_example PRIVATE friendly_plc_core)
add_test(NAME plc_filter_ramp COMMAND plc_filter_ramp_example)

add_executable(plc_alarm_latch_example
        examples/alarm_latch/main.c
)
target_link_libraries(plc_alarm_latch_example PRIVATE friendly_plc_core)
add_test(NAME plc_alarm_latch COMMAND plc_alarm_latch_example)

add_executable(plc_safe_output_example
        examples/safe_output/main.c
)
target_link_libraries(plc_safe_output_example PRIVATE friendly_plc_core)
add_test(NAME plc_safe_output COMMAND plc_safe_output_example)

add_executable(plc_window_hyst_example
        examples/window_hyst/main.c
)
target_link_libraries(plc_window_hyst_example PRIVATE friendly_plc_core)
add_test(NAME plc_window_hyst COMMAND plc_window_hyst_example)

add_executable(plc_invalid_graphs_example
        examples/invalid_graphs/main.c
)
target_link_libraries(plc_invalid_graphs_example PRIVATE friendly_plc_core)
add_test(NAME plc_invalid_graphs COMMAND plc_invalid_graphs_example)


add_test(NAME plc_graph_hot_swap COMMAND plc_graph_hot_swap_example)

add_test(NAME plc_force_timeout COMMAND plc_force_timeout_example)

add_test(NAME plc_safe_mode COMMAND plc_safe_mode_example)

add_test(NAME plc_mem_int COMMAND plc_mem_int_example)

add_test(NAME plc_encoder_compare_do COMMAND plc_encoder_compare_do_example)

add_test(NAME plc_hsc_compare_do COMMAND plc_hsc_compare_do_example)

add_test(NAME plc_validation_error COMMAND plc_validation_error_example)

enable_testing()

add_test(NAME plc_basic COMMAND plc_basic_example)
add_test(NAME plc_ton_di_do COMMAND plc_ton_di_do_example)
add_test(NAME plc_ai_compare_do COMMAND plc_ai_compare_do_example)
add_test(NAME plc_const_ao COMMAND plc_const_ao_example)
add_test(NAME plc_mem_bool_do COMMAND plc_mem_bool_do_example)

