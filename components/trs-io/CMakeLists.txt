
set(TRS_IO TRS-IO/src/esp/components)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    add_definitions(-DHAVE_CONFIG_H)
endif()

#FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/TRS-IO/src/esp/components/*.*)

#idf_component_register(SRCS ${app_sources}
idf_component_register(SRC_DIRS ${TRS_IO}/libsmb2/lib
                                ${TRS_IO}/trs-io
                                ${TRS_IO}/frehd
                                ${TRS_IO}/retrostore
                                ${TRS_IO}/trs-fs
                                ${TRS_IO}/tcpip
                       INCLUDE_DIRS ${TRS_IO}/libsmb2/include
                                    ${TRS_IO}/libsmb2/include/smb2
                                    ${TRS_IO}/libsmb2/lib
                                    ${TRS_IO}/libsmb2/include/esp
                                    ${TRS_IO}/trs-io/include
                                    ${TRS_IO}/frehd/include
                                    ${TRS_IO}/retrostore/include
                                    ${TRS_IO}/trs-fs/include
                                    ${TRS_IO}/tcpip/include
                       EMBED_TXTFILES ${TRS_IO}/trs-io/index.html
                                      ${TRS_IO}/trs-io/printer.html
                                      ${TRS_IO}/trs-io/AnotherMansTreasureMIII64C.ttf
                       EMBED_FILES ${TRS_IO}/retrostore/loader_basic.cmd
                                   ${TRS_IO}/retrostore/loader_cmd.bin
                                   ${TRS_IO}/retrostore/rsclient.cmd
                       REQUIRES main
                                mdns)

# Force static linking of modules
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theTrsIOCoreModule")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u init_frehd")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theTrsFileSystemModule")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theRetroStoreModule")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u theTCPIPModule")
