
TRS_IO=TRS-IO/src/esp/components

COMPONENT_ADD_LDFLAGS=-Wl,--whole-archive build/$(COMPONENT_NAME)/lib$(COMPONENT_NAME).a -Wl,--no-whole-archive

COMPONENT_SRCDIRS=$(TRS_IO)/libsmb2/lib
COMPONENT_PRIV_INCLUDEDIRS=$(TRS_IO)/libsmb2/lib $(TRS_IO)/libsmb2/include/esp
COMPONENT_ADD_INCLUDEDIRS=$(TRS_IO)/libsmb2/include $(TRS_IO)/libsmb2/include/smb2

COMPONENT_SRCDIRS += $(TRS_IO)/trs-io
COMPONENT_ADD_INCLUDEDIRS += $(TRS_IO)/trs-io/include
COMPONENT_EMBED_FILES := $(TRS_IO)/trs-io/index.html

COMPONENT_SRCDIRS += $(TRS_IO)/frehd
COMPONENT_ADD_INCLUDEDIRS += $(TRS_IO)/frehd/include

COMPONENT_SRCDIRS += $(TRS_IO)/retrostore
COMPONENT_PRIV_INCLUDEDIRS += $(TRS_IO)/retrostore/include
COMPONENT_EMBED_FILES += $(TRS_IO)/retrostore/loader_basic.cmd
COMPONENT_EMBED_FILES += $(TRS_IO)/retrostore/loader_cmd.bin
COMPONENT_EMBED_FILES += $(TRS_IO)/retrostore/rsclient.cmd

COMPONENT_SRCDIRS += $(TRS_IO)/trs-fs
COMPONENT_PRIV_INCLUDEDIRS += $(TRS_IO)/trs-fs/include

COMPONENT_SRCDIRS += $(TRS_IO)/tcpip
COMPONENT_PRIV_INCLUDEDIRS += $(TRS_IO)/tcpip/include

COMPONENT_SRCDIRS += $(TRS_IO)/trs-fs
COMPONENT_PRIV_INCLUDEDIRS += $(TRS_IO)/trs-fs/include

