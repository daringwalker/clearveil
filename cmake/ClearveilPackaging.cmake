set(CPACK_PACKAGE_NAME "clearveil")
set(CPACK_PACKAGE_VENDOR "daringwalker")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${CLEARVEIL_VERSION_LABEL}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME
    "clearveil-${CLEARVEIL_VERSION_LABEL}-linux-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "clearveil-${CLEARVEIL_VERSION_LABEL}-Source")

# CPack treats these values as regular expressions. Keep source archives free
# of local build trees, editor state and agent/runtime metadata.
set(CPACK_SOURCE_IGNORE_FILES
    "/build[^/]*/"
    "/cmake-build-[^/]*/"
    "/dist/"
    "/\\.git/"
    "/\\.agents/"
    "/\\.codex/"
    "/\\.flatpak-builder/"
    "/\\.cache/"
    "/CMakeFiles/"
    "/CMakeCache\\.txt$"
    "/cmake_install\\.cmake$"
    "/Makefile$"
    "/_CPack_Packages/"
    "/compile_commands\\.json$"
    "~$"
)

include(CPack)
