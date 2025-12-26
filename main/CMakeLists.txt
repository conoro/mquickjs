set(MQJS_ROOT ${CMAKE_SOURCE_DIR})

set(MQJS_SRCS
    ${MQJS_ROOT}/mqjs.c
    ${MQJS_ROOT}/mqjs_led.c
    ${MQJS_ROOT}/mquickjs.c
    ${MQJS_ROOT}/dtoa.c
    ${MQJS_ROOT}/libm.c
    ${MQJS_ROOT}/cutils.c
    ${MQJS_ROOT}/readline.c
    ${MQJS_ROOT}/readline_tty.c
    ${CMAKE_CURRENT_LIST_DIR}/mqjs_main.c
)

idf_component_register(
    SRCS ${MQJS_SRCS}
    INCLUDE_DIRS "." ${MQJS_ROOT}
    PRIV_INCLUDE_DIRS ${MQJS_ROOT}
)

# Enable LED support
target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_LED)
