///////////////////////////////////////////////////////////////////////////////
/// file:           glyph_info.c
/// author:         Jacob Adkins - jpadkins
/// description:    Managed a (uthash) hash map of glyph metrics from a BMFont
///                 file (.fnt) corresponding to a bitmap glyph atlas generated
///                 by FontBuilder, Bitmap Font Generator, etc...
///////////////////////////////////////////////////////////////////////////////

#include "bmfont.h"

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

// For strtok_r
#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "log.h"
#include "uthash.h"

///////////////////////////////////////////////////////////////////////////////
/// Structs
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int glyph_id;
    bmfont_info *info;
    UT_hash_handle hh;
} bmfont_info_hash;

struct bmfont_ {
    bmfont_info_hash *bmfont_hash;
};

///////////////////////////////////////////////////////////////////////////////
/// Static functions
///////////////////////////////////////////////////////////////////////////////

static void BMFont_Add(bmfont *this, int glyph, bmfont_info *info)
{
    bmfont_info_hash *entry = NULL;

    if (!this || !info) {
        LOG(LOG_WARN, "NULL argument");
        return;
    }

    HASH_FIND_INT(this->bmfont_hash, &glyph, entry);
    if (entry) {
        LOGFMT(LOG_WARN, "Glyph #%d has already been added", glyph);
        return;
    }

    if (!(entry = malloc(sizeof(bmfont_info_hash)))) {
        LOG(LOG_EXIT, "Memory alloc failed");
    }

    entry->info = info;
    entry->glyph_id = glyph;
    
    HASH_ADD_INT(this->bmfont_hash, glyph_id, entry);
}

///////////////////////////////////////////////////////////////////////////////
bmfont * BMFont_Create(const char *file_path)
{
    int glyph_id = 0;
    FILE *file = NULL;
    bmfont *this = NULL;
    unsigned long fsize = 0;
    bmfont_info *info = NULL;
    int linenum = 1, ival = 0;
    char *buff = NULL, *line = NULL, *lineptr = NULL, linebuff[1024],
        *tok = NULL, *tokptr = NULL, tokbuff[512], *val = NULL, *valptr = NULL,
        valbuff[512];

    // Alloc new bmfont struct
    if (!(this = malloc(sizeof(bmfont)))) {
        LOG(LOG_EXIT, "Memory alloc failed");
    }
    this->bmfont_hash = NULL;

    // Load the BMFont file into buff
    if (!(file = fopen(file_path, "r"))) {
        LOGFMT(LOG_WARN, "Could not open font info file: %s", file_path);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    fsize = (unsigned long)ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate file buffer
    if (!(buff = malloc(fsize + 1))) {
        LOG(LOG_EXIT, "Memory alloc failed");
    }

    // Copy into file buffer and close file
    fread(buff, fsize, 1, file);
    fclose(file);

    // Move to first line that begins with "char"
    line = strtok_r(buff, "\n", &lineptr);
    while (strncmp(line, "char", 4)) {
        ++linenum;
        line = strtok_r(NULL, "\n", &lineptr);
        if (!line) {
            LOG(LOG_WARN, "BMFont file malformed");
            free(this);
            free(buff);
            return NULL;
        }
    }

    // Helper macro to parse value out of next kv pair
#define NEXT_VAL(token) \
    if (!(tok = strtok_r(NULL, " ", &tokptr))) {\
        LOGFMT(LOG_EXIT, "BMFont malformed: line %d, tok %s", linenum, token);\
        BMFont_Destroy(this);\
        return NULL;\
    }\
    strcpy(tokbuff, tok);\
    val = strtok_r(tokbuff, "=", &valptr);\
    val = strtok_r(NULL, "=", &valptr);\
    strcpy(valbuff, val);\
    ival = (int)strtol(valbuff, NULL, 10);

    // Construct bmfont_info struct from line data
    do {
        if (!(info = malloc(sizeof(bmfont_info)))) {
            LOG(LOG_EXIT, "Memory alloc failed");
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
        BMFont_Add(this, glyph_id, info);
        info = NULL;
    } while ((line = strtok_r(NULL, "\n", &lineptr)));
    // TODO: Valgrind complains about this conditional jump depending on
    // an 'uninitialized value' - look into strtok_r alternative?

#undef NEXT_VAL

    free(buff);
    return this;
}

///////////////////////////////////////////////////////////////////////////////
bmfont_info const * BMFont_Info(bmfont *this, int glyph)
{
    bmfont_info_hash *entry = NULL;

    if (!this) {
        LOG(LOG_WARN, "Null argument");
        return NULL;
    }

    HASH_FIND_INT(this->bmfont_hash, &glyph, entry);
    if (!entry) {
        LOGFMT(LOG_WARN, "Glyph not in hash: %d", glyph);
        return NULL;
    }

    return entry->info;
}

///////////////////////////////////////////////////////////////////////////////
void BMFont_Destroy(bmfont *this)
{
    if (!this) {
        LOG(LOG_WARN, "NULL argument");
        return;
    }

    bmfont_info_hash *entry = NULL, *tmp = NULL;
    HASH_ITER(hh, this->bmfont_hash, entry, tmp) {
        HASH_DEL(this->bmfont_hash, entry);
        free(entry->info);
        free(entry);
    }

    free(this);
}
