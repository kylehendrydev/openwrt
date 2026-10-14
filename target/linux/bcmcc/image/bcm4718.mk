define Device/netgear_wnr3500l-v1-na
  DEVICE_MODEL := WNR3500L
  DEVICE_VARIANT := v1 (NA)
  CHIP_ID := 4718
  SOC = bcm$$(CHIP_ID)
  KERNEL_DEPENDS = $$(wildcard ../dts/$$(DEVICE_DTS).dts)
  DEVICE_DTS_DIR := ../dts
  DEVICE_DTS = $$(SOC)-wnr3500l-na-v1
  DEVICE_PACKAGES := kmod-b43 $(USB2_PACKAGES)
  $(Device/netgear)
  NETGEAR_BOARD_ID := U12H136T99_NETGEAR
  NETGEAR_REGION := 2
  SUPPORTED_DEVICES := netgear,wnr3500l
endef
TARGET_DEVICES += netgear_wnr3500l-v1-na
