# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2006-2008 OpenWrt.org

include $(TOPDIR)/rules.mk

ARCH:=mipsel
BOARD:=bcmcc
BOARDNAME:=Broadcom ChipCommon
FEATURES:=squashfs usb
SUBTARGETS:=bcm4718

KERNEL_PATCHVER:=6.18

define Target/Description
	Build firmware images for Broadcom based BCM47xx/BCM53xx routers with LE MIPS CPU
endef

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += nvram otrx \
	kmod-leds-gpio kmod-gpio-button-hotplug

$(eval $(call BuildTarget))
