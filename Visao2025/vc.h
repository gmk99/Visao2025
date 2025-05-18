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
        int perimeter;      // (Você deve calcular)
        int label;          // Label associado ao blob
        int circularity;    // Circularidade do blob
    } OVC;

    // Criação de nova imagem
    IVC* vc_image_new(int width, int height, int channels, int levels);

    // Libertar memória de imagem
    int vc_image_free(IVC* image);



    void vc_rgb_to_gray(IVC* image, unsigned char* gray);
    int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);




#ifdef __cplusplus
}
#endif

#endif // VC_H
