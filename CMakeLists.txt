# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_definitions(SNTP_STARTUP_DELAY=0)

add_compile_definitions(ADC1_CHANNEL_0_GPIO_NUM=36)
add_compile_definitions(ADC1_CHANNEL_1_GPIO_NUM=37)
add_compile_definitions(ADC1_CHANNEL_2_GPIO_NUM=38)
add_compile_definitions(ADC1_CHANNEL_3_GPIO_NUM=39)
add_compile_definitions(ADC1_CHANNEL_4_GPIO_NUM=32)
add_compile_definitions(ADC1_CHANNEL_5_GPIO_NUM=33)
add_compile_definitions(ADC1_CHANNEL_6_GPIO_NUM=34)
add_compile_definitions(ADC1_CHANNEL_7_GPIO_NUM=35)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(PocketTRS)
