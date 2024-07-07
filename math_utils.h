#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <math.h>

typedef struct {
    float m[4][4];
} Mat4;

void mat4_identity(Mat4 *mat);
void mat4_ortho(Mat4 *mat, float left, float right, float bottom, float top, float near, float far);
void mat4_translate(Mat4 *mat, float x, float y, float z);

#endif // MATH_UTILS_H