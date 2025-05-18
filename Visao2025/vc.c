#include <stdlib.h>
#include <string.h>
#include "vc.h"

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


///teste01

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
            image->data[pos + 0] = image->data[pos + 2]; // Red <- Green
            image->data[pos + 2] = image->data[pos + 1]; // Blue <- Green
        }
    }
    return 1;
}
