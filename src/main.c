///////////////////////////////////////////////////////////////////////////////
/// file:           main.c
/// author:         Jacob Adkins - jpadkins
/// description:    The roguelike application entry point.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glad.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////
/// Structs
///////////////////////////////////////////////////////////////////////////////

typedef struct { int x, y; } vec2;
typedef struct { int x, y, w, h; } rect;

///////////////////////////////////////////////////////////////////////////////
/// Static variables
///////////////////////////////////////////////////////////////////////////////

static FT_Face face;
static FT_Library freetype;
static bool running = true;
static SDL_GLContext context;
static SDL_Window *window = NULL;
//static SDL_Renderer *renderer = NULL;
static const vec2 window_size = {800, 600};
static const char *font_path = "res/unifont.ttf";
static const GLfloat vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

static const struct {
    struct {
        const GLchar *basic;
    } vertex;
    struct {
        const GLchar *basic;
    } fragment;
} shaders_src = {
    {
        "                                               \n\
        #version 330 core                               \n\
                                                        \n\
        layout (location = 0) in vec3 position;         \n\
                                                        \n\
        void main(void) {                               \n\
            gl_Position = vec4(position.xyz, 1.0f);     \n\
        }"
    },
    {
        "                                               \n\
        #version 330 core                               \n\
                                                        \n\
        out vec4 fragColor;                             \n\
                                                        \n\
        void main(void) {                               \n\
            fragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);   \n\
        }"
    }
};

///////////////////////////////////////////////////////////////////////////////
/// Main
///////////////////////////////////////////////////////////////////////////////

int main(void)
{
    GLint status;
    SDL_Event event;
    GLchar errmsg[512];
    GLuint VBO, VAO, vert, frag, program;

    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        LOG(LOG_EXIT, "SDL2 Initialization failed");
    }

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetSwapInterval(1);

    if (!(window = SDL_CreateWindow(
        "SDL2 Application",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_size.x,
        window_size.y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN))) {
        LOG(LOG_EXIT, "Window creation failed");
    }

    if (FT_Init_FreeType(&freetype)) {
        LOG(LOG_EXIT, "FreeType initialization failed");
    }

    if (FT_New_Face(freetype, font_path, 0, &face)) {
        LOG(LOG_EXIT, "Font face loading failed");
    }

    if (!(context = SDL_GL_CreateContext(window))) {
        LOG(LOG_EXIT, "OpenGL context creation failed");
    }

    gladLoadGLLoader(SDL_GL_GetProcAddress);
    if (!GL_VERSION_3_3) {
        LOG(LOG_EXIT, "Failed to load OpenGL > 3.3");
    }

    glViewport(0, 0, window_size.x, window_size.y);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    if (!(vert = glCreateShader(GL_VERTEX_SHADER))) {
        LOG(LOG_EXIT, "Vertex shader creation failed");
    }
    glShaderSource(vert, 1, &shaders_src.vertex.basic, NULL);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderInfoLog(vert, 512, NULL, errmsg);
        LOGFMT(LOG_EXIT, "Vertex shader compilation failed: %s", errmsg);
    }

    if (!(frag = glCreateShader(GL_FRAGMENT_SHADER))) {
        LOG(LOG_EXIT, "Fragment shader creation failed");
    }
    glShaderSource(frag, 1, &shaders_src.fragment.basic, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderInfoLog(frag, 512, NULL, errmsg);
        LOGFMT(LOG_EXIT, "Fragment shader compilation failed: %s", errmsg);
    }

    if (!(program = glCreateProgram())) {
        LOG(LOG_EXIT, "Shader program creation failed");
    }
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        glGetProgramInfoLog(program, 512, NULL, errmsg);
        LOGFMT(LOG_EXIT, "Shader program link failed: %s", errmsg);
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);

    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
        (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        glViewport(
                            0,
                            0,
                            event.window.data1,
                            event.window.data2
                            );
                    }
                    break;
                default:
                    break;
            }
        }

        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_ESCAPE]) {
            running = false;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        SDL_GL_SwapWindow(window);
    }

    glDeleteBuffers(1, &VBO);
    FT_Done_FreeType(freetype);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
