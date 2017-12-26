///////////////////////////////////////////////////////////////////////////////
/// @file:		main.c
/// @author:	Jacob Adkins (jpadkins)
/// @brief:		The roguelike application entry point.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "linmath.h"
#include "glad.h"

#include "log.h"
#include "common.h"
#include "bmfont.h"
#include "shaders.h"

///////////////////////////////////////////////////////////////////////////////
/// Static variables
///////////////////////////////////////////////////////////////////////////////

static float delta = 0.0f;
static bool running = true;
static SDL_GLContext context;
static SDL_Window *window = NULL;
static const struct { int x, y; } window_size = {800, 600};

///////////////////////////////////////////////////////////////////////////////
/// Helper functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// @brief Creates and compiles a new OpenGL shader
///
/// @param type Type of the shader
/// @param src  Pointer to the shader's source
///
/// @return Identifier of the newly created and compiled shader
///////////////////////////////////////////////////////////////////////////////
GLuint GL_ShaderNew(GLenum type, const char *src)
{
	GLint status;
	GLuint shader;
	GLchar errmsg[512];

	if (!(shader = glCreateShader(type))) {
		log_exit("Shader creation failed");
	}

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (!status) {
		glGetShaderInfoLog(shader, 512, NULL, errmsg);
		logfmt_exit("Shader compilation failed: %s", errmsg);
	}
	return shader;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Used internally by GL_ProgramNew
///////////////////////////////////////////////////////////////////////////////
GLuint GL_ProgramNewVarg(int num, ...)
{
	va_list ap;
	GLint status;
	GLuint program;
	GLchar errmsg[512];

	if (!(program = glCreateProgram())) {
		log_exit("Shader program creation failed");
	}

	va_start(ap, num);
	for (int i = 0; i < num; ++i) {
		glAttachShader(program, va_arg(ap, GLuint));
	}
	va_end(ap);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		glGetProgramInfoLog(program, 512, NULL, errmsg);
		logfmt_exit("Shader program linking failed: %s", errmsg);
	}

	return program;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Creates and links a new OpenGL shader program
///
/// @param ...	  Variable number of shaders to link (GLuint)
///
/// @return Identifier of the newly created and linked shader program
///////////////////////////////////////////////////////////////////////////////
#define GL_ProgramNew(...) GL_ProgramNewVarg(NUMARGS(__VA_ARGS__),__VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////
/// @brief Initialze SDL2, the window, OpenGL, and FreeType
///////////////////////////////////////////////////////////////////////////////
void App_Init(void)
{
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		log_exit("SDL2 Initialization failed");
	}

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	if (!(window = SDL_CreateWindow(
		"SDL2 Application",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_size.x,
		window_size.y,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN))) {
		log_exit("Window creation failed");
	}
	else if (!(context = SDL_GL_CreateContext(window))) {
		log_exit("OpenGL context creation failed");
	}

	gladLoadGLLoader(SDL_GL_GetProcAddress);
	if (!GL_VERSION_3_3) {
		log_exit("Failed to load OpenGL >= 3.3");
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, window_size.x, window_size.y);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	SDL_GL_SetSwapInterval(1);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Cleanup SDL2, the window, and FreeType
///////////////////////////////////////////////////////////////////////////////
void App_Quit(void)
{
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Update the application state
///////////////////////////////////////////////////////////////////////////////
void App_Update(void)
{
	SDL_Event event;
	static float tcurr = 0.0f, tprev = 0.0f;

	tcurr = SDL_GetPerformanceCounter();
	delta = ((tcurr - tprev) / SDL_GetPerformanceFrequency()) * 1000.0f;
	tprev = tcurr;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				glViewport(
					0,
					0,
					event.window.data1,
					event.window.data2
					);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_ESCAPE]) {
		running = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// Main
///////////////////////////////////////////////////////////////////////////////

typedef struct { int r, g, b, a; } RLHue;

typedef enum {
	RLTILE_TEXT,
	RLTILE_EXACT,
	RLTILE_FLOOR,
	RLTILE_CENTER
} RLTileType;

typedef struct {
	int glyph;
	RLHue hue;
	RLTileType type;
} RLTile;

typedef struct {
	struct {
		int x;
		int y;
	} position;
	struct {
		int width;
		int height;
	} size;
	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	int num_tiles;
	RLTile *tiles;
	int num_indices;
	GLuint *indices;
	int num_vertices;
	GLfloat *vertices;
} RLTileMap;

typedef struct {
	RLTileMap *tile_maps;
} RLDisplay;

int main(void)
{
	GLint utransform;
	mat4x4 mtransform;
	unsigned char *tex_data = NULL;
	struct { int x, y; } tex_size = {0, 0};
	GLuint VBO, EBO, VAO, vert, frag, prog, tex;
	const GLfloat vertices[] = {
		// Vertex coords	// Texture coords
		 0.9f,   0.9f,  0.0f,   1.0f,   1.0f,
		 0.9f,  -0.9f,  0.0f,   1.0f,   0.0f,
		-0.9f,  -0.9f,  0.0f,   0.0f,   0.0f,
		-0.9f,   0.9f,  0.0f,   0.0f,   1.0f
	};
	const GLuint indices[] = {
		0, 1, 3,
		1, 2, 3,
	};

	App_Init();

	{
		BMFont *font = BMFont_Create("res/unifont.fnt");
		const BMFontInfo *info = BMFont_GetInfoPtr(font, '@');
		logfmt_info(
			"'@' glyph metrics: x: %d, y: %d",
			info->position.x,
			info->position.y
			);
		BMFont_Destroy(font);
	}

	vert = GL_ShaderNew(GL_VERTEX_SHADER, shaders.vertex.basic);
	frag = GL_ShaderNew(GL_FRAGMENT_SHADER, shaders.fragment.basic);
	prog = GL_ProgramNew(vert, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);

	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	// Define simple VAO
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		sizeof(indices),
		indices,
		GL_STATIC_DRAW
		);
	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		5 * sizeof(GLfloat),
		(void *)0
		);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		1,
		2,
		GL_FLOAT,
		GL_FALSE, 5 * sizeof(GLfloat),
		(void *)(3 * sizeof(GLfloat))
		);
	glEnableVertexAttribArray(1);

	// Load texture
	stbi_set_flip_vertically_on_load(true);
	if (!(tex_data = stbi_load(
		"res/unifont.png",
		&tex_size.x,
		&tex_size.y,
		NULL,
		STBI_rgb_alpha))) {
		log_exit("Font atlas loading failed");
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		tex_size.x,
		tex_size.y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		tex_data
		);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(tex_data);

	mat4x4_identity(mtransform);
	utransform = glGetUniformLocation(prog, "transform");
	while (running) {
		App_Update();
		glClear(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, tex);
		glUseProgram(prog);
		glUniformMatrix4fv(utransform, 1, GL_FALSE, (GLfloat *)mtransform);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		SDL_GL_SwapWindow(window);
	}

	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);

	App_Quit();
	return 0;
}
