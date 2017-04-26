ADD_EXECUTABLE(Ci40-sezame-door-simulator main.cpp)
FIND_LIBRARY(LIB_LMC libletmecreate_core.so ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_LMC_CLICK libletmecreate_click.so ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_LMC_BOSCH libletmecreate_bosch.so ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_LMC_RPISENSEHAT libletmecreate_rpisensehat.so ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_STDCPP libstdc++.so ${STAGING_DIR}/toolchain-mipsel_mips32r2_gcc-5.3.0_musl-1.1.15/lib)

TARGET_LINK_LIBRARIES(Ci40-sezame-door-simulator ${LIB_LMC} ${LIB_LMC_CLICK} ${LIB_LMC_BOSCH} ${LIB_LMC_RPISENSEHAT} ${LIB_STDCPP})

INSTALL(TARGETS Ci40-sezame-door-simulator RUNTIME DESTINATION bin)
