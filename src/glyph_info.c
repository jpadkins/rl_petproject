///////////////////////////////////////////////////////////////////////////////
/// file:           glyph_info.c
/// author:         Jacob Adkins - jpadkins
/// description:    Managed a (uthash) hash map of glyph metrics from a BMFont
///                 file (.fnt) corresponding to a bitmap glyph atlas generated
///                 by FontBuilder, Bitmap Font Generator, etc...
///////////////////////////////////////////////////////////////////////////////

#include "glyph_info.h"

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
    glyph_info *info;
    UT_hash_handle hh;
} glyph_info_hash;

///////////////////////////////////////////////////////////////////////////////
/// Static variables
///////////////////////////////////////////////////////////////////////////////

static bool populated = false;
static glyph_info_hash *glyph_hash = NULL;

///////////////////////////////////////////////////////////////////////////////
/// Static functions
///////////////////////////////////////////////////////////////////////////////

static void GlyphInfo_Add(int glyph, glyph_info *info)
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

///////////////////////////////////////////////////////////////////////////////
void GlyphInfo_Populate(const char *file_path)
{
    int glyph_id = 0;
    FILE *file = NULL;
    glyph_info *info = NULL;
    unsigned long fsize = 0;
    int linenum = 1, ival = 0;
    char *buff = NULL, *line = NULL, *lineptr = NULL, linebuff[1024],
        *tok = NULL, *tokptr = NULL, tokbuff[512], *val = NULL, *valptr = NULL,
        valbuff[512];

    if (populated) {
        LOG(LOG_EXIT, "Tried to re-populate GlyphInfo before cleaning");
    }
    populated = true;

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
        GlyphInfo_Add(glyph_id, info);
        info = NULL;
    }

#undef NEXT_VAL

    free(buff);

}

///////////////////////////////////////////////////////////////////////////////
const glyph_info * GlyphInfo_Get(int glyph)
{
    glyph_info_hash *entry = NULL;

    if (!populated) {
        LOG(LOG_EXIT, "Tried to access before GlyphInfo was populated");
    }

    HASH_FIND_INT(glyph_hash, &glyph, entry);
    if (!entry) {
        LOGFMT(LOG_INFO, "Glyph not in hash: %d", glyph);
        return NULL;
    }

    return entry->info;
}

///////////////////////////////////////////////////////////////////////////////
void GlyphInfo_Cleanup(void)
{
    if (!populated) {
        LOG(LOG_EXIT, "Tried to cleanup GlyphInfo before populating");
    }
    populated = false;

    glyph_info_hash *entry = NULL, *tmp = NULL;
    HASH_ITER(hh, glyph_hash, entry, tmp) {
        HASH_DEL(glyph_hash, entry);
        free(entry->info);
        free(entry);
    }
}
