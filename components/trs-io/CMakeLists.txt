
set(TRS_IO TRS-IO/src/esp/components)
# TRS-IO's own app. We use its headers (settings, FPGA SPI, LED, reset) and
# its settings.cpp; host/ stands in for the board-specific rest.
set(TRS_IO_MAIN TRS-IO/src/esp/main)
set(RS_SDK ${TRS_IO}/retrostore-c-sdk/main)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    add_definitions(-DHAVE_CONFIG_H)
endif()

set(srcs "host/host.cpp"
         "${TRS_IO_MAIN}/settings.cpp"
         "${RS_SDK}/retrostore.cpp"
         "${RS_SDK}/data-fetcher-esp.cpp"
         "${RS_SDK}/proto/pb_common.c"
         "${RS_SDK}/proto/pb_decode.c"
         "${RS_SDK}/proto/pb_encode.c"
         "${RS_SDK}/proto/ApiProtos.pb.c")
foreach(dir libsmb2/lib trs-io frehd retrostore trs-fs tcpip xfer)
    file(GLOB dir_srcs "${CMAKE_CURRENT_LIST_DIR}/${TRS_IO}/${dir}/*.c"
                       "${CMAKE_CURRENT_LIST_DIR}/${TRS_IO}/${dir}/*.cpp")
    list(APPEND srcs ${dir_srcs})
endforeach()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${TRS_IO}/libsmb2/include
                                    ${TRS_IO}/libsmb2/include/smb2
                                    ${TRS_IO}/libsmb2/lib
                                    ${TRS_IO}/libsmb2/include/esp
                                    ${TRS_IO}/trs-io/include
                                    ${TRS_IO}/frehd/include
                                    ${TRS_IO}/retrostore/include
                                    ${TRS_IO}/trs-fs/include
                                    ${TRS_IO}/tcpip/include
                                    ${TRS_IO}/xfer/include
                                    ${RS_SDK}/include
                                    ${RS_SDK}/proto
                                    host/include
                       # TRS-IO's app headers are private: PocketTRS has its
                       # own settings.h, io.h, spi.h, ... in main.
                       PRIV_INCLUDE_DIRS ${TRS_IO_MAIN}/include
                                         ${TRS_IO}/keyb/include
                                         ${TRS_IO}/ptrs/include
                       EMBED_FILES ${TRS_IO}/retrostore/loader_basic.cmd
                                   ${TRS_IO}/retrostore/loader_cmd.bin
                                   ${TRS_IO}/retrostore/rsclient.cmd
                                   ${TRS_IO}/trs-io/xray-load.bin
                       REQUIRES esp_timer
                                esp_http_client
                                esp_wifi
                                esp_netif
                                esp_event
                                nvs_flash
                                spiffs
                                app_update
                                fatfs
                                lwip
                                esp_driver_uart
                                esp_driver_gpio
                                esp_driver_spi
                                esp_driver_sdmmc
                                esp_driver_sdspi)

# What upstream's project CMakeLists.txt sets for all of TRS-IO: mongoose's
# platform (public: it's in mongoose.h), and its u8"" printer table as
# plain char strings, as before C++20.
target_compile_definitions(${COMPONENT_LIB} PUBLIC MG_ARCH=MG_ARCH_ESP32 MG_ENABLE_POSIX_FS=1)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-char8_t>)

# The status screen and the web UI show TRS-IO's git revision (upstream
# generates these in main/version.cmake). Taken from the submodule when CMake
# runs.
execute_process(COMMAND git -C "${CMAKE_CURRENT_LIST_DIR}/TRS-IO" log --pretty=format:%h -n 1
                OUTPUT_VARIABLE trs_io_rev ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git -C "${CMAKE_CURRENT_LIST_DIR}/TRS-IO" rev-parse --abbrev-ref HEAD
                OUTPUT_VARIABLE trs_io_branch ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git -C "${CMAKE_CURRENT_LIST_DIR}/TRS-IO" describe --exact-match --tags
                OUTPUT_VARIABLE trs_io_tag ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
set_source_files_properties("host/host.cpp" PROPERTIES COMPILE_DEFINITIONS
    "TRS_IO_GIT_REV=\"${trs_io_rev}\";TRS_IO_GIT_BRANCH=\"${trs_io_branch}\";TRS_IO_GIT_TAG=\"${trs_io_tag}\"")

# upstream's init_settings() would clash with PocketTRS's; host.cpp calls it
# by this name.
set_source_files_properties("${TRS_IO_MAIN}/settings.cpp" PROPERTIES
                            COMPILE_DEFINITIONS "init_settings=trs_io_init_settings")

# Force static linking of modules
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theTrsIOCoreModule")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u init_frehd")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u frehd_in")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u frehd_out")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theTrsFileSystemModule")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theRetroStoreModule")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theTCPIPModule")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u xferModule")

# TRS-IO's web UI, served from the "html" SPIFFS partition. Upstream keeps it
# built in the repository (html/built, `make build-html` there rebuilds it);
# like upstream's `make copy-html`, gzip each file (the web server sends
# the .gz) and pack them into the partition image, flashed with `idf.py
# flash`.
set(html_src "${CMAKE_CURRENT_LIST_DIR}/TRS-IO/src/esp/html/built")
set(html_dir "${CMAKE_BINARY_DIR}/trs-io-html")
file(GLOB html_files "${html_src}/*")
set(html_copies "")
foreach(f ${html_files})
    get_filename_component(name "${f}" NAME)
    list(APPEND html_copies "${html_dir}/${name}")
endforeach()
add_custom_command(OUTPUT "${html_dir}/index.html.gz"
                   COMMAND ${CMAKE_COMMAND} -E rm -rf "${html_dir}"
                   COMMAND ${CMAKE_COMMAND} -E make_directory "${html_dir}"
                   COMMAND ${CMAKE_COMMAND} -E copy ${html_files} "${html_dir}"
                   COMMAND gzip --best -n ${html_copies}
                   DEPENDS ${html_files}
                   VERBATIM)
add_custom_target(trs_io_html DEPENDS "${html_dir}/index.html.gz")
spiffs_create_partition_image(html "${html_dir}" FLASH_IN_PROJECT DEPENDS trs_io_html)
