
AM_CFLAGS = -Wall -Werror 
AM_LDFLAGS = $(libartnet_LIBS)

if HAVE_NCURSES
NCURSES_PROGS = artnet_dmxconsole artnet_dmxmonitor
artnet_dmxconsole_SOURCES = artnet-dmxconsole.c
artnet_dmxconsole_LDADD = @LIBS@ -lcurses

artnet_dmxmonitor_SOURCES = artnet-dmxmonitor.c
artnet_dmxmonitor_LDADD = @LIBS@ -lcurses
endif


bin_PROGRAMS = artnet_usb artnet_discover artnet_flood_rx artnet_flood_tx artnet_firmware_node artnet_firmware_server artnet_multiport artnet_profile_tx artnet_profile_rx artnet_rdm_output artnet_setdmx $(NCURSES_PROGS)

artnet_usb_SOURCES = artnet-usb.c
artnet_usb_LDADD = @LIBS@ -lpthread
artnet_discover_SOURCES = artnet-discover.c
artnet_flood_rx_SOURCES = artnet-flood-rx.c
artnet_flood_tx_SOURCES = artnet-flood-tx.c
artnet_profile_tx_SOURCES = artnet-profile-tx.c
artnet_profile_rx_SOURCES = artnet-profile-rx.c
artnet_firmware_node_SOURCES = artnet-firmware-node.c
artnet_firmware_server_SOURCES = artnet-firmware-server.c
artnet_multiport_SOURCES = artnet-multiport.c
artnet_rdm_output_SOURCES = artnet-rdm-output.c
artnet_setdmx_SOURCES = artnet-setdmx.c

#artnet_test_master_SOURCES = artnet-test-master.c artnet-test-defaults.h
#artnet_test_slave_SOURCES = artnet-test-slave.c artnet-test-defaults.h artnet-test-defns.h artnet-test-defns.c
