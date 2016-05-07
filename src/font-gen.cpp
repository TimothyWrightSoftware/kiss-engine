// going to rework this sucker.  Need a tool to generate
// the bitmap font, and a library for using it, so I don't
// have to keep worrying about this crap.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
 
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BBOX_H

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

const int GLYPH_COUNT = 256;
const int IMG_WIDTH = 512;
const int IMG_HEIGHT = 512;

struct FontGlyph {

	int rx, ry, rw, rh;
	int tx, ty, tw, th;

	int pen_advance;
};

struct FontFace {
	FT_Library library;
	FT_Face face;
	const char* path;
	int point_size;
	int font_height;
	FontGlyph glyphs[GLYPH_COUNT];
	float max_width;
	float max_height;
	float left_min;
	float top_max;
	float pen_offset;
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

	font.font_height = font.face->size->metrics.height;

	return FT_SUCCESS;
}

void drawGlyphToImage( unsigned char* dest, int dest_width, int dest_height, FT_Bitmap* bitmap, int x, int y ) {

	for( unsigned int row = 0; row < bitmap->rows; ++row ) {
		for( unsigned int col = 0; col < bitmap->width; ++col ) {
			if( bitmap->buffer[ row * bitmap->width + col ] != 0 ) {
				int srcRow = row + y;
				int srcCol = col + x;
				unsigned char alpha = bitmap->buffer[ row * bitmap->width + col ];
				if( (srcRow * dest_width + srcCol) >= (dest_width * dest_height)  )
					continue;
				dest[ srcRow * dest_width + srcCol ] = alpha;
			}
		}	
	}	
}

void generateBitmapFont( FontFace& ff, unsigned char* texture, int texture_width, int texture_height ) {

	if( initialize_font( ff ) == FT_SUCCESS ) {
		cout << "WooHoo - made the font thingy" << endl;
	}
	FT_Face face = ff.face;
	FT_GlyphSlot  slot = ff.face->glyph;  
	
	int max_advance = 0;
	int max_index = 0;
	int left = 0;
	int advance_x = 0;
	int leftmin = 0;
	int topmax = std::numeric_limits<int>::min();   
	int rightmax = std::numeric_limits<int>::min();
	int bottommax = std::numeric_limits<int>::min();
	int max_width = 0;
	int max_height = 0;
	int top_pad = 3;

	int char_start = 0;
	int char_end = GLYPH_COUNT;	

	for( int i = char_start; i < char_end; ++i ) {
		
		FT_UInt glyph_index = FT_Get_Char_Index( face, i );

		FT_Int32 flags = FT_LOAD_DEFAULT;
		flags |= FT_LOAD_FORCE_AUTOHINT;
		FT_Load_Glyph( face, glyph_index, flags );
		FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );

		int true_advance = face->glyph->advance.x;
		if( true_advance > max_advance ) {
			max_advance = true_advance;
			max_index = i;
			left = face->glyph->bitmap_left;
			advance_x = face->glyph->advance.x;
		}
		leftmin = MIN( left, face->glyph->bitmap_left );
		topmax = MAX( topmax, face->glyph->bitmap_top );
		int right = left + face->glyph->bitmap.width;
		rightmax = MAX( rightmax, right );
		int bottom = -face->glyph->bitmap_top + face->glyph->bitmap.rows;
		bottommax = MAX( bottommax, bottom );

	}	

	int pen_x = 0 - (leftmin * 64);
	int pen_y = ff.font_height;

	max_width = rightmax - leftmin;
	max_height = topmax + bottommax;
	ff.max_width = max_width;
	ff.max_height = max_height;
	ff.left_min = leftmin;
	ff.top_max = topmax;
	ff.pen_offset = topmax;

	for( int i = char_start; i < char_end; ++i ) {

		FT_UInt glyph_index = FT_Get_Char_Index( face, i );

		int error = 0;
		FT_Int32 flags = FT_LOAD_DEFAULT;
		flags |= FT_LOAD_FORCE_AUTOHINT;
		error = FT_Load_Glyph( face, glyph_index, flags );
		if ( error )
			continue;  

		error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );

		if ( error )
			continue;

		int w_offset = 0;
		int h_offset = 0;
		if( ((pen_x + max_advance) >> 6) > texture_width - w_offset ) {
			pen_x = 0 - (leftmin * 64);
			pen_y += (top_pad + max_height) * 64;
			if( (pen_y >> 6) + max_height > texture_height - h_offset )
				continue;
		} 

		int bx = slot->bitmap.width;
		int mx =  max_width;
		int x = ( mx - bx ) / 2;
		int by = slot->bitmap.rows;
		int my = max_height;
		int y = ( my - by ) / 2;

		int pen_x_offset = x - slot->bitmap_left;
		int pen_y_offset = topmax - y - slot->bitmap_top;
		int real_pen_x = (pen_x >> 6) + pen_x_offset;
		int real_pen_y = (pen_y >> 6) - pen_y_offset;

		ff.glyphs[i].rx = leftmin - pen_x_offset;
		ff.glyphs[i].ry = topmax - pen_y_offset;
		ff.glyphs[i].rw = max_width;
		ff.glyphs[i].rh = max_height;
		ff.glyphs[i].pen_advance = slot->advance.x;

		ff.glyphs[i].tx = real_pen_x + ff.glyphs[i].rx;
		ff.glyphs[i].ty = real_pen_y - ff.glyphs[i].ry;
		ff.glyphs[i].tw = ff.glyphs[i].rw;
		ff.glyphs[i].th = ff.glyphs[i].rh;	
		
		drawGlyphToImage( texture, texture_width, texture_height, &slot->bitmap,
				(pen_x >> 6) + x,
				(pen_y >> 6) - (topmax - y ));

		pen_x += max_advance;
	}

}

void generateBitmapFont( FontFace& ff, string path ) {
	int width = IMG_WIDTH;
	int height = IMG_HEIGHT;
	unsigned char* texture = new unsigned char[width * height];
	generateBitmapFont( ff, texture, width, height );
	char* img = new char[width * height * 4];
	for( int i = 0; i < width*height; ++i ) {
		img[i*4 + 0] = 255;
		img[i*4 + 1] = 255;
		img[i*4 + 2] = 255;
		img[i*4 + 3] = texture[i];
	}
	stbi_write_png(path.c_str(), width, height, 4, img, width * 4);
	delete[] img;
	delete[] texture;
}

void generateDataFile( FontFace& ff, string path ) {
	ofstream binfile;
	binfile.open( path, ios::out | ios::binary);
	int height = (int)ff.max_height;
	binfile.write( (char*)(&height), sizeof(int) );
	for( int i = 0; i < GLYPH_COUNT; ++i  ) {
		auto& glyph = ff.glyphs[i];

		// pen offsets
		binfile.write( (char*)(&glyph.rx), sizeof(float));
		binfile.write( (char*)(&glyph.ry), sizeof(float));
		binfile.write( (char*)(&glyph.rw), sizeof(float));
		binfile.write( (char*)(&glyph.rh), sizeof(float));

		// text coords
		binfile.write( (char*)(&glyph.tx), sizeof(float));
		binfile.write( (char*)(&glyph.ty), sizeof(float));
		binfile.write( (char*)(&glyph.tw), sizeof(float));
		binfile.write( (char*)(&glyph.th), sizeof(float));

		// pen advance
		int adv = glyph.pen_advance >> 6;
		binfile.write( (char*)(&adv), sizeof(int) );
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

	return 0;

}
