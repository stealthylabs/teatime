/*
 * COPYRIGHT: Stealthy Labs LLC
 * DATE: 29th May 2015
 * AUTHOR: Stealthy Labs
 * SOFTWARE: Tea Time
 */
#ifndef __TEATIME_H__
#define __TEATIME_H__

#ifdef WIN32
    #include <windows.h>
    #define int32_t INT32
    #define int8_t INT8
    #define int64_t INT64
    #define int16_t INT16
    #define uint32_t UINT32
    #define uint8_t UINT8
    #define uint64_t UINT64
    #define uint16_t UINT16
#endif
#include <GL/glew.h>
#include <GL/glut.h>

typedef struct {
    GLuint ofb; /* off-screen framebuffer */
    GLint maxtexsz; /* maximum texture size */
    GLuint tex_size; /* texture size - calculated using input size */
    GLuint itexid; /* input texture id */
    GLuint otexid; /* output texture id */
    GLuint program; /* program reference */
    GLuint shader; /* shader reference */
    GLint locn_input; /* input variable location in shader */
    GLint locn_output; /* output variable location in shader */
    GLint locn_key; /* key location in shader */
    GLint locn_rounds; /* no. of rounds location in shader */
} teatime_t;

void teatime_print_version(FILE *fp);
teatime_t *teatime_setup();
void teatime_cleanup(teatime_t *obj);
int teatime_set_viewport(teatime_t *obj, uint32_t ilen);
int teatime_create_textures(teatime_t *obj, const uint32_t *input, uint32_t ilen);
void teatime_delete_textures(teatime_t *obj);
int teatime_read_textures(teatime_t *obj, uint32_t *output, uint32_t olen);
int teatime_create_program(teatime_t *obj, const char *source);
void teatime_delete_program(teatime_t *obj);
int teatime_run_program(teatime_t *obj, const uint32_t ikey[4], uint32_t rounds);
const char *teatime_encrypt_source();
const char *teatime_decrypt_source();
int teatime_check_gl_errors(int line, const char *fn_name);
int teatime_check_gl_fb_errors(int line, const char *fn_name);

#endif /* __TEATIME_H__ */
