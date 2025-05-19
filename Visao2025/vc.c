#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vc.h"

#define M_PI 3.14159265358979323846


#define MAX_LABELS 10000


/* Structs for coin color ranges */
typedef struct {
    int r_min, g_min, b_min;
    int r_max, g_max, b_max;
} ColorRange;

typedef struct {
    char* name;
    ColorRange range;
} CoinType;

/* Color ranges for all the different coins */
#define NUM_COINS 8
/* Adjusted initialization of the `coins` array to fix the "muitos valores de inicializadores" error.
   The issue was caused by incorrect nesting of the `ColorRange` struct. */

CoinType coins[NUM_COINS] = {
    {"1c",  {20, 50, 20, 40, 70, 60}},
    {"2c",  {20, 50, 20, 40, 70, 60}},
    {"5c",  {20, 50, 20, 40, 70, 60}},
    {"10c", {85, 80, 55, 115, 110, 75}},
    {"20c", {85, 80, 55, 115, 110, 75}},
    {"50c", {85, 80, 55, 115, 110, 75}},
    {"1eur",{170, 170, 170, 220, 220, 220}},
    {"2eur",{100, 100, 100, 150, 150, 150}}
};

/** ====================================================== DANGER ZONE ====================================================== */

// Converte imagem RGB para escala de cinza (grayscale)
void vc_rgb_to_gray(IVC* image, unsigned char* gray) {
    for (int i = 0; i < image->width * image->height; i++) {
        int r = image->data[3 * i];
        int g = image->data[3 * i + 1];
        int b = image->data[3 * i + 2];
        gray[i] = (unsigned char)((0.299 * r) + (0.587 * g) + (0.114 * b));
    }
}

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold) {
    if (src == NULL || dst == NULL || src->channels != 1 || dst->channels != 1)
        return 0;

    for (int i = 0; i < src->width * src->height; i++) {
        dst->data[i] = (src->data[i] >= threshold) ? 255 : 0;
    }

    return 1;
}

// --- Apply 2D convolution (zero‐padding) on single‐channel 8‑bit image ---
int vc_gaussian_blur(IVC* src, IVC* dst, int ksize, float sigma) {
    if (!src || !dst) return 0;
    if (src->channels != 1 || dst->channels != 1) return 0;
    if (src->width != dst->width || src->height != dst->height) return 0;
    if (ksize % 2 == 0 || ksize < 3) return 0;  // kernel must be odd ≥3

    int w = src->width, h = src->height, n = w * h;
    int half = ksize / 2;

    // build & normalize 2D Gaussian kernel
    float* kernel = malloc(ksize * ksize * sizeof(float));
    if (!kernel) return 0;
    float sum = 0.0f, s2 = 2.0f * sigma * sigma;
    for (int y = -half; y <= half; y++) {
        for (int x = -half; x <= half; x++) {
            float v = expf(-(x * x + y * y) / s2) / (M_PI * s2);
            kernel[(y + half) * ksize + (x + half)] = v;
            sum += v;
        }
    }
    for (int i = 0; i < ksize * ksize; i++) {
        kernel[i] /= sum;
    }

    // allocate temp if in-place
    unsigned char* tmp = NULL;
    unsigned char* out = dst->data;
    if (src == dst) {
        tmp = malloc(n);
        if (!tmp) { free(kernel); return 0; }
        out = tmp;
    }

    // convolve (zero‑padding)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float acc = 0.0f;
            for (int ky = -half; ky <= half; ky++) {
                int yy = y + ky;
                if (yy < 0 || yy >= h) continue;
                for (int kx = -half; kx <= half; kx++) {
                    int xx = x + kx;
                    if (xx < 0 || xx >= w) continue;
                    float kval = kernel[(ky + half) * ksize + (kx + half)];
                    acc += kval * src->data[yy * w + xx];
                }
            }
            int iv = (int)(acc + 0.5f);
            out[y * w + x] = (unsigned char)(iv < 0 ? 0 : (iv > 255 ? 255 : iv));
        }
    }

    // if we blurred into tmp, copy back
    if (tmp) {
        memcpy(dst->data, tmp, n);
        free(tmp);
    }

    free(kernel);
    return 1;
}


#define MIN_AREA 200 // Minimum area for a blob to be considered valid

// --- Detect black blobs in a binary image with minimum area ---
OVC* vc_detect_blobs(IVC* src, int* nblobs) {
    if (!src || src->channels != 1 || src->width <= 0 || src->height <= 0) {
        *nblobs = 0;
        return NULL;
    }

    int width = src->width, height = src->height;
    int* labels = (int*)calloc(width * height, sizeof(int)); // Label map
    OVC* blobs = (OVC*)malloc(MAX_LABELS * sizeof(OVC)); // Blob array
    if (!labels || !blobs) {
        free(labels);
        free(blobs);
        *nblobs = 0;
        return NULL;
    }

    int current_label = 0;
    *nblobs = 0;

    // First pass: Label connected components
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pos = y * width + x;
            if (src->data[pos] == 0 && labels[pos] == 0) { // Black pixel, unlabeled
                if (current_label >= MAX_LABELS) break;

                // Initialize blob
                blobs[current_label].area = 0;
                blobs[current_label].x = 0;
                blobs[current_label].y = 0;
                blobs[current_label].width = 0;
                blobs[current_label].height = 0;
                blobs[current_label].label = current_label + 1;
                blobs[current_label].perimeter = 0;
                blobs[current_label].circularity = 0;

                // Flood-fill
                int min_x = x, max_x = x, min_y = y, max_y = y;
                int area = 0;
                int sum_x = 0, sum_y = 0;

                int stack_size = width * height;
                int* stack = (int*)malloc(stack_size * sizeof(int));
                int stack_top = 0;
                stack[stack_top++] = pos;

                while (stack_top > 0) {
                    int p = stack[--stack_top];
                    int px = p % width;
                    int py = p / width;
                    if (labels[p] != 0 || src->data[p] != 0) continue;

                    labels[p] = current_label + 1;
                    area++;
                    sum_x += px;
                    sum_y += py;
                    if (px < min_x) min_x = px;
                    if (px > max_x) max_x = px;
                    if (py < min_y) min_y = py;
                    if (py > max_y) max_y = py;

                    // Check 4-connectivity
                    if (px + 1 < width && stack_top < stack_size) stack[stack_top++] = p + 1;
                    if (px - 1 >= 0 && stack_top < stack_size) stack[stack_top++] = p - 1;
                    if (py + 1 < height && stack_top < stack_size) stack[stack_top++] = p + width;
                    if (py - 1 >= 0 && stack_top < stack_size) stack[stack_top++] = p - width;
                }

                free(stack);

                // Only store blob if area meets minimum requirement
                if (area >= MIN_AREA) {
                    blobs[current_label].area = area;
                    blobs[current_label].x = sum_x / area;
                    blobs[current_label].y = sum_y / area;
                    blobs[current_label].width = max_x - min_x + 1;
                    blobs[current_label].height = max_y - min_y + 1;
                    current_label++;
                }
            }
        }
    }

    free(labels);
    *nblobs = current_label;
    if (current_label == 0) {
        free(blobs);
        return NULL;
    }

    // Reallocate to exact size
    blobs = (OVC*)realloc(blobs, current_label * sizeof(OVC));
    return blobs;
}




/*
OVC* vc_detect_blobs(IVC* src, int* nblobs) {
    if (!src || src->channels != 1 || src->width <= 0 || src->height <= 0) {
        *nblobs = 0;
        return NULL;
    }

    int width = src->width, height = src->height;
    int* labels = (int*)calloc(width * height, sizeof(int)); // Label map
    OVC* blobs = (OVC*)malloc(MAX_LABELS * sizeof(OVC)); // Blob array
    if (!labels || !blobs) {
        free(labels);
        free(blobs);
        *nblobs = 0;
        return NULL;
    }

    int current_label = 0;
    *nblobs = 0;

    // First pass: Label connected components
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pos = y * width + x;
            if (src->data[pos] == 0 && labels[pos] == 0) { // Black pixel, unlabeled
                if (current_label >= MAX_LABELS) break;

                // Initialize blob
                blobs[current_label].area = 0;
                blobs[current_label].x = 0;
                blobs[current_label].y = 0;
                blobs[current_label].width = 0;
                blobs[current_label].height = 0;
                blobs[current_label].label = current_label + 1;
                blobs[current_label].perimeter = 0;
                blobs[current_label].circularity = 0;

                // Flood-fill
                int min_x = x, max_x = x, min_y = y, max_y = y;
                int area = 0;
                int sum_x = 0, sum_y = 0;

                int stack_size = width * height;
                int* stack = (int*)malloc(stack_size * sizeof(int));
                int stack_top = 0;
                stack[stack_top++] = pos;

                while (stack_top > 0) {
                    int p = stack[--stack_top];
                    int px = p % width;
                    int py = p / width;
                    if (labels[p] != 0 || src->data[p] != 0) continue;

                    labels[p] = current_label + 1;
                    area++;
                    sum_x += px;
                    sum_y += py;
                    if (px < min_x) min_x = px;
                    if (px > max_x) max_x = px;
                    if (py < min_y) min_y = py;
                    if (py > max_y) max_y = py;

                    // Check 4-connectivity
                    if (px + 1 < width && stack_top < stack_size) stack[stack_top++] = p + 1;
                    if (px - 1 >= 0 && stack_top < stack_size) stack[stack_top++] = p - 1;
                    if (py + 1 < height && stack_top < stack_size) stack[stack_top++] = p + width;
                    if (py - 1 >= 0 && stack_top < stack_size) stack[stack_top++] = p - width;
                }

                free(stack);

                if (area > 0) {
                    blobs[current_label].area = area;
                    blobs[current_label].x = sum_x / area;
                    blobs[current_label].y = sum_y / area;
                    blobs[current_label].width = max_x - min_x + 1;
                    blobs[current_label].height = max_y - min_y + 1;
                    current_label++;
                }
            }
        }
    }

    free(labels);
    *nblobs = current_label;
    if (current_label == 0) {
        free(blobs);
        return NULL;
    }

    // Reallocate to exact size
    blobs = (OVC*)realloc(blobs, current_label * sizeof(OVC));
    return blobs;
}
*/






/** ========================================================================================================================= */

IVC* vc_image_new(int width, int height, int channels, int levels)
{
    IVC* image = (IVC*)malloc(sizeof(IVC));
    if (image == NULL)
        return NULL;

    image->width = width;
    image->height = height;
    image->channels = channels;
    image->levels = levels;
    image->data = (unsigned char*)malloc(width * height * channels);

    if (image->data == NULL)
    {
        free(image);
        return NULL;
    }

    return image;
}

int vc_image_free(IVC* image)
{
    if (image != NULL)
    {
        if (image->data != NULL)
            free(image->data);
        free(image);
        return 1;
    }
    return 0;
}
