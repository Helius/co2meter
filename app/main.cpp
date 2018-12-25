#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <uart.h>
#include <SSD1306.h>


class SSegmentRender {

public:
	SSegmentRender(uint8_t width, uint8_t pages)
		: width(width), pages(pages) {}

	SSegmentRender() = delete;

	int draw(int s, uint8_t width, uint8_t pages, uint8_t * b) {
		memset(b, 0, width * pages);
		const uint8_t c = d2s[s];
		const uint8_t top = pages*2;
		const uint8_t h = pages * 8 - top;
		const uint8_t half = h/2;

		if(c&0b10000000) {
			width = width/2;
		}
		
		if(c&0b00000001) {
			drawLineH(b, 0, 0, width);
		}
		if(c&0b00000010) {
			drawLineV(b, width-1, 0, half);
		}
		if(c&0b00000100) {
			drawLineV(b, width-1, half, half);
		}
		if(c&0b00001000) {
			drawLineH(b, 0, h-1, width);
		}
		if(c&0b00010000) {
			drawLineV(b, 0, h/2, h/2);
		}
		if(c&0b00100000) {
			drawLineV(b, 0, 0, h/2);
		}
		if(c&0b01000000) {
			drawLineH(b, 0, h/2, width);
		}
		return width;
	}

private:
	uint8_t width = 0;
	uint8_t pages = 0;
/*
	 Segment codes
   ***** <- 0

	 **0** <- top
	 *   *
	 5   1
	 **6**
	 *   *
	 4   2
	 **3** <- width-1
 */
 const uint8_t d2s[15] = {
		0b00111111, // 0
		0b10000110, // 1
		0b01011011, // 2
		0b01001111, // 3
		0b01100110, // 4
		0b01101101, // 5
		0b01111101, // 6
		0b00000111, // 7
		0b01111111, // 8
		0b01101111, // 9
		0b11000000, // - (10)
		0b01110110, // H (11)
		0b00111001, // C (12)
		0b00111110, // U (13)
		0b01110011, // P (14)
	};

 void drawLineH(uint8_t * b, uint8_t x, uint8_t y, uint8_t len) {
		uint8_t pattern = 1;
		pattern <<= y%8;
		for(int i = 0; i < len; ++i) {
			b[(y/8)*width + x + i] |= pattern;
		}
 }

	void drawLineV(uint8_t * b, uint8_t x, uint8_t y, uint8_t len) {

		int startPage = y/8;
		int startPageYOffset = y%8;

		uint8_t pattern = 1;
		for(int j = 1; j < len; ++j) {
			pattern <<= 1;
			pattern += 1;
		}

		while(startPageYOffset--) {
			pattern <<= 1;
		}
		
		b[x + startPage * width] |= pattern;
		// remove drawed part from len

		if(len >= 8-(y%8)) {
			len -= 8-(y%8);
		} else {
			return;
		}
		
		int k = 0;
		while(len > 8) {
			k++;
			len -= 8;
			b[(startPage + k)*width + x] = 0xFF;
		}


		if(len == 0) {
			return;
		}

		pattern = 1;
		while(len-- > 1) {
			pattern <<= 1;
			pattern |= 1;
		}
		b[(startPage+k+1)*width + x] |= pattern;
	}
};


class NumberPrinter {
	public:
		NumberPrinter() = delete;
		NumberPrinter(SSD1306 & oled, uint8_t width, uint8_t pages)
			: width(width), pages(pages), oled(oled), render(SSegmentRender(width, pages))
		{
			buf = static_cast<uint8_t*>(malloc(width*pages));
		}
		~NumberPrinter() {
			free(buf);
		}
		int print(int s, int offset, int startPage) {
			memset(buf, 0, width*pages);
			int glyphWidth = render.draw(s, width, pages, buf);
			for (int i = 0; i < pages; ++i) {
				oled.drawPage(buf + i * width, startPage + i, offset, glyphWidth);
			}

			return glyphWidth;
		}
		uint8_t width;
		uint8_t pages;

	private:
		SSD1306 & oled;
		SSegmentRender render;
		uint8_t * buf = nullptr;
};

class NumberStr {
	public:
		NumberStr() = delete;
		NumberStr(NumberPrinter & p, NumberPrinter & l, uint8_t x, uint8_t topPage, uint8_t width)
			: printer(p), label(l), x(x), topPage(topPage), width(width)
		{}
			
			void setNumber(int16_t value, uint8_t lblChar) 
			{
				constexpr int len = 9;
				uint8_t str[len];
				uint8_t x_offset = 0;
				
				sprintf((char*)str, "%d", value);

				for(int i = 0; i < len; ++i) {
					int c;
					if(str[i] == 0) {
						break;
					}
					if(str[i] >= 0x30 && str[i] <= 0x39) {
						c = str[i] - 0x30;
					} else {
						c = 10; // symbol of '-'
					}
					x_offset += printChar(c, x_offset);
				}
				label.print(lblChar, x + x_offset + printer.width/4, topPage + printer.pages - label.pages);
			}
	private:
			int	printChar(int c, int offset) {
				int charWidth = printer.print(c, offset + x, topPage);
				int distance = printer.width/3;
				if(distance < 2) {
					distance = 2;
				}
				return charWidth + distance;
			}
			NumberPrinter & printer;
			NumberPrinter & label;
			uint8_t x;
			uint8_t topPage;
			uint8_t width;
};


class DataArray {
public:
	DataArray ()
	{
		memset(data, 0, width*row);
	}

	void addValue(uint8_t v0, uint8_t v1, uint8_t v2)
	{
		data[cur % width][0] = v0;
		data[cur % width][1] = v1;
		data[cur % width][2] = v2;
		++cur;
	}

	uint8_t getMax(int row)
	{
		uint8_t max = 0;
		for(int i = 0; i < 128; ++i) {
			if(data[i][row] > max) {
				max = data[i][row];
			}
		}
		return max;
	}
	
	uint8_t getMin(int row)
	{
		uint8_t min = 0xFF;
		for(int i = 0; i < 128; ++i) {
			if(data[i][row] < min) {
				min = data[i][row];
			}
		}
		return min;
	}

	uint8_t getLast(int row, uint8_t offset)
	{
		uint16_t ind = cur+offset;
		return data[ind % width][row];
	}

private:
	static constexpr uint8_t width = 128;
	static constexpr uint8_t row = 3;
	uint8_t cur = 0;
	uint8_t data[width][row];
};


class ChartWidget
{

public:

	ChartWidget(SSD1306 & oled, DataArray & arr)
		: oled(oled)
		, arr(arr)
	{}

	void draw()
	{
		for(int i = 0; i < 128; ++i) {
			drawColumn(i);
		}
	}

	void drawColumn(uint8_t ind)
	{
		constexpr uint8_t pages = 7;
		uint8_t pb[pages];
		memset(pb, 0, pages);

		for(int r = 0; r < 3; ++r) {
			if((r == 1) && (ind & 1)) {
				continue;
			}

			uint8_t max = arr.getMax(r);
			uint8_t min = arr.getMin(r);
			if(max == min) {
				max++;
				if(min>0) {
					min--;
				}

			}
			uint8_t val = arr.getLast(r, ind);
			uint16_t pt = ((val - min) * (pages*8-1))/(max-min);


			for(uint8_t i = 0; i < pages; ++i) {
				if( (pt >= i * 8) && (pt < static_cast<uint8_t>(((i+1)*8))) ) {
					uint16_t pattern = 0b10000000;
					pattern >>= pt - i * 8;
					pb[pages - i - 1] |= pattern;
				}
			}
		}

		for(int i = 0; i < pages; ++i) {
			oled.drawPage(pb + i, 1 + i, ind, 1);
		}
	}

private:
	SSD1306 & oled;
	DataArray & arr;
};


class IScreen {
	public:
		virtual void draw() = 0;
};

class MainScreen
	: public IScreen
{
	public:
		MainScreen(SSD1306 & oled) 
			: pSmall(NumberPrinter(oled, 12, 4))
			, pBig(NumberPrinter(oled, 12, 4))
			, pLbl(NumberPrinter(oled, 7, 2))
			, str0(NumberStr(pBig, pLbl, 0, 0, 64))
			, str1(NumberStr(pLbl, pLbl, 90, 0, 32))
			, str2(NumberStr(pSmall, pLbl, 0, 4, 64))
			, str3(NumberStr(pSmall, pLbl, 64, 4, 64))
		{
		}

		void draw() override {
				str0.setNumber(740, 14);
				str1.setNumber(420, 13);
				str2.setNumber(-32, 12);
				str3.setNumber(79, 11);
		};
	private:
		NumberPrinter pSmall;
		NumberPrinter pBig;
		NumberPrinter pLbl;

		NumberStr str0;
		NumberStr str1;
		NumberStr str2;
		NumberStr str3;
};

class ChartScreen
	: public IScreen
{
public:
	ChartScreen(SSD1306 & oled, DataArray & arr) 
		: pSmall(NumberPrinter(oled, 4, 1))
		, str0(NumberStr(pSmall, pSmall, 0, 0, 32))
		, str1(NumberStr(pSmall, pSmall, 32, 0, 32))
		, str2(NumberStr(pSmall, pSmall, 64, 0, 32))
		, str3(NumberStr(pSmall, pSmall, 96, 0, 32))
		, chart(ChartWidget(oled, arr))
	{
	}
	void draw() override {
		str0.setNumber(-12, 12);
		str1.setNumber(79, 11);
		str2.setNumber(740, 14);
		str3.setNumber(410, 13);
		chart.draw();
	};
private:
		NumberPrinter pSmall;
		NumberStr str0;
		NumberStr str1;
		NumberStr str2;
		NumberStr str3;
		ChartWidget chart;
};




SSD1306 oled;
DataArray arr;

IScreen * screens[2];
MainScreen mainScreen(oled);
ChartScreen chartScreen(oled, arr);


void toggleLed(void)
{
	// toggle pin
	PORTB ^= (1 << PIN5);
}


void uart_rx_handler(unsigned char ch)
{
	toggleLed();
}

int main(void)
{
	int screenIndex = 1;
	int cnt = 0;
	cnt++;

	screens[0] = static_cast<IScreen*>(&mainScreen);
	screens[1] = static_cast<IScreen*>(&chartScreen);

	toggleLed();
	_delay_ms(100);
	PORTB = 0;

	oled.init();
	oled.clear();

	_delay_ms(100);

	for(int i = 0; i < 128; ++i) {
		double rad = (i*10.0)/128;
		double v = 100 + 100 * sin(rad);
		double v1 = 100 + 100 * cos(rad*0.4);
		arr.addValue(v, i, v1);
	}

	
	while(1) {
		
		oled.clear();
		screens[(++screenIndex)%2]->draw();
		//screens[(screenIndex)%2]->draw();

		PORTB ^= (1 << PIN5);
		_delay_ms(4000);
	}
}

