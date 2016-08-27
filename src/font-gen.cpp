// going to rework this sucker.  Need a tool to generate
// the bitmap font, and a library for using it, so I don't
// have to keep worrying about this crap.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define KISS_FONT_IMPLEMENTATION
#include "kiss_font.h"
 
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BBOX_H

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

const int GLYPH_COUNT = 512;
const int IMG_WIDTH = 512;
const int IMG_HEIGHT = 128;

struct FontGlyph {

	int src_x, src_y;
	int width, height;

	int pen_x_offset;
	int pen_y_offset;
	int pen_advance;
};

struct FontFace {
	FT_Library library;
	FT_Face face;
	const char* path;
	int point_size;
	int font_height;
	FontGlyph glyphs[GLYPH_COUNT];
};

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

const int FT_SUCCESS = 0;

int initialize_font( FontFace& font ) {

	int error = FT_Init_FreeType( &font.library );
	if ( error ) {  
		return error;
	}

	error = FT_New_Face( font.library, font.path, 0, &font.face );
	if ( error ) {
		return error;
	}

	error = FT_Set_Char_Size(
			font.face,    /* handle to face object           */
			0,       /* char_width in 1/64th of points  */
			font.point_size*64,   /* char_height in 1/64th of points */
			0,     /* horizontal device resolution    */
			0 );   /* vertical device resolution      */

	if( error ) {
		return error;
	}

	font.font_height = font.face->size->metrics.height >> 6;

	return FT_SUCCESS;
}

void drawGlyphToImage( unsigned char* dest, int dest_width, int dest_height, FT_Bitmap* bitmap, int x, int y ) {

	for( unsigned int row = 0; row < bitmap->rows; ++row ) {
		for( unsigned int col = 0; col < bitmap->width; ++col ) {
			if( bitmap->buffer[ row * bitmap->width + col ] != 0 ) {
				int srcRow = row + y;
				int srcCol = col + x;
				unsigned char alpha = bitmap->buffer[ row * bitmap->width + col ];
				int index = srcRow * dest_width + srcCol;
				if( index < 0 || (index >= (dest_width * dest_height))  )
					continue;
				dest[ srcRow * dest_width + srcCol ] = alpha;
			}
		}	
	}	
}

void draw_hline(unsigned char* dest, int dest_width, int dest_height, int y, int x0, int x1, unsigned char c) {
	for (int i = x0; i <= x1; ++i) {
		int line = y * dest_width + i;
		if (line >= 0 && line < dest_width * dest_height)
			dest[y * dest_width + i] = c;
	}
}

void drawBoundingBox(unsigned char* dest, int dest_width, int dest_height, int x0, int y0, int x1, int y1, unsigned char c) {
	printf("(%d, %d, %d, %d)\n", x0, y0, x1, y1);
	for (int i = x0; i <= x1; ++i) {
		int top = y0 * dest_width + i;
		if(top >= 0 && top < dest_width * dest_height)
			dest[y0 * dest_width + i] = c;
		int bot = y1 * dest_width + i;
		if(bot >= 0 && bot < dest_width * dest_height)
			dest[y1 * dest_width + i] = c;
	}

	for (int i = y0; i <= y1; ++i) {
		int left = i * dest_width + x0;
		if(left >= 0 && left < dest_width * dest_height)
			dest[i * dest_width + x0] = c;
		int right = i * dest_width + x1;
		if(right >= 0 && right < dest_width * dest_height)
			dest[i * dest_width + x1] = c;
	}
}

void generateBitmapFont( FontFace& ff, unsigned char* texture, int texture_width, int texture_height ) {

	if( initialize_font( ff ) == FT_SUCCESS ) {
		cout << "WooHoo - made the font thingy" << endl;
	}
	FT_Face face = ff.face;
	FT_GlyphSlot  glyph = ff.face->glyph;  

	int newline = ff.font_height + 1;
	int pen_x = 1;
	int pen_y = newline;

	for( int i = 0; i < 256; ++i ) {

		FT_UInt glyph_index = FT_Get_Char_Index( face, i );

		int error = 0;
		FT_Int32 flags = FT_LOAD_DEFAULT;
		flags |= FT_LOAD_FORCE_AUTOHINT;
		error = FT_Load_Glyph( face, glyph_index, flags );
		if ( error )
			continue;  

		error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if ( error )
			continue;

		int pen_advance = (glyph->advance.x >> 6);
		if (pen_x + pen_advance + 1 >= texture_width) {
			pen_x = 1;
			pen_y += newline;
		}

		int x_offset = glyph->bitmap_left;
		int y_offset = -glyph->bitmap_top;
		int width = glyph->bitmap.width;
		int height = glyph->bitmap.rows;

		int top = pen_y + y_offset;
		int left = pen_x + x_offset;
		int bottom = top + height;
		int right = left + width;

		FontGlyph* g = &ff.glyphs[i];
		g->src_x = left;
		g->src_y = top;
		g->width = width;
		g->height = height;
		g->pen_x_offset = x_offset;
		g->pen_y_offset = y_offset;
		g->pen_advance = pen_advance;

		if (bottom >= texture_height)
			continue;

		drawGlyphToImage(texture, texture_width, texture_height, &glyph->bitmap, left, top);
		pen_x += pen_advance + 1;
	}

}

void generateBitmapFont( FontFace& ff, string path ) {
	int width = IMG_WIDTH;
	int height = IMG_HEIGHT;
	unsigned char* texture = new unsigned char[width * height];
	memset(texture, 0, width*height);
	generateBitmapFont( ff, texture, width, height );
	char* img = new char[width * height * 4];
	for( int i = 0; i < width*height; ++i ) {
		img[i * 4 + 0] = texture[i];
		img[i * 4 + 1] = texture[i];
		img[i * 4 + 2] = texture[i];
		img[i * 4 + 3] = texture[i];
	}
	printf("Generate img: %d, %d, %d\n", width, height, width * 4);
	stbi_write_png(path.c_str(), width, height, 4, img, width * 4);
	delete[] img;
	delete[] texture;
}

void generateDataFile( FontFace& ff, string path ) {
	ofstream binfile;
	binfile.open( path, ios::out | ios::binary);
	int img_width = IMG_WIDTH;
	binfile.write((char*)(&img_width), sizeof(int));
	int img_height = IMG_HEIGHT;
	binfile.write((char*)(&img_height), sizeof(int));
	int height = (int)ff.font_height;
	binfile.write( (char*)(&height), sizeof(int) );
	for( int i = 0; i < GLYPH_COUNT; ++i  ) {
		auto& glyph = ff.glyphs[i];

		binfile.write( (char*)(&glyph.src_x), sizeof(int));
		binfile.write( (char*)(&glyph.src_y), sizeof(int));
		binfile.write( (char*)(&glyph.width), sizeof(int));
		binfile.write( (char*)(&glyph.height), sizeof(int));

		binfile.write( (char*)(&glyph.pen_x_offset), sizeof(int));
		binfile.write( (char*)(&glyph.pen_y_offset), sizeof(int));
		binfile.write( (char*)(&glyph.pen_advance), sizeof(int));
	}

	//@TODO - also need kerning info here.
	//Not sure how that works
	binfile.close();
	
}

string createPNGPath( char* path, int point ) {
	string str(path);
	return str.substr( 0, str.find_last_of(".") ) + to_string(point) + ".png";
}

string createBINPath( char* path, int point ) {
	string str(path);
	return str.substr( 0, str.find_last_of(".") ) + to_string(point) + ".bin";
}

void testFontFile(string pngPath, string binPath) {
	
	void* data = 0;
	{ // load in the binary font file
		FILE* file = fopen(binPath.c_str(), "rb");
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		data = calloc(file_size, sizeof(char));
		fread(data, file_size, 1, file);
		fclose(file);
	}
	KissFont* font = loadFont(data);
	free(data);

	int x,y,n;
	unsigned int *img = (unsigned int *)stbi_load(pngPath.c_str(), &x, &y, &n, 4);

	// lets test out the data...
	int pen_x = 20;
	int pen_y = 420;
	const char* str = "The quick brown fox jumped over the lazy river. int* x[] = {1,2,3};"
		"0123456789 ~!@#$%^&*()_+-=`[]\;',./{}|:\"<>?";
	for (int i = 0; i < strlen(str); ++i) {
		char c = str[i];
		KissGlyph* glyph = &font->glyphs[(int)c];

		int srcx = glyph->src_x;
		int srcy = glyph->src_y;
		int srcw = glyph->width;
		int srch = glyph->height;

		int x_offset = glyph->pen_x_offset;
		int y_offset = glyph->pen_y_offset;
		int advance = glyph->pen_advance;

		int destx = pen_x + x_offset;
		if (destx + srcw >= x) {
			pen_x = 20;
			destx = pen_x + x_offset;
			pen_y += font->newline;
		}
		int desty = pen_y + y_offset;

		// copy pixels from src to dest...
		for (int row = srcy, drow = desty; row <= (srcy + srch); ++row, ++drow) {
			for (int col = srcx, dcol = destx; col <= (srcx + srcw); ++col, ++dcol) {
				int src = row * x + col;
				int dest = drow * x + dcol;
				img[dest] = img[src];
			}
		}

		pen_x += advance;
	}

	// free all the memories
	stbi_write_png(pngPath.c_str(), x, y, 4, img, x * 4);
	stbi_image_free(img);
	freeFont(font);
}

int main( int argc, char* argv[] ) {

	cout << "Welcome to the bitmap font tool!" << endl;

	if( argc != 3 ) {
		cout << "argc: " << argc << endl;
		cout << "Really?  fontfile.ttf 14 -> is that so hard?" << endl;
		cout << argv[0] << endl;
		return -1;
	}

	char* file = argv[1];
	int point = atoi(argv[2]);
	cout << "font path: " << file << endl;
	string pngFile = createPNGPath( file, point );
	cout << "png path: " << pngFile << endl;
	string binFile = createBINPath( file, point );
	cout << "bin path: " << binFile << endl;

	FontFace ff;
	ff.path = file;
	ff.point_size = point;

	generateBitmapFont( ff, pngFile );
	generateDataFile( ff, binFile );
	//testFontFile(pngFile, binFile);

	return 0;

}
