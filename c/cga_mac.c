/*
 * ========================================================================
 * AI 
 * ========================================================================
 */

/* ============================================================================
   FILE: cga_mac.m
   ============================================================================
   CGA Renderer for macOS - C90 compliant
   Compile: gcc -std=c90 -framework Cocoa -o program cga_mac.m bios_mac.c main.c
   ============================================================================ */

#import <Cocoa/Cocoa.h>
#import <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "cga_mac.h"

#define CGA_WIDTH 320
#define CGA_HEIGHT 200
#define CGA_BYTES_PER_LINE 80
#define PIXEL_SCALE 2
#define BORDER_SIZE 32
#define SCREEN_WIDTH ((CGA_WIDTH * PIXEL_SCALE) + (BORDER_SIZE * 2))
#define SCREEN_HEIGHT ((CGA_HEIGHT * PIXEL_SCALE) + (BORDER_SIZE * 2))
#define CGA_TOTAL_MEMORY_SIZE 16384
#define CGA_BANK_DATA_SIZE 8000
#define CGA_BANK1_OFFSET 8192

/* Public globals */
unsigned char *memory = NULL;
unsigned char color = 0;
int key = 0;

/* CGA Palette - declared static to avoid multiple definition */
static NSColor *g_cga16ColorPalette[16];
static int palette_initialized = 0;

/* Forward declarations */
static void initialize_palette(void);
static unsigned char mac_keycode_to_dos_scancode(unsigned short keyCode);

/* Initialize CGA color palette */
static void initialize_palette(void) {
    if (palette_initialized) return;
    
    g_cga16ColorPalette[0] = [NSColor colorWithRed:0.0/255.0 green:0.0/255.0 blue:0.0/255.0 alpha:1.0];
    g_cga16ColorPalette[1] = [NSColor colorWithRed:0.0/255.0 green:0.0/255.0 blue:170.0/255.0 alpha:1.0];
    g_cga16ColorPalette[2] = [NSColor colorWithRed:0.0/255.0 green:170.0/255.0 blue:0.0/255.0 alpha:1.0];
    g_cga16ColorPalette[3] = [NSColor colorWithRed:0.0/255.0 green:170.0/255.0 blue:170.0/255.0 alpha:1.0];
    g_cga16ColorPalette[4] = [NSColor colorWithRed:170.0/255.0 green:0.0/255.0 blue:0.0/255.0 alpha:1.0];
    g_cga16ColorPalette[5] = [NSColor colorWithRed:170.0/255.0 green:0.0/255.0 blue:170.0/255.0 alpha:1.0];
    g_cga16ColorPalette[6] = [NSColor colorWithRed:170.0/255.0 green:85.0/255.0 blue:0.0/255.0 alpha:1.0];
    g_cga16ColorPalette[7] = [NSColor colorWithRed:170.0/255.0 green:170.0/255.0 blue:170.0/255.0 alpha:1.0];
    g_cga16ColorPalette[8] = [NSColor colorWithRed:85.0/255.0 green:85.0/255.0 blue:85.0/255.0 alpha:1.0];
    g_cga16ColorPalette[9] = [NSColor colorWithRed:85.0/255.0 green:85.0/255.0 blue:255.0/255.0 alpha:1.0];
    g_cga16ColorPalette[10] = [NSColor colorWithRed:85.0/255.0 green:255.0/255.0 blue:85.0/255.0 alpha:1.0];
    g_cga16ColorPalette[11] = [NSColor colorWithRed:85.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1.0];
    g_cga16ColorPalette[12] = [NSColor colorWithRed:255.0/255.0 green:85.0/255.0 blue:85.0/255.0 alpha:1.0];
    g_cga16ColorPalette[13] = [NSColor colorWithRed:255.0/255.0 green:85.0/255.0 blue:255.0/255.0 alpha:1.0];
    g_cga16ColorPalette[14] = [NSColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:85.0/255.0 alpha:1.0];
    g_cga16ColorPalette[15] = [NSColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1.0];
    
    palette_initialized = 1;
}

/* Convert macOS keycode to DOS scancode */
static unsigned char mac_keycode_to_dos_scancode(unsigned short keyCode) {
    switch (keyCode) {
        case 0x00: return 0x1E; /* A */
        case 0x0B: return 0x30; /* B */
        case 0x08: return 0x2E; /* C */
        case 0x02: return 0x20; /* D */
        case 0x0E: return 0x12; /* E */
        case 0x03: return 0x21; /* F */
        case 0x05: return 0x22; /* G */
        case 0x04: return 0x23; /* H */
        case 0x22: return 0x17; /* I */
        case 0x26: return 0x24; /* J */
        case 0x28: return 0x25; /* K */
        case 0x25: return 0x26; /* L */
        case 0x2E: return 0x32; /* M */
        case 0x2D: return 0x31; /* N */
        case 0x1F: return 0x18; /* O */
        case 0x23: return 0x19; /* P */
        case 0x0C: return 0x10; /* Q */
        case 0x0F: return 0x13; /* R */
        case 0x01: return 0x1F; /* S */
        case 0x11: return 0x14; /* T */
        case 0x20: return 0x16; /* U */
        case 0x09: return 0x2F; /* V */
        case 0x0D: return 0x11; /* W */
        case 0x07: return 0x2D; /* X */
        case 0x10: return 0x15; /* Y */
        case 0x06: return 0x2C; /* Z */
        case 0x12: return 0x02; /* 1 */
        case 0x13: return 0x03; /* 2 */
        case 0x14: return 0x04; /* 3 */
        case 0x15: return 0x05; /* 4 */
        case 0x17: return 0x06; /* 5 */
        case 0x16: return 0x07; /* 6 */
        case 0x1A: return 0x08; /* 7 */
        case 0x1C: return 0x09; /* 8 */
        case 0x19: return 0x0A; /* 9 */
        case 0x1D: return 0x0B; /* 0 */
        case 0x1B: return 0x0C; /* - */
        case 0x18: return 0x0D; /* = */
        case 0x21: return 0x1A; /* [ */
        case 0x1E: return 0x1B; /* ] */
        case 0x2A: return 0x2B; /* \ */
        case 0x29: return 0x27; /* ; */
        case 0x27: return 0x28; /* ' */
        case 0x32: return 0x29; /* ` */
        case 0x2B: return 0x33; /* , */
        case 0x2F: return 0x34; /* . */
        case 0x2C: return 0x35; /* / */
        case 0x31: return 0x39; /* Space */
        case 0x24: return 0x1C; /* Return */
        case 0x33: return 0x0E; /* Delete/Backspace */
        case 0x35: return 0x01; /* Escape */
        case 0x30: return 0x0F; /* Tab */
        case 0x7E: return 0x48; /* Up Arrow */
        case 0x7D: return 0x50; /* Down Arrow */
        case 0x7B: return 0x4B; /* Left Arrow */
        case 0x7C: return 0x4D; /* Right Arrow */
        case 0x73: return 0x47; /* Home */
        case 0x77: return 0x4F; /* End */
        case 0x74: return 0x49; /* Page Up */
        case 0x79: return 0x51; /* Page Down */
        case 0x7A: return 0x3B; /* F1 */
        case 0x78: return 0x3C; /* F2 */
        case 0x63: return 0x3D; /* F3 */
        case 0x76: return 0x3E; /* F4 */
        case 0x60: return 0x3F; /* F5 */
        case 0x61: return 0x40; /* F6 */
        case 0x62: return 0x41; /* F7 */
        case 0x64: return 0x42; /* F8 */
        case 0x65: return 0x43; /* F9 */
        case 0x6D: return 0x44; /* F10 */
        default: return 0x00;
    }
}

/* Custom View for Rendering */
@interface CGAView : NSView
{
    NSTimer *timer;
    int cgaColorId[4];
}
@end

@implementation CGAView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        initialize_palette();
        
        /* Start timer for 20 FPS */
        timer = [NSTimer scheduledTimerWithTimeInterval:0.05
                                                 target:self
                                               selector:@selector(timerFired:)
                                               userInfo:nil
                                                repeats:YES];
        [timer retain];
    }
    return self;
}

- (void)dealloc {
    [timer invalidate];
    [timer release];
    [super dealloc];
}

- (void)timerFired:(NSTimer *)t {
    (void)t; /* Unused parameter */
    [self setNeedsDisplay:YES];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    unsigned short scancode;
    NSString *chars;
    unsigned char ascii;
    unsigned char dos_scancode;
    unichar ch;
    
    scancode = [event keyCode];
    chars = [event charactersIgnoringModifiers];
    ascii = 0;
    
    if ([chars length] > 0) {
        ch = [chars characterAtIndex:0];
        if (ch < 256) {
            ascii = (unsigned char)ch;
        }
    }
    
    dos_scancode = mac_keycode_to_dos_scancode(scancode);
    key = (dos_scancode << 8) | ascii;
}

- (void)keyUp:(NSEvent *)event {
    (void)event; /* Unused parameter */
    key = 0;
}

- (void)drawRect:(NSRect)dirtyRect {
    int intensityOffset;
    int newPaletteState;
    int y, x;
    int is_odd, scanline_index, bank_offset, line_offset;
    int byte_index, bit_shift, palette_index;
    unsigned char pixel_byte;
    NSRect pixelRect;
    
    (void)dirtyRect; /* Unused parameter */
    
    /* Calculate Active Palette from color register */
    cgaColorId[0] = color & 0x0F;
    intensityOffset = (color & 0x08) ? 8 : 0;
    newPaletteState = (color & 0x20);
    
    if ((newPaletteState & 0x20) == 0) {
        /* Palette 0: Green, Red, Brown */
        cgaColorId[1] = 2 + intensityOffset;
        cgaColorId[2] = 4 + intensityOffset;
        cgaColorId[3] = 6 + intensityOffset;
    } else {
        /* Palette 1: Cyan, Magenta, Light Gray */
        cgaColorId[1] = 3 + intensityOffset;
        cgaColorId[2] = 5 + intensityOffset;
        cgaColorId[3] = 7 + intensityOffset;
    }
    
    /* Fill entire view with background color (border) */
    [g_cga16ColorPalette[cgaColorId[0]] setFill];
    NSRectFill([self bounds]);
    
    /* Render CGA pixel buffer */
    if (memory != NULL) {
        for (y = 0; y < CGA_HEIGHT; y++) {
            /* Calculate bank and offset (interleaved scanlines) */
            is_odd = y % 2;
            scanline_index = y / 2;
            bank_offset = is_odd ? CGA_BANK1_OFFSET : 0;
            line_offset = bank_offset + (scanline_index * CGA_BYTES_PER_LINE);
            
            for (x = 0; x < CGA_WIDTH; x++) {
                byte_index = x / 4;
                
                if ((line_offset + byte_index) >= CGA_TOTAL_MEMORY_SIZE) {
                    continue;
                }
                
                /* Extract 2-bit pixel value */
                pixel_byte = memory[line_offset + byte_index];
                bit_shift = (3 - (x % 4)) * 2;
                palette_index = (pixel_byte >> bit_shift) & 0x03;
                
                /* Calculate pixel rectangle (flip Y for macOS coordinates) */
                pixelRect = NSMakeRect(
                    (x * PIXEL_SCALE) + BORDER_SIZE,
                    SCREEN_HEIGHT - ((y * PIXEL_SCALE) + BORDER_SIZE + PIXEL_SCALE),
                    PIXEL_SCALE,
                    PIXEL_SCALE
                );
                
                /* Draw the pixel */
                [g_cga16ColorPalette[cgaColorId[palette_index]] setFill];
                NSRectFill(pixelRect);
            }
        }
    }
}

@end

/* Window Controller */
@interface CGAWindowController : NSWindowController <NSWindowDelegate>
@end

@implementation CGAWindowController

- (void)windowWillClose:(NSNotification *)notification {
    (void)notification; /* Unused parameter */
    [NSApp terminate:nil];
}

@end

/* GUI Thread Function */
static void* gui_thread_func(void *arg) {
    NSAutoreleasePool *pool;
    NSRect windowRect;
    NSWindow *window;
    CGAView *cgaView;
    CGAWindowController *controller;
    
    (void)arg; /* Unused parameter */
    
    pool = [[NSAutoreleasePool alloc] init];
    
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    windowRect = NSMakeRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    window = [[NSWindow alloc] 
        initWithContentRect:windowRect
        styleMask:(NSTitledWindowMask | 
                  NSClosableWindowMask | 
                  NSMiniaturizableWindowMask)
        backing:NSBackingStoreBuffered
        defer:NO];
    
    [window setTitle:@"CGA Renderer"];
    [window center];
    
    cgaView = [[CGAView alloc] initWithFrame:windowRect];
    [window setContentView:cgaView];
    [cgaView release];
    
    controller = [[CGAWindowController alloc] initWithWindow:window];
    [window setDelegate:controller];
    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:cgaView];
    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
    
    [controller release];
    [window release];
    [pool release];
    
    return NULL;
}

/* Public API Functions */
void init_cga(void) {
    memory = (unsigned char*)malloc(CGA_TOTAL_MEMORY_SIZE);
    if (memory) {
        memset(memory, 0, CGA_TOTAL_MEMORY_SIZE);
    }
}

void destroy_cga(void) {
    if (memory) {
        free(memory);
        memory = NULL;
    }
}

void set_mode_cga320(void) {
    pthread_t thread;
    pthread_create(&thread, NULL, gui_thread_func, NULL);
    pthread_detach(thread);
    
    /* Give the GUI thread time to start */
    usleep(100000); /* 100ms */
}

void set_mode_text80(void) {
    /* Not implemented */
}

void set_color_reg(unsigned char value) {
    color = value;
}

void cls(void) {
    if (!memory) return;
    memset(memory, 0, CGA_BANK_DATA_SIZE);
    memset(memory + CGA_BANK1_OFFSET, 0, CGA_BANK_DATA_SIZE);
}

void fill(int fill_byte) {
    if (!memory) return;
    memset(memory, (unsigned char)fill_byte, CGA_BANK_DATA_SIZE);
    memset(memory + CGA_BANK1_OFFSET, (unsigned char)fill_byte, CGA_BANK_DATA_SIZE);
}