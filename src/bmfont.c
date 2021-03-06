///////////////////////////////////////////////////////////////////////////////
/// \file	bmfont.c
/// \author	Jacob Adkins (jpadkins)
/// \brief	Manages a hash map of glyph metrics from a BMFont file (.fnt)
///			corresponding to a bitmap glyph atlas generated by FontBuilder,
///			Bitmap Font Generator, etc...
///////////////////////////////////////////////////////////////////////////////

#include "bmfont.h"

///////////////////////////////////////////////////////////////////////////////
/// Headers
///////////////////////////////////////////////////////////////////////////////

#define _POSIX_C_SOURCE 1
#include <string.h>
#include <glib.h>

#include "common.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////
/// Structs
///////////////////////////////////////////////////////////////////////////////

struct _BMFont {
	GHashTable *bmfont_hash;
};

///////////////////////////////////////////////////////////////////////////////
/// Static functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
static gboolean BMFont_PtrEq(gconstpointer a, gconstpointer b)
{
	return ((BMFontInfo *)a)->glyph == ((BMFontInfo *)b)->glyph;
}

///////////////////////////////////////////////////////////////////////////////
static void BMFont_AddInfo(BMFont *this, BMFontInfo *info)
{
	if (CONDBIND(
		this && this->bmfont_hash && info,
		log_warn,
		"NULL argument")) {

		if (!g_hash_table_insert(this->bmfont_hash, &info->glyph, info)) {
			logfmt_warn("Key %d already exists", info->glyph);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
int BMFont_ParseValue(char *token)
{
	char *val = NULL, *valptr = NULL, valbuff[128];

	strtok_r(token, "=", &valptr);
	if (!(val = strtok_r(NULL, "=", &valptr))) {
		logfmt_warn("Malformed key value pair: <%s>", token);
		return 0;
	}
	else {
		g_strlcpy(valbuff, val, sizeof(valbuff));
		return (int)g_ascii_strtoll(valbuff, NULL, 10);
	}
}

///////////////////////////////////////////////////////////////////////////////
BMFontInfo * BMFont_ParseLine(char *line)
{
	BMFontInfo *info = NULL;
	char *tok = NULL, *tokptr = NULL, tokbuff[512];

	// Alloc a new BMFontInfo struct
	info = g_new0(BMFontInfo, 1);

	// Move past "char" token
	tok = strtok_r(line, " ", &tokptr);

	// Token #1 is id=<glyph>
	tok = strtok_r(NULL, " ", &tokptr);
	g_strlcpy(tokbuff, tok, sizeof(tokbuff));
	info->glyph = BMFont_ParseValue(tokbuff);

	// Token #2 is x=<x coord>
	tok = strtok_r(NULL, " ", &tokptr);
	g_strlcpy(tokbuff, tok, sizeof(tokbuff));
	info->position.x = BMFont_ParseValue(tokbuff);

	// Token #3 is y=<y coord>
	tok = strtok_r(NULL, " ", &tokptr);
	g_strlcpy(tokbuff, tok, sizeof(tokbuff));
	info->position.y = BMFont_ParseValue(tokbuff);

	// Token #4 is width=<glyph width>
	tok = strtok_r(NULL, " ", &tokptr);
	g_strlcpy(tokbuff, tok, sizeof(tokbuff));
	info->size.width = BMFont_ParseValue(tokbuff);

	// Token #5 is height=<glyph height>
	tok = strtok_r(NULL, " ", &tokptr);
	g_strlcpy(tokbuff, tok, sizeof(tokbuff));
	info->size.height = BMFont_ParseValue(tokbuff);

	// Token #6 is xoffset=<x offset>
	tok = strtok_r(NULL, " ", &tokptr);
	g_strlcpy(tokbuff, tok, sizeof(tokbuff));
	info->offset.x = BMFont_ParseValue(tokbuff);

	// Token #7 is yoffset=<y offset>
	tok = strtok_r(NULL, " ", &tokptr);
	g_strlcpy(tokbuff, tok, sizeof(tokbuff));
	info->offset.y = BMFont_ParseValue(tokbuff);

	// Return new BMFontInfo
	return info;
}

///////////////////////////////////////////////////////////////////////////////
/// Public functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
BMFont * BMFont_Create(const char *filename)
{
	BMFont *this = NULL;
	BMFontInfo *info = NULL;
	g_autofree char *file = NULL;
	char *line = NULL, *lineptr = NULL, linebuff[1024];

	// Alloc new BMFont struct
	this = g_new0(BMFont, 1);

	// Alloc new GHashTable
	if (!(this->bmfont_hash = g_hash_table_new_full(
		g_int_hash,
		BMFont_PtrEq,
		NULL,
		g_free
		))) {

		log_warn("GHashTable creation failed");
		goto error_hash;
	}

	// Read contents of BMFont file
	if (!g_file_get_contents(filename, &file, NULL, NULL)) {
		logfmt_warn("File reading failed: %s", filename);
		goto error_file;
	}

	// Move to end of file header
	line = strtok_r(file, "\n", &lineptr);
	while (strncmp(line, "char", 4)) {
		if (!(line = strtok_r(NULL, "\n", &lineptr))) {
			log_warn("BMFont file malformed");
			goto error_head;
		}
	}

	// Construct and insert a BMFontInfo into for each line
	do {
		g_strlcpy(linebuff, line, sizeof(linebuff));
		if (!(info = BMFont_ParseLine(linebuff))) {
			log_warn("Malformed line");
			goto error_info;
		}
		BMFont_AddInfo(this, info);
	} while ((line = strtok_r(NULL, "\n", &lineptr)), line);

	return this;

error_info:
error_head:
error_file:

	g_hash_table_destroy(this->bmfont_hash);

error_hash:

	g_free(this);
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
BMFontInfo const * BMFont_GetInfoPtr(BMFont *this, int glyph)
{
	if (!this) {
		log_warn("Null argument");
		return NULL;
	}
	else {
		return g_hash_table_lookup(this->bmfont_hash, &glyph);
	}
}

///////////////////////////////////////////////////////////////////////////////
void BMFont_Destroy(BMFont *this)
{
	if (CONDBIND(this && this->bmfont_hash, log_warn, "NULL argument")) {
		g_hash_table_destroy(this->bmfont_hash);
	}
	if (CONDBIND(this, log_warn, "NULL argument")) {
		g_free(this);
	}
}
