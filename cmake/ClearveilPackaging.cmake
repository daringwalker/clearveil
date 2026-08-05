set(CPACK_PACKAGE_NAME "clearveil")
set(CPACK_PACKAGE_VENDOR "daringwalker")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${CLEARVEIL_VERSION_LABEL}")
set(CPACK_PACKAGE_CONTACT
    "daringwalker <daringwalker@users.noreply.github.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL
    "https://github.com/daringwalker/clearveil")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME
    "clearveil-${CLEARVEIL_VERSION_LABEL}-linux-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "clearveil-${CLEARVEIL_VERSION_LABEL}-Source")

# Native package managers use different prerelease separators. Keep the
# user-facing version unchanged while producing valid, correctly ordered
# Debian and RPM package versions.
string(REPLACE "-" "~" CLEARVEIL_DEBIAN_VERSION
    "${CLEARVEIL_VERSION_LABEL}")
string(REPLACE "-" "~" CLEARVEIL_RPM_VERSION
    "${CLEARVEIL_VERSION_LABEL}")

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_VERSION "${CLEARVEIL_DEBIAN_VERSION}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if(TARGET PkgConfig::TESSERACT)
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS
        "tesseract-ocr-eng, tesseract-ocr-chi-sim")
endif()

set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_VERSION "${CLEARVEIL_RPM_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)

# CPack treats these values as regular expressions. Keep source archives free
# of local build trees, editor state and agent/runtime metadata.
set(CPACK_SOURCE_IGNORE_FILES
    "/build[^/]*/"
    "/cmake-build-[^/]*/"
    "/dist/"
    "/[.]git/"
    "/[.]agents/"
    "/[.]codex/"
    "/[.]flatpak-builder/"
    "/[.]cache/"
    "/CMakeFiles/"
    "/CMakeCache[.]txt$"
    "/cmake_install[.]cmake$"
    "/Makefile$"
    "/_CPack_Packages/"
    "/compile_commands[.]json$"
    "~$"
)

include(CPack)
