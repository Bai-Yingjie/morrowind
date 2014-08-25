
#
# Rules used only when building outside of buildroot
#

# Set following variables without 'override'. This will make sure that they can
# be overridden when building from within buildroot. As a result, the following
# definitions are only used when building outside of buildroot.
ifneq (,$(BUILDROOT)) # On some machines (Ubuntu), /.config exists as a directory
-include $(BUILDROOT)/.config
endif

ARCH := $(BR2_ARCH)
TOOLS_PREFIX := $(BR2_TOOLCHAIN_EXTERNAL_PREFIX)-

BUILDROOT_OUTPUT=$(BUILDROOT)/output
CC := $(BUILDROOT_OUTPUT)/host/usr/bin/$(TOOLS_PREFIX)gcc
AS := $(BUILDROOT_OUTPUT)/host/usr/bin/$(TOOLS_PREFIX)as
DESTDIR := $(BUILDROOT_OUTPUT)/target/isam/user

all: check_vars build install

check_vars:
	@if [ ! -d "$(BUILDROOT)" ]; then (echo "Directory '$(BUILDROOT)' does not exist. Please specify a valid BUILDROOT on the command line"; false); fi

#
# Rules used both when building from within as outside of buildroot
#
# vipr-a uses the ndps-a build, but needs the correct name for wdog application.
# Therefore, we create a symbolic link and make sure the link is copied as-is.
NOBUILD_TARGETS= calamares vipr-a_cpld_wdog ndps-b_cpld_wdog ndps-c_cpld_wdog

TARGETS= calamares_bin nant-e_cpld_wdog ndps-a_cpld_wdog \
         fuse_devices fuse_cpuload fuse_watchdog \
         clearmem kick_dev_watchdog ltmon uiomon \
         fuse_pca9502 \
         fgltb_packet_filter i2cmon pbdd_update_version \
         copyFromMem cpld_spi \
         forward_client forward_server

# Set flags
# Note: we need override because buildroot also sets CFLAGS/LDFLAGS
# -Os to mimic in buildroot-building better
override CFLAGS += -Wall -Werror -g -Os
# Explicitly set -O2 (both from within as outside of buildroot)
override CFLAGS += -O2
nant-e_cpld_wdog ndps-a_cpld_wdog fuse_watchdog ltmon uiomon: override LDFLAGS += -luio
fuse_devices fuse_cpuload fuse_watchdog fuse_pca9502: override CFLAGS += -D_FILE_OFFSET_BITS=64
fuse_devices fuse_cpuload fuse_watchdog fuse_pca9502: override LDFLAGS += -lfuse
fuse_cpuload fuse_watchdog: override LDFLAGS += -lpthread
fgltb_packet_filter: override LDFLAGS += -lpthread 
forward_server: override LDFLAGS += -lpthread

# Additional dependencies for some builds:
EXTRA_FUSE_WATCHDOG_TARGETS += fuse_watchdog_kick_cpld.o fuse_watchdog_kick_dev.o
fuse_watchdog: $(EXTRA_FUSE_WATCHDOG_TARGETS)

EXTRA_KICK_DEV_WATCHDOG_TARGETS += fuse_watchdog_kick_dev.o
kick_dev_watchdog: $(EXTRA_KICK_DEV_WATCHDOG_TARGETS)

EXTRA_RIP_RELATED_TARGETS += inifile.o
cpld_spi: $(EXTRA_RIP_RELATED_TARGETS)

EXTRA_FORWARD_SERVER_RELATED_TARGETS += ini_file.o
forward_server: $(EXTRA_FORWARD_SERVER_RELATED_TARGETS)

EXTRA_FORWARD_CLIENT_RELATED_TARGETS += parse_pkt.o
forward_client: $(EXTRA_FORWARD_CLIENT_RELATED_TARGETS)

# Rules
build: $(TARGETS)

install:
	mkdir -p $(DESTDIR)
	cp --no-dereference $(TARGETS) $(NOBUILD_TARGETS) $(DESTDIR)/


CLEAN_TARGETS += $(TARGETS)
CLEAN_TARGETS += $(EXTRA_FUSE_WATCHDOG_TARGETS) $(EXTRA_KICK_DEV_WATCHDOG_TARGETS)

clean:
	rm -rf $(CLEAN_TARGETS)
