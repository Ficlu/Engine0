#include "math_utils.h"

void mat4_identity(Mat4 *mat) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mat->m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

void mat4_ortho(Mat4 *mat, float left, float right, float bottom, float top, float near, float far) {
    mat4_identity(mat);
    mat->m[0][0] = 2.0f / (right - left);
    mat->m[1][1] = 2.0f / (top - bottom);
    mat->m[2][2] = -2.0f / (far - near);
    mat->m[3][0] = -(right + left) / (right - left);
    mat->m[3][1] = -(top + bottom) / (top - bottom);
    mat->m[3][2] = -(far + near) / (far - near);
}

void mat4_translate(Mat4 *mat, float x, float y, float z) {
    mat->m[3][0] += x;
    mat->m[3][1] += y;
    mat->m[3][2] += z;
}