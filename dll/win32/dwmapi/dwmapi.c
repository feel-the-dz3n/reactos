#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "dwmapi.h"

BOOL WINAPI DwmIsCompositionEnabled()
{
	return FALSE;
}
