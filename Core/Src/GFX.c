/* The MIT License
 *
 * Copyright (c) 2020 Piotr Duba
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

#include "GFX.h"
#include "font_ascii_5x7.h"

/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
/**************************************************************************/
void GFX_draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y)
{
	int8_t i, j;
	uint8_t line;

	if((x >= WIDTH) || (y >= HEIGHT) || ((x + 6 * size_x - 1) < 0) || ((y + 8 * size_y - 1) < 0))
	{
		return;
	}

	for(i = 0; i < 5; i++)  // Char bitmap = 5 columns
	{
		line = (*(const unsigned char *)(&font[c * 5 + i]));
		for(j = 7; j >= 0; j--, line >>= 1)
		{
			if(line & 1)
			{
				if(size_x == 1 && size_y == 1)
				{
					SSD1306_draw_pixel(x + i, y + j, color);
				}
				else
				{
					GFX_draw_fill_rect(x + i * size_x, y + j * size_y, size_x, size_y, color);
				}
			}
			else if(bg != color)
			{
				if(size_x == 1 && size_y == 1)
				{
					SSD1306_draw_pixel(x + i, y + j, bg);
				}
				else
				{
					GFX_draw_fill_rect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
				}
			}
		}
	}

	if(bg != color) // If opaque, draw vertical line for last column
	{
		if(size_x == 1 && size_y == 1)
		{
			SSD1306_draw_fast_vline(x + 5, y, 8, bg);
		}
		else
		{
			GFX_draw_fill_rect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
		}
	}
}

/**************************************************************************/
/*!
   @brief   Draw a set of characters
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed characters (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
/**************************************************************************/
void GFX_draw_string(int16_t x, int16_t y, unsigned char * c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y)
{
	uint8_t offset = 0;
	while(*c)
	{
		GFX_draw_char(x+offset, y, *c, color, bg, size_x, size_y);
		offset += (5 + 2) * size_x;
		c++;
	}
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX_draw_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	for(int16_t i = x; i < x + w; i++)
	{
		SSD1306_draw_fast_vline(i, y, h, color);
	}
}
