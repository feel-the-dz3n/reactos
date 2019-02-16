/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              win32ss/gdi/ntgdi/device.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <win32k.h>

// #define NDEBUG
#include <debug.h>

PDC defaultDCstate = NULL;

VOID FASTCALL
IntGdiReferencePdev(PPDEVOBJ ppdev)
{
    UNIMPLEMENTED;
}

VOID FASTCALL
IntGdiUnreferencePdev(PPDEVOBJ ppdev, DWORD CleanUpType)
{
    UNIMPLEMENTED;
}

BOOL FASTCALL
IntCreatePrimarySurface(VOID)
{
    /* Create surface */
    if (!PDEVOBJ_pSurface(gppdevPrimary))
        return FALSE;

    DPRINT("IntCreatePrimarySurface() gppdevPrimary = 0x%p, gppdevPrimary->pSurface = 0x%p\n",
        gppdevPrimary, gppdevPrimary->pSurface);

    /* Init Primary Display Device Capabilities */
    PDEVOBJ_vGetDeviceCaps(gppdevPrimary, &GdiHandleTable->DevCaps);

    return TRUE;
}

VOID FASTCALL
IntDestroyPrimarySurface(VOID)
{
    DPRINT("IntDestroyPrimarySurface() gppdevPrimary = 0x%p, gppdevPrimary->pSurface = 0x%p\n",
        gppdevPrimary, gppdevPrimary->pSurface);

    ASSERT(gppdevPrimary->pSurface);
    SURFACE_ShareUnlockSurface(gppdevPrimary->pSurface);
    gppdevPrimary->pSurface = NULL;

    UNIMPLEMENTED;
}

PPDEVOBJ FASTCALL
IntEnumHDev(VOID)
{
// I guess we will soon have more than one primary surface.
// This will do for now.
    return gppdevPrimary;
}


INT
APIENTRY
NtGdiDrawEscape(
    IN HDC hdc,
    IN INT iEsc,
    IN INT cjIn,
    IN OPTIONAL LPSTR pjIn)
{
    UNIMPLEMENTED;
    return 0;
}


