/*
 * COPYRIGHT: Stealthy Labs LLC
 * DATE: 29th May 2015
 * AUTHOR: Stealthy Labs
 * SOFTWARE: Tea Time
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <teatime.h>

static int teatime_check_gl_version(uint32_t *major, uint32_t *minor);
static int teatime_check_program_errors(GLuint program);
static int teatime_check_shader_errors(GLuint shader);

#define TEATIME_BREAKONERROR(FN,RC)  if ((RC = teatime_check_gl_errors(__LINE__, #FN )) < 0) break
#define TEATIME_BREAKONERROR_FB(FN,RC)  if ((RC = teatime_check_gl_fb_errors(__LINE__, #FN )) < 0) break

teatime_t *teatime_setup()
{
    int rc = 0;
    teatime_t *obj = calloc(1, sizeof(teatime_t));
    if (!obj) {
        fprintf(stderr, "Out of memory allocating %zu bytes\n",
                sizeof(teatime_t));
        return NULL;
    }
    do {
        uint32_t version[2] = { 0, 0};
        if (teatime_check_gl_version(&version[0], &version[1]) < 0) {
            fprintf(stderr, "Unable to verify OpenGL version\n");
            rc = -1;
            break;
        }
        if (version[0] < 3) {
            fprintf(stderr, "Minimum Required OpenGL version 3.0. You have %u.%u\n",
                    version[0], version[1]);
            rc = -1;
            break;
        }
        /* initialize off-screen framebuffer */
        /*
         * This is the EXT_framebuffer_object OpenGL extension that allows us to
         * use an offscreen buffer as a target for rendering operations such as
         * vector calculations, providing full precision and removing unwanted
         * clamping issues.
         * we are turning off the traditional framebuffer here apparently.
         */
        glGenFramebuffersEXT(1, &(obj->ofb));
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, obj->ofb);
        TEATIME_BREAKONERROR(glBindFramebufferEXT, rc);
        fprintf(stderr, "Successfully created off-screen framebuffer with id: %d\n",
                obj->ofb);
        /* get the texture size */
        obj->maxtexsz = -1;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(obj->maxtexsz));
        fprintf(stderr, "Maximum Texture size for the GPU: %d\n", obj->maxtexsz);
        obj->itexid = obj->otexid = 0;
        obj->shader = obj->program = 0;
    } while (0);
    if (rc < 0) {
        teatime_cleanup(obj);
        obj = NULL;
    }
    return obj;
}

void teatime_cleanup(teatime_t *obj)
{
    if (obj) {
        teatime_delete_program(obj);
        teatime_delete_textures(obj);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        glDeleteFramebuffersEXT(1, &(obj->ofb));
        glFlush();
        free(obj);
        obj = NULL;
    }
}

int teatime_set_viewport(teatime_t *obj, uint32_t ilen)
{
    uint32_t texsz = (uint32_t)((long)(sqrt(ilen / 4.0)));
    if (obj && texsz > 0 && texsz < (GLuint)obj->maxtexsz) {
        /* viewport mapping 1:1 pixel = texel = data mapping */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, texsz, 0.0, texsz);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glViewport(0, 0, texsz, texsz);
        obj->tex_size = texsz;
        fprintf(stderr, "Texture size: %u x %u\n", texsz, texsz);
        return 0;
    } else if (obj) {
        fprintf(stderr, "Max. texture size is %d. Calculated: %u from input length: %u\n",
                obj->maxtexsz, texsz, ilen);
    }
    return -EINVAL;
}


int teatime_create_textures(teatime_t *obj, const uint32_t *input, uint32_t ilen)
{
    if (obj && input && ilen > 0) {
        int rc = 0;
        do {
            uint32_t texsz = (uint32_t)((long)(sqrt(ilen / 4.0)));
            if (texsz != obj->tex_size) {
                fprintf(stderr, "Viewport texture size(%u) != Input texture size (%u)\n",
                        obj->tex_size, texsz);
                rc = -EINVAL;
                break;
            }
            glGenTextures(1, &(obj->itexid));
            glGenTextures(1, &(obj->otexid));
            fprintf(stderr, "Created input texture with ID: %u\n", obj->itexid);
            fprintf(stderr, "Created output texture with ID: %u\n", obj->otexid);
            /** BIND ONE TEXTURE AT A TIME **/
            /* the texture target can vary depending on GPU */
            glBindTexture(GL_TEXTURE_2D, obj->itexid);
            TEATIME_BREAKONERROR(glBindTexture, rc);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            /* turn off filtering and set proper wrap mode - this is obligatory for
             * floating point textures */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            /* create a 2D texture of the same size as the data
             * internal format: GL_RGBA32UI_EXT
             * texture format: GL_RGBA_INTEGER
             * texture type: GL_UNSIGNED_INT
             */
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI_EXT,
                    texsz, texsz, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, NULL);
            TEATIME_BREAKONERROR(glTexImage2D, rc);
            /* transfer data to texture */
#ifdef WIN32
            glTexSubImage2D
#else
            glTexSubImage2DEXT
#endif
                (GL_TEXTURE_2D, 0, 0, 0, obj->tex_size,
                    obj->tex_size, GL_RGBA_INTEGER, GL_UNSIGNED_INT, input);
#ifdef WIN32
            TEATIME_BREAKONERROR(glTexSubImage2D, rc);
#else
            TEATIME_BREAKONERROR(glTexSubImage2DEXT, rc);
#endif
            fprintf(stderr, "Successfully transferred input data to texture ID: %u\n", obj->itexid);
            /* BIND the OUTPUT texture and work on it */
            /* the texture target can vary depending on GPU */
            glBindTexture(GL_TEXTURE_2D, obj->otexid);
            TEATIME_BREAKONERROR(glBindTexture, rc);
            /* turn off filtering and set proper wrap mode - this is obligatory for
             * floating point textures */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            TEATIME_BREAKONERROR(glTexParameteri, rc);
            /* create a 2D texture of the same size as the data
             * internal format: GL_RGBA32UI_EXT
             * texture format: GL_RGBA_INTEGER
             * texture type: GL_UNSIGNED_INT
             */
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI_EXT,
                    texsz, texsz, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, NULL);
            TEATIME_BREAKONERROR(glTexImage2D, rc);
            /* change tex-env to replace instead of the default modulate */
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            TEATIME_BREAKONERROR(glTexEnvi, rc);
            /* attach texture */
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                    GL_TEXTURE_2D, obj->otexid, 0);
            TEATIME_BREAKONERROR(glFramebufferTexture2DEXT, rc);
            TEATIME_BREAKONERROR_FB(glFramebufferTexture2DEXT, rc);
            glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
            TEATIME_BREAKONERROR(glDrawBuffer, rc);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                    GL_TEXTURE_2D, obj->otexid, 0);
            TEATIME_BREAKONERROR(glFramebufferTexture2DEXT, rc);
            TEATIME_BREAKONERROR_FB(glFramebufferTexture2DEXT, rc);
            rc = 0;
        } while (0);
        return rc;
    }
    return -EINVAL;
}

int teatime_read_textures(teatime_t *obj, uint32_t *output, uint32_t olen)
{
    if (obj && output && olen > 0 && obj->otexid > 0) {
        int rc = 0;
        do {
            uint32_t texsz = (uint32_t)((long)(sqrt(olen / 4.0)));
            if (texsz != obj->tex_size) {
                fprintf(stderr, "Viewport texture size(%u) != Input texture size (%u)\n",
                        obj->tex_size, texsz);
                rc = -EINVAL;
                break;
            }
            /* read the texture back */
            glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
            TEATIME_BREAKONERROR(glReadBuffer, rc);
            glReadPixels(0, 0, obj->tex_size, obj->tex_size, GL_RGBA_INTEGER, GL_UNSIGNED_INT, output);
            TEATIME_BREAKONERROR(glReadPixels, rc);
            fprintf(stderr, "Successfully read data from the texture\n");
        } while (0);
        return rc;
    }
    return -EINVAL;
}

int teatime_create_program(teatime_t *obj, const char *source)
{
    if (obj && source) {
        int rc = 0;
        do {
            obj->program = glCreateProgram();
            TEATIME_BREAKONERROR(glCreateProgram, rc);
            obj->shader = glCreateShader(GL_FRAGMENT_SHADER_ARB);
            TEATIME_BREAKONERROR(glCreateShader, rc);
            glShaderSource(obj->shader, 1, &source, NULL);
            TEATIME_BREAKONERROR(glShaderSource, rc);
            glCompileShader(obj->shader);
            rc = teatime_check_shader_errors(obj->shader);
            if (rc < 0) break;
            TEATIME_BREAKONERROR(glCompileShader, rc);
            glAttachShader(obj->program, obj->shader);
            TEATIME_BREAKONERROR(glAttachShader, rc);
            glLinkProgram(obj->program);
            rc = teatime_check_program_errors(obj->program);
            if (rc < 0) break;
            TEATIME_BREAKONERROR(glLinkProgram, rc);
            obj->locn_input = glGetUniformLocation(obj->program, "idata");
            TEATIME_BREAKONERROR(glGetUniformLocation, rc);
            obj->locn_output = glGetUniformLocation(obj->program, "odata");
            TEATIME_BREAKONERROR(glGetUniformLocation, rc);
            obj->locn_key = glGetUniformLocation(obj->program, "ikey");
            TEATIME_BREAKONERROR(glGetUniformLocation, rc);
            obj->locn_rounds = glGetUniformLocation(obj->program, "rounds");
            TEATIME_BREAKONERROR(glGetUniformLocation, rc);
            rc = 0;
        } while (0);
        return rc;
    }
    return -EINVAL;
}

int teatime_run_program(teatime_t *obj, const uint32_t ikey[4], uint32_t rounds)
{
    if (obj && obj->program > 0) {
        int rc = 0;
        do {
            glUseProgram(obj->program);
            TEATIME_BREAKONERROR(glUseProgram, rc);
            glActiveTexture(GL_TEXTURE0);	
            TEATIME_BREAKONERROR(glActiveTexture, rc);
            glBindTexture(GL_TEXTURE_2D, obj->itexid);
            TEATIME_BREAKONERROR(glBindTexture, rc);
            glUniform1i(obj->locn_input, 0);
            TEATIME_BREAKONERROR(glUniform1i, rc);
            glActiveTexture(GL_TEXTURE1);	
            TEATIME_BREAKONERROR(glActiveTexture, rc);
            glBindTexture(GL_TEXTURE_2D, obj->otexid);
            TEATIME_BREAKONERROR(glBindTexture, rc);
            glUniform1i(obj->locn_output, 1);
            TEATIME_BREAKONERROR(glUniform1i, rc);
            glUniform4uiv(obj->locn_key, 1, ikey);
            TEATIME_BREAKONERROR(glUniform1uiv, rc);
            glUniform1ui(obj->locn_rounds, rounds);
            TEATIME_BREAKONERROR(glUniform1ui, rc);
            glFinish();
            glPolygonMode(GL_FRONT, GL_FILL);
            /* render */
            glBegin(GL_QUADS);
                glTexCoord2i(0, 0);
                glVertex2i(0, 0);
                //glTexCoord2i(obj->tex_size, 0);
                glTexCoord2i(1, 0);
                glVertex2i(obj->tex_size, 0);
                //glTexCoord2i(obj->tex_size, obj->tex_size);
                glTexCoord2i(1, 1);
                glVertex2i(obj->tex_size, obj->tex_size);
                glTexCoord2i(0, 1);
                //glTexCoord2i(0, obj->tex_size);
                glVertex2i(0, obj->tex_size);
            glEnd();
            glFinish();
            TEATIME_BREAKONERROR_FB(Rendering, rc);
            TEATIME_BREAKONERROR(Rendering, rc);
            rc = 0;
        } while (0);
        return rc;
    }
    return -EINVAL;
}

void teatime_delete_textures(teatime_t *obj)
{
    if (obj) {
        if (obj->itexid != 0) {
            glDeleteTextures(1, &(obj->itexid));
            obj->itexid = 0;
        }
        if (obj->otexid != 0) {
            glDeleteTextures(1, &(obj->otexid));
            obj->otexid = 0;
        }
    }
}

void teatime_delete_program(teatime_t *obj)
{
    if (obj) {
        if (obj->shader > 0 && obj->program > 0) {
            glDetachShader(obj->program, obj->shader);
        }
        if (obj->shader > 0)
            glDeleteShader(obj->shader);
        if (obj->program > 0)
            glDeleteProgram(obj->program);
        obj->shader = 0;
        obj->program = 0;
    }
}

void teatime_print_version(FILE *fp)
{
    const GLubyte *version = NULL;
    version = glGetString(GL_VERSION);
    fprintf(fp, "GL Version: %s\n", (const char *)version);
    version = glGetString(GL_SHADING_LANGUAGE_VERSION);
    fprintf(fp, "GLSL Version: %s\n", (const char *)version);
    version = glGetString(GL_VENDOR);
    fprintf(fp, "GL Vendor: %s\n", (const char *)version);
}

int teatime_check_gl_version(uint32_t *major, uint32_t *minor)
{
    const GLubyte *version = NULL;
    version = glGetString(GL_VERSION);
    if (version) {
        uint32_t ver[2] = { 0, 0 };
        char *endp = NULL;
        char *endp2 = NULL;
        errno = 0;
        ver[0] = strtol((const char *)version, &endp, 10);
        if (errno == ERANGE || (const void *)endp == (const void *)version) {
            fprintf(stderr, "Version string %s cannot be parsed\n", (const char *)version);
            return -1;
        }
        /* endp[0] = '.' and endp[1] points to minor */
        errno = 0;
        ver[1] = strtol((const char *)&endp[1], &endp2, 10);
        if (errno == ERANGE || endp2 == &endp[1]) {
            fprintf(stderr, "Version string %s cannot be parsed\n", (const char *)version);
            return -1;
        }
        if (major)
            *major = ver[0];
        if (minor)
            *minor = ver[1];
        return 0;
    }
    return -1;
}

int teatime_check_gl_errors(int line, const char *fn_name)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        const GLubyte *estr = gluErrorString(err);
        fprintf(stderr, "%s(): GL Error(%d) on line %d: %s\n", fn_name,
                err, line, (const char *)estr);
        return -1;
    }
    return 0;
}

int teatime_check_gl_fb_errors(int line, const char *fn_name)
{
    GLenum st = (GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch (st) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
        return 0;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        fprintf(stderr, "%s(): GL FB error on line %d: incomplete attachment\n", fn_name, line);
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        fprintf(stderr, "%s(): GL FB error on line %d: unsupported\n", fn_name, line);
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        fprintf(stderr, "%s(): GL FB error on line %d: incomplete missing attachment\n", fn_name, line);
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        fprintf(stderr, "%s(): GL FB error on line %d: incomplete dimensions\n", fn_name, line);
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        fprintf(stderr, "%s(): GL FB error on line %d: incomplete formats\n", fn_name, line);
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        fprintf(stderr, "%s(): GL FB error on line %d: incomplete draw buffer\n", fn_name, line);
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        fprintf(stderr, "%s(): GL FB error on line %d: incomplete read buffer\n", fn_name, line);
        break;
    default:
        fprintf(stderr, "%s(): GL FB error on line %d: Unknown. Error Value: %d\n", fn_name, line, st);
        break;
    }
    return -1;
}

int teatime_check_program_errors(GLuint program)
{
    GLint ilen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ilen);
    if (ilen > 1) {
        GLsizei wb = 0;
        GLchar *buf = calloc(ilen, sizeof(GLchar));
        if (!buf) {
            fprintf(stderr, "Out of memory allocating %d bytes\n", ilen);
            return -ENOMEM;
        }
        glGetProgramInfoLog(program, ilen, &wb, buf);
        buf[wb] = '\0';
        fprintf(stderr, "Program Errors:\n%s\n",
                (const char *)buf);
        free(buf);
    }
    return 0;
}

int teatime_check_shader_errors(GLuint shader)
{
    GLint ilen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &ilen);
    if (ilen > 1) {
        GLsizei wb = 0;
        GLchar *buf = calloc(ilen, sizeof(GLchar));
        if (!buf) {
            fprintf(stderr, "Out of memory allocating %d bytes\n", ilen);
            return -ENOMEM;
        }
        glGetShaderInfoLog(shader, ilen, &wb, buf);
        buf[wb] = '\0';
        fprintf(stderr, "Shader Errors:\n%s\n",
                (const char *)buf);
        free(buf);
    }
    return 0;
}

#define TEA_ENCRYPT_SOURCE \
"#version 130\n" \
"#extension GL_EXT_gpu_shader4 : enable\n" \
"uniform usampler2D idata;\n" \
"uniform uvec4 ikey; \n" \
"uniform uint rounds; \n" \
"out uvec4 odata; \n" \
"void main(void) {\n" \
" uvec4 x = texture(idata, gl_TexCoord[0].st);\n" \
" uint delta = uint(0x9e3779b9); \n" \
" uint sum = uint(0); \n" \
" for (uint i = uint(0); i < rounds; ++i) {\n" \
"  sum += delta; \n" \
"  x[0] += (((x[1] << 4) + ikey[0]) ^ (x[1] + sum)) ^ ((x[1] >> 5) + ikey[1]);\n" \
"  x[1] += (((x[0] << 4) + ikey[2]) ^ (x[0] + sum)) ^ ((x[0] >> 5) + ikey[3]);\n" \
"  x[2] += (((x[3] << 4) + ikey[0]) ^ (x[3] + sum)) ^ ((x[3] >> 5) + ikey[1]);\n" \
"  x[3] += (((x[2] << 4) + ikey[2]) ^ (x[2] + sum)) ^ ((x[2] >> 5) + ikey[3]);\n" \
" }\n" \
" odata = x; \n" \
"}\n"

#define TEA_DECRYPT_SOURCE \
"#version 130\n" \
"#extension GL_EXT_gpu_shader4 : enable\n" \
"uniform usampler2D idata;\n" \
"uniform uvec4 ikey; \n" \
"uniform uint rounds; \n" \
"out uvec4 odata; \n" \
"void main(void) {\n" \
" uvec4 x = texture(idata, gl_TexCoord[0].st);\n" \
" uint delta = uint(0x9e3779b9); \n" \
" uint sum = delta * rounds; \n" \
" for (uint i = uint(0); i < rounds; ++i) {\n" \
"  x[1] -= (((x[0] << 4) + ikey[2]) ^ (x[0] + sum)) ^ ((x[0] >> 5) + ikey[3]);\n" \
"  x[0] -= (((x[1] << 4) + ikey[0]) ^ (x[1] + sum)) ^ ((x[1] >> 5) + ikey[1]);\n" \
"  x[3] -= (((x[2] << 4) + ikey[2]) ^ (x[2] + sum)) ^ ((x[2] >> 5) + ikey[3]);\n" \
"  x[2] -= (((x[3] << 4) + ikey[0]) ^ (x[3] + sum)) ^ ((x[3] >> 5) + ikey[1]);\n" \
"  sum -= delta; \n" \
" }\n" \
" odata = x; \n" \
"}\n"

const char *teatime_encrypt_source()
{
    return TEA_ENCRYPT_SOURCE;
}

const char *teatime_decrypt_source()
{
    return TEA_DECRYPT_SOURCE;
}
