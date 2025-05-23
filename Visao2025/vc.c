#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vc.h"

#define M_PI 3.14159265358979323846
#define MAX_LABELS 10000

/* Estrutura para cor das moedas */
typedef struct {
    int r_min, g_min, b_min;
    int r_max, g_max, b_max;
} ColorRange;

typedef struct {
    int size_min, size_max;
} SizeRange;

typedef struct {
    char* name;
    ColorRange range;
    SizeRange sizeRange;
} CoinType;

/* Espaço de cor para todas as moedas */
#define NUM_COINS 8

CoinType coins[NUM_COINS] = {
    // {"name", {r_min, g_min, b_min, r_max, g_max, b_max}, {size_min, size_max}}
    {"1cent",  {20,  36,  65,   56,  76,  85},  {110, 130}},
    {"2cent",  {20,  36,  65,   56,  62,  82},  {130, 140}},
    {"5cent",  {20,  36,  65,   56,  76,  90},  {150, 165}},
    {"10cent", {32,  66,  72,   69, 108, 112}, {130, 150}},
    {"20cent", {30,  55,  60,   69, 108, 112}, {150, 170}},
    {"50cent", {38,  66,  72,   66, 108, 112}, {170, 185}},
    {"1eur",   {39,  53,  56,   84,  97,  97},  {165, 185}},
    {"2eur",   {60,  75,  77,   67,  83,  87},  {180, 200}}
};

/** ====================================================== ZONA DE PERIGO ====================================================== */

// Converte imagem RGB para escala de cinza
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

// --- Aplica convolução 2D (com preenchimento de zeros) em imagem de canal único de 8 bits ---
int vc_gaussian_blur(IVC* src, IVC* dst, int ksize, float sigma) {
    if (!src || !dst) return 0;
    if (src->channels != 1 || dst->channels != 1) return 0;
    if (src->width != dst->width || src->height != dst->height) return 0;
    if (ksize % 2 == 0 || ksize < 3) return 0;  // o kernel deve ser ímpar e ≥3

    int w = src->width, h = src->height, n = w * h;
    int half = ksize / 2;

    // constrói e normaliza o kernel gaussiano 2D
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

    // aloca memória temporária se for no mesmo lugar
    unsigned char* tmp = NULL;
    unsigned char* out = dst->data;
    if (src == dst) {
        tmp = malloc(n);
        if (!tmp) { free(kernel); return 0; }
        out = tmp;
    }

    // faz a convolução (com preenchimento de zeros)
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

    // se a convolução foi feita em tmp, copia de volta
    if (tmp) {
        memcpy(dst->data, tmp, n);
        free(tmp);
    }

    free(kernel);
    return 1;
}

#define MIN_AREA 200 // Área mínima para que um blob seja considerado válido

// --- Deteta blobs pretos numa imagem binária com área mínima ---
OVC* vc_detect_blobs(IVC* src, int* nblobs) {
    if (!src || src->channels != 1 || src->width <= 0 || src->height <= 0) {
        *nblobs = 0;
        return NULL;
    }

    int width = src->width, height = src->height;
    int* labels = (int*)calloc(width * height, sizeof(int)); // Mapa de etiquetas
    OVC* blobs = (OVC*)malloc(MAX_LABELS * sizeof(OVC)); // Array de blobs
    if (!labels || !blobs) {
        free(labels);
        free(blobs);
        *nblobs = 0;
        return NULL;
    }

    int current_label = 0;
    *nblobs = 0;

    // Primeira passagem: Etiqueta componentes conectados e calcula o perímetro
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pos = y * width + x;
            if (src->data[pos] == 0 && labels[pos] == 0) { // Pixel preto, não etiquetado
                if (current_label >= MAX_LABELS) break;

                // Inicializa blob
                blobs[current_label].area = 0;
                blobs[current_label].x = 0;
                blobs[current_label].y = 0;
                blobs[current_label].width = 0;
                blobs[current_label].height = 0;
                blobs[current_label].label = current_label + 1;
                blobs[current_label].perimeter = 0;
                blobs[current_label].bb_perimeter = 0;
                blobs[current_label].circularity = 0;

                // Preenchimento por inundação
                int min_x = x, max_x = x, min_y = y, max_y = y;
                int area = 0;
                int sum_x = 0, sum_y = 0;
                int perimeter = 0;

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

                    // Verifica se o pixel está na fronteira
                    int is_boundary = 0;
                    if (px + 1 >= width || px - 1 < 0 || py + 1 >= height || py - 1 < 0 ||
                        src->data[p + 1] == 255 || src->data[p - 1] == 255 ||
                        src->data[p + width] == 255 || src->data[p - width] == 255) {
                        is_boundary = 1;
                        perimeter++;
                    }

                    // Verifica conectividade-4
                    if (px + 1 < width && stack_top < stack_size) stack[stack_top++] = p + 1;
                    if (px - 1 >= 0 && stack_top < stack_size) stack[stack_top++] = p - 1;
                    if (py + 1 < height && stack_top < stack_size) stack[stack_top++] = p + width;
                    if (py - 1 >= 0 && stack_top < stack_size) stack[stack_top++] = p - width;
                }

                free(stack);

                // Armazena o blob apenas se a área cumprir o requisito mínimo
                if (area >= MIN_AREA) {
                    blobs[current_label].area = area;
                    blobs[current_label].x = sum_x / area;
                    blobs[current_label].y = sum_y / area;
                    blobs[current_label].width = max_x - min_x + 1;
                    blobs[current_label].height = max_y - min_y + 1;
                    blobs[current_label].perimeter = perimeter;
                    blobs[current_label].bb_perimeter = 2 * (blobs[current_label].width + blobs[current_label].height);
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

    // Realoca para o tamanho exato
    blobs = (OVC*)realloc(blobs, current_label * sizeof(OVC));
    if (blobs == NULL) {
        free(labels);
        *nblobs = 0;
        return NULL;
    }
    return blobs;
}

// --- Filtra blobs semelhantes a círculos com base na circularidade ---
OVC* vc_filter_circular_blobs(OVC* blobs, int* nblobs, float min_circularity) {
    if (!blobs || *nblobs <= 0) {
        *nblobs = 0;
        return NULL;
    }

    // Aloca array temporário para blobs circulares
    OVC* circular_blobs = (OVC*)malloc(*nblobs * sizeof(OVC));
    if (!circular_blobs) {
        *nblobs = 0;
        return NULL;
    }

    int new_nblobs = 0;

    // Processa cada blob
    for (int i = 0; i < *nblobs; i++) {
        // Usa bb_perimeter para cálculo da circularidade
        int bb_perimeter = blobs[i].bb_perimeter;
        float circularity = (bb_perimeter > 0) ? (4.0f * M_PI * blobs[i].area) / (bb_perimeter * bb_perimeter) : 0.0f;
        blobs[i].circularity = circularity;

        // Mantém apenas blobs com circularidade suficiente
        if (circularity >= min_circularity) {
            circular_blobs[new_nblobs] = blobs[i];
            new_nblobs++;
        }
    }

    // Atualiza nblobs
    *nblobs = new_nblobs;

    if (new_nblobs == 0) {
        free(circular_blobs);
        return NULL;
    }

    // Realoca para o tamanho exato
    circular_blobs = (OVC*)realloc(circular_blobs, new_nblobs * sizeof(OVC));
    return circular_blobs;
}

CoinResult* vc_identify_coin(IVC* image_rgb, IVC* image_bin, OVC* blob) {
    if (!image_rgb || !image_bin || !blob || image_rgb->channels != 3 || image_bin->channels != 1) return NULL;

    int x_min = blob->x - blob->width / 2;
    int x_max = blob->x + blob->width / 2;
    int y_min = blob->y - blob->height / 2;
    int y_max = blob->y + blob->height / 2;

    x_min = x_min < 0 ? 0 : x_min;
    x_max = x_max >= image_rgb->width ? image_rgb->width - 1 : x_max;
    y_min = y_min < 0 ? 0 : y_min;
    y_max = y_max >= image_rgb->height ? image_rgb->height - 1 : y_max;

    float r_sum = 0, g_sum = 0, b_sum = 0;
    int pixel_count = 0;

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int pos_rgb = (y * image_rgb->width + x) * 3;
            int pos_bin = y * image_bin->width + x;
            if (image_bin->data[pos_bin] == 0) { // Inclui apenas pixels da moeda (pretos na imagem binária)
                r_sum += image_rgb->data[pos_rgb];
                g_sum += image_rgb->data[pos_rgb + 1];
                b_sum += image_rgb->data[pos_rgb + 2];
                pixel_count++;
            }
        }
    }

    CoinResult* result = (CoinResult*)malloc(sizeof(CoinResult));
    if (pixel_count == 0 || !result) {
        free(result);
        return NULL;
    }

    result->r_avg = (int)(r_sum / pixel_count);
    result->g_avg = (int)(g_sum / pixel_count);
    result->b_avg = (int)(b_sum / pixel_count);

    // Verifica intervalos de cor e tamanho
    for (int i = 0; i < NUM_COINS; i++) {
        ColorRange* range = &coins[i].range;
        SizeRange* size = &coins[i].sizeRange;
        if (result->r_avg >= range->r_min && result->r_avg <= range->r_max &&
            result->g_avg >= range->g_min && result->g_avg <= range->g_max &&
            result->b_avg >= range->b_min && result->b_avg <= range->b_max &&
            blob->width >= size->size_min && blob->width <= size->size_max) {
            result->coin_name = coins[i].name;
            return result;
        }
    }

    result->coin_name = NULL;
    return result;
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