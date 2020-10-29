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
/*!
 * @file Adafruit_SSD1306.cpp
 *
 * @mainpage Arduino library for monochrome OLEDs based on SSD1306 drivers.
 *
 * @section intro_sec Introduction
 *
 * This is documentation for Adafruit's SSD1306 library for monochrome
 * OLED displays: http://www.adafruit.com/category/63_98
 *
 * These displays use I2C or SPI to communicate. I2C requires 2 pins
 * (SCL+SDA) and optionally a RESET pin. SPI requires 4 pins (MOSI, SCK,
 * select, data/command) and optionally a reset pin. Hardware SPI or
 * 'bitbang' software SPI are both supported.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on <a
 * href="https://github.com/adafruit/Adafruit-GFX-Library"> Adafruit_GFX</a>
 * being present on your system. Please make sure you have installed the latest
 * version before using this library.
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * @section license License
 *
 * BSD license, all text above, and the splash screen included below,
 * must be included in any redistribution.
 *
 */

#include "SSD1306.h"
#include "i2c.h"
#include "gpio.h"


#define ssd1306_swap(a, b)                                                     \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation

static void SSD1306_send_com(uint8_t c);
static uint8_t platform_write(uint8_t reg, uint8_t *bufp, uint16_t len);
static uint8_t platform_write_dma(uint8_t reg, uint8_t *bufp, uint16_t len);

static uint8_t * buffer;
static uint8_t rotation;

static uint8_t platform_write(uint8_t reg, uint8_t *bufp, uint16_t len)
{
	HAL_I2C_Mem_Write(&SSD1306_I2C_BUS, SSD1306_I2C_ADDRESS, reg, 1, bufp, len, 100);
	return 0;
}

static uint8_t platform_write_dma(uint8_t reg, uint8_t *bufp, uint16_t len)
{
	HAL_I2C_Mem_Write_DMA(&SSD1306_I2C_BUS, SSD1306_I2C_ADDRESS, reg, 1, bufp, len);
	return 0;
}

static void SSD1306_send_com(uint8_t c)
{
	platform_write(0x00, &c, 1);
}

bool SSD1306_init(void)
{
  uint8_t comPins = 0x02, contrast = 0x8F, vccstate = SSD1306_SWITCHCAPVCC;

  if ((!buffer) && !(buffer = (uint8_t *)malloc(SSD1306_WIDTH * ((SSD1306_HEIGHT + 7) / 8))))
  {
    return false;
  }

  SSD1306_display_clear();

  // Init sequence
  SSD1306_send_com(SSD1306_DISPLAYOFF);
  SSD1306_send_com(SSD1306_SETDISPLAYCLOCKDIV);
  SSD1306_send_com(0xE0);
  SSD1306_send_com(SSD1306_SETMULTIPLEX);
  SSD1306_send_com(SSD1306_HEIGHT - 1);

  SSD1306_send_com(SSD1306_SETDISPLAYOFFSET);
  SSD1306_send_com(0x00);
  SSD1306_send_com(SSD1306_SETSTARTLINE | 0x00);
  SSD1306_send_com(SSD1306_CHARGEPUMP);

  SSD1306_send_com((vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0x14);

  SSD1306_send_com(SSD1306_MEMORYMODE);
  SSD1306_send_com(0x00);
  SSD1306_send_com(SSD1306_SEGREMAP | 0x10);
  SSD1306_send_com(SSD1306_COMSCANDEC);


  if((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
  {
    comPins = 0x02;
    contrast = 0x8F;
  }
  else if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
  {
    comPins = 0x12;
    contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x9F : 0xCF;
  }
  else if ((SSD1306_WIDTH == 96) && (SSD1306_HEIGHT == 16))
  {
    comPins = 0x2; // ada x12
    contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0xAF;
  }

  SSD1306_send_com(SSD1306_SETCOMPINS);
  SSD1306_send_com(comPins);
  SSD1306_send_com(SSD1306_SETCONTRAST);
  SSD1306_send_com(contrast);

  SSD1306_send_com(SSD1306_SETPRECHARGE);
  SSD1306_send_com((vccstate == SSD1306_EXTERNALVCC) ? 0x22 : 0xF1);

  SSD1306_send_com(SSD1306_SETVCOMDETECT);
  SSD1306_send_com(0x40);
  SSD1306_send_com(SSD1306_DISPLAYALLON_RESUME);
  SSD1306_send_com(SSD1306_NORMALDISPLAY);
  SSD1306_send_com(SSD1306_DEACTIVATE_SCROLL);
  SSD1306_send_com(SSD1306_DISPLAYON);

  SSD1306_set_rotation(SSD1306_HORIZONTAL_MODE2);
  return true;
}

// DRAWING FUNCTIONS -------------------------------------------------------

/*!
    @brief  Set/clear/invert a single pixel. This is also invoked by the
            Adafruit_GFX library in generating many higher-level graphics
            primitives.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @param  color
            Pixel color, one of: SSD1306_BLACK, SSD1306_WHITE or SSD1306_INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void SSD1306_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x >= 0) && (x < SSD1306_WIDTH) && (y >= 0) && (y < SSD1306_HEIGHT))
	{
		/* Pixel is in-bounds. Rotate coordinates if needed. */
		switch (SSD1306_get_rotation())
		{
			case 1:
				ssd1306_swap(x, y);
				x = SSD1306_WIDTH - x - 1;
				break;
			case 2:
				x = SSD1306_WIDTH - x - 1;
				y = SSD1306_HEIGHT - y - 1;
				break;
			case 3:
				ssd1306_swap(x, y);
				y = SSD1306_HEIGHT - y - 1;
				break;
		}

		switch (color)
		{
			case SSD1306_WHITE:
				buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
				break;
			case SSD1306_BLACK:
				buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
				break;
			case SSD1306_INVERSE:
				buffer[x + (y / 8) * SSD1306_WIDTH] ^= (1 << (y & 7));
				break;
		}
	}
}

/*!
    @brief  Clear contents of display buffer (set all pixels to off).
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void SSD1306_display_clear(void)
{
	memset(buffer, 0, SSD1306_WIDTH * ((SSD1306_HEIGHT + 7) / 8));
}

/*!
    @brief  Draw a horizontal line. This is also invoked by the Adafruit_GFX
            library in generating many higher-level graphics primitives.
    @param  x
            Leftmost column -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @param  w
            Width of line, in pixels.
    @param  color
            Line color, one of: SSD1306_BLACK, SSD1306_WHITE or SSD1306_INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void SSD1306_draw_fast_hline(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	bool bSwap = false;

	switch (SSD1306_get_rotation())
	{
		case 1:
			// 90 degree rotation, swap x & y for rotation, then invert x
			bSwap = true;
			ssd1306_swap(x, y);
			x = SSD1306_WIDTH - x - 1;
			break;
		case 2:
			// 180 degree rotation, invert x and y, then shift y around for height.
			x = SSD1306_WIDTH - x - 1;
			y = SSD1306_HEIGHT - y - 1;
			x -= (w - 1);
			break;
		case 3:
			// 270 degree rotation, swap x & y for rotation,
			// then invert y and adjust y for w (not to become h)
			bSwap = true;
			ssd1306_swap(x, y);
			y = SSD1306_HEIGHT - y - 1;
			y -= (w - 1);
			break;
	}

	if (bSwap)
	{
		SSD1306_draw_fast_vline_internal(x, y, w, color);
	}
	else
	{
		SSD1306_draw_fast_hline_internal(x, y, w, color);
	}
}

void SSD1306_draw_fast_hline_internal(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	if ((y >= 0) && (y < SSD1306_HEIGHT))
	{
		// Y coord in bounds?
		if (x < 0)
		{
			// Clip left
			w += x;
			x = 0;
		}
		if ((x + w) > SSD1306_WIDTH)
		{
			// Clip right
			w = (SSD1306_WIDTH - x);
		}
		if (w > 0)
		{
			// Proceed only if width is positive
			uint8_t *pBuf = &buffer[(y / 8) * SSD1306_WIDTH + x], mask = 1 << (y & 7);
			switch (color)
			{
				case SSD1306_WHITE:
					while (w--)
					{
						*pBuf++ |= mask;
					};
					break;
				case SSD1306_BLACK:
					mask = ~mask;
					while (w--)
					{
						*pBuf++ &= mask;
					};
					break;
				case SSD1306_INVERSE:
					while (w--)
					{
						*pBuf++ ^= mask;
					};
					break;
			}
		}
	}
}

/*!
    @brief  Draw a vertical line. This is also invoked by the Adafruit_GFX
            library in generating many higher-level graphics primitives.
    @param  x
            Column of display -- 0 at left to (screen width -1) at right.
    @param  y
            Topmost row -- 0 at top to (screen height - 1) at bottom.
    @param  h
            Height of line, in pixels.
    @param  color
            Line color, one of: SSD1306_BLACK, SSD1306_WHITE or SSD1306_INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void SSD1306_draw_fast_vline(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	bool bSwap = false;
	switch (SSD1306_get_rotation())
	{
		case 1:
			// 90 degree rotation, swap x & y for rotation,
			// then invert x and adjust x for h (now to become w)
			bSwap = true;
  			ssd1306_swap(x, y);
  			x = SSD1306_WIDTH - x - 1;
  			x -= (h - 1);
  			break;
  	    case 2:
  	    	// 180 degree rotation, invert x and y, then shift y around for height.
  	    	x = SSD1306_WIDTH - x - 1;
  	    	y = SSD1306_HEIGHT - y - 1;
  	    	y -= (h - 1);
  	    	break;
  	    case 3:
  	    	// 270 degree rotation, swap x & y for rotation, then invert y
  	    	bSwap = true;
  	    	ssd1306_swap(x, y);
  	    	y = SSD1306_HEIGHT - y - 1;
  	    	break;
	}

	if (bSwap)
	{
		SSD1306_draw_fast_hline_internal(x, y, h, color);
	}
	else
	{
		SSD1306_draw_fast_vline_internal(x, y, h, color);
	}
}

void SSD1306_draw_fast_vline_internal(int16_t x, int16_t __y, int16_t __h, uint16_t color)
{
	if ((x >= 0) && (x < SSD1306_WIDTH))
	{
		// X coord in bounds?
		if (__y < 0)
		{
			// Clip top
			__h += __y;
			__y = 0;
		}
		if ((__y + __h) > SSD1306_HEIGHT)
		{
			// Clip bottom
			__h = (SSD1306_HEIGHT - __y);
		}
		if (__h > 0)
		{
			// Proceed only if height is now positive
			// this display doesn't need ints for coordinates,
			// use local byte registers for faster juggling
			uint8_t y = __y, h = __h;
			uint8_t *pBuf = &buffer[(y / 8) * SSD1306_WIDTH + x];

			// do the first partial byte, if necessary - this requires some masking
			uint8_t mod = (y & 7);
			if (mod)
			{
				// mask off the high n bits we want to set
				mod = 8 - mod;
				// note - lookup table results in a nearly 10% performance
				// improvement in fill* functions
				// uint8_t mask = ~(0xFF >> mod);
				static const uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
				uint8_t mask = (*(const unsigned char *)(&premask[mod]));
				// adjust the mask if we're not going to reach the end of this byte
				if (h < mod)
				{
					mask &= (0XFF >> (mod - h));
				}

				switch (color)
				{
					case SSD1306_WHITE:
						*pBuf |= mask;
						break;
					case SSD1306_BLACK:
						*pBuf &= ~mask;
						break;
					case SSD1306_INVERSE:
						*pBuf ^= mask;
						break;
				}
				pBuf += SSD1306_WIDTH;
			}

			if (h >= mod)
			{
				// More to go?
				h -= mod;
				// Write solid bytes while we can - effectively 8 rows at a time
				if (h >= 8)
				{
					if (color == SSD1306_INVERSE)
					{
						// separate copy of the code so we don't impact performance of
						// black/white write version with an extra comparison per loop
						do
						{
							*pBuf ^= 0xFF; // Invert byte
							pBuf += SSD1306_WIDTH; // Advance pointer 8 rows
							h -= 8;        // Subtract 8 rows from height
						} while (h >= 8);
					}
					else
					{
						// store a local value to work with
						uint8_t val = (color != SSD1306_BLACK) ? 255 : 0;
						do
						{
							*pBuf = val;   // Set byte
							pBuf += SSD1306_WIDTH; // Advance pointer 8 rows
							h -= 8;        // Subtract 8 rows from height
						} while (h >= 8);
					}
				}

				if (h)
				{
					// Do the final partial byte, if necessary
					mod = h & 7;
					// this time we want to mask the low bits of the byte,
					// vs the high bits we did above
					// uint8_t mask = (1 << mod) - 1;
					// note - lookup table results in a nearly 10% performance
					// improvement in fill* functions
					static const uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07,
                                                      0x0F, 0x1F, 0x3F, 0x7F};
					uint8_t mask = (*(const unsigned char *)(&postmask[mod]));
					switch (color)
					{
						case SSD1306_WHITE:
							*pBuf |= mask;
							break;
						case SSD1306_BLACK:
							*pBuf &= ~mask;
							break;
						case SSD1306_INVERSE:
							*pBuf ^= mask;
							break;
					}
				}
			}
		} // endif positive height
	}   // endif x in bounds
}

/*!
    @brief  Return color of a single pixel in display buffer.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @return true if pixel is set (usually SSD1306_WHITE, unless display invert
   mode is enabled), false if clear (SSD1306_BLACK).
    @note   Reads from buffer contents; may not reflect current contents of
            screen if display() has not been called.
*/
bool SSD1306_get_pixel(int16_t x, int16_t y)
{
    if ((x >= 0) && (x < SSD1306_WIDTH) && (y >= 0) && (y < SSD1306_HEIGHT))
    {
    	// Pixel is in-bounds. Rotate coordinates if needed.
    	switch (SSD1306_get_rotation())
    	{
    		case 1:
    			ssd1306_swap(x, y);
    			x = SSD1306_WIDTH - x - 1;
    			break;
    		case 2:
    			x = SSD1306_WIDTH - x - 1;
    			y = SSD1306_HEIGHT - y - 1;
    			break;
    		case 3:
    			ssd1306_swap(x, y);
    			y = SSD1306_HEIGHT - y - 1;
    			break;
    	}

    	return (buffer[x + (y / 8) * SSD1306_WIDTH] & (1 << (y & 7)));
    }
    return false; // Pixel out of bounds
}

/*!
    @brief  Get base address of display buffer for direct reading or writing.
    @return Pointer to an unsigned 8-bit array, column-major, columns padded
            to full byte boundary if needed.
*/
uint8_t* SSD1306_get_buffer(void)
{
	return buffer;
}

/*!
    @brief  Push data currently in RAM to SSD1306 display.
    @return None (void).
    @note   Drawing operations are not visible until this function is
            called. Call after each graphics command, or after a whole set
            of graphics commands, as best needed by one's own application.
*/
void SSD1306_display_repaint(void)
{
	uint16_t buf_len = SSD1306_WIDTH * ((SSD1306_HEIGHT + 7) / 8);

	SSD1306_send_com(SSD1306_PAGEADDR);
	SSD1306_send_com(0x00);
	SSD1306_send_com(0xFF);
	SSD1306_send_com(SSD1306_COLUMNADDR);
	SSD1306_send_com(0x00);

	SSD1306_send_com(SSD1306_WIDTH - 1); // Column end address

	platform_write_dma(SSD1306_SETSTARTLINE, buffer, buf_len);
}

/*!
    @brief  Activate a right-handed scroll for all or part of the display.
    @param  start
            First row.
    @param  stop
            Last row.
    @return None (void).
*/
/* To scroll the whole display, run: display.startscrollright(0x00, 0x0F) */
void SSD1306_start_scroll_right(uint8_t start, uint8_t stop)
{
	SSD1306_send_com(SSD1306_RIGHT_HORIZONTAL_SCROLL);
	SSD1306_send_com(0x00);

	SSD1306_send_com(start);
	SSD1306_send_com(0x00);
	SSD1306_send_com(stop);

	SSD1306_send_com(0x00);
	SSD1306_send_com(0xFF);
	SSD1306_send_com(SSD1306_ACTIVATE_SCROLL);
}

/*!
    @brief  Activate a left-handed scroll for all or part of the display.
    @param  start
            First row.
    @param  stop
            Last row.
    @return None (void).
*/
/* To scroll the whole display, run: display.startscrollleft(0x00, 0x0F) */
void SSD1306_start_scroll_left(uint8_t start, uint8_t stop)
{
	SSD1306_send_com(SSD1306_LEFT_HORIZONTAL_SCROLL);
	SSD1306_send_com(0x00);

	SSD1306_send_com(start);
	SSD1306_send_com(0X00);
	SSD1306_send_com(stop);

	SSD1306_send_com(0x00);
	SSD1306_send_com(0xFF);
	SSD1306_send_com(SSD1306_ACTIVATE_SCROLL);
}

/*!
    @brief  Activate a diagonal scroll for all or part of the display.
    @param  start
            First row.
    @param  stop
            Last row.
    @return None (void).
*/
/* display.startscrolldiagright(0x00, 0x0F) */
void SSD1306_start_scroll_diagright(uint8_t start, uint8_t stop)
{
	SSD1306_send_com(SSD1306_SET_VERTICAL_SCROLL_AREA);
	SSD1306_send_com(0x00);
  	SSD1306_send_com(SSD1306_HEIGHT);

  	SSD1306_send_com(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
  	SSD1306_send_com(0x00);

  	SSD1306_send_com(start);
    SSD1306_send_com(0X00);
  	SSD1306_send_com(stop);

  	SSD1306_send_com(0x01);
  	SSD1306_send_com(SSD1306_ACTIVATE_SCROLL);
}

/*!
    @brief  Activate alternate diagonal scroll for all or part of the display.
    @param  start
            First row.
    @param  stop
            Last row.
    @return None (void).
*/
/* To scroll the whole display, run: display.startscrolldiagleft(0x00, 0x0F) */
void SSD1306_start_scroll_diagleft(uint8_t start, uint8_t stop)
{
	SSD1306_send_com(SSD1306_SET_VERTICAL_SCROLL_AREA);
	SSD1306_send_com(0x00);
	SSD1306_send_com(SSD1306_HEIGHT);

	SSD1306_send_com(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
	SSD1306_send_com(0x00);

    SSD1306_send_com(start);
    SSD1306_send_com(0X00);
    SSD1306_send_com(stop);

  	SSD1306_send_com(0x01);
  	SSD1306_send_com(SSD1306_ACTIVATE_SCROLL);
}

/*!
    @brief  Cease a previously-begun scrolling action.
    @return None (void).
*/
void SSD1306_stop_scroll(void)
{
	SSD1306_send_com(SSD1306_DEACTIVATE_SCROLL);
}

/*!
    @brief  Enable or disable display invert mode (white-on-black vs
            black-on-white).
    @param  i
            If true, switch to invert mode (black-on-white), else normal
            mode (white-on-black).
    @return None (void).
    @note   This has an immediate effect on the display, no need to call the
            display() function -- buffer contents are not changed, rather a
            different pixel mode of the display hardware is used. When
            enabled, drawing SSD1306_BLACK (value 0) pixels will actually draw
            white, SSD1306_WHITE (value 1) will draw black.
*/
void SSD1306_display_invert(bool i)
{
	SSD1306_send_com(i ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

/*!
    @brief  Dim the display.
    @param  dim
            true to enable lower brightness mode, false for full brightness.
    @return None (void).
    @note   This has an immediate effect on the display, no need to call the
            display() function -- buffer contents are not changed.
*/
void SSD1306_set_contrast(uint8_t contrast)
{
    /*
     * The range of contrast to too small to be really useful
     * it is useful to dim the display
     */
    SSD1306_send_com(SSD1306_SETCONTRAST);
    SSD1306_send_com(contrast);
}

void SSD1306_set_rotation(uint8_t rot)
{
	rotation = rot;
}

uint8_t SSD1306_get_rotation(void)
{
	return rotation;
}
