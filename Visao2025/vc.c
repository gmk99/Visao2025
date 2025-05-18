#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vc.h"


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



int vc_rgb_get_green(IVC* image)
{
    if (image == NULL || image->data == NULL || image->channels != 3)
        return 0;

    int x, y;
    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width; x++)
        {
            int pos = (y * image->width + x) * 3;
            image->data[pos + 0] = image->data[pos + 1]; // Red <- Green
            image->data[pos + 2] = image->data[pos + 1]; // Blue <- Green
        }
    }
    return 1;
}
