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

void teatime_demo(void);

void TEA_cpu_encrypt(const uint32_t input[2],
                   const uint32_t key[4],
                   uint32_t output[2], uint16_t rounds)
{
    uint32_t v0 = input[0];
    uint32_t v1 = input[1];
    const uint32_t DELTA = 0x9e3779b9;
    uint32_t sum = 0;
    for (uint16_t i = 0; i < rounds; ++i) {
        sum += DELTA;
        v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
    }
    output[0] = v0;
    output[1] = v1;
}

void TEA_cpu_decrypt(const uint32_t input[2],
                   const uint32_t key[4],
                   uint32_t output[2], uint16_t rounds)
{
    uint32_t v0 = input[0];
    uint32_t v1 = input[1];
    const uint32_t DELTA = 0x9e3779b9;
    uint32_t sum = DELTA * rounds;
    for (uint16_t i = 0; i < rounds; ++i) {
        v1 -= ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
        v0 -= ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        sum -= DELTA;
    }
    output[0] = v0;
    output[1] = v1;
}

void teapot_reshape(int w, int h)
{
    if (h == 0)
        h = 1;
    float ratio = ((float)w) / ((float)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45, ratio, 1, 1000);
    glMatrixMode(GL_MODELVIEW);
}

void teapot_render(void)
{
    float lpos[4] = { 1, 0.5, 1, 0};
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);
    glLoadIdentity();
	gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, -1.0, 0.0f, 1.0f, 0.0f);
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);
	glutSolidTeapot(1);
	glutSwapBuffers();
}

void teapot_idle(void)
{
    static int _demo = 0;
    if (_demo++ == 0)
        teatime_demo();
}

#define INPUT_SZ 64
#define TEA_ROUNDS 32

void teatime_demo(void)
{
    int rc = 0;
    teatime_t *tea = NULL;
    do {
        uint32_t ilen = INPUT_SZ;
        uint32_t input[INPUT_SZ];
        uint32_t olen = INPUT_SZ;
        uint32_t output[INPUT_SZ];
        uint32_t elen = INPUT_SZ;
        uint32_t expected[INPUT_SZ];
        uint32_t ikey[4] = { 0xDEADBEEF, 0xCAFEFACE, 0xFACEB00C, 0xF00D1337 };
        uint32_t rounds = TEA_ROUNDS;
        for (uint32_t i = 0; i < ilen; ++i)
            input[i] = 0xFFFF0000 |(i + 1) * 5;
        for (uint32_t i = 0; i < olen; ++i)
            output[i] = 0;
        for (uint32_t i = 0; i < elen; ++i)
            expected[i] = 0;
        tea = teatime_setup();
        if (!tea) {
            rc = -ENOMEM;
            break;
        }
        teatime_print_version(stdout);
        rc = teatime_set_viewport(tea, ilen);
        if (rc < 0)
            break;
        /* Check encryption */
        rc = teatime_create_textures(tea, input, ilen);
        if (rc < 0)
            break;
        rc = teatime_create_program(tea, teatime_encrypt_source());
        if (rc < 0)
            break;
        rc = teatime_run_program(tea, ikey, rounds);
        if (rc < 0)
           break;
        rc = teatime_read_textures(tea, output, olen);
        if (rc < 0)
            break;
        for (uint32_t i = 0; i < olen && i < ilen; i += 2) {
            TEA_cpu_encrypt(&input[i], ikey, &expected[i], rounds);
            printf("%u. Encrypting Input = %08x Output = %08x Expected = %08x\n", i,
                    input[i], output[i], expected[i]);
            printf("%u. Encrypting Input = %08x Output = %08x Expected = %08x\n", i + 1,
                    input[i + 1], output[i + 1], expected[i + 1]);
        }
        for (uint32_t i = 0; i < elen; ++i)
            expected[i] = 0;
        for (uint32_t i = 0; i < olen && i < elen; i += 2) {
            TEA_cpu_decrypt(&output[i], ikey, &expected[i], rounds);
            printf("%u. Decrypting Input = %08x Output = %08x Expected = %08x\n", i, output[i],
                    expected[i], input[i]);
            printf("%u. Decrypting Input = %08x Output = %08x Expected = %08x\n", i + 1,
                    output[i + 1], expected[i + 1], input[i + 1]);
        }
        teatime_delete_textures(tea);
        teatime_delete_program(tea);
        /* check decryption */
        for (uint32_t i = 0; i < elen; ++i)
            expected[i] = 0;
        rc = teatime_create_textures(tea, output, olen);
        if (rc < 0)
            break;
        rc = teatime_create_program(tea, teatime_decrypt_source());
        if (rc < 0)
            break;
        rc = teatime_run_program(tea, ikey, rounds);
        if (rc < 0)
           break;
        rc = teatime_read_textures(tea, expected, elen);
        if (rc < 0)
            break;
        for (uint32_t i = 0; i < olen && i < elen; i++) {
            printf("%u. Decrypting Input = %08x Output = %08x Expected = %08x\n", i, output[i],
                    expected[i], input[i]);
        }
        teatime_delete_textures(tea);
        teatime_delete_program(tea);
    } while (0);
    teatime_cleanup(tea);
}

int main(int argc, char **argv)
{
    GLenum err = GLEW_OK;
    GLint wnd; /* window handle */
    /* initialize GLUT */
    glutInit(&argc, argv);
    wnd = glutCreateWindow(argv[0]);
    /* initialize GLEW */
    if ((err = glewInit()) != GLEW_OK) {
        fprintf(stderr, "glewInit() error: %s\n",
                (const char *)glewGetErrorString(err));
        glutDestroyWindow(wnd);
        return -1;
    }
    /* version can be checked only after a context has been established */
    if (!glewIsSupported("GL_VERSION_3_0")) {
        fprintf(stderr, "Minimum Required OpenGL version 3.0. You don't have it\n");
        glutDestroyWindow(wnd);
        return -1;
    }
    glutDisplayFunc(teapot_render);
    glutIdleFunc(teapot_idle);
    glutReshapeFunc(teapot_reshape);
    glutMainLoop();
    return 0;
}
