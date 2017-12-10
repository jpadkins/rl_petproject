///////////////////////////////////////////////////////////////////////////////
/// file:           main.c
/// author:         Jacob Adkins - jpadkins
/// description:    The roguelike application entry point.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

// For strtok_r
#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "log.h"
#include "glad.h"
#include "uthash.h"

///////////////////////////////////////////////////////////////////////////////
/// Macros
///////////////////////////////////////////////////////////////////////////////

#define NUMARGS(...) (sizeof((int[]){__VA_ARGS__})/sizeof(int))
#define UNUSED(x) (void)x

///////////////////////////////////////////////////////////////////////////////
/// Structs
///////////////////////////////////////////////////////////////////////////////

typedef struct { int x, y; } ivec2;

typedef struct {
    struct {
        int x;
        int y;
    } position;
    struct {
        int width;
        int height;
    } size;
    struct {
        int x;
        int y;
    } offset;
} glyph_info;

typedef struct {
    int glyph_id;
    glyph_info *info;
    UT_hash_handle hh;
} glyph_info_hash;

///////////////////////////////////////////////////////////////////////////////
/// Static variables
///////////////////////////////////////////////////////////////////////////////

static FT_Face face;
static float delta = 0.0f;
static FT_Library freetype;
static bool running = true;
static SDL_GLContext context;
static SDL_Window *window = NULL;
static glyph_info_hash *glyph_hash = NULL;
static const ivec2 window_size = {800, 600};
static const char *font_path = "res/unifont.ttf";

static const struct {
    struct {
        const GLchar *basic;
    } vertex;
    struct {
        const GLchar *basic;
    } fragment;
} shaders = {
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
/// Helper functions
///////////////////////////////////////////////////////////////////////////////

void Glyph_CleanupHash(void)
{
    glyph_info_hash *entry = NULL, *tmp = NULL;
    HASH_ITER(hh, glyph_hash, entry, tmp) {
        HASH_DEL(glyph_hash, entry);
        free(entry->info);
        free(entry);
    }
}

void Glyph_AddInfo(int glyph, glyph_info *info)
{
    glyph_info_hash *entry = NULL;

    if (!info) {
        LOGFMT(LOG_EXIT, "Null passed in as *info with glyph #%d", glyph);
    }

    HASH_FIND_INT(glyph_hash, &glyph, entry);
    if (entry) {
        LOGFMT(LOG_EXIT, "Glyph #%d info has already been added", glyph);
    }

    if (!(entry = malloc(sizeof(glyph_info_hash)))) {
        LOG(LOG_EXIT, "Failed to alloc memory for hash entry");
    }

    entry->info = info;
    entry->glyph_id = glyph;
    
    HASH_ADD_INT(glyph_hash, glyph_id, entry);
}

void Glyph_PopulateHash(const char *file_path)
{
    int glyph_id = 0;
    FILE *file = NULL;
    glyph_info *info = NULL;
    unsigned long fsize = 0;
    int linenum = 1, ival = 0;
    char *buff = NULL, *line = NULL, *lineptr = NULL, linebuff[1024],
        *tok = NULL, *tokptr = NULL, tokbuff[512], *val = NULL, *valptr = NULL,
        valbuff[512];

    // Load the BMFont file into buff
    if (!(file = fopen(file_path, "r"))) {
        LOGFMT(LOG_EXIT, "Could not open font info file: %s", file_path);
    }

    fseek(file, 0, SEEK_END);
    fsize = (unsigned long)ftell(file);
    fseek(file, 0, SEEK_SET);

    if (!(buff = malloc(fsize + 1))) {
        LOG(LOG_EXIT, "Failed to alloc memory for file buffer");
    }

    fread(buff, fsize, 1, file);
    fclose(file);

    // Move to first line that begins with "char"
    line = strtok_r(buff, "\n", &lineptr);
    while (strncmp(line, "char", 4)) {
        ++linenum;
        line = strtok_r(NULL, "\n", &lineptr);
        if (!line) {
            LOG(LOG_EXIT, "BMFont file malformed");
        }
    }

#define NEXT_VAL(token) \
    if (!(tok = strtok_r(NULL, " ", &tokptr))) {\
        LOGFMT(LOG_EXIT, "BMFont malformed: line %d, tok %s", linenum, token);\
    }\
    strcpy(tokbuff, tok);\
    val = strtok_r(tokbuff, "=", &valptr);\
    val = strtok_r(NULL, "=", &valptr);\
    strcpy(valbuff, val);\
    ival = (int)strtol(valbuff, NULL, 10);

    while ((line = strtok_r(NULL, "\n", &lineptr))) {
        if (!(info = malloc(sizeof(glyph_info)))) {
            LOG(LOG_EXIT, "Failed to alloc memory for glyph info");
        }
        strcpy(linebuff, line);
        tok = strtok_r(linebuff, " ", &tokptr);
        // Token #1 is id=<glyph>
        NEXT_VAL("id");
        glyph_id = ival;
        // Token #2 is x=<x coord>
        NEXT_VAL("x");
        info->position.x = ival;
        // Token #3 is y=<y coord>
        NEXT_VAL("y");
        info->position.y = ival;
        // Token #4 is width=<glyph width>
        NEXT_VAL("width");
        info->size.width = ival;
        // Token #5 is height=<glyph height>
        NEXT_VAL("height");
        info->size.height = ival;
        // Token #6 is xoffset=<x offset>
        NEXT_VAL("xoffset");
        info->offset.x = ival;
        // Token #7 is yoffset=<y offset>
        NEXT_VAL("yoffset");
        info->offset.y = ival;
        // We don't care about the other tokens
        Glyph_AddInfo(glyph_id, info);
        info = NULL;
    }

#undef NEXT_VAL

    free(buff);
}

const glyph_info * Glyph_GetInfo(int glyph)
{
    glyph_info_hash *entry = NULL;

    HASH_FIND_INT(glyph_hash, &glyph, entry);
    if (!entry) {
        LOGFMT(LOG_INFO, "Glyph not in hash: %d", glyph);
        return NULL;
    }

    return entry->info;
}

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
        LOG(LOG_EXIT, "Shader creation failed");
    }

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderInfoLog(shader, 512, NULL, errmsg);
        LOGFMT(LOG_EXIT, "Shader compilation failed: %s", errmsg);
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
        LOG(LOG_EXIT, "Shader program creation failed");
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
        LOGFMT(LOG_EXIT, "Shader program linking failed: %s", errmsg);
    }

    return program;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Creates and links a new OpenGL shader program
///
/// @param ...      Variable number of shaders to link (GLuint)
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
        LOG(LOG_EXIT, "SDL2 Initialization failed");
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
        LOG(LOG_EXIT, "Window creation failed");
    }
    else if (!(context = SDL_GL_CreateContext(window))) {
        LOG(LOG_EXIT, "OpenGL context creation failed");
    }

    gladLoadGLLoader(SDL_GL_GetProcAddress);
    if (!GL_VERSION_3_3) {
        LOG(LOG_EXIT, "Failed to load OpenGL >= 3.3");
    }

    glViewport(0, 0, window_size.x, window_size.y);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    SDL_GL_SetSwapInterval(1);

    if (FT_Init_FreeType(&freetype)) {
        LOG(LOG_EXIT, "FreeType initialization failed");
    }
    else if (FT_New_Face(freetype, font_path, 0, &face)) {
        LOG(LOG_EXIT, "Font face loading failed");
    }
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Cleanup SDL2, the window, and FreeType
///////////////////////////////////////////////////////////////////////////////
void App_Quit(void)
{
    FT_Done_FreeType(freetype);
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

int main(void)
{
    GLuint VBO, VAO, vert, frag, prog;
    const GLfloat vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    App_Init();

    FT_Set_Pixel_Sizes(face, 0, 16);
    if (FT_Load_Char(face, L'x', FT_LOAD_RENDER)) {
        LOG(LOG_EXIT, "Glyph loading failed");
    }

    vert = GL_ShaderNew(GL_VERTEX_SHADER, shaders.vertex.basic);
    frag = GL_ShaderNew(GL_FRAGMENT_SHADER, shaders.fragment.basic);
    prog = GL_ProgramNew(vert, frag);
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

    {
    Glyph_PopulateHash("res/unifont.fnt");
    const glyph_info *info = Glyph_GetInfo(L'▂');
    printf("Glyph info: pos %dx%d\n", info->position.x, info->position.y);
    Glyph_CleanupHash();
    }

    while (running) {
        App_Update();
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        SDL_GL_SwapWindow(window);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    App_Quit();
    return 0;
}
