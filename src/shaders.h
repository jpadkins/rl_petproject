///////////////////////////////////////////////////////////////////////////////
/// file:           shaders.h
/// author:         Jacob Adkins - jpadkins
/// description:    Struct containing sources for different shaders
///////////////////////////////////////////////////////////////////////////////

#ifndef SHADERS_H
#define SHADERS_H

extern const struct shaders_ {
    struct {
        const char *basic;
    } vertex;
    struct {
        const char *basic;
    } fragment;
} shaders;

#endif
