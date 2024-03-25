#
# RasPi3 (BCM2837) SPI LED driver
#
# Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
# 
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
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
function(RPi_SPI_LED_DeclareCAmkESComponent name)

    cmake_parse_arguments(
        PARSE_ARGV
        1
        RPi_SPI_LED
        ""
        ""
        "LIBS"
    )

    DeclareCAmkESComponent(
        ${name}
        SOURCES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/RPi_SPI_LED.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/bcm2837_gpio.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/bcm2837_spi.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/max7219.c
        C_FLAGS
            -Wall
            -Werror
        LIBS
            os_core_api
            lib_debug
            ${RPi_SPI_LED_LIBS}
    )

endfunction()
