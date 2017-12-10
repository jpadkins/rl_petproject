///////////////////////////////////////////////////////////////////////////////
/// file:           glyph_info.h
/// author:         Jacob Adkins - jpadkins
/// description:    Managed a (uthash) hash map of glyph metrics from a BMFont
///                 file (.fnt) corresponding to a bitmap glyph atlas generated
///                 by FontBuilder, Bitmap Font Generator, etc...
///
/// LOW: Restructure the API to allow for multiple fonts to be loaded.
///////////////////////////////////////////////////////////////////////////////

#ifndef GLYPH_INFO_H
#define GLYPH_INFO_H

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

///////////////////////////////////////////////////////////////////////////////
/// @brief Populates internal map with glyph information from a .fnt file
///
/// Call this once before any calls to GlyphInfo_Get
///
/// @param file_path    Path to .fnt file
///////////////////////////////////////////////////////////////////////////////
void GlyphInfo_Populate(const char *file_path);

///////////////////////////////////////////////////////////////////////////////
/// @brief Returns a pointer toa glyph_info struct for a given glyph
///
/// @param glyph    UTF-32 value of the glyph
///
/// @return Pointer to the glyph_info struct for a glyph
///////////////////////////////////////////////////////////////////////////////
const glyph_info * GlyphInfo_Get(int glyph);

///////////////////////////////////////////////////////////////////////////////
/// @brief Frees the memory allocated from populating
///////////////////////////////////////////////////////////////////////////////
void GlyphInfo_Cleanup(void);

#endif
