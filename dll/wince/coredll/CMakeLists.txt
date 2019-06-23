
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(coredll.dll coredll.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/coredll_stubs.c)

add_library(coredll SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/coredll.def)

set_module_type(coredll win32dll)
target_link_libraries(coredll wine)
add_importlibs(coredll msvcrt kernel32 ntdll)
add_cd_file(TARGET coredll DESTINATION reactos/system32 FOR all)
