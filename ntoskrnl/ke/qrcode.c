/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/qrcode.c
 * PURPOSE:         QR-Code Generator
 * PROGRAMMERS:     Richard Moore
 *                  Yaroslav Kibysh
 *                  Nikita Ivanov
 */
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Richard Moore
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 *  Special thanks to Nayuki (https://www.nayuki.io/) from which this library was
 *  heavily inspired and compared against.
 *
 *  See: https://github.com/nayuki/QR-Code-generator/tree/master/cpp
 */

#include <qrcode.h>

#include <stdlib.h>
#include <string.h>

#include <ntoskrnl.h>

#if QR_LOCK_VERSION == 0

static const SHORT NUM_ERROR_CORRECTION_CODEWORDS[4][40] = 
{
    // 1,  2,  3,  4,  5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40    Error correction level
    { 10, 16, 26, 36, 48,  64,  72,  88, 110, 130, 150, 176, 198, 216, 240, 280, 308, 338, 364, 416, 442, 476, 504, 560,  588,  644,  700,  728,  784,  812,  868,  924,  980, 1036, 1064, 1120, 1204, 1260, 1316, 1372},  // Medium
    {  7, 10, 15, 20, 26,  36,  40,  48,  60,  72,  80,  96, 104, 120, 132, 144, 168, 180, 196, 224, 224, 252, 270, 300,  312,  336,  360,  390,  420,  450,  480,  510,  540,  570,  570,  600,  630,  660,  720,  750},  // Low
    { 17, 28, 44, 64, 88, 112, 130, 156, 192, 224, 264, 308, 352, 384, 432, 480, 532, 588, 650, 700, 750, 816, 900, 960, 1050, 1110, 1200, 1260, 1350, 1440, 1530, 1620, 1710, 1800, 1890, 1980, 2100, 2220, 2310, 2430},  // High
    { 13, 22, 36, 52, 72,  96, 108, 132, 160, 192, 224, 260, 288, 320, 360, 408, 448, 504, 546, 600, 644, 690, 750, 810,  870,  952, 1020, 1050, 1140, 1200, 1290, 1350, 1440, 1530, 1590, 1680, 1770, 1860, 1950, 2040},  // Quartile
};

static const unsigned char NUM_ERROR_CORRECTION_BLOCKS[4][40] =
 {
    // Version: (note that index 0 is for padding, and is set to an illegal value)
    // 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
    {  1, 1, 1, 2, 2, 4, 4, 4, 5, 5,  5,  8,  9,  9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},  // Medium
    {  1, 1, 1, 1, 1, 2, 2, 2, 2, 4,  4,  4,  4,  4,  6,  6,  6,  6,  7,  8,  8,  9,  9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},  // Low
    {  1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81},  // High
    {  1, 1, 2, 2, 4, 4, 6, 6, 8, 8,  8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},  // Quartile
};

static const SHORT NUM_RAW_DATA_MODULES[40] = 
{
    //  1,   2,   3,   4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
      208, 359, 567, 807, 1079, 1383, 1568, 1936, 2336, 2768, 3232, 3728, 4256, 4651, 5243, 5867, 6523,
    //   18,   19,   20,   21,    22,    23,    24,    25,   26,    27,     28,    29,    30,    31,
       7211, 7931, 8683, 9252, 10068, 10916, 11796, 12708, 13652, 14628, 15371, 16411, 17483, 18587,
    //    32,    33,    34,    35,    36,    37,    38,    39,    40
       19723, 20891, 22091, 23008, 24272, 25568, 26896, 28256, 29648
};

// @TODO: Put other QR_LOCK_VERSIONS here
#elif QR_LOCK_VERSION == 3

static const int16_t NUM_ERROR_CORRECTION_CODEWORDS[4] = 
{
    26, 15, 44, 36
};

static const char NUM_ERROR_CORRECTION_BLOCKS[4] =
{
    1, 1, 2, 2
};

static const SHORT NUM_RAW_DATA_MODULES = 567;

#else

#error Unsupported QR_LOCK_VERSION (add it...)

#endif


static int Max(int a,
               int b) 
{
    if (a > b) 
    {
        return a; 
    }
    
    return b;
}

/*
static int abs(int value) {
    if (value < 0) { return -value; }
    return value;
}
*/


static char getAlphanumeric(char c) 
{
    
    if (c >= '0' && c <= '9')
        return (c - '0');
    
    if (c >= 'A' && c <= 'Z')
        return (c - 'A' + 10);
    
    switch (c) 
    {
        case ' ':
             return 36;
        case '$':
             return 37;
        case '%':
             return 38;
        case '*':
             return 39;
        case '+':
             return 40;
        case '-':
             return 41;
        case '.':
             return 42;
        case '/':
             return 43;
        case ':':
             return 44;
    }
    
    return -1;
}

static BOOLEAN isAlphanumeric(const char *text,
                              SHORT length) 
{
    while (length != 0) 
    {
        if (getAlphanumeric(text[--length]) == -1)
            return FALSE;
    }
    return TRUE;
}


static BOOLEAN isNumeric(const char *text,
                         SHORT length) 
{
    while (length != 0) 
    {
        char c = text[--length];
        if (c < '0' || c > '9')
            return FALSE;
    }
    return TRUE;
}

// We store the following tightly packed (less 8) in modeInfo
//               <=9  <=26  <= 40
// NUMERIC      ( 10,   12,    14);
// ALPHANUMERIC (  9,   11,    13);
// BYTE         (  8,   16,    16);
static char getModeBits(unsigned char version, 
                        unsigned char mode) 
{
    // Note: We use 15 instead of 16; since 15 doesn't exist and we cannot store 16 (8 + 8) in 3 bits
    // hex(int("".join(reversed([('00' + bin(x - 8)[2:])[-3:] for x in [10, 9, 8, 12, 11, 15, 14, 13, 15]])), 2))
    unsigned int modeInfo = 0x7bbb80a;
    char result;
    
#if QR_LOCK_VERSION == 0 || QR_LOCK_VERSION > 9
    if (version > 9) 
        modeInfo >>= 9;
#endif
    
#if QR_LOCK_VERSION == 0 || QR_LOCK_VERSION > 26
    if (version > 26) 
        modeInfo >>= 9;
#endif
    
    result = 8 + ((modeInfo >> (3 * mode)) & 0x07);
    
    if (result == 15) 
        result = 16;
    
    return result;
}


typedef struct BitBucket
{
    LONG bitOffsetOrWidth;
    SHORT capacityBytes;
    unsigned char *data;
} BitBucket;

/*
VOID bb_dump(BitBucket *bitBuffer)
{
    printf("Buffer: ");
    for (LONG i = 0; i < bitBuffer->capacityBytes; i++) {
        printf("%02x", bitBuffer->data[i]);
        if ((i % 4) == 3) { printf(" "); }
    }
    printf("\n");
}
*/

static SHORT bb_getGridSizeBytes(unsigned char size)
{
    return (((size * size) + 7) / 8);
}

static SHORT bb_getBufferSizeBytes(LONG bits)
{
    return ((bits + 7) / 8);
}

static VOID bb_initBuffer(BitBucket *bitBuffer,
                          unsigned char *data,
                          int32_t capacityBytes)
{
    bitBuffer->bitOffsetOrWidth = 0;
    bitBuffer->capacityBytes = capacityBytes;
    bitBuffer->data = data;
    
    memset(data, 0, bitBuffer->capacityBytes);
}

static VOID bb_initGrid(BitBucket *bitGrid,
                        unsigned char *data,
                        unsigned char size)
{
    bitGrid->bitOffsetOrWidth = size;
    bitGrid->capacityBytes = bb_getGridSizeBytes(size);
    bitGrid->data = data;

    memset(data, 0, bitGrid->capacityBytes);
}

static VOID bb_appendBits(BitBucket *bitBuffer,
                          LONG val,
                          unsigned char length)
{
    LONG offset = bitBuffer->bitOffsetOrWidth;
    char i;
    for (i = length - 1; i >= 0; i--, offset++) 
    {
        bitBuffer->data[offset >> 3] |= ((val >> i) & 1) << (7 - (offset & 7));
    }
    bitBuffer->bitOffsetOrWidth = offset;
}
/*
VOID bb_setBits(BitBucket *bitBuffer, LONG val, int offset, unsigned char length) {
    for (char i = length - 1; i >= 0; i--, offset++) {
        bitBuffer->data[offset >> 3] |= ((val >> i) & 1) << (7 - (offset & 7));
    }
}
*/
static VOID bb_setBit(BitBucket *bitGrid,
                      unsigned char x,
                      unsigned char y,
                      BOOLEAN on)
{
    LONG offset = y * bitGrid->bitOffsetOrWidth + x;
    unsigned char mask = 1 << (7 - (offset & 0x07));
    
    if (on)
    {
        bitGrid->data[offset >> 3] |= mask;
    }
    else
    {
        bitGrid->data[offset >> 3] &= ~mask;
    }
}

static VOID bb_invertBit(BitBucket *bitGrid,
                         unsigned char x,
                         unsigned char y,
                         BOOLEAN invert)
{
    LONG offset = y * bitGrid->bitOffsetOrWidth + x;
    unsigned char mask = 1 << (7 - (offset & 0x07));
    BOOLEAN on = ((bitGrid->data[offset >> 3] & (1 << (7 - (offset & 0x07)))) != 0);
    
    if (on ^ invert)
    {
        bitGrid->data[offset >> 3] |= mask;
    }
    else
    {
        bitGrid->data[offset >> 3] &= ~mask;
    }
}

static BOOLEAN bb_getBit(BitBucket *bitGrid,
                         unsigned char x,
                         unsigned char y) 
{
    LONG offset = y * bitGrid->bitOffsetOrWidth + x;
    return (bitGrid->data[offset >> 3] & (1 << (7 - (offset & 0x07)))) != 0;
}

// XORs the data modules in this QR Code with the given mask pattern. Due to XOR's mathematical
// properties, calling applyMask(m) twice with the same value is equivalent to no change at all.
// This means it is possible to apply a mask, undo it, and try another mask. Note that a final
// well-formed QR Code symbol needs exactly one mask applied (not zero, not two, etc.).
static VOID applyMask(BitBucket *modules,
                      BitBucket *isFunction,
                      unsigned char mask) 
{
    unsigned char size = modules->bitOffsetOrWidth;
    unsigned char x, y;
    BOOLEAN invert;
    
    for (y = 0; y < size; y++) 
    {
        for (x = 0; x < size; x++)
        {
            if (bb_getBit(isFunction, x, y))  
                continue; 
            
            invert = 0;
            switch (mask) 
            {
                case 0:
                     invert = (x + y) % 2 == 0;
                     break;
                case 1:
                     invert = y % 2 == 0;
                     break;
                case 2:
                     invert = x % 3 == 0;
                     break;
                case 3:
                     invert = (x + y) % 3 == 0;
                     break;
                case 4:
                     invert = (x / 3 + y / 2) % 2 == 0;
                     break;
                case 5:
                     invert = x * y % 2 + x * y % 3 == 0;
                     break;
                case 6:
                     invert = (x * y % 2 + x * y % 3) % 2 == 0;
                     break;
                case 7:
                     invert = ((x + y) % 2 + x * y % 3) % 2 == 0;
                     break;
            }
            bb_invertBit(modules, x, y, invert);
        }
    }
}

static VOID setFunctionModule(BitBucket *modules, BitBucket *isFunction, unsigned char x, unsigned char y, BOOLEAN on)
{
    bb_setBit(modules, x, y, on);
    bb_setBit(isFunction, x, y, TRUE);
}

// Draws a 9*9 finder pattern including the border separator, with the center module at (x, y).
static VOID drawFinderPattern(BitBucket *modules,
                              BitBucket *isFunction,
                              unsigned char x,
                              unsigned char y)
{
    unsigned char size = modules->bitOffsetOrWidth;
    unsigned char dist;
    int16_t xx, yy;
    char i, j;

    for (i = -4; i <= 4; i++)
    {
        for (j = -4; j <= 4; j++)
        {
            dist = Max(abs(i), abs(j));  // Chebyshev/infinity norm
            xx = x + j;
            yy = y + i;
            if (0 <= xx && xx < size && 0 <= yy && yy < size) 
                setFunctionModule(modules, isFunction, xx, yy, dist != 2 && dist != 4);
        }
    }
}

// Draws a 5*5 alignment pattern, with the center module at (x, y).
static VOID drawAlignmentPattern(BitBucket *modules,
                                 BitBucket *isFunction,
                                 unsigned char x, unsigned char y)
{
    char i, j;
    for (i = -2; i <= 2; i++)
    {
        for (j = -2; j <= 2; j++)
        {
            setFunctionModule(modules, isFunction, x + j, y + i, Max(abs(i), abs(j)) != 1);
        }
    }
}

// Draws two copies of the format bits (with its own error correction code)
// based on the given mask and this object's error correction level field.
static VOID drawFormatBits(BitBucket *modules,
                           BitBucket *isFunction,
                           unsigned char ecc,
                           unsigned char mask)
{
    
    unsigned char size = modules->bitOffsetOrWidth;

    // Calculate error correction code and pack bits
    LONG data = ecc << 3 | mask;  // errCorrLvl is uint2, mask is uint3
    LONG rem = data;
    unsigned char c;
    int i;
    char a;
    
    for (i = 0; i < 10; i++) 
    {
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    }
    
    data = data << 10 | rem;
    data ^= 0x5412;  // uint15
    
    // Draw first copy
    for (c = 0; c <= 5; c++)
    {
        setFunctionModule(modules, isFunction, 8, c, ((data >> c) & 1) != 0);
    }
    
    setFunctionModule(modules, isFunction, 8, 7, ((data >> 6) & 1) != 0);
    setFunctionModule(modules, isFunction, 8, 8, ((data >> 7) & 1) != 0);
    setFunctionModule(modules, isFunction, 7, 8, ((data >> 8) & 1) != 0);
    
    for (a = 9; a < 15; a++) 
    {
        setFunctionModule(modules, isFunction, 14 - a, 8, ((data >> a) & 1) != 0);
    }
    
    // Draw second copy
    for (a = 0; a <= 7; a++) 
    {
        setFunctionModule(modules, isFunction, size - 1 - a, 8, ((data >> a) & 1) != 0);
    }
    
    for (a = 8; a < 15; a++) 
    {
        setFunctionModule(modules, isFunction, 8, size - 15 + a, ((data >> a) & 1) != 0);
    }
    
    setFunctionModule(modules, isFunction, 8, size - 8, TRUE);
}


// Draws two copies of the version bits (with its own error correction code),
// based on this object's version field (which only has an effect for 7 <= version <= 40).
static VOID drawVersion(BitBucket *modules,
                        BitBucket *isFunction,
                        unsigned char version) 
{
    
    char size = modules->bitOffsetOrWidth;
    unsigned char i, a, b;
    BOOLEAN bit;
    LONG rem, data;

#if QR_LOCK_VERSION != 0 && QR_LOCK_VERSION < 7
    return;
    
#else
    if (version < 7)
        return;
    
    // Calculate error correction code and pack bits
    rem = version;  // version is uint6, in the range [7, 40]
    for (i = 0; i < 12; i++) 
    {
        rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
    }
    
    data = version << 12 | rem;  // uint18
    
    // Draw two copies
    for (i = 0; i < 18; i++) 
    {
        bit = ((data >> i) & 1) != 0;
        a = size - 11 + i % 3, b = i / 3;
        setFunctionModule(modules, isFunction, a, b, bit);
        setFunctionModule(modules, isFunction, b, a, bit);
    }
    
#endif
}

static VOID drawFunctionPatterns(BitBucket *modules,
                                 BitBucket *isFunction,
                                 unsigned char version,
                                 unsigned char ecc)
{
    
    unsigned char size = modules->bitOffsetOrWidth;
    unsigned char i, step, j, pos;
    unsigned char alignCount = version / 7 + 2;
    unsigned char alignPositionIndex = alignCount - 1;
    unsigned char alignPosition[alignCount];
    
    // Draw the horizontal and vertical timing patterns
    for (i = 0; i < size; i++) 
    {
        setFunctionModule(modules, isFunction, 6, i, i % 2 == 0);
        setFunctionModule(modules, isFunction, i, 6, i % 2 == 0);
    }
    
    // Draw 3 finder patterns (all corners except bottom right; overwrites some timing modules)
    drawFinderPattern(modules, isFunction, 3, 3);
    drawFinderPattern(modules, isFunction, size - 4, 3);
    drawFinderPattern(modules, isFunction, 3, size - 4);
    
#if QR_LOCK_VERSION == 0 || QR_LOCK_VERSION > 1

    if (version > 1)
    {

        // Draw the numerous alignment patterns
        
        
        if (version != 32)
        {
            step = (version * 4 + alignCount * 2 + 1) / (2 * alignCount - 2) * 2;  // ceil((size - 13) / (2*numAlign - 2)) * 2
        }
        else
        {
            // C-C-C-Combo breaker!
            step = 26;
        }
        
       
        alignPosition[0] = 6;
        
        size = version * 4 + 17;
        for (i = 0, pos = size - 7; i < alignCount - 1; i++, pos -= step) 
        {
            alignPosition[alignPositionIndex--] = pos;
        }
        
        for (i = 0; i < alignCount; i++)
        {
            for (j = 0; j < alignCount; j++)
            {
                if ((i == 0 && j == 0) || (i == 0 && j == alignCount - 1) || (i == alignCount - 1 && j == 0)) 
                {
                    continue;  // Skip the three finder corners
                } 
                else 
                {
                    drawAlignmentPattern(modules, isFunction, alignPosition[i], alignPosition[j]);
                }
            }
        }
    }
    
#endif
    // Draw configuration data
    drawFormatBits(modules, isFunction, ecc, 0);  // Dummy mask value; overwritten later in the constructor
    drawVersion(modules, isFunction, version);
}


// Draws the given sequence of 8-bit codewords (data and error correction) onto the entire
// data area of this QR Code symbol. Function modules need to be marked off before this is called.
static VOID drawCodewords(BitBucket *modules,
                          BitBucket *isFunction,
                          BitBucket *codewords)
{
    
    LONG bitLength = codewords->bitOffsetOrWidth;
    unsigned char *data = codewords->data;
    
    unsigned char size = modules->bitOffsetOrWidth;
    
    // Bit index into the data
    LONG i = 0;
    
    // Do the funny zigzag scan
    int16_t right;
    unsigned char vert;
    unsigned char x, y;
    BOOLEAN upwards;
    int j;
    
    for (right = size - 1; right >= 1; right -= 2)
    {  // Index of right column in each column pair
        if (right == 6)
        { 
            right = 5;
        }
        
        for ( vert = 0; vert < size; vert++) // Vertical counter
        { 
            for (j = 0; j < 2; j++)
            {
                x = right - j;  // Actual x coordinate
                upwards = ((right & 2) == 0) ^ (x < 6);
                y = upwards ? size - 1 - vert : vert;  // Actual y coordinate
                if (!bb_getBit(isFunction, x, y) && i < bitLength) 
                {
                    bb_setBit(modules, x, y, ((data[i >> 3] >> (7 - (i & 7))) & 1) != 0);
                    i++;
                }
                // If there are any remainder bits (0 to 7), they are already
                // set to 0/FALSE/white when the grid of modules was initialized
            }
        }
    }
}

#define PENALTY_N1      3
#define PENALTY_N2      3
#define PENALTY_N3     40
#define PENALTY_N4     10

// Calculates and returns the penalty score based on state of this QR Code's current modules.
// This is used by the automatic mask choice algorithm to find the mask pattern that yields the lowest score.
// @TODO: This can be optimized by working with the bytes instead of bits.
static LONG getPenaltyScore(BitBucket *modules)
{
    BOOLEAN colorY, cy, color, colorUL, colorUR, colorL, colorX, cx;
    SHORT bitsRow = 0, bitsCol = 0;
    SHORT black = 0;
    SHORT total, k;
    LONG result = 0;
    unsigned char y, x, runY, runX;
    unsigned char size = modules->bitOffsetOrWidth;
    
    // Adjacent modules in row having same color
    for (y = 0; y < size; y++) 
    {
        
        colorX = bb_getBit(modules, 0, y);
        for (x = 1, runX = 1; x < size; x++) 
        {
            cx = bb_getBit(modules, x, y);
            if (cx != colorX) 
            {
                colorX = cx;
                runX = 1;
            } 
            else 
            {
                runX++;
                if (runX == 5) 
                {
                    result += PENALTY_N1;
                } 
                else if (runX > 5) 
                {
                    result++;
                }
            }
        }
    }
    
    // Adjacent modules in column having same color
    for (x = 0; x < size; x++) 
    {
        colorY = bb_getBit(modules, x, 0);
        for (y = 1, runY = 1; y < size; y++) 
        {
            cy = bb_getBit(modules, x, y);
            
            if (cy != colorY)
            {
                colorY = cy;
                runY = 1;
            } 
            else 
            {
                runY++;
                if (runY == 5) 
                {
                    result += PENALTY_N1;
                }
                else if (runY > 5) 
                {
                    result++;
                }
            }
        }
    }
    
    for (y = 0; y < size; y++)
    {
        for (x = 0; x < size; x++) 
        {
            color = bb_getBit(modules, x, y);

            // 2*2 blocks of modules having same color
            if (x > 0 && y > 0) 
            {
                colorUL = bb_getBit(modules, x - 1, y - 1);
                colorUR = bb_getBit(modules, x, y - 1);
                colorL = bb_getBit(modules, x - 1, y);
                
                if (color == colorUL && color == colorUR && color == colorL) 
                {
                    result += PENALTY_N2;
                }
            }

            // Finder-like pattern in rows and columns
            bitsRow = ((bitsRow << 1) & 0x7FF) | color;
            bitsCol = ((bitsCol << 1) & 0x7FF) | bb_getBit(modules, y, x);

            // Needs 11 bits accumulated
            if (x >= 10) 
            {
                if (bitsRow == 0x05D || bitsRow == 0x5D0) 
                {
                    result += PENALTY_N3;
                }
                if (bitsCol == 0x05D || bitsCol == 0x5D0)
                {
                    result += PENALTY_N3;
                }
            }

            // Balance of black and white modules
            if (color)  
                black++; 
        }
    }

    // Find smallest k such that (45-5k)% <= dark/total <= (55+5k)%
    total = size * size;
    for (k = 0; black * 20 < (9 - k) * total || black * 20 > (11 + k) * total; k++)
    {
        result += PENALTY_N4;
    }
    
    return result;
}

static unsigned char rs_multiply(unsigned char x,
                                 unsigned char y)
{
    // Russian peasant multiplication
    // See: https://en.wikipedia.org/wiki/Ancient_Egyptian_multiplication
    SHORT z = 0;
    char i;
    for (i = 7; i >= 0; i--)
    {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return z;
}

static VOID rs_init(unsigned char degree,
                    unsigned char *coeff) 
{
    SHORT root = 1;
    unsigned char i, j;
    
    memset(coeff, 0, degree);
    coeff[degree - 1] = 1;
    
    // Compute the product polynomial (x - r^0) * (x - r^1) * (x - r^2) * ... * (x - r^{degree-1}),
    // drop the highest term, and store the rest of the coefficients in order of descending powers.
    // Note that r = 0x02, which is a generator element of this field GF(2^8/0x11D).
    for (i = 0; i < degree; i++) 
    {
        // Multiply the current product by (x - r^i)
        for (j = 0; j < degree; j++)
        {
            coeff[j] = rs_multiply(coeff[j], root);
            if (j + 1 < degree)
            {
                coeff[j] ^= coeff[j + 1];
            }
        }
        root = (root << 1) ^ ((root >> 7) * 0x11D);  // Multiply by 0x02 mod GF(2^8/0x11D)
    }
}

static VOID rs_getRemainder(unsigned char degree,
                            unsigned char *coeff,
                            unsigned char *data,
                            unsigned char length,
                            unsigned char *result,
                            unsigned char stride)
{
    unsigned char i, j, factor;
    // Compute the remainder by performing polynomial division
    
    //for (unsigned char i = 0; i < degree; i++) { result[] = 0; }
    //memset(result, 0, degree);
    
    for (i = 0; i < length; i++) 
    {
        factor = data[i] ^ result[0];
        for (j = 1; j < degree; j++) 
        {
            result[(j - 1) * stride] = result[j * stride];
        }
        result[(degree - 1) * stride] = 0;
        
        for (j = 0; j < degree; j++) 
        {
            result[j * stride] ^= rs_multiply(coeff[j], factor);
        }
    }
}

static char encodeDataCodewords(BitBucket *dataCodewords,
                                const unsigned char *text,
                                SHORT length,
                                unsigned char version)
{
    char mode = MODE_BYTE;
    SHORT i;
    SHORT accumData = 0;
    unsigned char accumCount = 0;
    
    if (isNumeric((char*)text, length))
    {
        mode = MODE_NUMERIC;
        bb_appendBits(dataCodewords, 1 << MODE_NUMERIC, 4);
        bb_appendBits(dataCodewords, length, getModeBits(version, MODE_NUMERIC));

        accumData = 0;
        accumCount = 0;
        for (i = 0; i < length; i++)
        {
            accumData = accumData * 10 + ((char)(text[i]) - '0');
            accumCount++;
            if (accumCount == 3)
            {
                bb_appendBits(dataCodewords, accumData, 10);
                accumData = 0;
                accumCount = 0;
            }
        }
        
        // 1 or 2 digits remaining
        if (accumCount > 0)
        {
            bb_appendBits(dataCodewords, accumData, accumCount * 3 + 1);
        }
        
    } 
    else if (isAlphanumeric((char*)text, length)) 
    {
        mode = MODE_ALPHANUMERIC;
        bb_appendBits(dataCodewords, 1 << MODE_ALPHANUMERIC, 4);
        bb_appendBits(dataCodewords, length, getModeBits(version, MODE_ALPHANUMERIC));

        accumData = 0;
        accumCount = 0;
        
        for (i = 0; i  < length; i++)
        {
            accumData = accumData * 45 + getAlphanumeric((char)(text[i]));
            accumCount++;
            if (accumCount == 2)
            {
                bb_appendBits(dataCodewords, accumData, 11);
                accumData = 0;
                accumCount = 0;
            }
        }
        
        // 1 character remaining
        if (accumCount > 0)
        {
            bb_appendBits(dataCodewords, accumData, 6);
        }
        
    }
    else
    {
        bb_appendBits(dataCodewords, 1 << MODE_BYTE, 4);
        bb_appendBits(dataCodewords, length, getModeBits(version, MODE_BYTE));
        for (i = 0; i < length; i++)
        {
            bb_appendBits(dataCodewords, (char)(text[i]), 8);
        }
    }
    
    //bb_setBits(dataCodewords, length, 4, getModeBits(version, mode));
    
    return mode;
}

static VOID
performErrorCorrection(unsigned char version,
                       unsigned char ecc, 
					   BitBucket *data)
{
    
    // See: http://www.thonky.com/qr-code-tutorial/structure-final-message
    
#if QR_LOCK_VERSION == 0
    unsigned char numBlocks = NUM_ERROR_CORRECTION_BLOCKS[ecc][version - 1];
    SHORT totalEcc = NUM_ERROR_CORRECTION_CODEWORDS[ecc][version - 1];
    SHORT moduleCount = NUM_RAW_DATA_MODULES[version - 1];
#else
    unsigned char numBlocks = NUM_ERROR_CORRECTION_BLOCKS[ecc];
    SHORT totalEcc = NUM_ERROR_CORRECTION_CODEWORDS[ecc];
    SHORT moduleCount = NUM_RAW_DATA_MODULES;
#endif
    
    unsigned char blockEccLen = totalEcc / numBlocks;
    unsigned char numShortBlocks = numBlocks - moduleCount / 8 % numBlocks;
    unsigned char shortBlockLen = moduleCount / 8 / numBlocks;
    unsigned char blockSize, blockNum;
    unsigned char shortDataBlockLen = shortBlockLen - blockEccLen;
    unsigned char stride;
    unsigned char result[data->capacityBytes];
    unsigned char coeff[blockEccLen];
    unsigned char i;
    SHORT index;
    SHORT offset = 0;
    unsigned char *dataBytes = data->data;
    
    
    memset(result, 0, sizeof(result));
    
    rs_init(blockEccLen, coeff);
    
    
    // Interleave all short blocks
    for (i = 0; i < shortDataBlockLen; i++)
    {
        index = i;
        stride = shortDataBlockLen;
        for (blockNum = 0; blockNum < numBlocks; blockNum++)
        {
            result[offset++] = dataBytes[index];
            
#if QR_LOCK_VERSION == 0 || QR_LOCK_VERSION >= 5
            if (blockNum == numShortBlocks)
                stride++; 
#endif
            index += stride;
        }
    }
    
    // Version less than 5 only have short blocks
#if QR_LOCK_VERSION == 0 || QR_LOCK_VERSION >= 5
    {
        // Interleave long blocks
        index = shortDataBlockLen * (numShortBlocks + 1);
        stride = shortDataBlockLen;
        for (blockNum = 0; blockNum < numBlocks - numShortBlocks; blockNum++)
        {
            result[offset++] = dataBytes[index];
            
            if (blockNum == 0) 
                stride++;
            
            index += stride;
        }
    }
#endif
    
    // Add all ecc blocks, interleaved
    blockSize = shortDataBlockLen;
    for (blockNum = 0; blockNum < numBlocks; blockNum++)
    {
        
#if QR_LOCK_VERSION == 0 || QR_LOCK_VERSION >= 5
        if (blockNum == numShortBlocks)
            blockSize++; 
#endif
        rs_getRemainder(blockEccLen, coeff, dataBytes, blockSize, &result[offset + blockNum], numBlocks);
        dataBytes += blockSize;
    }
    
    memcpy(data->data, result, data->capacityBytes);
    data->bitOffsetOrWidth = moduleCount;
}

// We store the Format bits tightly packed into a single byte (each of the 4 modes is 2 bits)
// The format bits can be determined by ECC_FORMAT_BITS >> (2 * ecc)
static const unsigned char ECC_FORMAT_BITS = (0x02 << 6) | (0x03 << 4) | (0x00 << 2) | (0x01 << 0);

SHORT qrcode_getBufferSize(unsigned char version)
{
    return bb_getGridSizeBytes(4 * version + 17);
}

// @TODO: Return error if data is too big.
char qrcode_initBytes(QRCode *qrcode, unsigned char *modules, unsigned char version, unsigned char ecc, unsigned char *data, SHORT length)
{
    unsigned char size = version * 4 + 17;
    unsigned char eccFormatBits = (ECC_FORMAT_BITS >> (2 * ecc)) & 0x03;
    struct BitBucket codewords;
    unsigned char padByte;
    unsigned char isFunctionGridBytes[bb_getGridSizeBytes(size)];
    
#if QR_LOCK_VERSION == 0
    SHORT moduleCount = NUM_RAW_DATA_MODULES[version - 1];
    SHORT dataCapacity = moduleCount / 8 - NUM_ERROR_CORRECTION_CODEWORDS[eccFormatBits][version - 1];
#else
    version = QR_LOCK_VERSION;
    SHORT moduleCount = NUM_RAW_DATA_MODULES;
    SHORT dataCapacity = moduleCount / 8 - NUM_ERROR_CORRECTION_CODEWORDS[eccFormatBits];
#endif
    unsigned char codewordBytes[bb_getBufferSizeBytes(moduleCount)];
    char mode;
    LONG padding;
    unsigned char i;
    BitBucket modulesGrid, isFunctionGrid;
    unsigned char mask = 0;
    int32_t minPenalty = INT32_MAX;
    int penalty;
    
    qrcode->version = version;
    qrcode->size = size;
    qrcode->ecc = ecc;
    qrcode->modules = modules;
    
    
    
    
    bb_initBuffer(&codewords, codewordBytes, (int32_t)sizeof(codewordBytes));
    
    // Place the data code words into the buffer
    mode = encodeDataCodewords(&codewords, data, length, version);
    
    if (mode < 0)
        return -1; 
    
    qrcode->mode = mode;
    
    // Add terminator and pad up to a byte if applicable
    padding = (dataCapacity * 8) - codewords.bitOffsetOrWidth;
    if (padding > 4)
        padding = 4;
    bb_appendBits(&codewords, 0, padding);
    bb_appendBits(&codewords, 0, (8 - codewords.bitOffsetOrWidth % 8) % 8);
    
    // Pad with alternate bytes until data capacity is reached
    for (padByte = 0xEC; codewords.bitOffsetOrWidth < (dataCapacity * 8); padByte ^= 0xEC ^ 0x11)
    {
        bb_appendBits(&codewords, padByte, 8);
    }

    bb_initGrid(&modulesGrid, modules, size);
    
    bb_initGrid(&isFunctionGrid, isFunctionGridBytes, size);
    
    
    // Draw function patterns, draw all codewords, do masking
    drawFunctionPatterns(&modulesGrid, &isFunctionGrid, version, eccFormatBits);
    performErrorCorrection(version, eccFormatBits, &codewords);
    drawCodewords(&modulesGrid, &isFunctionGrid, &codewords);
   
    // Find the best (lowest penalty) mask
    for (i = 0; i < 8; i++)
    {
        drawFormatBits(&modulesGrid, &isFunctionGrid, eccFormatBits, i);
        applyMask(&modulesGrid, &isFunctionGrid, i);
        penalty = getPenaltyScore(&modulesGrid);
        if (penalty < minPenalty) 
        {
            mask = i;
            minPenalty = penalty;
        }
        applyMask(&modulesGrid, &isFunctionGrid, i);  // Undoes the mask due to XOR
    }
    
    qrcode->mask = mask;
    
    // Overwrite old format bits
    drawFormatBits(&modulesGrid, &isFunctionGrid, eccFormatBits, mask);
    
    // Apply the final choice of mask
    applyMask(&modulesGrid, &isFunctionGrid, mask);

    return 0;
}

char qrcode_initText(QRCode *qrcode,
                     unsigned char *modules,
                     unsigned char version, 
                     unsigned char ecc,
                     const char *data)
{
    return qrcode_initBytes(qrcode, modules, version, ecc, (unsigned char*)data, strlen(data));
}

BOOLEAN qrcode_getModule(QRCode *qrcode,
                         unsigned char x,
                         unsigned char y)
{
    LONG offset;
    
    if (x < 0 || x >= qrcode->size || y < 0 || y >= qrcode->size) {
        return FALSE;
    }

    offset = y * qrcode->size + x;
    return (qrcode->modules[offset >> 3] & (1 << (7 - (offset & 0x07)))) != 0;
}

/*
unsigned char qrcode_getHexLength(QRCode *qrcode)
{
    return ((qrcode->size * qrcode->size) + 7) / 4;
}
VOID qrcode_getHex(QRCode *qrcode, char *result)
{
    
}
*/
