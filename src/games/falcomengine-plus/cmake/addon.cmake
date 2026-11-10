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
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_probe.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/rtxdi_raw_guard_contract.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/rtxdi_lighting_consumer_contract.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_reconstruct.h")
target_sources(falcom-rtxdi-bridge PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_hitdistance.h")
target_sources(falcom-rtxdi-bridge PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_sampling_contract.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_coverage_contract.h")
target_sources(falcomengine-plus PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_prep.h"
  "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_debug_distance.h")

option(FALCOM_NRD_ENABLED "Enable the private NRD path in falcomengine-plus" ON)
set(FALCOM_NRD_SDK_ROOT "" CACHE PATH "Optional NRD 4.17.1 SDK root for the private D3D12 adapter harness")
if(NOT FALCOM_NRD_SDK_ROOT AND EXISTS "${CMAKE_BINARY_DIR}/nrd-sdk/include/NRD.h")
  set(FALCOM_NRD_SDK_ROOT "${CMAKE_BINARY_DIR}/nrd-sdk" CACHE PATH "Optional NRD 4.17.1 SDK root for the private D3D12 adapter harness" FORCE)
endif()
if(FALCOM_NRD_ENABLED AND NOT FALCOM_NRD_SDK_ROOT)
  message(WARNING "NRD SDK not found (set FALCOM_NRD_SDK_ROOT or place it in ${CMAKE_BINARY_DIR}/nrd-sdk); building falcomengine-plus without NRD")
  set(FALCOM_NRD_ENABLED OFF CACHE BOOL "Enable the private NRD path in falcomengine-plus" FORCE)
endif()
if(FALCOM_NRD_SDK_ROOT)
  if(NOT EXISTS "${FALCOM_NRD_SDK_ROOT}/include/NRD.h"
      OR NOT EXISTS "${FALCOM_NRD_SDK_ROOT}/lib/Debug/NRD.lib"
      OR NOT EXISTS "${FALCOM_NRD_SDK_ROOT}/lib/Release/NRD.lib"
      OR NOT EXISTS "${FALCOM_NRD_SDK_ROOT}/lib/Debug/ShaderMakeBlob.lib"
      OR NOT EXISTS "${FALCOM_NRD_SDK_ROOT}/lib/Release/ShaderMakeBlob.lib")
    message(FATAL_ERROR "FALCOM_NRD_SDK_ROOT must contain include/NRD.h and Debug/Release NRD and ShaderMakeBlob static libraries")
  endif()

  add_library(falcom-nrd-shadermakeblob STATIC IMPORTED GLOBAL)
  set_target_properties(falcom-nrd-shadermakeblob PROPERTIES
    IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
    IMPORTED_LOCATION_DEBUG "${FALCOM_NRD_SDK_ROOT}/lib/Debug/ShaderMakeBlob.lib"
    IMPORTED_LOCATION_RELEASE "${FALCOM_NRD_SDK_ROOT}/lib/Release/ShaderMakeBlob.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${FALCOM_NRD_SDK_ROOT}/lib/Release/ShaderMakeBlob.lib"
    IMPORTED_LOCATION_MINSIZEREL "${FALCOM_NRD_SDK_ROOT}/lib/Release/ShaderMakeBlob.lib")
  add_library(falcom-nrd-sdk STATIC IMPORTED GLOBAL)
  set_target_properties(falcom-nrd-sdk PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${FALCOM_NRD_SDK_ROOT}/include"
    INTERFACE_LINK_LIBRARIES falcom-nrd-shadermakeblob
    IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
    IMPORTED_LOCATION_DEBUG "${FALCOM_NRD_SDK_ROOT}/lib/Debug/NRD.lib"
    IMPORTED_LOCATION_RELEASE "${FALCOM_NRD_SDK_ROOT}/lib/Release/NRD.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${FALCOM_NRD_SDK_ROOT}/lib/Release/NRD.lib"
    IMPORTED_LOCATION_MINSIZEREL "${FALCOM_NRD_SDK_ROOT}/lib/Release/NRD.lib")

  add_executable(falcom-nrd-bridge EXCLUDE_FROM_ALL "${ADDON_PATH}/world/rt12/tests/nrd_d3d12_bridge.cpp")
  target_link_libraries(falcom-nrd-bridge PRIVATE d3d12 dxgi falcom-nrd-sdk)
  target_include_directories(falcom-nrd-bridge PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed")
  target_sources(falcom-nrd-bridge PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_probe.h"
    "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_prep.h"
    "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_publish.h"
    "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_debug_distance.h")
  if(FALCOM_NRD_ENABLED)
    target_link_libraries(${ADDON} PRIVATE d3d12 falcom-nrd-sdk)
    target_compile_definitions(${ADDON} PRIVATE FALCOM_NRD_ENABLED=1)
    target_sources(${ADDON} PRIVATE
      "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_hitdistance.h"
      "${CMAKE_CURRENT_BINARY_DIR}/${ADDON}.include/embed/world_rtxdi_nrd_publish.h")
  endif()
endif()
