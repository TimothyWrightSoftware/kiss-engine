/*
 * glutil - version 0.1
 *
 */

#ifndef GLUTIL_H
#define GLUTIL_H

#ifdef GLUTIL_STATIC
#define GLUTIL_DEF static
#else
#define GLUTIL_DEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "GL/glew.h"
static GLenum error;

#define GL_ERROR() 		error = glGetError(); \
	if (error != GL_NO_ERROR) { \
		debug("error %s", error_enum(error)); \
		SDL_assert(!"Got a funcking error"); \
	} \


class GLPushPop {
public:
	GLPushPop(const char* name, const char* file, int line) {
		char message[1024];
		sprintf_s(message, 1024, "[%s]%s: %d\n", name, file, line);
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 42, strlen(message), message);
	}
	~GLPushPop() {
		glPopDebugGroup();
	}
};
#define GLLOG(name) GLPushPop glpp(name, __FILE__, __LINE__)

#define GLUTIL_CREATE_NEW	-1

typedef struct {
	float x;
	float y;
	float w;
	float h;
	kiss_vec4 color;
} GLRect;

GLUTIL_DEF void glutil_setup();
GLUTIL_DEF void glutil_clear_render_data();
GLUTIL_DEF GLuint compile_program( const char* vertex, const char* fragment );
GLUTIL_DEF GLuint compile_shader(GLenum type, const char* source);
GLUTIL_DEF void glutil_flip_vert(unsigned char* data, int width, int height);
GLUTIL_DEF void glutil_render_objects(kiss_mat4 *mvp);
GLUTIL_DEF void glutil_render_sprites(kiss_mat4 *mvp);
GLUTIL_DEF void glutil_render_text(kiss_mat4 *mvp);

GLUTIL_DEF void glutil_update_sprite_model(GLint sprite_id, kiss_mat4* model);
GLUTIL_DEF void glutil_update_object_model(GLint sprite_id, kiss_mat4* model);
GLUTIL_DEF void glutil_update_text_model(GLint obj_id, kiss_mat4* model);

GLUTIL_DEF GLint glutil_create_linesf(float* lines, GLint id = GLUTIL_CREATE_NEW);
GLUTIL_DEF GLint glutil_create_texture(unsigned char* data, int w, int h, bool flip_vert = true);
GLUTIL_DEF GLint glutil_create_sprite(GLRect* src, GLRect* dest, GLuint texture_id, GLint sprite_id = GLUTIL_CREATE_NEW);
GLUTIL_DEF GLint glutil_create_string(const char* string, float x, float y, kiss_vec3* color = nullptr, GLint text_id = GLUTIL_CREATE_NEW);
GLUTIL_DEF GLint glutil_create_strings(char** strings, float x, float y, kiss_vec3* color = nullptr, GLint text_id = GLUTIL_CREATE_NEW);
GLUTIL_DEF GLint glutil_create_rects(GLRect* rects, GLuint count, GLint obj_id = GLUTIL_CREATE_NEW);
GLUTIL_DEF GLint glutil_create_quads(float* rects, bool fill, GLint obj_id = GLUTIL_CREATE_NEW);

GLUTIL_DEF void glutil_create_cube(kiss_vec3 *pos, float radius, kiss_vec4 *color);
GLUTIL_DEF void glutil_create_grid(kiss_vec3 *pos, kiss_vec3* normal, float width, float height, float divisions, kiss_vec4 *color);

#ifdef __cplusplus
}
#endif

#endif // end GLUTIL_H

#ifdef GLUTIL_IMPLEMENTATION

#include <stdio.h>
#include "kiss_dbg.h"
#define KISS_FONT_IMPLEMENTATION
#include "kiss_font.h"

typedef struct {
	kiss_mat4 model;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	GLenum elementMode;
	GLsizei elementCount;
	GLenum elementType;
	const GLvoid* elementOffset;
} GLObject;

typedef struct {
	kiss_mat4 model;
	GLint num;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	GLRect src;
	GLRect dest;
	GLenum el_mode;
	GLsizei el_count;
	GLenum el_type;
	const GLvoid* el_offset;
} GLSprite;

typedef struct {
	kiss_mat4 model;
	GLint num;
	kiss_vec3 color;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	GLRect src;
	GLRect dest;
	GLenum el_mode;
	GLsizei el_count;
	GLenum el_type;
	const GLvoid* el_offset;
} GLText;

//#define FONT_PNG "../../squirrel/res/Anonymous_Pro24.png"
//#define FONT_BIN "../../squirrel/res/Anonymous_Pro24.bin"
#define FONT_PNG "res/sansation18.png"
#define FONT_BIN "res/sansation18.bin"

static const char* glutil_vs_shader =
R"a(
#version 440

in vec4 position;
in vec4 color;
uniform mat4 MVP;
out vec4 colorFromShader;

void main() {
	gl_Position = MVP * position;
	colorFromShader = color;
}
)a";

static const char* glutil_fs_shader = R"a(
#version 440

in vec4 colorFromShader;
out vec4 outColor;

void main() {
	outColor = colorFromShader;
}
)a";

static const char* glutil_sprite_fs = R"a(
#version 440

in vec2 texCoord0;
out vec4 fragColor;
uniform sampler2D sampler;

void main() {
	fragColor = texture2D(sampler, texCoord0.st);
}
)a";

static const char* glutil_sprite_vs = R"a(
#version 440

layout(location = 0) in vec4 pos;
layout(location = 1) in vec2 texCoord;

uniform mat4 MVP;
out vec2 texCoord0;

void main() {

	gl_Position = MVP * pos;
	texCoord0 = texCoord;

	}
)a";

static const char* glutil_text_fs = R"aaa(
#version 440

in vec2 texCoord0;
out vec4 fragColor;
uniform sampler2D sampler;
uniform vec3 textColor;

void main() {
	float a = texture2D(sampler, texCoord0.st).a;
	fragColor = vec4(textColor * a, a);
}
)aaa";

static const char* glutil_text_vs = R"a(
#version 440

layout(location = 0) in vec4 pos;
layout(location = 1) in vec2 texCoord;

uniform mat4 MVP;
out vec2 texCoord0;

void main() {

	gl_Position = MVP * pos;
	texCoord0 = texCoord;

	}
)a";

static GLint glutil_program_id;
static GLint sprite_program_id;
static GLint text_program_id;

static GLuint* tbo_list;
static GLObject *render_objs;
static GLSprite *sprite_objs;
static GLText *text_objs;

static KissFont* font;
static GLint font_tex;

GLUTIL_DEF GLuint compile_shader( GLenum type, const char* source ) {

	GLuint shader = glCreateShader( type );
 	glShaderSource(shader, 1, &source, NULL);
 	glCompileShader(shader);
 
	GLint isCompiled = 0;
 	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if( isCompiled == GL_FALSE ) {
		glDeleteShader(shader);
		GLint infoLogLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		if( infoLogLength > 0 ) {
			char* msg = (char*) calloc( infoLogLength, sizeof(char) );
			glGetShaderInfoLog(shader, infoLogLength, NULL, msg);
			printf("%s\n", msg);
			free(msg);
			return 0;
		}
	}
	return shader;
}

GLUTIL_DEF GLuint compile_program( const char* vertex, const char* fragment ) {

	GLuint vertex_shader = compile_shader( GL_VERTEX_SHADER, vertex );
	SDL_assert( vertex_shader != 0 );
	if( vertex_shader == 0 ) {
		return 0;
	}

	GLuint fragment_shader = compile_shader( GL_FRAGMENT_SHADER, fragment );
	SDL_assert( fragment_shader != 0 );
	if( fragment_shader == 0 ) {
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader( program, vertex_shader );
	glAttachShader( program, fragment_shader );
	glLinkProgram( program );

	GLint isLinked = 0;
	glGetProgramiv( program, GL_LINK_STATUS, (int *)&isLinked );
	if( isLinked == GL_FALSE ) {
		GLint infoLogLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		char* message = (char*)calloc( infoLogLength, sizeof(char) );
		glGetProgramInfoLog(program, infoLogLength, &infoLogLength, message);
		printf( "%s\n", message );
		free( message );
		
		glDeleteProgram(program);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
		return 0;
	}

	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);

	return program;
}

static const char* source_string(GLenum source) {
	switch (source) {
	case GL_DEBUG_SOURCE_API: return "GL_DEBUG_SOURCE_API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return "GL_DEBUG_SOURCE_SHADER_COMPILER";
	case GL_DEBUG_SOURCE_THIRD_PARTY: return "GL_DEBUG_SOURCE_THIRD_PARTY";
	case GL_DEBUG_SOURCE_APPLICATION: return "GL_DEBUG_SOURCE_APPLICATION";
	case GL_DEBUG_SOURCE_OTHER: return "GL_DEBUG_SOURCE_OTHER";
	default: return "UNKNOWN_SOURCE";
	}
}

static const char* severity_string(GLenum severity) {
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH: return "GL_DEBUG_SEVERITY_HIGH";
	case GL_DEBUG_SEVERITY_MEDIUM: return "GL_DEBUG_SEVERITY_MEDIUM";
	case GL_DEBUG_SEVERITY_LOW: return "GL_DEBUG_SEVERITY_LOW";
	case GL_DEBUG_SEVERITY_NOTIFICATION: return "GL_DEBUG_SEVERITY_NOTIFICATION";
	default: return "UNKNOWN_SEVERITY";
	}
}

static const char* type_string(GLenum type) {
	switch (type) {
	case GL_DEBUG_TYPE_ERROR: return "GL_DEBUG_TYPE_ERROR";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
	case GL_DEBUG_TYPE_PORTABILITY: return "GL_DEBUG_TYPE_PORTABILITY";
	case GL_DEBUG_TYPE_PERFORMANCE: return "GL_DEBUG_TYPE_PERFORMANCE";
	case GL_DEBUG_TYPE_MARKER: return "GL_DEBUG_TYPE_MARKER";
	case GL_DEBUG_TYPE_PUSH_GROUP: return "GL_DEBUG_TYPE_PUSH_GROUP";
	case GL_DEBUG_TYPE_POP_GROUP: return "GL_DEBUG_TYPE_POP_GROUP";
	case GL_DEBUG_TYPE_OTHER: return "GL_DEBUG_TYPE_OTHER";
	default: return "UNKNOWN_TYPE";
	}
}

static const char* error_enum(GLenum type) {
	switch (type) {
	case GL_NO_ERROR: return "GL_NO_ERROR";
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
	default: return "UNKNOWN_TYPE";
	}
}

static void APIENTRY debug_log(GLenum source, GLenum type, GLuint id,
	GLenum severity​, GLsizei length, const GLchar* message, const void* userParam) {

	printf("source: %s, severity: %s, type: %s, id: %d\n", source_string(source), severity_string(severity​), type_string(type), id);
	printf("message: %s\n", message);
}

GLUTIL_DEF void glutil_setup() {

	{ // setup debugging
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(debug_log, nullptr);
		glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PUSH_GROUP, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
		glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_POP_GROUP, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
		GLuint ids[] = { 131185 };
		glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 1, ids, GL_FALSE);
	}

	GLLOG("setup");
	
	{ // setup render objects
		glutil_program_id = compile_program(glutil_vs_shader, glutil_fs_shader);
		SDL_assert(glutil_program_id != 0);
	}

	{ // setup sprite objects
		sprite_program_id = compile_program(glutil_sprite_vs, glutil_sprite_fs);
		SDL_assert(sprite_program_id != 0);
	}

	{ // setup text objects
		text_program_id = compile_program(glutil_text_vs, glutil_text_fs);
		SDL_assert(text_program_id != 0);
	}

	{ // load font binary file
		FILE* file = fopen(FONT_BIN, "rb");
		SDL_assert(file != nullptr);
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char* font_data = (char*)calloc(file_size, sizeof(char));
		SDL_assert(font_data != nullptr);
		fread(font_data, file_size, 1, file);
		fclose(file);
		font = loadFont(font_data);
		free(font_data);
	}

	{ // load font texture
		int x, y, n;
		unsigned char *img_data = stbi_load(FONT_PNG, &x, &y, &n, 4);
		SDL_assert(img_data != nullptr);
		font_tex = glutil_create_texture(img_data, x, y);
		stbi_image_free(img_data);
	}

}

GLUTIL_DEF void glutil_clear_render_data() {
	{ // clear render objs
		for (int i = 0; i < sb_count(render_objs); ++i) {
			GLObject* obj = &render_objs[i];
			if (obj == 0) continue;
			glDeleteVertexArrays(1, &obj->vao);
			glDeleteBuffers(1, &obj->vbo);
			glDeleteBuffers(1, &obj->ebo);
		}
		sb_free(render_objs);
		render_objs = 0;
	}
	{ // clear sprite objs
		for (int i = 0; i < sb_count(sprite_objs); ++i) {
			GLSprite* obj = &sprite_objs[i];
			if (obj == 0) continue;
			glDeleteVertexArrays(1, &obj->vao);
			glDeleteBuffers(1, &obj->vbo);
			glDeleteBuffers(1, &obj->ebo);
		}
		sb_free(sprite_objs);
		sprite_objs = 0;
	}
	{ // clear text objs
		for (int i = 0; i < sb_count(text_objs); ++i) {
			GLText* obj = &text_objs[i];
			if (obj == 0) continue;
			glDeleteVertexArrays(1, &obj->vao);
			glDeleteBuffers(1, &obj->vbo);
			glDeleteBuffers(1, &obj->ebo);
		}
		sb_free(text_objs);
		text_objs = 0;
	}
}

GLUTIL_DEF void glutil_update_sprite_model(GLint sprite_id, kiss_mat4* model) {
	GLSprite* sprite = &sprite_objs[sprite_id];
	memcpy(sprite->model.m, model->m, sizeof(model->m));
}

GLUTIL_DEF void glutil_update_object_model(GLint obj_id, kiss_mat4* model) {
	GLObject* obj = &render_objs[obj_id];
	memcpy(obj->model.m, model->m, sizeof(model->m));
}

GLUTIL_DEF void glutil_update_text_model(GLint obj_id, kiss_mat4* model) {
	GLText* obj = &text_objs[obj_id];
	memcpy(obj->model.m, model->m, sizeof(model->m));
}

GLUTIL_DEF void glutil_flip_vert(unsigned char* data, int w, int h) {
	unsigned int* pixels = (unsigned int*)data;
	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h / 2; y++) {
			int A = y * w + x;
			int B = (h - y - 1) * w + x;
			int temp = pixels[A];
			pixels[A] = pixels[B];
			pixels[B] = temp;
		}
	}
}

GLUTIL_DEF GLint glutil_create_texture(unsigned char* data, int w, int h, bool flip_vert) {
	GLLOG("Create Texture");
	GLint active_texture = sb_count(tbo_list);
	sb_add(tbo_list, 1);

	glActiveTexture(GL_TEXTURE0 + active_texture);
	glGenTextures(1, &tbo_list[active_texture]);
	glBindTexture(GL_TEXTURE_2D, tbo_list[active_texture]);

	if (flip_vert) {
		glutil_flip_vert(data, w, h);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	return active_texture;
}

GLUTIL_DEF GLint glutil_create_sprite(GLRect* src, GLRect* dest, GLuint texture_id, GLint sprite_id) {

	GLLOG("Create Sprite");
	GLSprite* sprite = nullptr;
	GLint id = sprite_id;

	glUseProgram(sprite_program_id);

	if (id == GLUTIL_CREATE_NEW) {

		id = sb_count(sprite_objs);
		sprite = sb_add(sprite_objs, 1);
		
		sprite->model = KISS_MAT4();
		sprite->num = texture_id;

		glGenVertexArrays(1, &sprite->vao);
		glBindVertexArray(sprite->vao);

		// create/bind vertex buffer obj
		glGenBuffers(1, &sprite->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, sprite->vbo);

		// set vertex attrib pointers for vertex/color data
		GLint loc = glGetAttribLocation(sprite_program_id, "pos");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
		glEnableVertexAttribArray(loc);

		loc = glGetAttribLocation(sprite_program_id, "texCoord");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(loc);

		GLuint elements[] = {
			0, 1, 2, 3,
		};

		sprite->el_count = 4;//  sizeof(elements);
		debug("sizeof: %d", sizeof(elements));
		sprite->el_type = GL_UNSIGNED_INT;
		sprite->el_mode = GL_TRIANGLE_STRIP;
		sprite->el_offset = 0;

		glGenBuffers(1, &sprite->ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	} else {
		sprite = &sprite_objs[sprite_id];
		sprite->num = texture_id;
	}

	float src_left = src->x;
	float src_right = src->x + src->w;
	float src_top = src->y;
	float src_bot = src->y + src->h;
	float dest_left = dest->x;
	float dest_right = dest->x + dest->w;
	float dest_top = dest->y;
	float dest_bot = dest->y + dest->h;

	float data[] = {
		dest_left, dest_top, 0.0f, 1.0f, src_left, src_bot,
		dest_right, dest_top, 0.0f, 1.0f, src_right, src_bot,
		dest_left, dest_bot, 0.0f, 1.0f, src_left, src_top,
		dest_right, dest_bot, 0.0f, 1.0f, src_right, src_top,
	};

	// buffer data
	GLsizeiptr sizeInBytes = sizeof(data);
	glBindBuffer(GL_ARRAY_BUFFER, sprite->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_DYNAMIC_DRAW);

	glUseProgram(0);
	return id;
}

GLUTIL_DEF GLint glutil_create_string(const char* string, float x, float y, kiss_vec3* color, GLint text_id) {
	GLLOG("Create String Text");

	glUseProgram(text_program_id);

	GLText *text = nullptr;
	GLint id = text_id;

	// if new sprite
	if (id == GLUTIL_CREATE_NEW) {
		id = sb_count(text_objs);
		text = sb_add(text_objs, 1);
		text->model = KISS_MAT4();
		if (color != nullptr) {
			text->color = KISS_VEC3(color->v[0], color->v[1], color->v[2]);
		}
		text->num = font_tex;

		// create/bind vertex array 
		glGenVertexArrays(1, &text->vao);
		glBindVertexArray(text->vao);

		// create/bind vertex buffer obj
		glGenBuffers(1, &text->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, text->vbo);

		// set vertex attrib pointers for vertex/color data
		GLint loc = glGetAttribLocation(text_program_id, "pos");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
		glEnableVertexAttribArray(loc);

		loc = glGetAttribLocation(text_program_id, "texCoord");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(loc);

		// generate element buffer
		glGenBuffers(1, &text->ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->ebo);

	}
	else {
		text = &text_objs[id];
		if (color != nullptr) {
			text->color = KISS_VEC3(color->v[0], color->v[1], color->v[2]);
		}
	}

	{ // create bitmap fonts for each letter

		int data_per_sprite = 24; // 4*4 for sprite + 2*4 for tex coords
		size_t string_len = strlen(string);
		size_t data_byte_size = data_per_sprite * string_len * sizeof(float);
		float* data = (float*)calloc(data_per_sprite * string_len, sizeof(float));

		for (size_t i = 0; i < strlen(string); ++i) {
			KissGlyph* glyph = &font->glyphs[(int)(string[i])];

			float src_left = glyph->tx0;
			float src_right = glyph->tx1;
			float src_top = glyph->ty0;
			float src_bot = glyph->ty1;
			float dest_left = x + glyph->pen_x_offset;
			float dest_right = dest_left + glyph->width;
			float dest_top = y + glyph->pen_y_offset;
			float dest_bot = dest_top + glyph->height;

			int offset = i * data_per_sprite;
			data[offset + 0] = dest_left;
			data[offset + 1] = dest_top;
			data[offset + 2] = 0.0f;
			data[offset + 3] = 1.0f;
			data[offset + 4] = src_left;
			data[offset + 5] = src_top;

			data[offset + 6] = dest_right;
			data[offset + 7] = dest_top;
			data[offset + 8] = 0.0f;
			data[offset + 9] = 1.0f;
			data[offset + 10] = src_right;
			data[offset + 11] = src_top;

			data[offset + 12] = dest_left;
			data[offset + 13] = dest_bot;
			data[offset + 14] = 0.0f;
			data[offset + 15] = 1.0f;
			data[offset + 16] = src_left;
			data[offset + 17] = src_bot;

			data[offset + 18] = dest_right;
			data[offset + 19] = dest_bot;
			data[offset + 20] = 0.0f;
			data[offset + 21] = 1.0f;
			data[offset + 22] = src_right;
			data[offset + 23] = src_bot;

			x += glyph->pen_advance;

		}

		// buffer data
		glBindBuffer(GL_ARRAY_BUFFER, text->vbo);
		glBufferData(GL_ARRAY_BUFFER, data_byte_size, data, GL_DYNAMIC_DRAW);
		free(data);

	}

	{ // Create element array 
		size_t string_len = strlen(string);
		size_t ele_per_letter = 6;
		size_t ver_per_letter = 4;
		size_t num_elements = string_len * ele_per_letter;
		size_t element_byte_size = num_elements * sizeof(GLuint);
		GLuint *elements = (GLuint*)calloc(num_elements, sizeof(GLuint));

		// 0 = left_top
		// 1 = right_top
		// 2 = left_bot
		// 3 = right_bot
		for (size_t i = 0; i < string_len; ++i) {
			int ele_offset = i * ele_per_letter;
			int vert_offset = i * ver_per_letter;
			elements[ele_offset + 0] = vert_offset + 2;
			elements[ele_offset + 1] = vert_offset + 0;
			elements[ele_offset + 2] = vert_offset + 1;
			elements[ele_offset + 3] = vert_offset + 1;
			elements[ele_offset + 4] = vert_offset + 3;
			elements[ele_offset + 5] = vert_offset + 2;
		}

		glBindVertexArray(text->vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_byte_size, elements, GL_DYNAMIC_DRAW);
		free(elements);

		text->el_count = num_elements;
		text->el_type = GL_UNSIGNED_INT;
		text->el_mode = GL_TRIANGLES;
		text->el_offset = 0;

	}

	glUseProgram(0);

	return id;

}

GLUTIL_DEF GLint glutil_create_strings(char** strings, float x, float y, kiss_vec3* color, GLint text_id) {
	GLLOG("Create String Text");

	glUseProgram(text_program_id);

	GLText *text = nullptr;
	GLint id = text_id;

	// if new sprite
	if (id == GLUTIL_CREATE_NEW) {
		id = sb_count(text_objs);
		text = sb_add(text_objs, 1);
		text->model = KISS_MAT4();
		if (color != nullptr) {
			text->color = KISS_VEC3(color->v[0], color->v[1], color->v[2]);
		}
		text->num = font_tex;

		// create/bind vertex array 
		glGenVertexArrays(1, &text->vao);
		glBindVertexArray(text->vao);

		// create/bind vertex buffer obj
		glGenBuffers(1, &text->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, text->vbo);

		// set vertex attrib pointers for vertex/color data
		GLint loc = glGetAttribLocation(text_program_id, "pos");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
		glEnableVertexAttribArray(loc);

		loc = glGetAttribLocation(text_program_id, "texCoord");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(loc);

		// generate element buffer
		glGenBuffers(1, &text->ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->ebo);

	}
	else {
		text = &text_objs[id];
		if (color != nullptr) {
			text->color = KISS_VEC3(color->v[0], color->v[1], color->v[2]);
		}
	}

	size_t string_len = 0;
	{ // create bitmap fonts for each letter

		int data_per_sprite = 24; // 4*4 for sprite + 2*4 for tex coords
		for (int i = 0; i < sb_count(strings); ++i) {
			string_len += strlen(strings[i]);
		}
		
		size_t data_byte_size = data_per_sprite * string_len * sizeof(float);
		float* data = (float*)calloc(data_per_sprite * string_len, sizeof(float));
		int startx = x;
		int letters = 0;
		for (int s = 0; s < sb_count(strings); ++s) {
			for (size_t i = 0; i < strlen(strings[s]); ++i) {
				int index = (int)(strings[s][i]);
				KissGlyph* glyph = &font->glyphs[index];

				float src_left = glyph->tx0;
				float src_right = glyph->tx1;
				float src_top = glyph->ty0;
				float src_bot = glyph->ty1;
				float dest_left = x + glyph->pen_x_offset;
				float dest_right = dest_left + glyph->width;
				float dest_top = y + glyph->pen_y_offset;
				float dest_bot = dest_top + glyph->height;

				int offset = (letters + i) * data_per_sprite;

				data[offset + 0] = dest_left;
				data[offset + 1] = dest_top;
				data[offset + 2] = 0.0f;
				data[offset + 3] = 1.0f;
				data[offset + 4] = src_left;
				data[offset + 5] = src_top;

				data[offset + 6] = dest_right;
				data[offset + 7] = dest_top;
				data[offset + 8] = 0.0f;
				data[offset + 9] = 1.0f;
				data[offset + 10] = src_right;
				data[offset + 11] = src_top;

				data[offset + 12] = dest_left;
				data[offset + 13] = dest_bot;
				data[offset + 14] = 0.0f;
				data[offset + 15] = 1.0f;
				data[offset + 16] = src_left;
				data[offset + 17] = src_bot;

				data[offset + 18] = dest_right;
				data[offset + 19] = dest_bot;
				data[offset + 20] = 0.0f;
				data[offset + 21] = 1.0f;
				data[offset + 22] = src_right;
				data[offset + 23] = src_bot;

				x += glyph->pen_advance;

			}
			letters += strlen(strings[s]);

			x = startx;
			y += font->newline;
		}

		// buffer data
		glBindBuffer(GL_ARRAY_BUFFER, text->vbo);
		glBufferData(GL_ARRAY_BUFFER, data_byte_size, data, GL_DYNAMIC_DRAW);
		free(data);

	}

	{ // Create element array 
		size_t ele_per_letter = 6;
		size_t ver_per_letter = 4;
		size_t num_elements = string_len * ele_per_letter;
		size_t element_byte_size = num_elements * sizeof(GLuint);
		GLuint *elements = (GLuint*)calloc(num_elements, sizeof(GLuint));

		// 0 = left_top
		// 1 = right_top
		// 2 = left_bot
		// 3 = right_bot
		for (size_t i = 0; i < string_len; ++i) {
			int ele_offset = i * ele_per_letter;
			int vert_offset = i * ver_per_letter;
			elements[ele_offset + 0] = vert_offset + 2;
			elements[ele_offset + 1] = vert_offset + 0;
			elements[ele_offset + 2] = vert_offset + 1;
			elements[ele_offset + 3] = vert_offset + 1;
			elements[ele_offset + 4] = vert_offset + 3;
			elements[ele_offset + 5] = vert_offset + 2;
		}

		glBindVertexArray(text->vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_byte_size, elements, GL_DYNAMIC_DRAW);
		free(elements);

		text->el_count = num_elements;
		text->el_type = GL_UNSIGNED_INT;
		text->el_mode = GL_TRIANGLES;
		text->el_offset = 0;

	}

	glUseProgram(0);

	return id;

}

GLUTIL_DEF void glutil_render_objects(kiss_mat4 *mvp) {
	glUseProgram( glutil_program_id );
	GLuint loc = glGetUniformLocation(glutil_program_id, "MVP" );
	SDL_assert( loc != -1 );
	kiss_mat4 MVP = KISS_MAT4();
	for( int i = 0; i < sb_count(render_objs); ++i ) {
		GLObject *obj = &render_objs[i];
		if (obj == 0) continue;
		kiss_mul_m4(&MVP, &obj->model, mvp);
		glUniformMatrix4fv(loc, 1, GL_FALSE, MVP.m);
		glBindVertexArray( obj->vao );
		glDrawElements(obj->elementMode, obj->elementCount, obj->elementType, obj->elementOffset);
	}
	glBindVertexArray(0);
	glUseProgram( 0 );
}

GLUTIL_DEF void glutil_render_sprites(kiss_mat4 *mvp) {
	glUseProgram(sprite_program_id);
	GLuint loc = glGetUniformLocation(sprite_program_id, "MVP");
	SDL_assert(loc != -1);
	kiss_mat4 MVP = KISS_MAT4();
	for (int i = 0; i < sb_count(sprite_objs); ++i) {
		GLSprite *sprite = &sprite_objs[i];
		if (sprite == 0) continue;
		kiss_mul_m4(&MVP, &sprite->model, mvp);
		glUniformMatrix4fv(loc, 1, GL_FALSE, MVP.m);
		glBindVertexArray(sprite->vao);
		glActiveTexture(GL_TEXTURE0 + sprite->num);
		glBindTexture(GL_TEXTURE_2D, tbo_list[sprite->num]);
		glUniform1i(glGetUniformLocation(sprite_program_id, "sampler"), sprite->num);
		glDrawElements(sprite->el_mode, sprite->el_count, sprite->el_type, sprite->el_offset);
	}
	glUseProgram(0);
}

GLUTIL_DEF void glutil_render_text(kiss_mat4 *mvp) {
	glUseProgram(text_program_id);
	GLuint loc = glGetUniformLocation(text_program_id, "MVP");
	SDL_assert(loc != -1);
	kiss_mat4 MVP = KISS_MAT4();
	for (int i = 0; i < sb_count(text_objs); ++i) {
		GLText *text = &text_objs[i];
		if (text == 0) continue;
		kiss_mul_m4(&MVP, &text->model, mvp);
		glUniformMatrix4fv(loc, 1, GL_FALSE, MVP.m);
		glBindVertexArray(text->vao);
		glActiveTexture(GL_TEXTURE0 + text->num);
		glBindTexture(GL_TEXTURE_2D, tbo_list[text->num]);
		glUniform1i(glGetUniformLocation(text_program_id, "sampler"), text->num);
		glUniform3fv(glGetUniformLocation(text_program_id, "textColor"), 1, text->color.v);
		glDrawElements(text->el_mode, text->el_count, text->el_type, text->el_offset);
	}
	glUseProgram(0);
}

// TODO draw rect
GLUTIL_DEF void glutil_create_grid( kiss_vec3 *pos, kiss_vec3* normal, float width, float height, float divisions, kiss_vec4 *color ) {
	//Now you can rotate the plane however you want. If you to have the plane to have 0 roll 
	//(that is, only ever modify yaw and pitch), you can take the normalized cross product of the world "up" 
	//vector (0,0,1) and normalize(V) to get the horizontal vector U for the rectangle. 
	//Take the normalized cross product of normalize(V) and U to get the vertical vector W for the rectangle.

	//The corners of your rectangle are now:

	//C1 = P0 + (width / 2) * U + (height / 2) * W
	//C2 = P0 + (width / 2) * U - (height / 2) * W
	//C3 = P0 - (width / 2) * U + (height / 2) * W
	//C4 = P0 - (width / 2) * U - (height / 2) * W
}

GLUTIL_DEF void glutil_create_cube( kiss_vec3 *pos, float radius, kiss_vec4 *color ) {
	GLLOG("Create Cube");

	GLObject* obj = sb_add(render_objs, 1);
	obj->model = KISS_MAT4(); // identity

	glUseProgram( glutil_program_id );
	
	glGenVertexArrays( 1, &obj->vao );
	glBindVertexArray( obj->vao );
	
	// create/bind vertex buffer obj
	glGenBuffers( 1, &obj->vbo );
	glBindBuffer( GL_ARRAY_BUFFER, obj->vbo );
	
	// testing
	kiss_vec4 red = KISS_VEC4( 1.0, 0.0f, 0.0f, 1.0f );
	kiss_vec4 green = KISS_VEC4( 0.0, 1.0f, 0.0f, 1.0f );
	kiss_vec4 blue = KISS_VEC4( 0.0, 0.0f, 1.0f, 1.0f );
	
	float data[] = {
		// x, y, z, w, r, g, b, a
		-1.0f*radius+pos->v[0], +1.0f*radius+pos->v[1], +1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(&red), //LTN 0
		-1.0f*radius+pos->v[0], -1.0f*radius+pos->v[1], +1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(&blue), //LBN 1
		+1.0f*radius+pos->v[0], -1.0f*radius+pos->v[1], +1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(&green), //RBN 2
		+1.0f*radius+pos->v[0], +1.0f*radius+pos->v[1], +1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(color), //RTN 3

		-1.0f*radius+pos->v[0], +1.0f*radius+pos->v[1], -1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(color), //LTF 4
		-1.0f*radius+pos->v[0], -1.0f*radius+pos->v[1], -1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(color), //LBF 5
		+1.0f*radius+pos->v[0], -1.0f*radius+pos->v[1], -1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(color), //RBF 6
		+1.0f*radius+pos->v[0], +1.0f*radius+pos->v[1], -1.0f*radius+pos->v[2], +1.0f, KISS_VEC4X(color), //RTF 7
	};

	// buffer data
	GLsizeiptr sizeInBytes = sizeof( data );
	glBufferData( GL_ARRAY_BUFFER, sizeInBytes, data, GL_STATIC_DRAW );
	
	// set vertex attrib pointers for vertex/color data
	GLint loc = glGetAttribLocation( glutil_program_id, "position" );
	SDL_assert( loc != -1 );	
	glVertexAttribPointer( loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0 );
	glEnableVertexAttribArray( loc );

	loc = glGetAttribLocation( glutil_program_id, "color" );
	SDL_assert( loc != -1 );
	glVertexAttribPointer( loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float) ) );
	glEnableVertexAttribArray( loc );

	GLuint elements[] = {
		2, 1, 3, 3, 1, 0,
		0, 4, 3, 3, 4, 7,
		6, 7, 4, 4, 5, 6,
		1, 5, 6, 6, 1, 2,
		3, 2, 6, 6, 7, 3,
		4, 5, 1, 1, 0, 4,
	};

	obj->elementMode = GL_TRIANGLES;
	obj->elementType = GL_UNSIGNED_INT;
	obj->elementCount = sizeof( elements ) / sizeof( elements[0] );

	glGenBuffers( 1, &obj->ebo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, obj->ebo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( elements ), elements, GL_STATIC_DRAW );

	glUseProgram(0);
}

// line start: x, y, z, w, r, g, b, a
// line end: x, y, z, w, r, g, b, a
GLUTIL_DEF GLint glutil_create_linesf(float* lines, GLint obj_id) {
	GLLOG("Create Lines");

	glUseProgram(glutil_program_id);

	GLObject* obj = nullptr;
	GLint id = obj_id;

	if (id == GLUTIL_CREATE_NEW) {
		id = sb_count(render_objs);
		obj = sb_add(render_objs, 1);
		obj->model = KISS_MAT4(); // identity

		// create.bind vertex array obj
		glGenVertexArrays(1, &obj->vao);
		glBindVertexArray(obj->vao);

		// create/bind vertex buffer obj
		glGenBuffers(1, &obj->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);

		// set vertex attrib pointers for vertex/color data
		GLint loc = glGetAttribLocation(glutil_program_id, "position");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
		glEnableVertexAttribArray(loc);

		loc = glGetAttribLocation(glutil_program_id, "color");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(loc);

		// generate index array buffer
		glGenBuffers(1, &obj->ebo);
		obj->elementMode = GL_LINES;
		obj->elementType = GL_UNSIGNED_INT;
		obj->elementOffset = 0;

	}
	else {
		obj = &render_objs[id];
	}

	glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
	glBufferData(GL_ARRAY_BUFFER, sb_count(lines) * sizeof(float), lines, GL_DYNAMIC_DRAW);

	// create a new data array
	int elements_per_line = 2;
	int element_count = elements_per_line * sb_count(lines);
	GLuint element_byte_size = element_count * sizeof(GLuint);
	GLuint* elements = (GLuint*)calloc(element_count, sizeof(GLuint));
	SDL_assert(elements != nullptr);

	for (int i = 0; i < element_count; ++i) {
		elements[i] = (GLuint)i;
	}

	obj->elementCount = element_count;

	glBindVertexArray(obj->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_byte_size, elements, GL_DYNAMIC_DRAW);
	free(elements);

	glBindVertexArray(0);
	glUseProgram(0);

	return id;
}

// x,y,z,w r,g,b,a X 4 CCW
GLUTIL_DEF GLint glutil_create_quads(float* quads, bool fill, GLint obj_id) {

	glUseProgram(glutil_program_id);

	GLObject* obj = nullptr;
	GLint id = obj_id;

	if (id == GLUTIL_CREATE_NEW) {
		id = sb_count(render_objs);
		obj = sb_add(render_objs, 1);
		obj->model = KISS_MAT4(); // identity

		// create.bind vertex array obj
		glGenVertexArrays(1, &obj->vao);
		glBindVertexArray(obj->vao);

		// create/bind vertex buffer obj
		glGenBuffers(1, &obj->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);

		// set vertex attrib pointers for vertex/color data
		GLint loc = glGetAttribLocation(glutil_program_id, "position");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
		glEnableVertexAttribArray(loc);

		loc = glGetAttribLocation(glutil_program_id, "color");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(loc);

		// generate index array buffer
		glGenBuffers(1, &obj->ebo);

	}
	else {
		obj = &render_objs[id];
	}

	glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
	glBufferData(GL_ARRAY_BUFFER, sb_count(quads) * sizeof(float), quads, GL_DYNAMIC_DRAW);

	{ // Create element array 
		size_t ele_per_rect = 6;
		size_t ver_per_rect = 4;
		size_t num_quads = sb_count(quads) / 32;
		size_t num_elements = num_quads * ele_per_rect;
		size_t element_byte_size = num_elements * sizeof(GLuint);
		GLuint *elements = (GLuint*)calloc(num_elements, sizeof(GLuint));

		// CCW
		for (size_t i = 0; i < num_quads; ++i) {
			int ele_offset = i * ele_per_rect;
			int vert_offset = i * ver_per_rect;
			elements[ele_offset + 0] = vert_offset + 0;
			elements[ele_offset + 1] = vert_offset + 1;
			elements[ele_offset + 2] = vert_offset + 2;
			elements[ele_offset + 3] = vert_offset + 2;
			elements[ele_offset + 4] = vert_offset + 3;
			elements[ele_offset + 5] = vert_offset + 0;
		}

		glBindVertexArray(obj->vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_byte_size, elements, GL_DYNAMIC_DRAW);
		free(elements);

		obj->elementCount = num_elements;
		obj->elementType = GL_UNSIGNED_INT;
		obj->elementMode = fill ? GL_TRIANGLES : GL_LINES;
		obj->elementOffset = 0;

	}

	glUseProgram(0);

	return id;

}

GLUTIL_DEF GLint glutil_create_rects(GLRect* rects, GLuint count, GLint obj_id) {
	GLLOG("Create Rects");

	glUseProgram(glutil_program_id);

	GLObject* obj = nullptr;
	GLint id = obj_id;

	if (id == GLUTIL_CREATE_NEW) {
		id = sb_count(render_objs);
		obj = sb_add(render_objs, 1);
		obj->model = KISS_MAT4(); // identity

		// create.bind vertex array obj
		glGenVertexArrays(1, &obj->vao);
		glBindVertexArray(obj->vao);

		// create/bind vertex buffer obj
		glGenBuffers(1, &obj->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);

		// set vertex attrib pointers for vertex/color data
		GLint loc = glGetAttribLocation(glutil_program_id, "position");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
		glEnableVertexAttribArray(loc);

		loc = glGetAttribLocation(glutil_program_id, "color");
		SDL_assert(loc != -1);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
		glEnableVertexAttribArray(loc);

		// generate index array buffer
		glGenBuffers(1, &obj->ebo);
		obj->elementMode = GL_TRIANGLE_STRIP;
		obj->elementType = GL_UNSIGNED_INT;

	}
	else {
		obj = &render_objs[id];
	}

	// create a new data array
	int floats_per_line = 32;
	int float_count = floats_per_line * count;
	GLuint vertex_byte_size = float_count * sizeof(float);
	float* data = (float*)calloc(float_count, sizeof(float));
	SDL_assert(data != nullptr);

	for (int i = 0; i < count; ++i) {
		GLRect* rect = &rects[i];
		float left = rect->x;
		float right = rect->x + rect->w;
		float top = rect->y;
		float bot = rect->y + rect->h;
		kiss_vec4* fill = &rect->color;

		int offset = i * floats_per_line;
		data[offset + 0] = left;
		data[offset + 1] = top;
		data[offset + 2] = 0.0f;
		data[offset + 3] = 1.0f;
		data[offset + 4] = fill->v[0];
		data[offset + 5] = fill->v[1];
		data[offset + 6] = fill->v[2];
		data[offset + 7] = fill->v[3];

		data[offset + 8] = right;
		data[offset + 9] = top;
		data[offset + 10] = 0.0f;
		data[offset + 11] = 1.0f;
		data[offset + 12] = fill->v[0];
		data[offset + 13] = fill->v[1];
		data[offset + 14] = fill->v[2];
		data[offset + 15] = fill->v[3];

		data[offset + 16] = left;
		data[offset + 17] = bot;
		data[offset + 18] = 0.0f;
		data[offset + 19] = 1.0f;
		data[offset + 20] = fill->v[0];
		data[offset + 21] = fill->v[1];
		data[offset + 22] = fill->v[2];
		data[offset + 23] = fill->v[3];

		data[offset + 24] = right;
		data[offset + 25] = bot;
		data[offset + 26] = 0.0f;
		data[offset + 27] = 1.0f;
		data[offset + 28] = fill->v[0];
		data[offset + 29] = fill->v[1];
		data[offset + 30] = fill->v[2];
		data[offset + 31] = fill->v[3];
	}

	glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_byte_size, data, GL_DYNAMIC_DRAW);
	free(data);

	{ // Create element array 
		size_t ele_per_rect = 6;
		size_t ver_per_rect = 4;
		size_t num_elements = count * ele_per_rect;
		size_t element_byte_size = num_elements * sizeof(GLuint);
		GLuint *elements = (GLuint*)calloc(num_elements, sizeof(GLuint));

		// 0 = left_top
		// 1 = right_top
		// 2 = left_bot
		// 3 = right_bot
		for (size_t i = 0; i < count; ++i) {
			int ele_offset = i * ele_per_rect;
			int vert_offset = i * ver_per_rect;
			elements[ele_offset + 0] = vert_offset + 2;
			elements[ele_offset + 1] = vert_offset + 0;
			elements[ele_offset + 2] = vert_offset + 1;
			elements[ele_offset + 3] = vert_offset + 1;
			elements[ele_offset + 4] = vert_offset + 3;
			elements[ele_offset + 5] = vert_offset + 2;
		}

		glBindVertexArray(obj->vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_byte_size, elements, GL_DYNAMIC_DRAW);
		free(elements);

		obj->elementCount = num_elements;
		obj->elementType = GL_UNSIGNED_INT;
		obj->elementMode = GL_TRIANGLES;
		obj->elementOffset = 0;

	}

	glUseProgram(0);

	return id;
}

#endif // end GLUTIL_IMPLEMENTATION




