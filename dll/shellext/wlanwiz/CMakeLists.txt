
project(SHELL)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0xA00 -DNTDDI_VERSION=NTDDI_WIN10_FE)
spec2def(wlanwiz.dll wlanwiz.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    wlanwiz.cpp
    advsettings.cpp
    draw.cpp
    helpers.cpp
    keyb.cpp
    scan.cpp
    subclassproc_lb.cpp
    subclassproc_btngb.cpp)

file(GLOB wlanwiz_rc_deps res/*.*)
add_rc_deps(wlanwiz.rc ${wlanwiz_rc_deps})

add_library(wlanwiz MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/wlanwiz.def
    wlanwiz.rc)

set_module_type(wlanwiz win32dll UNICODE)
set_target_cpp_properties(wlanwiz WITH_EXCEPTIONS)
target_link_libraries(wlanwiz cpprt cppstl uuid atl_classes)
add_importlibs(wlanwiz version gdi32 user32 advapi32 comctl32 ole32 msvcrt msimg32 kernel32 shell32 shlwapi ntdll uxtheme wlanapi xmllite)
add_cd_file(TARGET wlanwiz DESTINATION reactos/system32 FOR all)
set_property(TARGET wlanwiz PROPERTY CXX_STANDARD 20)