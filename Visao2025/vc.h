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

    // Criação de nova imagem
    IVC* vc_image_new(int width, int height, int channels, int levels);

    // Libertar memória de imagem
    int vc_image_free(IVC* image);

    // Exemplo de função: extrai o canal verde de uma imagem RGB
    int vc_rgb_get_green(IVC* image);


    // Converte imagem RGB para escala de cinza (1 canal)
    void vc_rgb_to_gray(IVC* image, unsigned char* gray);

    // Aplica filtro Sobel para bordas simples, saída binária em edges


#ifdef __cplusplus
}
#endif

#endif // VC_H
