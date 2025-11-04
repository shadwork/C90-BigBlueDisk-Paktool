
/*
 * ========================================================================
 * AI 
 * ========================================================================
 */

#include "cga_win.h"
#include <windows.h>
#include <process.h>    /* For _beginthreadex */
#include <string.h>     /* For memset */
#include <stdio.h>      /* For printf (in WM_KEYDOWN) */

/*
 * ========================================================================
 * DEFINES AND CONSTANTS
 * ========================================================================
 */
#define ID_TIMER_REPAINT 1
#define CGA_WIDTH 320
#define CGA_HEIGHT 200
#define CGA_BYTES_PER_LINE 80
#define PIXEL_SCALE 2
#define BORDER_SIZE 32
#define SCREEN_WIDTH ((CGA_WIDTH * PIXEL_SCALE) + (BORDER_SIZE * 2))
#define SCREEN_HEIGHT ((CGA_HEIGHT * PIXEL_SCALE) + (BORDER_SIZE * 2))

/* CGA memory map details */
#define CGA_TOTAL_MEMORY_SIZE 16384 /* Total 16KB buffer (16 * 1024) */
#define CGA_BANK_DATA_SIZE 8000     /* Size of one bank's pixel data */
#define CGA_BANK1_OFFSET 8192       /* Offset to Bank 1 (B800:2000) */


/*
 * ========================================================================
 * PUBLIC GLOBALS
 * ========================================================================
 */
unsigned char *memory;
unsigned char color;
int key;

/*
 * ========================================================================
 * PRIVATE (STATIC) GLOBALS
 * ========================================================================
 */

/* Full 16-color CGA palette lookup table */
static COLORREF g_cga16ColorPalette[16] = {
    RGB(0, 0, 0),       /* 0: Black */
    RGB(0, 0, 170),     /* 1: Blue */
    RGB(0, 170, 0),     /* 2: Green */
    RGB(0, 170, 170),   /* 3: Cyan */
    RGB(170, 0, 0),     /* 4: Red */
    RGB(170, 0, 170),   /* 5: Magenta */
    RGB(170, 85, 0),    /* 6: Brown */
    RGB(170, 170, 170), /* 7: Light Gray */
    RGB(85, 85, 85),    /* 8: Dark Gray */
    RGB(85, 85, 255),   /* 9: Bright Blue */
    RGB(85, 255, 85),   /* 10: Bright Green */
    RGB(85, 255, 255),  /* 11: Bright Cyan */
    RGB(255, 85, 85),   /* 12: Bright Red */
    RGB(255, 85, 255),  /* 13: Bright Magenta */
    RGB(255, 255, 85),  /* 14: Yellow */
    RGB(255, 255, 255)  /* 15: White */
};

/* GDI objects for double buffering */
static HBITMAP g_hOffscreenBitmap = NULL;
static HDC     g_hOffscreenDC     = NULL;

/* GDI brushes for all 16 CGA colors */
static HBRUSH  g_hCgaBrushesAll[16] = {NULL};

/* Array holding the 4 active palette indexes (0=BG, 1,2,3=FG) */
static int     g_hCgaColorId[4];


/*
 * ========================================================================
 * SCANCODE HELPER TABLES (Set 2 to Set 1)
 * ========================================================================
 */
static const unsigned char set2_to_set1_normal[133] = {
    /* 0x00 */ 0x00, /* 0x01 */ 0x01, /* 0x02 */ 0x02, /* 0x03 */ 0x03,
    /* 0x04 */ 0x04, /* 0x05 */ 0x05, /* 0x06 */ 0x06, /* 0x07 */ 0x07,
    /* 0x08 */ 0x08, /* 0x09 */ 0x09, /* 0x0A */ 0x0A, /* 0x0B */ 0x0B,
    /* 0x0C */ 0x0C, /* 0x0D */ 0x0D, /* 0x0E */ 0x0E, /* 0x0F */ 0x0F,
    /* 0x10 */ 0x10, /* 0x11 */ 0x11, /* 0x12 */ 0x12, /* 0x13 */ 0x13,
    /* 0x14 */ 0x14, /* 0x15 */ 0x15, /* 0x16 */ 0x16, /* 0x17 */ 0x17,
    /* 0x18 */ 0x18, /* 0x19 */ 0x19, /* 0x1A */ 0x1A, /* 0x1B */ 0x1B,
    /* 0x1C */ 0x1C, /* 0x1D */ 0x1D, /* 0x1E */ 0x1E, /* 0x1F */ 0x1F,
    /* 0x20 */ 0x20, /* 0x21 */ 0x21, /* 0x22 */ 0x22, /* 0x23 */ 0x23,
    /* 0x24 */ 0x24, /* 0x25 */ 0x25, /* 0x26 */ 0x26, /* 0x27 */ 0x27,
    /* 0x28 */ 0x28, /* 0x29 */ 0x29, /* 0x2A */ 0x2A, /* 0x2B */ 0x2B,
    /* 0x2C */ 0x2C, /* 0x2D */ 0x2D, /* 0x2E */ 0x2E, /* 0x2F */ 0x2F,
    /* 0x30 */ 0x30, /* 0x31 */ 0x31, /* 0x32 */ 0x32, /* 0x33 */ 0x33,
    /* 0x34 */ 0x34, /* 0x35 */ 0x35, /* 0x36 */ 0x2A, /* 0x37 */ 0x37,
    /* 0x38 */ 0x38, /* 0x39 */ 0x39, /* 0x3A */ 0x3A, /* 0x3B */ 0x3B,
    /* 0x3C */ 0x3C, /* 0x3D */ 0x3D, /* 0x3E */ 0x3E, /* 0x3F */ 0x3F,
    /* 0x40 */ 0x40, /* 0x41 */ 0x41, /* 0x42 */ 0x42, /* 0x43 */ 0x43,
    /* 0x44 */ 0x44, /* 0x45 */ 0x45, /* 0x46 */ 0x46, /* 0x47 */ 0x47,
    /* 0x48 */ 0x48, /* 0x49 */ 0x49, /* 0x4A */ 0x4A, /* 0x4B */ 0x4B,
    /* 0x4C */ 0x4C, /* 0x4D */ 0x4D, /* 0x4E */ 0x4E, /* 0x4F */ 0x4F,
    /* 0x50 */ 0x50, /* 0x51 */ 0x51, /* 0x52 */ 0x52, /* 0x53 */ 0x53,
    /* 0x54 */ 0x54, /* 0x55 */ 0x00, /* 0x56 */ 0x56, /* 0x57 */ 0x57,
    /* 0x58 */ 0x58, /* 0x59 */ 0x00, /* 0x5A */ 0x00, /* 0x5B */ 0x00,
    /* 0x5C */ 0x00, /* 0x5D */ 0x00, /* 0x5E */ 0x00, /* 0x5F */ 0x00,
    /* 0x60 */ 0x00, /* 0x61 */ 0x00, /* 0x62 */ 0x00, /* 0x63 */ 0x00,
    /* 0x64 */ 0x00, /* 0x65 */ 0x00, /* 0x66 */ 0x00, /* 0x67 */ 0x00,
    /* 0x68 */ 0x00, /* 0x69 */ 0x00, /* 0x6A */ 0x00, /* 0x6B */ 0x00,
    /* 0x6C */ 0x00, /* 0x6D */ 0x00, /* 0x6E */ 0x00, /* 0x6F */ 0x00,
    /* 0x70 */ 0x00, /* 0x71 */ 0x00, /* 0x72 */ 0x00, /* 0x73 */ 0x00,
    /* 0x74 */ 0x00, /* 0x75 */ 0x00, /* 0x76 */ 0x1C, /* 0x77 */ 0x00,
    /* 0x78 */ 0x00, /* 0x79 */ 0x00, /* 0x7A */ 0x00, /* 0x7B */ 0x00,
    /* 0x7C */ 0x37, /* 0x7D */ 0x00, /* 0x7E */ 0x00, /* 0x7F */ 0x00,
    /* 0x80 */ 0x00, /* 0x81 */ 0x00, /* 0x82 */ 0x00, /* 0x83 */ 0x85,
    /* 0x84 */ 0x86
};

static const unsigned char set2_to_set1_extended[84] = {
    /* 0x00 */ 0x00, /* 0x01 */ 0x00, /* 0x02 */ 0x00, /* 0x03 */ 0x00,
    /* 0x04 */ 0x00, /* 0x05 */ 0x00, /* 0x06 */ 0x00, /* 0x07 */ 0x00,
    /* 0x08 */ 0x00, /* 0x09 */ 0x00, /* 0x0A */ 0x00, /* 0x0B */ 0x00,
    /* 0x0C */ 0x00, /* 0x0D */ 0x00, /* 0x0E */ 0x00, /* 0x0F */ 0x00,
    /* 0x10 */ 0x00, /* 0x11 */ 0x38, /* 0x12 */ 0x00, /* 0x13 */ 0x00,
    /* 0x14 */ 0x1D, /* 0x15 */ 0x00, /* 0x16 */ 0x00, /* 0x17 */ 0x00,
    /* 0x18 */ 0x00, /* 0x19 */ 0x00, /* 0x1A */ 0x00, /* 0x1B */ 0x00,
    /* 0x1C */ 0x1C, /* 0x1D */ 0x1D, /* 0x1E */ 0x00, /* 0x1F */ 0x00,
    /* 0x20 */ 0x00, /* 0x21 */ 0x00, /* 0x22 */ 0x00, /* 0x23 */ 0x00,
    /* 0x24 */ 0x00, /* 0x25 */ 0x00, /* 0x26 */ 0x00, /* 0x27 */ 0x00,
    /* 0x28 */ 0x00, /* 0x29 */ 0x00, /* 0x2A */ 0x00, /* 0x2B */ 0x00,
    /* 0x2C */ 0x00, /* 0x2D */ 0x00, /* 0x2E */ 0x00, /* 0x2F */ 0x00,
    /* 0x30 */ 0x00, /* 0x31 */ 0x00, /* 0x32 */ 0x00, /* 0x33 */ 0x00,
    /* 0x34 */ 0x00, /* 0x35 */ 0x35, /* 0x36 */ 0x00, /* 0x37 */ 0x37,
    /* 0x38 */ 0x38, /* 0x39 */ 0x00, /* 0x3A */ 0x00, /* 0x3B */ 0x00,
    /* 0x3C */ 0x00, /* 0x3D */ 0x00, /* 0x3E */ 0x00, /* 0x3F */ 0x00,
    /* 0x40 */ 0x00, /* 0x41 */ 0x00, /* 0x42 */ 0x00, /* 0x43 */ 0x00,
    /* 0x44 */ 0x00, /* 0x45 */ 0x00, /* 0x46 */ 0x00, /* 0x47 */ 0x47,
    /* 0x48 */ 0x48, /* 0x49 */ 0x49, /* 0x4A */ 0x00, /* 0x4B */ 0x4B,
    /* 0x4C */ 0x00, /* 0x4D */ 0x4D, /* 0x4E */ 0x00, /* 0x4F */ 0x4F,
    /* 0x50 */ 0x50, /* 0x51 */ 0x51, /* 0x52 */ 0x52, /* 0x53 */ 0x53
};


/*
 * ========================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ========================================================================
 */

/* GUI Thread */
static unsigned int __stdcall GuiThreadFunc(void* pParam);

/* Window Message Handler */
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Scancode Conversion Helpers */
static unsigned char ConvertScanCodeSet2ToSet1(unsigned char scancode, int isExtended);
static unsigned short GetDosScanCode(WPARAM wParam, LPARAM lParam);


/*
 * ========================================================================
 * PUBLIC API FUNCTIONS
 * ========================================================================
 */

/* Allocates the 16KB CGA memory buffer */
void init_cga(void)
{
    memory = (unsigned char*)malloc(CGA_TOTAL_MEMORY_SIZE);
    if (memory)
    {
        /* Clear the entire buffer on init */
        memset(memory, 0, CGA_TOTAL_MEMORY_SIZE);
    }
}

/* Frees the CGA memory buffer */
void destroy_cga(void)
{
    free(memory);
    memory = NULL;
}

/* Starts the GUI thread to create the CGA window */
void set_mode_cga320(void)
{
    HANDLE hGuiThread;
    unsigned int uGuiThreadId;

    /* Launch the GUI thread */
    hGuiThread = (HANDLE)_beginthreadex(
        NULL, 0, GuiThreadFunc, NULL, 0, &uGuiThreadId
    );

    /*
     * We don't wait for the thread. We close the handle
     * so it doesn't leak and return to main immediately.
     */
    if (hGuiThread)
    {
        CloseHandle(hGuiThread);
    }
}

/* (Stub for text mode) */
void set_mode_text80(void)
{
    /* Not implemented */
}

/* Sets the global color register */
void set_color_reg(unsigned char value)
{
    color = value;
}

/* Clears both 8KB banks of the CGA buffer */
void cls(void)
{
    if (!memory) return;
    /* Clear Bank 0 (even scanlines) */
    memset(memory, 0, CGA_BANK_DATA_SIZE);
    /* Clear Bank 1 (odd scanlines) */
    memset(memory + CGA_BANK1_OFFSET, 0, CGA_BANK_DATA_SIZE);
}

/* Fills both 8KB banks of the CGA buffer with a value */
void fill(int fill_byte)
{
    if (!memory) return;
    /* Fill Bank 0 (even scanlines) */
    memset(memory, (unsigned char)fill_byte, CGA_BANK_DATA_SIZE);
    /* Fill Bank 1 (odd scanlines) */
    memset(memory + CGA_BANK1_OFFSET, (unsigned char)fill_byte, CGA_BANK_DATA_SIZE);
}


/*
 * ========================================================================
 * PRIVATE (STATIC) HELPER FUNCTIONS
 * ========================================================================
 */

/**
 * Converts a Set 2 scancode to its Set 1 equivalent.
 */
static unsigned char ConvertScanCodeSet2ToSet1(unsigned char scancode, int isExtended)
{
    if (isExtended)
    {
        if (scancode >= (sizeof(set2_to_set1_extended) / sizeof(set2_to_set1_extended[0])))
        {
            return 0x00;
        }
        return set2_to_set1_extended[scancode];
    }
    else
    {
        if (scancode >= (sizeof(set2_to_set1_normal) / sizeof(set2_to_set1_normal[0])))
        {
            return 0x00;
        }
        return set2_to_set1_normal[scancode];
    }
}

/**
 * Converts Windows WM_KEYDOWN params to a 16-bit DOS/BIOS (INT 16h)
 * scancode, where high byte is Set 1 scancode and low byte is ASCII.
 */
static unsigned short GetDosScanCode(WPARAM wParam, LPARAM lParam)
{
    /* C90: Variables declared at top of function */
    UINT scancode_set2;
    int is_extended;
    unsigned char scancode_set1;
    BYTE keyboard_state[256];
    WORD ascii_buffer;
    int result;
    unsigned char ascii_char;

    /* --- Part 1: Get the 8-bit Scancode Set 1 (the HIGH BYTE) --- */

    scancode_set2 = (lParam >> 16) & 0xFF;
    is_extended   = (lParam >> 24) & 0x01;

    scancode_set1 = ConvertScanCodeSet2ToSet1(
        (unsigned char)scancode_set2,
        is_extended
    );

    /* --- Part 2: Get the 8-bit ASCII character (the LOW BYTE) --- */

    /* Get the state of all keys (Shift, Ctrl, Alt, CapsLock) */
    if (!GetKeyboardState(keyboard_state))
    {
        /* Handle error if needed */
    }

    /* Translate the key */
    result = ToAscii(
        (UINT)wParam,      /* Virtual Key */
        (lParam >> 16),    /* Scancode Set 2 */
        keyboard_state,    /* Keyboard state */
        &ascii_buffer,     /* Output buffer */
        0                  /* Flags */
    );

    if (result == 1)
    {
        /* We got a character */
        ascii_char = (unsigned char)(ascii_buffer & 0xFF);
    }
    else
    {
        /* No character (e.g., F1, Home, Alt+A) */
        ascii_char = 0x00;
    }

    /* --- Part 3: Combine and return the 16-bit INT 16 value --- */

    return (scancode_set1 << 8) | ascii_char;
}


/*
 * ========================================================================
 * GUI THREAD AND WINDOW PROCEDURE
 * ========================================================================
 */

/**
 * This is the entry point for the new GUI thread.
 * It creates the window and runs the message loop.
 */
static unsigned int __stdcall GuiThreadFunc(void* pParam)
{
    /* C90: Variables declared at top of function */
    HINSTANCE hInstance;
    const char* CLASS_NAME = "MyCgaWindowClass";
    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;
    RECT wr;
    DWORD dwStyle;
    int windowWidth, windowHeight;
    UINT_PTR timerResult;

    hInstance = GetModuleHandle(NULL);

    memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; /* We paint the background ourselves */
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        return 1; /* Failed to register class */
    }

    /* Calculate window size needed for the desired client area size */
    wr.left = 0;
    wr.top = 0;
    wr.right = SCREEN_WIDTH;
    wr.bottom = SCREEN_HEIGHT;
    dwStyle = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&wr, dwStyle, FALSE);
    windowWidth = wr.right - wr.left;
    windowHeight = wr.bottom - wr.top;

    hwnd = CreateWindowEx(
        0, CLASS_NAME, "CGA Renderer",
        dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight, NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL)
    {
        return 1; /* Failed to create window */
    }

    /* Set a 50ms timer (20 FPS) to trigger repaints */
    timerResult = SetTimer(hwnd, ID_TIMER_REPAINT, 50, NULL);
    if (timerResult == 0)
    {
        return 1; /* Failed to create timer */
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    /* Main message loop for the GUI thread */
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}


/**
 * The Window Procedure (Message Handler) for the GUI thread.
 */
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /* C90: Variables declared at top of function */
    PAINTSTRUCT ps;
    HDC hdc;
    int y, x, i;
    RECT r;
    int is_odd, scanline_index, bank_offset, line_offset;
    int byte_index, bit_shift;
    unsigned char pixel_byte;
    int palette_index;
    int newPaletteState;
    int intensityOffset;
    unsigned short dos_code;

    switch (uMsg)
    {
        case WM_CREATE:
            /* Create GDI brushes for all 16 colors */
            for (i = 0; i < 16; i++)
            {
                g_hCgaBrushesAll[i] = CreateSolidBrush(g_cga16ColorPalette[i]);
            }
            break;

        case WM_TIMER:
            /* Timer just triggers a repaint. */
            if (wParam == ID_TIMER_REPAINT)
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case WM_KEYDOWN:
            dos_code = GetDosScanCode(wParam, lParam);
            /* Uncomment to debug scancodes: */
            /* printf("SCANCODE Key: %c (0x%x)\n", (char)dos_code, dos_code); */
            key = dos_code;
            break;

        case WM_KEYUP:
            key = 0;
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);

            /* Create the offscreen buffer on first paint */
            if (g_hOffscreenDC == NULL)
            {
                g_hOffscreenDC = CreateCompatibleDC(hdc);
                g_hOffscreenBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
                SelectObject(g_hOffscreenDC, g_hOffscreenBitmap);
            }

            /*
             * --- Calculate Active Palette ---
             * This logic uses the 'color' global, which is set by set_color_reg().
             * It uses bits 4 and 5 for palette selection.
             */

            /* Get palette state from bits 4 (Intensity) AND 5 (Palette) */
            newPaletteState = (color & 0x20); /* e.g., 0x00, 0x10, 0x20, or 0x30 */

            /* Brush 0 is always the border/background color (bits 0-3) */
            g_hCgaColorId[0] = color & 0x0F;

            /* Check bit 4 (0x10) for intensity. */
            intensityOffset = color & 0x8 ? 8 : 0; /* 0 or +8 */

            /* Check bit 5 (0x20) for palette select */
            if ((newPaletteState & 0x20) == 0) /* Palette 0 */
            {
                /* Base: Green[2], Red[4], Brown[6] */
                g_hCgaColorId[1] = 2 + intensityOffset;
                g_hCgaColorId[2] = 4 + intensityOffset;
                g_hCgaColorId[3] = 6 + intensityOffset;
            }
            else /* Palette 1 */
            {
                /* Base: Cyan[3], Magenta[5], Light Gray[7] */
                g_hCgaColorId[1] = 3 + intensityOffset;
                g_hCgaColorId[2] = 5 + intensityOffset;
                g_hCgaColorId[3] = 7 + intensityOffset;
            }

            /*
             * --- Render to Offscreen Buffer ---
             */

            /* 1. Render the border (fill all with Brush 0) */
            r.left = 0;
            r.top = 0;
            r.right = SCREEN_WIDTH;
            r.bottom = SCREEN_HEIGHT;
            FillRect(g_hOffscreenDC, &r, g_hCgaBrushesAll[g_hCgaColorId[0]]);

            /* 2. Render the CGA pixel buffer from 'memory' */
            if (memory != NULL)
            {
                for (y = 0; y < CGA_HEIGHT; y++)
                {
                    /* Find the correct scanline in the correct bank */
                    is_odd = y % 2;
                    scanline_index = y / 2;
                    bank_offset = is_odd ? CGA_BANK1_OFFSET : 0;
                    line_offset = bank_offset + (scanline_index * CGA_BYTES_PER_LINE);

                    for (x = 0; x < CGA_WIDTH; x++)
                    {
                        /* Find the byte */
                        byte_index = x / 4;

                        /* This check should not fail if logic is correct */
                        if ((line_offset + byte_index) >= CGA_TOTAL_MEMORY_SIZE)
                        {
                            continue;
                        }

                        /* Extract the 2-bit pixel from the byte */
                        pixel_byte = memory[line_offset + byte_index];
                        bit_shift = (3 - (x % 4)) * 2; /* 6, 4, 2, or 0 */
                        palette_index = (pixel_byte >> bit_shift) & 0x03;

                        /* Calculate draw rectangle */
                        r.left = (x * PIXEL_SCALE) + BORDER_SIZE;
                        r.top = (y * PIXEL_SCALE) + BORDER_SIZE;
                        r.right = r.left + PIXEL_SCALE;
                        r.bottom = r.top + PIXEL_SCALE;

                        /* Draw the scaled pixel */
                        FillRect(g_hOffscreenDC, &r, g_hCgaBrushesAll[g_hCgaColorId[palette_index]]);
                    }
                }
            }

            /* 3. Blit the final image from the offscreen buffer to the screen */
            BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                   g_hOffscreenDC, 0, 0, SRCCOPY);

            EndPaint(hwnd, &ps);
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            /* Clean up all GDI resources */
            KillTimer(hwnd, ID_TIMER_REPAINT);
            if (g_hOffscreenDC)     { DeleteDC(g_hOffscreenDC); }
            if (g_hOffscreenBitmap) { DeleteObject(g_hOffscreenBitmap); }
            for (i = 0; i < 16; i++)
            {
                if (g_hCgaBrushesAll[i]) { DeleteObject(g_hCgaBrushesAll[i]); }
            }
            PostQuitMessage(0); /* Kills the GUI thread */
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}