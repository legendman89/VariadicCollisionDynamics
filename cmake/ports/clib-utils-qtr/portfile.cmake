# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 18e53f05721b343f65c528fe478c0573862e1a9c
    SHA512 fe8ba37496d87af01f947247da06b546060180e41c40d1a0143e185be8e04d3749eab08dba52a6dcdc4088f8d9dbb87b4574715590bd6840c81c405ef0921a72
    HEAD_REF main
)

set(DRAW_DEBUG_HPP "${SOURCE_PATH}/include/CLibUtilsQTR/DrawDebug.hpp")
file(READ "${DRAW_DEBUG_HPP}" DRAW_DEBUG_CONTENT)
string(REPLACE
    "constexpr float DRAW_LOC_MAX_DIF = 5.0f;"
    "constexpr float DRAW_LOC_MAX_DIF = 1.0f;"
    DRAW_DEBUG_PATCHED
    "${DRAW_DEBUG_CONTENT}"
)
if(DRAW_DEBUG_PATCHED STREQUAL DRAW_DEBUG_CONTENT)
    if(NOT DRAW_DEBUG_CONTENT MATCHES "constexpr float DRAW_LOC_MAX_DIF = 2\\.0f;")
        message(FATAL_ERROR "Failed to patch DRAW_LOC_MAX_DIF in ${DRAW_DEBUG_HPP}")
    endif()
endif()
file(WRITE "${DRAW_DEBUG_HPP}" "${DRAW_DEBUG_PATCHED}")

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
