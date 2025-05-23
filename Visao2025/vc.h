#pragma once
#ifndef VC_H
#define VC_H

#ifdef __cplusplus
extern "C"
{
#endif

    // Estrutura de imagem
    typedef struct
    {
        unsigned char* data; // Dados da imagem
        int width, height;   // Dimensões da imagem
        int channels;        // Número de canais (1=BW, 3=RGB)
        int levels;          // Níveis de cinzento por canal (por ex: 256)
    } IVC;

    typedef struct {
        int x, y;           // Coordenadas do centro
        int width, height;  // Dimensões do bounding box
        int area;           // Número de píxeis do blob
        int perimeter;      // Perímetro real do blob (em píxeis)
        int bb_perimeter;   // Perímetro aproximado do bounding box (usado para circularidade)
        int label;          // Label associado ao blob
        float circularity;  // Circularidade do blob
    } OVC;

    typedef struct {
        const char* coin_name;
        int r_avg;
        int g_avg;
        int b_avg;
    } CoinResult;

    // Criação de nova imagem
    IVC* vc_image_new(int width, int height, int channels, int levels);

    // WORKS
    int vc_image_free(IVC* image);
    void vc_rgb_to_gray(IVC* image, unsigned char* gray);
    int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);
    int vc_gaussian_blur(IVC* src, IVC* dst, int ksize, float sigma);
    OVC* vc_detect_blobs(IVC* src, int* nblobs);
    OVC* vc_filter_circular_blobs(OVC* blobs, int* nblobs, float min_circularity);

    //WIP
    CoinResult* vc_identify_coin(IVC* image_rgb, IVC* image_bin, OVC* blob);

#ifdef __cplusplus
}
#endif

#endif // VC_H