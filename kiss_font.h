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

struct KissGlyph {
	int dest[ 4 ];
	int src[ 4 ];
	int pen_advance;
};

struct KissFont {
	int height;
	KissGlyph glyphs[256];
};

KISS_FONT_DEF KissFont* loadFont( const void* fontData );
KISS_FONT_DEF void freeFont( KissFont* font );

#ifdef __cplusplus
}
#endif

#endif // end KISS_FONT_H

#ifdef KISS_FONT_IMPLEMENTATION

KISS_FONT_DEF KissFont* loadFont( const void* fontData ) {

	KissFont* font = new KissFont();
	int* intData = (int*)fontData;

	int index = 0;
	font->height = intData[index++];

	for( int i = 0; i < 256; ++i ) {
		// left, top, width, height
		font->glyphs[i].dest[0] = intData[index++];
		font->glyphs[i].dest[1] = intData[index++];
		font->glyphs[i].dest[2] = intData[index++];
		font->glyphs[i].dest[3] = intData[index++];
		// tx0, ty0, tx1, ty1
		font->glyphs[i].src[0] = intData[index++];
		font->glyphs[i].src[1] = intData[index++];
		font->glyphs[i].src[2] = intData[index++];
		font->glyphs[i].src[3] = intData[index++];
		
		font->glyphs[i].pen_advance = intData[index++];
	}

	return font;

}

KISS_FONT_DEF void freeFont( KissFont* font ) {
	delete font;
}

#endif // end SQUTIL_IMPLEMENTATION


