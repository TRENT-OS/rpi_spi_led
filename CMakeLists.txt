#
# RasPi3 (BCM2837) SPI LED driver
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.17)

#-------------------------------------------------------------------------------
# Declare RasPi3 (BCM2837) SPI LED Driver CAmkES Component
#
# Parameters:
#
#   <name>
#     required, component instance name
#
#   LIBS <lib_1> <lib_2> ...
#     additional libraries. At least a TimeServer client library is required
#
function(DeclareCAmkESComponent_RPI_SPI_LED name)

    cmake_parse_arguments(
        PARSE_ARGV
        1
        RPI_SPI_LED
        ""
        ""
        "LIBS"
    )

    DeclareCAmkESComponent(
        ${name}
        SOURCES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/RPI_SPI_LED.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/bcm2837_gpio.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/bcm2837_spi.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/max7219.c
        C_FLAGS
            -Wall
            -Werror
        LIBS
            os_core_api
            os_libs
            ${RPI_SPI_LED_LIBS}
    )

endfunction()
