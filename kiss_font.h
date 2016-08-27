/*
* kiss_font - version 0.1
*
* This is a utility to create a bitmap font in memory using
* a font file and the FreeType library.
*/

#ifndef KISS_FONT_H
#define KISS_FONT_H

#ifdef KISS_FONT_STATIC
#define KISS_FONT_DEF static
#else
#define KISS_FONT_DEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

	// texture source is {src_x, src_y, width, height}
	// destination pen_x, pen_y are baseline coords.
	// destination is {pen_x + pen_x_offset, pen_y + pen_y_offset, width, height}
	// pen_x += pen_advance;
	// for newline, pen_y += newline;
	struct KissGlyph {
		int src_x, src_y;
		int width, height;
		float tx0, ty0, tx1, ty1;

		int pen_x_offset;
		int pen_y_offset;
		int pen_advance;
	};

	struct KissFont {
		int img_width;
		int img_height;
		int newline;
		KissGlyph glyphs[256];
	};

	KISS_FONT_DEF KissFont* loadFont(const void* fontData);
	KISS_FONT_DEF void freeFont(KissFont* font);

#ifdef __cplusplus
}
#endif

#endif // end KISS_FONT_H

#ifdef KISS_FONT_IMPLEMENTATION

KISS_FONT_DEF KissFont* loadFont(const void* fontData) {

	KissFont* font = new KissFont();
	int* intData = (int*)fontData;

	int index = 0;
	font->img_width = intData[index++];
	font->img_height = intData[index++];
	font->newline = intData[index++];

	for (int i = 0; i < 256; ++i) {
		font->glyphs[i].src_x = intData[index++];
		font->glyphs[i].src_y = intData[index++];
		font->glyphs[i].width = intData[index++];
		font->glyphs[i].height = intData[index++];
		font->glyphs[i].pen_x_offset = intData[index++];
		font->glyphs[i].pen_y_offset = intData[index++];
		font->glyphs[i].pen_advance = intData[index++];

		font->glyphs[i].tx0 = (float)font->glyphs[i].src_x / (float)font->img_width;
		font->glyphs[i].ty0 = 1.0f - (float)font->glyphs[i].src_y / (float)font->img_height;
		font->glyphs[i].tx1 = font->glyphs[i].tx0 + ((float)font->glyphs[i].width / (float)font->img_width);
		font->glyphs[i].ty1 = font->glyphs[i].ty0 - ((float)font->glyphs[i].height / (float)font->img_height);
	}

	return font;

}

KISS_FONT_DEF void freeFont(KissFont* font) {
	delete font;
}

#endif // end SQUTIL_IMPLEMENTATION


