/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

#include "inbv/logo.h"

/* See also mm/ARM3/miarm.h */
#define MM_READONLY     1   // PAGE_READONLY
#define MM_READWRITE    4   // PAGE_WRITECOPY

#ifndef TAG_OSTR
#define TAG_OSTR    'RTSO'
#endif

/* GLOBALS *******************************************************************/

/*
 * Enable this define if you want Inbv to use coloured headless mode.
 */
// #define INBV_HEADLESS_COLORS

/*
 * ReactOS uses the same boot screen for all the products.
 * Also it doesn't use a parallel system thread for the
 * rotating "progress" bar.
 */

/*
 * Enable this define when ReactOS will have different SKUs
 * (Workstation, Server, Storage Server, Cluster Server, etc...).
 */
// #define REACTOS_SKUS

typedef struct _INBV_PROGRESS_STATE
{
    ULONG Floor;
    ULONG Ceiling;
    ULONG Bias;
} INBV_PROGRESS_STATE;

typedef struct _BT_PROGRESS_INDICATOR
{
    ULONG Count;
    ULONG Expected;
    ULONG Percentage;
} BT_PROGRESS_INDICATOR, *PBT_PROGRESS_INDICATOR;

typedef enum _ROT_BAR_TYPE
{
    RB_UNSPECIFIED,
    RB_SQUARE_CELLS,
    RB_PROGRESS_BAR
} ROT_BAR_TYPE;

/*
 * Enable this define when Inbv will support rotating progress bar.
 */
#define INBV_ROTBAR_IMPLEMENTED

static KSPIN_LOCK BootDriverLock;
static KIRQL InbvOldIrql;
static INBV_DISPLAY_STATE InbvDisplayState = INBV_DISPLAY_STATE_DISABLED;
BOOLEAN InbvBootDriverInstalled = FALSE;
static BOOLEAN InbvDisplayDebugStrings = FALSE;
static INBV_DISPLAY_STRING_FILTER InbvDisplayFilter = NULL;
static INBV_PROGRESS_STATE InbvProgressState;
static BT_PROGRESS_INDICATOR InbvProgressIndicator = {0, 25, 0};
static INBV_RESET_DISPLAY_PARAMETERS InbvResetDisplayParameters = NULL;
static ULONG ResourceCount = 0;
static PUCHAR ResourceList[1 + IDB_MAX_RESOURCE]; // First entry == NULL, followed by 'ResourceCount' entries.

static ULONG ProgressBarLeft = 0, ProgressBarTop = 0;
static ULONG ProgressBarWidth = 0, ProgressBarHeight = 0;
static ULONG ProgressBarBackground = 0, ProgressBarForeground = BV_COLOR_WHITE;
static BOOLEAN ProgressBarRounding = FALSE, ProgressBarShowBkg = FALSE;
static BOOLEAN ShowProgressBar = FALSE;

#ifdef INBV_ROTBAR_IMPLEMENTED
/*
 * Change this to modify progress bar behaviour
 */
#define ROT_BAR_DEFAULT_MODE    RB_PROGRESS_BAR

/*
 * Values for PltRotBarStatus:
 * - PltRotBarStatus == 1, do palette fading-in (done elsewhere in ReactOS);
 * - PltRotBarStatus == 2, do rotation bar animation;
 * - PltRotBarStatus == 3, stop the animation thread.
 * - Any other value is ignored and the animation thread continues to run.
 */
typedef enum _ROT_BAR_STATUS
{
    RBS_FADEIN = 1,
    RBS_ANIMATE,
    RBS_STOP_ANIMATE,
    RBS_STATUS_MAX
} ROT_BAR_STATUS;

static BOOLEAN RotBarThreadActive = FALSE;
static ROT_BAR_TYPE RotBarSelection = RB_UNSPECIFIED;
static ROT_BAR_STATUS PltRotBarStatus = 0;
static UCHAR RotBarBuffer[24 * 9];
static UCHAR RotLineBuffer[SCREEN_WIDTH * VID_ROTBAR_HEIGHT];
#endif


/*
 * Headless terminal text colors
 */

#ifdef INBV_HEADLESS_COLORS

// Conversion table CGA to ANSI color index
static const UCHAR CGA_TO_ANSI_COLOR_TABLE[16] =
{
    0,  // Black
    4,  // Blue
    2,  // Green
    6,  // Cyan
    1,  // Red
    5,  // Magenta
    3,  // Brown/Yellow
    7,  // Grey/White

    60, // Bright Black
    64, // Bright Blue
    62, // Bright Green
    66, // Bright Cyan
    61, // Bright Red
    65, // Bright Magenta
    63, // Bright Yellow
    67  // Bright Grey (White)
};

#define CGA_TO_ANSI_COLOR(CgaColor) \
    CGA_TO_ANSI_COLOR_TABLE[CgaColor & 0x0F]

#endif

// Default colors: text in white, background in black
static ULONG InbvTerminalTextColor = 37;
static ULONG InbvTerminalBkgdColor = 40;


/* FADING FUNCTION ***********************************************************/

/** From include/psdk/wingdi.h **/
typedef struct tagRGBQUAD
{
    UCHAR    rgbBlue;
    UCHAR    rgbGreen;
    UCHAR    rgbRed;
    UCHAR    rgbReserved;
} RGBQUAD,*LPRGBQUAD;
/*******************************/

static RGBQUAD MainPalette[16];

#define PALETTE_FADE_STEPS  9
#define PALETTE_FADE_TIME   (2 * 1000) /* 2 ms */

/** From bootvid/precomp.h **/
//
// Bitmap Header
//
typedef struct tagBITMAPINFOHEADER
{
    ULONG biSize;
    LONG biWidth;
    LONG biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG biCompression;
    ULONG biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    ULONG biClrUsed;
    ULONG biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
/****************************/

//
// Needed prototypes
//
VOID NTAPI InbvAcquireLock(VOID);
VOID NTAPI InbvReleaseLock(VOID);

static VOID
BootLogoFadeIn(VOID)
{
    UCHAR PaletteBitmapBuffer[sizeof(BITMAPINFOHEADER) + sizeof(MainPalette)];
    PBITMAPINFOHEADER PaletteBitmap = (PBITMAPINFOHEADER)PaletteBitmapBuffer;
    LPRGBQUAD Palette = (LPRGBQUAD)(PaletteBitmapBuffer + sizeof(BITMAPINFOHEADER));
    ULONG Iteration, Index, ClrUsed;

    LARGE_INTEGER Delay;
    Delay.QuadPart = -(PALETTE_FADE_TIME * 10);

    /* Check if we are installed and we own the display */
    if (!InbvBootDriverInstalled ||
        (InbvDisplayState != INBV_DISPLAY_STATE_OWNED))
    {
        return;
    }

    /*
     * Build a bitmap containing the fade-in palette. The palette entries
     * are then processed in a loop and set using VidBitBlt function.
     */
    ClrUsed = RTL_NUMBER_OF(MainPalette);
    RtlZeroMemory(PaletteBitmap, sizeof(BITMAPINFOHEADER));
    PaletteBitmap->biSize = sizeof(BITMAPINFOHEADER);
    PaletteBitmap->biBitCount = 4;
    PaletteBitmap->biClrUsed = ClrUsed;

    /*
     * Main animation loop.
     */
    for (Iteration = 0; Iteration <= PALETTE_FADE_STEPS; ++Iteration)
    {
        for (Index = 0; Index < ClrUsed; Index++)
        {
            Palette[Index].rgbRed = (UCHAR)
                (MainPalette[Index].rgbRed * Iteration / PALETTE_FADE_STEPS);
            Palette[Index].rgbGreen = (UCHAR)
                (MainPalette[Index].rgbGreen * Iteration / PALETTE_FADE_STEPS);
            Palette[Index].rgbBlue = (UCHAR)
                (MainPalette[Index].rgbBlue * Iteration / PALETTE_FADE_STEPS);
        }

        /* Do the animation */
        InbvAcquireLock();
        VidBitBlt(PaletteBitmapBuffer, 0, 0);
        InbvReleaseLock();

        /* Wait for a bit */
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);
    }
}

VOID
NTAPI
InbvBitBltPalette(
    IN PVOID Image,
    IN BOOLEAN NoPalette,
    IN ULONG X,
    IN ULONG Y)
{
    LPRGBQUAD Palette;
    RGBQUAD OrigPalette[RTL_NUMBER_OF(MainPalette)];

    /* If requested, remove the palette from the image */
    if (NoPalette)
    {
        /* Get bitmap header and palette */
        PBITMAPINFOHEADER BitmapInfoHeader = Image;
        Palette = (LPRGBQUAD)((PUCHAR)Image + BitmapInfoHeader->biSize);

        /* Save the image original palette and remove palette information */
        RtlCopyMemory(OrigPalette, Palette, sizeof(OrigPalette));
        RtlZeroMemory(Palette, sizeof(OrigPalette));
    }

    /* Draw the image */
    InbvBitBlt(Image, X, Y);

    /* Restore the image original palette */
    if (NoPalette)
    {
        RtlCopyMemory(Palette, OrigPalette, sizeof(OrigPalette));
    }
}

VOID
NTAPI
InbvBitBltAligned(
    IN PVOID Image,
    IN BOOLEAN NoPalette,
    IN BBLT_HORZ_ALIGNMENT HorizontalAlignment,
    IN BBLT_VERT_ALIGNMENT VerticalAlignment,
    IN ULONG MarginLeft,
    IN ULONG MarginTop,
    IN ULONG MarginRight,
    IN ULONG MarginBottom)
{
    PBITMAPINFOHEADER BitmapInfoHeader = Image;
    ULONG X, Y;

    /* Calculate X */
    switch (HorizontalAlignment)
    {
        case AL_HORIZONTAL_LEFT:
            X = MarginLeft - MarginRight;
            break;

        case AL_HORIZONTAL_CENTER:
            X = MarginLeft - MarginRight + (SCREEN_WIDTH - BitmapInfoHeader->biWidth + 1) / 2;
            break;

        case AL_HORIZONTAL_RIGHT:
            X = MarginLeft - MarginRight + SCREEN_WIDTH - BitmapInfoHeader->biWidth;
            break;

        default:
            /* Unknown */
            return;
    }

    /* Calculate Y */
    switch (VerticalAlignment)
    {
        case AL_VERTICAL_TOP:
            Y = MarginTop - MarginBottom;
            break;

        case AL_VERTICAL_CENTER:
            Y = MarginTop - MarginBottom + (SCREEN_HEIGHT - BitmapInfoHeader->biHeight + 1) / 2;
            break;

        case AL_VERTICAL_BOTTOM:
            Y = MarginTop - MarginBottom + SCREEN_HEIGHT - BitmapInfoHeader->biHeight;
            break;

        default:
            /* Unknown */
            return;
    }

    /* Finally draw the image */
    InbvBitBltPalette(Image, NoPalette, X, Y);
}

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
PVOID
NTAPI
FindBitmapResource(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                   IN ULONG ResourceId)
{
    UNICODE_STRING UpString = RTL_CONSTANT_STRING(L"ntoskrnl.exe");
    UNICODE_STRING MpString = RTL_CONSTANT_STRING(L"ntkrnlmp.exe");
    PLIST_ENTRY NextEntry, ListHead;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    LDR_RESOURCE_INFO ResourceInfo;
    NTSTATUS Status;
    PVOID Data = NULL;

    /* Loop the driver list */
    ListHead = &LoaderBlock->LoadOrderListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Check for a match */
        if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &UpString, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &MpString, TRUE))
        {
            /* Break out */
            break;
        }
    }

    /* Check if we found it */
    if (NextEntry != ListHead)
    {
        /* Try to find the resource */
        ResourceInfo.Type = 2; // RT_BITMAP;
        ResourceInfo.Name = ResourceId;
        ResourceInfo.Language = 0;
        Status = LdrFindResource_U(LdrEntry->DllBase,
                                   &ResourceInfo,
                                   RESOURCE_DATA_LEVEL,
                                   &ResourceDataEntry);
        if (NT_SUCCESS(Status))
        {
            /* Access the resource */
            ULONG Size = 0;
            Status = LdrAccessResource(LdrEntry->DllBase,
                                       ResourceDataEntry,
                                       &Data,
                                       &Size);
            if ((Data) && (ResourceId < 3))
            {
                KiBugCheckData[4] ^= RtlComputeCrc32(0, Data, Size);
            }
            if (!NT_SUCCESS(Status)) Data = NULL;
        }
    }

    /* Return the pointer */
    return Data;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
InbvDriverInitialize(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                     IN ULONG Count)
{
    PCHAR CommandLine;
    BOOLEAN ResetMode = FALSE; // By default do not reset the video mode
    ULONG i;

    /* Quit if we're already installed */
    if (InbvBootDriverInstalled) return TRUE;

    /* Initialize the lock and check the current display state */
    KeInitializeSpinLock(&BootDriverLock);
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* Reset the video mode in case we do not have a custom boot logo */
        CommandLine = (LoaderBlock->LoadOptions ? _strupr(LoaderBlock->LoadOptions) : NULL);
        ResetMode   = (CommandLine == NULL) || (strstr(CommandLine, "BOOTLOGO") == NULL);
    }

    /* Initialize the video */
    InbvBootDriverInstalled = VidInitialize(ResetMode);
    if (InbvBootDriverInstalled)
    {
        /* Find bitmap resources in the kernel */
        ResourceCount = min(Count, RTL_NUMBER_OF(ResourceList) - 1);
        for (i = 1; i <= ResourceCount; i++)
        {
            /* Do the lookup */
            ResourceList[i] = FindBitmapResource(LoaderBlock, i);
        }

        /* Set the progress bar ranges */
        InbvSetProgressBarSubset(0, 100);
    }

    /* Return install state */
    return InbvBootDriverInstalled;
}

VOID
NTAPI
InbvAcquireLock(VOID)
{
    KIRQL OldIrql;

    /* Check if we're at dispatch level or lower */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql <= DISPATCH_LEVEL)
    {
        /* Loop until the lock is free */
        while (!KeTestSpinLock(&BootDriverLock));

        /* Raise IRQL to dispatch level */
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }

    /* Acquire the lock */
    KiAcquireSpinLock(&BootDriverLock);
    InbvOldIrql = OldIrql;
}

VOID
NTAPI
InbvReleaseLock(VOID)
{
    KIRQL OldIrql;

    /* Capture the old IRQL */
    OldIrql = InbvOldIrql;

    /* Release the driver lock */
    KiReleaseSpinLock(&BootDriverLock);

    /* If we were at dispatch level or lower, restore the old IRQL */
    if (InbvOldIrql <= DISPATCH_LEVEL) KeLowerIrql(OldIrql);
}

VOID
NTAPI
InbvEnableBootDriver(IN BOOLEAN Enable)
{
    /* Check if we're installed */
    if (InbvBootDriverInstalled)
    {
        /* Check for lost state */
        if (InbvDisplayState >= INBV_DISPLAY_STATE_LOST) return;

        /* Acquire the lock */
        InbvAcquireLock();

        /* Cleanup the screen if we own it */
        if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED) VidCleanUp();

        /* Set the new display state */
        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED :
                                    INBV_DISPLAY_STATE_DISABLED;

        /* Release the lock */
        InbvReleaseLock();
    }
    else
    {
        /* Set the new display state */
        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED :
                                    INBV_DISPLAY_STATE_DISABLED;
    }
}

VOID
NTAPI
InbvAcquireDisplayOwnership(VOID)
{
    /* Check if we have a callback and we're just acquiring it now */
    if ((InbvResetDisplayParameters) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_LOST))
    {
        /* Call the callback */
        InbvResetDisplayParameters(80, 50);
    }

    /* Acquire the display */
    InbvDisplayState = INBV_DISPLAY_STATE_OWNED;
}

VOID
NTAPI
InbvSetDisplayOwnership(IN BOOLEAN DisplayOwned)
{
    /* Set the new display state */
    InbvDisplayState = DisplayOwned ? INBV_DISPLAY_STATE_OWNED :
                                      INBV_DISPLAY_STATE_LOST;
}

BOOLEAN
NTAPI
InbvCheckDisplayOwnership(VOID)
{
    /* Return if we own it or not */
    return InbvDisplayState != INBV_DISPLAY_STATE_LOST;
}

INBV_DISPLAY_STATE
NTAPI
InbvGetDisplayState(VOID)
{
    /* Return the actual state */
    return InbvDisplayState;
}

BOOLEAN
NTAPI
InbvDisplayString(IN PCHAR String)
{
    /* Make sure we own the display */
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* If we're not allowed, return success anyway */
        if (!InbvDisplayDebugStrings) return TRUE;

        /* Check if a filter is installed */
        if (InbvDisplayFilter) InbvDisplayFilter(&String);

        /* Acquire the lock */
        InbvAcquireLock();

        /* Make sure we're installed and display the string */
        if (InbvBootDriverInstalled) VidDisplayString((PUCHAR)String);

        /* Print the string on the EMS port */
        HeadlessDispatch(HeadlessCmdPutString,
                         String,
                         strlen(String) + sizeof(ANSI_NULL),
                         NULL,
                         NULL);

        /* Release the lock */
        InbvReleaseLock();

        /* All done */
        return TRUE;
    }

    /* We don't own it, fail */
    return FALSE;
}

BOOLEAN
NTAPI
InbvEnableDisplayString(IN BOOLEAN Enable)
{
    BOOLEAN OldSetting;

    /* Get the old setting */
    OldSetting = InbvDisplayDebugStrings;

    /* Update it */
    InbvDisplayDebugStrings = Enable;

    /* Return the old setting */
    return OldSetting;
}

VOID
NTAPI
InbvInstallDisplayStringFilter(IN INBV_DISPLAY_STRING_FILTER Filter)
{
    /* Save the filter */
    InbvDisplayFilter = Filter;
}

BOOLEAN
NTAPI
InbvIsBootDriverInstalled(VOID)
{
    /* Return driver state */
    return InbvBootDriverInstalled;
}

VOID
NTAPI
InbvNotifyDisplayOwnershipLost(IN INBV_RESET_DISPLAY_PARAMETERS Callback)
{
    /* Check if we're installed */
    if (InbvBootDriverInstalled)
    {
        /* Acquire the lock and cleanup if we own the screen */
        InbvAcquireLock();
        if (InbvDisplayState != INBV_DISPLAY_STATE_LOST) VidCleanUp();

        /* Set the reset callback and display state */
        InbvResetDisplayParameters = Callback;
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;

        /* Release the lock */
        InbvReleaseLock();
    }
    else
    {
        /* Set the reset callback and display state */
        InbvResetDisplayParameters = Callback;
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;
    }
}

BOOLEAN
NTAPI
InbvResetDisplay(VOID)
{
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the reset */
        VidResetDisplay(TRUE);
        return TRUE;
    }

    /* Nothing to reset */
    return FALSE;
}

VOID
NTAPI
InbvSetScrollRegion(IN ULONG Left,
                    IN ULONG Top,
                    IN ULONG Right,
                    IN ULONG Bottom)
{
    /* Just call bootvid */
    VidSetScrollRegion(Left, Top, Right, Bottom);
}

VOID
NTAPI
InbvSetTextColor(IN ULONG Color)
{
    HEADLESS_CMD_SET_COLOR HeadlessSetColor;

    /* Set color for EMS port */
#ifdef INBV_HEADLESS_COLORS
    InbvTerminalTextColor = 30 + CGA_TO_ANSI_COLOR(Color);
#else
    InbvTerminalTextColor = 37;
#endif
    HeadlessSetColor.TextColor = InbvTerminalTextColor;
    HeadlessSetColor.BkgdColor = InbvTerminalBkgdColor;
    HeadlessDispatch(HeadlessCmdSetColor,
                     &HeadlessSetColor,
                     sizeof(HeadlessSetColor),
                     NULL,
                     NULL);

    /* Update the text color */
    VidSetTextColor(Color);
}

VOID
NTAPI
InbvSolidColorFill(IN ULONG Left,
                   IN ULONG Top,
                   IN ULONG Right,
                   IN ULONG Bottom,
                   IN ULONG Color)
{
    HEADLESS_CMD_SET_COLOR HeadlessSetColor;

    /* Make sure we own it */
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Check if we're installed */
        if (InbvBootDriverInstalled)
        {
            /* Call bootvid */
            VidSolidColorFill(Left, Top, Right, Bottom, (UCHAR)Color);
        }

        /* Set color for EMS port and clear display */
#ifdef INBV_HEADLESS_COLORS
        InbvTerminalBkgdColor = 40 + CGA_TO_ANSI_COLOR(Color);
#else
        InbvTerminalBkgdColor = 40;
#endif
        HeadlessSetColor.TextColor = InbvTerminalTextColor;
        HeadlessSetColor.BkgdColor = InbvTerminalBkgdColor;
        HeadlessDispatch(HeadlessCmdSetColor,
                         &HeadlessSetColor,
                         sizeof(HeadlessSetColor),
                         NULL,
                         NULL);
        HeadlessDispatch(HeadlessCmdClearDisplay,
                         NULL, 0,
                         NULL, NULL);

        /* Release the lock */
        InbvReleaseLock();
    }
}

CODE_SEG("INIT")
VOID
NTAPI
InbvUpdateProgressBar(IN ULONG Progress)
{
    ULONG FillCount, BoundedProgress;

    /* Make sure the progress bar is enabled, that we own and are installed */
    if (ShowProgressBar &&
        InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Compute fill count */
        BoundedProgress = (InbvProgressState.Floor / 100) + Progress;
        FillCount = ProgressBarWidth * (InbvProgressState.Bias * BoundedProgress) / 1000000;

        /* Acquire the lock */
        InbvAcquireLock();

        /* Fill the progress bar */
        VidSolidColorFill(ProgressBarLeft,
                          ProgressBarTop,
                          ProgressBarLeft + FillCount,
                          ProgressBarTop + ProgressBarHeight,
                          BV_COLOR_WHITE);

        /* Release the lock */
        InbvReleaseLock();
    }
}

VOID
NTAPI
InbvBufferToScreenBlt(IN PUCHAR Buffer,
                      IN ULONG X,
                      IN ULONG Y,
                      IN ULONG Width,
                      IN ULONG Height,
                      IN ULONG Delta)
{
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the blit */
        VidBufferToScreenBlt(Buffer, X, Y, Width, Height, Delta);
    }
}

VOID
NTAPI
InbvBitBlt(IN PUCHAR Buffer,
           IN ULONG X,
           IN ULONG Y)
{
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Do the blit */
        VidBitBlt(Buffer, X, Y);

        /* Release the lock */
        InbvReleaseLock();
    }
}

VOID
NTAPI
InbvScreenToBufferBlt(OUT PUCHAR Buffer,
                      IN ULONG X,
                      IN ULONG Y,
                      IN ULONG Width,
                      IN ULONG Height,
                      IN ULONG Delta)
{
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the blit */
        VidScreenToBufferBlt(Buffer, X, Y, Width, Height, Delta);
    }
}

CODE_SEG("INIT")
VOID
NTAPI
InbvSetProgressBarSettings(IN ULONG Left,
                           IN ULONG Top,
                           IN ULONG Width,
                           IN ULONG Height,
                           IN ULONG ForegroundColor)
{
    /* Update the settings */
    ProgressBarLeft       = Left;
    ProgressBarTop        = Top;
    ProgressBarWidth      = Width;
    ProgressBarHeight     = Height;
    ProgressBarForeground = ForegroundColor;
    ProgressBarShowBkg    = FALSE;
    ProgressBarRounding   = FALSE;

    /* Enable the progress bar */
    ShowProgressBar = TRUE;
}

CODE_SEG("INIT")
VOID
NTAPI
InbvSetProgressBarBackground(IN BOOLEAN Enable,
                             IN ULONG Background)
{
    ProgressBarBackground = Background;
    ProgressBarShowBkg    = Enable;
}

INIT_FUNCTION
VOID
NTAPI
InbvSetProgressBarRounding(IN BOOLEAN Enable)
{
    ProgressBarRounding = Enable;
}

INIT_FUNCTION
VOID
NTAPI
InbvSetProgressBarSubset(IN ULONG Floor,
                         IN ULONG Ceiling)
{
    /* Sanity checks */
    ASSERT(Floor < Ceiling);
    ASSERT(Ceiling <= 100);

    /* Update the progress bar state */
    InbvProgressState.Floor = Floor * 100;
    InbvProgressState.Ceiling = Ceiling * 100;
    InbvProgressState.Bias = (Ceiling * 100) - Floor;
}

CODE_SEG("INIT")
VOID
NTAPI
InbvIndicateProgress(VOID)
{
    ULONG Percentage;

    /* Increase progress */
    InbvProgressIndicator.Count++;

    /* Compute new percentage */
    Percentage = min(100 * InbvProgressIndicator.Count /
                           InbvProgressIndicator.Expected,
                     99);
    if (Percentage != InbvProgressIndicator.Percentage)
    {
        /* Percentage has moved, update the progress bar */
        InbvProgressIndicator.Percentage = Percentage;
        InbvUpdateProgressBar(Percentage);
    }
}

PUCHAR
NTAPI
InbvGetResourceAddress(IN ULONG ResourceNumber)
{
    /* Validate the resource number */
    if (ResourceNumber > ResourceCount) return NULL;

    /* Return the address */
    return ResourceList[ResourceNumber];
}

NTSTATUS
NTAPI
NtDisplayString(IN PUNICODE_STRING DisplayString)
{
    NTSTATUS Status;
    UNICODE_STRING CapturedString;
    OEM_STRING OemString;
    ULONG OemLength;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* We require the TCB privilege */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
        return STATUS_PRIVILEGE_NOT_HELD;

    /* Capture the string */
    Status = ProbeAndCaptureUnicodeString(&CapturedString, PreviousMode, DisplayString);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Do not display the string if it is empty */
    if (CapturedString.Length == 0 || CapturedString.Buffer == NULL)
    {
        Status = STATUS_SUCCESS;
        goto Quit;
    }

    /*
     * Convert the string since INBV understands only ANSI/OEM. Allocate the
     * string buffer in non-paged pool because INBV passes it down to BOOTVID.
     * We cannot perform the allocation using RtlUnicodeStringToOemString()
     * since its allocator uses PagedPool.
     */
    OemLength = RtlUnicodeStringToOemSize(&CapturedString);
    if (OemLength > MAXUSHORT)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Quit;
    }
    RtlInitEmptyAnsiString((PANSI_STRING)&OemString, NULL, (USHORT)OemLength);
    OemString.Buffer = ExAllocatePoolWithTag(NonPagedPool, OemLength, TAG_OSTR);
    if (OemString.Buffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }
    Status = RtlUnicodeStringToOemString(&OemString, &CapturedString, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(OemString.Buffer, TAG_OSTR);
        goto Quit;
    }

    /* Display the string */
    InbvDisplayString(OemString.Buffer);

    /* Free the string buffer */
    ExFreePoolWithTag(OemString.Buffer, TAG_OSTR);

    Status = STATUS_SUCCESS;

Quit:
    /* Free the captured string */
    ReleaseCapturedUnicodeString(&CapturedString, PreviousMode);

    return Status;
}

#ifdef INBV_ROTBAR_IMPLEMENTED
static
VOID
NTAPI
InbvRotationThread(
    _In_ PVOID Context)
{
    ULONG X, Y, Index, Total;
    LARGE_INTEGER Delay = {{0}};

    InbvAcquireLock();
    if (RotBarSelection == RB_SQUARE_CELLS)
    {
        Index = 0;
    }
    else
    {
        Index = 32;
    }
    X = ProgressBarLeft + 2;
    Y = ProgressBarTop + 2;
    InbvReleaseLock();

    while (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* Wait for a bit */
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);

        InbvAcquireLock();

        /* Unknown unexpected command */
        ASSERT(PltRotBarStatus < RBS_STATUS_MAX);

        if (PltRotBarStatus == RBS_STOP_ANIMATE)
        {
            /* Stop the thread */
            InbvReleaseLock();
            break;
        }

        if (RotBarSelection == RB_SQUARE_CELLS)
        {
            Delay.QuadPart = -800000; // 80 ms
            Total = 18;
            Index %= Total;

            if (Index >= 3)
            {
                /* Fill previous bar position */
                VidSolidColorFill(X + ((Index - 3) * 8), Y, (X + ((Index - 3) * 8)) + 8 - 1, Y + 9 - 1, BV_COLOR_BLACK);
            }
            if (Index < Total - 1)
            {
                /* Draw the progress bar bit */
                if (Index < 2)
                {
                    /* Appearing from the left */
                    VidBufferToScreenBlt(RotBarBuffer + 8 * (2 - Index) / 2, X, Y, 22 - 8 * (2 - Index), 9, 24);
                }
                else if (Index >= Total - 3)
                {
                    /* Hiding to the right */
                    VidBufferToScreenBlt(RotBarBuffer, X + ((Index - 2) * 8), Y, 22 - 8 * (4 - (Total - Index)), 9, 24);
                }
                else
                {
                    VidBufferToScreenBlt(RotBarBuffer, X + ((Index - 2) * 8), Y, 22, 9, 24);
                }
            }
            Index++;
        }
        else if (RotBarSelection == RB_PROGRESS_BAR)
        {
            Delay.QuadPart = -600000; // 60 ms
            Total = SCREEN_WIDTH;
            Index %= Total;

            /* Right part */
            VidBufferToScreenBlt(RotLineBuffer, Index, SCREEN_HEIGHT-VID_ROTBAR_HEIGHT, SCREEN_WIDTH - Index, VID_ROTBAR_HEIGHT, SCREEN_WIDTH);
            if (Index > 0)
            {
                /* Left part */
                VidBufferToScreenBlt(RotLineBuffer + (SCREEN_WIDTH - Index) / 2, 0, SCREEN_HEIGHT-VID_ROTBAR_HEIGHT, Index - 2, VID_ROTBAR_HEIGHT, SCREEN_WIDTH);
            }
            Index += 32;
        }

        InbvReleaseLock();
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

CODE_SEG("INIT")
VOID
NTAPI
InbvRotBarInit(VOID)
{
    PltRotBarStatus = RBS_FADEIN;
    /* Perform other initialization if needed */
}
#endif

CODE_SEG("INIT")
VOID
NTAPI
DisplayBootBitmap(IN BOOLEAN TextMode)
{
    PVOID BootCopy = NULL, BootLogo = NULL, Header = NULL, Footer = NULL;

#ifdef INBV_ROTBAR_IMPLEMENTED
    UCHAR Buffer[24 * 9];
    PVOID Bar = NULL, LineBmp = NULL;
    ROT_BAR_TYPE TempRotBarSelection = RB_UNSPECIFIED;
    NTSTATUS Status;
    HANDLE ThreadHandle = NULL;
#endif

#ifdef REACTOS_SKUS
    PVOID Text = NULL;
#endif

#ifdef INBV_ROTBAR_IMPLEMENTED
    /* Check if the animation thread has already been created */
    if (RotBarThreadActive)
    {
        /* Yes, just reset the progress bar but keep the thread alive */
        InbvAcquireLock();
        RotBarSelection = RB_UNSPECIFIED;
        InbvReleaseLock();
    }
#endif

    ShowProgressBar = FALSE;

    /* Check if this is text mode */
    if (TextMode)
    {
        /*
         * Make the kernel resource section temporarily writable,
         * as we are going to change the bitmaps' palette in place.
         */
        MmChangeKernelResourceSectionProtection(MM_READWRITE);

        /* Check the type of the OS: workstation or server */
        if (SharedUserData->NtProductType == NtProductWinNt)
        {
            /* Workstation; set colors */
            InbvSetTextColor(BV_COLOR_WHITE);
            InbvSolidColorFill(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BV_COLOR_DARK_GRAY);
            InbvSolidColorFill(0, VID_FOOTER_BG_TOP, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BV_COLOR_RED);

            /* Get resources */
            Header = InbvGetResourceAddress(IDB_WKSTA_HEADER);
            Footer = InbvGetResourceAddress(IDB_WKSTA_FOOTER);
        }
        else
        {
            /* Server; set colors */
            InbvSetTextColor(BV_COLOR_LIGHT_CYAN);
            InbvSolidColorFill(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BV_COLOR_CYAN);
            InbvSolidColorFill(0, VID_FOOTER_BG_TOP, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BV_COLOR_RED);

            /* Get resources */
            Header = InbvGetResourceAddress(IDB_SERVER_HEADER);
            Footer = InbvGetResourceAddress(IDB_SERVER_FOOTER);
        }

        /* Set the scrolling region */
        InbvSetScrollRegion(VID_SCROLL_AREA_LEFT, VID_SCROLL_AREA_TOP,
                            VID_SCROLL_AREA_RIGHT, VID_SCROLL_AREA_BOTTOM);

        /* Make sure we have resources */
        if (Header && Footer)
        {
            /* BitBlt them on the screen */
            InbvBitBltAligned(Footer,
                          TRUE,
                          AL_HORIZONTAL_CENTER,
                          AL_VERTICAL_BOTTOM,
                          0, 0, 0, 59);
            InbvBitBltAligned(Header,
                          FALSE,
                          AL_HORIZONTAL_CENTER,
                          AL_VERTICAL_TOP,
                          0, 0, 0, 0);
        }

        /* Restore the kernel resource section protection to be read-only */
        MmChangeKernelResourceSectionProtection(MM_READONLY);
    }
    else
    {
        /* Is the boot driver installed? */
        if (!InbvBootDriverInstalled) return;

        /*
         * Make the kernel resource section temporarily writable,
         * as we are going to change the bitmaps' palette in place.
         */
        MmChangeKernelResourceSectionProtection(MM_READWRITE);

        /* Load boot screen logo */
        BootLogo = InbvGetResourceAddress(IDB_LOGO_DEFAULT);

#ifdef REACTOS_SKUS
        Text = NULL;
        if (SharedUserData->NtProductType == NtProductWinNt)
        {
#ifdef INBV_ROTBAR_IMPLEMENTED
            /* Workstation product, use appropriate status bar color */
            Bar = InbvGetResourceAddress(IDB_BAR_WKSTA);
#endif
        }
        else
        {
            /* Display correct branding based on server suite */
            if (ExVerifySuite(StorageServer))
            {
                /* Storage Server Edition */
                Text = InbvGetResourceAddress(IDB_STORAGE_SERVER);
            }
            else if (ExVerifySuite(ComputeServer))
            {
                /* Compute Cluster Edition */
                Text = InbvGetResourceAddress(IDB_CLUSTER_SERVER);
            }
            else
            {
                /* Normal edition */
                Text = InbvGetResourceAddress(IDB_SERVER_LOGO);
            }

#ifdef INBV_ROTBAR_IMPLEMENTED
            /* Server product, use appropriate status bar color */
            Bar = InbvGetResourceAddress(IDB_BAR_DEFAULT);
#endif
        }
#else
        /* Use default status bar */
        Bar = InbvGetResourceAddress(IDB_BAR_WKSTA);
#endif

        /* Make sure we have a logo */
        if (BootLogo)
        {
            /* Save the main image palette for implementing the fade-in effect */
            PBITMAPINFOHEADER BitmapInfoHeader = BootLogo;
            LPRGBQUAD Palette = (LPRGBQUAD)((PUCHAR)BootLogo + BitmapInfoHeader->biSize);
            RtlCopyMemory(MainPalette, Palette, sizeof(MainPalette));

            /* Draw the logo at the center of the screen */
            InbvBitBltAligned(BootLogo,
                          TRUE,
                          VID_BOOTLOGO_HALIGNMENT,
                          VID_BOOTLOGO_VALIGNMENT,
                          VID_BOOTLOGO_MARGIN_LEFT,
                          VID_BOOTLOGO_MARGIN_TOP,
                          VID_BOOTLOGO_MARGIN_RIGHT,
                          VID_BOOTLOGO_MARGIN_BOTTOM);

#ifdef INBV_ROTBAR_IMPLEMENTED
            /* Choose progress bar */
            TempRotBarSelection = ROT_BAR_DEFAULT_MODE;
#endif

            /* Configure progress bar */
            InbvSetProgressBarSettings(VID_PROGRESS_BAR_LEFT,
                                       VID_PROGRESS_BAR_TOP,
                                       VID_PROGRESS_BAR_WIDTH,
                                       VID_PROGRESS_BAR_HEIGHT,
                                       VID_PROGRESS_BAR_FRGROUND);
            InbvSetProgressBarBackground(TRUE, VID_PROGRESS_BAR_BKGROUND);
            InbvSetProgressBarRounding(TRUE);

            /* Update progress bar on screen to show a background */
            InbvUpdateProgressBar(0);

#ifdef REACTOS_SKUS
            /* Check for non-workstation products */
            if (SharedUserData->NtProductType != NtProductWinNt)
            {
                /* Overwrite part of the logo for a server product */
                InbvScreenToBufferBlt(Buffer, VID_SKU_SAVE_AREA_LEFT,
                                      VID_SKU_SAVE_AREA_TOP, 7, 7, 8);
                InbvSolidColorFill(VID_SKU_AREA_LEFT, VID_SKU_AREA_TOP,
                                   VID_SKU_AREA_RIGHT, VID_SKU_AREA_BOTTOM, BV_COLOR_BLACK);
                InbvBufferToScreenBlt(Buffer, VID_SKU_SAVE_AREA_LEFT,
                                      VID_SKU_SAVE_AREA_TOP, 7, 7, 8);

                /* In setup mode, you haven't selected a SKU yet */
                if (ExpInTextModeSetup) Text = NULL;
            }
#endif
        }

        /* Load and draw copyright text bitmap */
        BootCopy = InbvGetResourceAddress(IDB_COPYRIGHT);
        InbvBitBltAligned(BootCopy,
                      TRUE,
                      VID_BOOTCOPY_HALIGNMENT,
                      VID_BOOTCOPY_VALIGNMENT,
                      VID_BOOTCOPY_MARGIN_LEFT,
                      VID_BOOTCOPY_MARGIN_TOP,
                      VID_BOOTCOPY_MARGIN_RIGHT,
                      VID_BOOTCOPY_MARGIN_BOTTOM);

#ifdef REACTOS_SKUS
        /* Draw the SKU text if it exits */
        if (Text)
            InbvBitBltPalette(Text, TRUE, VID_SKU_TEXT_LEFT, VID_SKU_TEXT_TOP);
#endif

#ifdef INBV_ROTBAR_IMPLEMENTED
        if ((TempRotBarSelection == RB_SQUARE_CELLS) && Bar)
        {
            /* Save previous screen pixels to buffer */
            InbvScreenToBufferBlt(Buffer, 0, 0, 22, 9, 24);
            /* Draw the progress bar bit */
            InbvBitBltPalette(Bar, TRUE, 0, 0);
            /* Store it in global buffer */
            InbvScreenToBufferBlt(RotBarBuffer, 0, 0, 22, 9, 24);
            /* Restore screen pixels */
            InbvBufferToScreenBlt(Buffer, 0, 0, 22, 9, 24);
        }

        /*
         * Add a rotating bottom horizontal bar when using a progress bar,
         * to show that ReactOS can be still alive when the bar does not
         * appear to progress.
         */
        if (TempRotBarSelection == RB_PROGRESS_BAR)
        {
            LineBmp = InbvGetResourceAddress(IDB_ROTATING_LINE);
            if (LineBmp)
            {
                /* Draw the line and store it in global buffer */
                InbvBitBltPalette(LineBmp, TRUE, 0, SCREEN_HEIGHT-VID_ROTBAR_HEIGHT);
                InbvScreenToBufferBlt(RotLineBuffer, 0, SCREEN_HEIGHT-VID_ROTBAR_HEIGHT, SCREEN_WIDTH, VID_ROTBAR_HEIGHT, SCREEN_WIDTH);
            }
        }
        else
        {
            /* Hide the simple progress bar if not used */
            ShowProgressBar = FALSE;
        }
#endif

        /* Restore the kernel resource section protection to be read-only */
        MmChangeKernelResourceSectionProtection(MM_READONLY);

        /* Display the boot logo and fade it in */
        BootLogoFadeIn();

#ifdef INBV_ROTBAR_IMPLEMENTED
        if (!RotBarThreadActive && TempRotBarSelection != RB_UNSPECIFIED)
        {
            /* Start the animation thread */
            Status = PsCreateSystemThread(&ThreadHandle,
                                          0,
                                          NULL,
                                          NULL,
                                          NULL,
                                          InbvRotationThread,
                                          NULL);
            if (NT_SUCCESS(Status))
            {
                /* The thread has started, close the handle as we don't need it */
                RotBarThreadActive = TRUE;
                ObCloseHandle(ThreadHandle, KernelMode);
            }
        }
#endif

        /* Set filter which will draw text display if needed */
        InbvInstallDisplayStringFilter(DisplayFilter);
    }

#ifdef INBV_ROTBAR_IMPLEMENTED
    /* Do we have the animation thread? */
    if (RotBarThreadActive)
    {
        /* We do, initialize the progress bar */
        InbvAcquireLock();
        RotBarSelection = TempRotBarSelection;
        InbvRotBarInit();
        InbvReleaseLock();
    }
#endif
}

CODE_SEG("INIT")
VOID
NTAPI
DisplayFilter(PCHAR *String)
{
    /* Windows hack to skip first dots */
    static BOOLEAN DotHack = TRUE;

    /* If "." is given set *String to empty string */
    if (DotHack && strcmp(*String, ".") == 0)
        *String = "";

    if (**String)
    {
        /* Remove the filter */
        InbvInstallDisplayStringFilter(NULL);

        DotHack = FALSE;

        /* Draw text screen */
        DisplayBootBitmap(TRUE);
    }
}

CODE_SEG("INIT")
VOID
NTAPI
FinalizeBootLogo(VOID)
{
    /* Reset progress bar */
#ifdef INBV_ROTBAR_IMPLEMENTED
    PltRotBarStatus = RBS_STOP_ANIMATE;
    RotBarThreadActive = FALSE;
#endif
}
