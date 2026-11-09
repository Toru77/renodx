set(FALCOM_RTXDI_ROOT "${ADDON_PATH}/world/rtxdi/vendor/rtxdi")
add_library(falcom-rtxdi-sdk STATIC
  "${FALCOM_RTXDI_ROOT}/Source/ReSTIRDI.cpp"
  "${FALCOM_RTXDI_ROOT}/Source/RtxdiUtils.cpp")
target_include_directories(falcom-rtxdi-sdk SYSTEM PUBLIC "${FALCOM_RTXDI_ROOT}/Include")
target_link_libraries(${ADDON} PRIVATE falcom-rtxdi-sdk)
set_property(TARGET ${ADDON} PROPERTY RENODX_DXC_INCLUDE_DIRS "${FALCOM_RTXDI_ROOT}/Include")
add_executable(falcom-rtxdi-contract EXCLUDE_FROM_ALL "${ADDON_PATH}/world/rtxdi/tests/rtxdi_contract.cpp")
target_link_libraries(falcom-rtxdi-contract PRIVATE falcom-rtxdi-sdk)
add_executable(falcom-rtxdi-bridge EXCLUDE_FROM_ALL
  "${ADDON_PATH}/world/rtxdi/tests/rtxdi_bridge.cpp"
  "${ADDON_PATH}/world/rtxdi/tests/rtxdi_dxr_bridge.cpp")
target_link_libraries(falcom-rtxdi-bridge PRIVATE d3d11 d3d12 dxgi falcom-rtxdi-sdk)
target_include_directories(falcom-rtxdi-bridge PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed")
target_sources(falcom-rtxdi-bridge PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_pack_inputs.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_pack_records.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_unpack_records.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/rtxdi_surface_contract.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_sun_visibility.h")
target_sources(falcom-rtxdi-bridge PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_classify_lights.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_initial_di.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_spatial_di.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_temporal_di.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/rtxdi_raw_guard_contract.h")
