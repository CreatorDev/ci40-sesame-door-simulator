include $(TOPDIR)/rules.mk

PKG_NAME:=Ci40-sezame-door-simulator
PKG_VERSION:=HEAD
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git@gitlab.flowcloud.systems:creator/Ci40-sezame-door-simulator.git

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_VERSION:=$(PKG_VERSION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

CMAKE_INSTALL:=1

CMAKE_OPTIONS += -DSTAGING_DIR=$(STAGING_DIR)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/Ci40-sezame-door-simulator
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:= +letmecreate +libstdcpp
  TITLE:=Ci40 door simulator using CD tray
endef

define Package/Ci40-sezame-door-simulator/description
  Imagination Technologies - Ci40 door simulator using CD tray
endef

define Package/Ci40-sezame-door-simulator/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
