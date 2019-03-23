
spec2def(dwmapi.dll dwmapi.spec)

list(APPEND SOURCE
    dwmapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi_stubs.c)

add_library(dwmapi SHARED 
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def)

set_module_type(dwmapi win32dll)
add_importlibs(dwmapi msvcrt kernel32 ntdll)
add_cd_file(TARGET dwmapi DESTINATION reactos/system32 FOR all)
