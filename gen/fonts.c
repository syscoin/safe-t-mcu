#include "fonts.h"

const uint8_t * const font_data[2][256] = {
	{
#include"font.inc"
	},
	{
#include"fontfixed.inc"
	},
};

int fontCharWidth(int font, char c) {
	return font_data[font][c & 0xff][0];
}

const uint8_t *fontCharData(int font, char c) {
	return font_data[font][c & 0xff] + 1;
}
