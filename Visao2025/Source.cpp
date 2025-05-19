#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}

void vc_timer(void) {
    static bool running = false;
    static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

    if (!running) {
        running = true;
    }
    else {
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

        // Tempo em segundos
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
        double nseconds = time_span.count();

        std::cout << "Tempo decorrido: " << nseconds << " segundos" << std::endl;
        std::cout << "Prima qualquer tecla para continuar...\n";
        std::cin.get();
    }
}

int main(void) {
    // Vídeo
    char videofile[20] = "video2.mp4";
    cv::VideoCapture capture;
    struct
    {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;
    // Outros
    std::string str;
    int key = 0;

    // Arrays para contar cada tipo de moeda e rastrear área e perímetro
    int coin_counts[8] = { 0 }; // 1cent, 2cent, 5cent, 10cent, 20cent, 50cent, 1eur, 2eur
    long long coin_areas[8] = { 0 }; // Área total por tipo de moeda
    long long coin_perimeters[8] = { 0 }; // Perímetro total por tipo de moeda
    const char* coin_names[8] = { "1cent", "2cent", "5cent", "10cent", "20cent", "50cent", "1eur", "2eur" };
    // Valores das moedas em euros
    const double coin_values[8] = { 0.01, 0.02, 0.05, 0.10, 0.20, 0.50, 1.00, 2.00 };

    /* Leitura de vídeo de um ficheiro */
    capture.open(videofile);

    /* Verifica se foi possível abrir o ficheiro de vídeo */
    if (!capture.isOpened())
    {
        std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
        return 1;
    }

    /* Número total de frames no vídeo */
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    /* Frame rate do vídeo */
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    /* Resolução do vídeo */
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    /* Cria uma janela para exibir o vídeo */
    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

    /* Inicia o temporizador */
    vc_timer();

    cv::Mat frame;
    while (key != 'q') {
        /* Leitura de uma frame do vídeo */
        capture.read(frame);

        /* Verifica se conseguiu ler a frame */
        if (frame.empty()) break;

        /* Número da frame a processar */
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        /* Exemplo de inserção de texto na frame */
        str = std::string("RESOLUÇÃO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("N.º DA FRAME: ").append(std::to_string(video.nframe));
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        // Processa apenas a cada 62ª frame para contagem de moedas
        bool process_coins = (video.nframe % 62 == 0);

        // Cria imagem IVC (RGB)
        IVC* image_rgb = vc_image_new(video.width, video.height, 3, 255);
        memcpy(image_rgb->data, frame.data, video.width * video.height * 3);

        // Cria imagem em escala de cinza
        IVC* image_gray = vc_image_new(video.width, video.height, 1, 255);
        vc_rgb_to_gray(image_rgb, image_gray->data);

        // Cria imagem binária
        IVC* image_bin = vc_image_new(video.width, video.height, 1, 255);
        vc_gray_to_binary(image_gray, image_bin, 110); // limiar

        // Converte IVC para cv::Mat para desfocagem gaussiana
        cv::Mat mat_bin(video.height, video.width, CV_8UC1, image_bin->data);
        cv::Mat mat_blur;
        cv::GaussianBlur(mat_bin, mat_blur, cv::Size(15, 15), 4.0);

        // Cria imagem para desfocagem e copia os dados do cv::Mat
        IVC* image_blur = vc_image_new(video.width, video.height, 1, 255);
        memcpy(image_blur->data, mat_blur.data, video.width * video.height * 1);
        vc_gray_to_binary(image_blur, image_bin, 110); // limiar

        int nblobs;
        OVC* blobs = vc_detect_blobs(image_bin, &nblobs);

#define MIN_CIRCULARITY 0.52 // Circularidade mínima para considerar um blob como circular
        int n_circular_blobs = nblobs;
        OVC* circular_blobs = vc_filter_circular_blobs(blobs, &n_circular_blobs, MIN_CIRCULARITY);

        // Desenha caixas delimitadoras para todos os blobs (verde)
        if (blobs) {
            for (int i = 0; i < nblobs; i++) {
                cv::rectangle(frame,
                    cv::Point(blobs[i].x - blobs[i].width / 2, blobs[i].y - blobs[i].height / 2),
                    cv::Point(blobs[i].x + blobs[i].width / 2, blobs[i].y + blobs[i].height / 2),
                    cv::Scalar(0, 255, 0), 2);
            }
        }

        if (circular_blobs) {
            for (int i = 0; i < n_circular_blobs; i++) {
                cv::rectangle(frame,
                    cv::Point(circular_blobs[i].x - circular_blobs[i].width / 2, circular_blobs[i].y - circular_blobs[i].height / 2),
                    cv::Point(circular_blobs[i].x + circular_blobs[i].width / 2, circular_blobs[i].y + circular_blobs[i].height / 2),
                    cv::Scalar(0, 0, 255), 2);

                // Desenha ponto central (círculo)
                cv::circle(frame,
                    cv::Point(circular_blobs[i].x, circular_blobs[i].y),
                    3, // Raio de 3 píxeis
                    cv::Scalar(255, 255, 255));

                char text[32];
                snprintf(text, sizeof(text), "%.2f", circular_blobs[i].circularity);
                cv::putText(frame,
                    text,
                    cv::Point(circular_blobs[i].x, circular_blobs[i].y - circular_blobs[i].height / 2 - 10),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.5,
                    cv::Scalar(0, 0, 255),
                    1);

                char area_text[32];
                snprintf(area_text, sizeof(area_text), "Área: %d", circular_blobs[i].area);
                cv::putText(frame,
                    area_text,
                    cv::Point(circular_blobs[i].x, circular_blobs[i].y - circular_blobs[i].height / 2 - 70),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.5,
                    cv::Scalar(0, 0, 255),
                    1);

                char perimeter_text[32];
                snprintf(perimeter_text, sizeof(perimeter_text), "Perímetro: %d", circular_blobs[i].perimeter);
                cv::putText(frame,
                    perimeter_text,
                    cv::Point(circular_blobs[i].x, circular_blobs[i].y - circular_blobs[i].height / 2 - 50),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.5,
                    cv::Scalar(0, 0, 255),
                    1);

                CoinResult* result = vc_identify_coin(image_rgb, image_bin, &circular_blobs[i]);
                if (result) {
                    char rgb_text[32];
                    snprintf(rgb_text, sizeof(rgb_text), "RGB: %d,%d,%d", result->r_avg, result->g_avg, result->b_avg);
                    cv::putText(frame,
                        rgb_text,
                        cv::Point(circular_blobs[i].x, circular_blobs[i].y - circular_blobs[i].height / 2 - 30),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.5,
                        cv::Scalar(0, 0, 255),
                        1);

                    cv::putText(frame,
                        result->coin_name ? result->coin_name : "Desconhecido",
                        cv::Point(circular_blobs[i].x, circular_blobs[i].y + circular_blobs[i].height / 2 + 20),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.5,
                        cv::Scalar(0, 0, 255),
                        1);

                    // Incrementa a contagem de moedas, área e perímetro se identificado e numa 62ª frame
                    if (result->coin_name && process_coins) {
                        for (int j = 0; j < 8; j++) {
                            if (strcmp(result->coin_name, coin_names[j]) == 0) {
                                coin_counts[j]++;
                                coin_areas[j] += circular_blobs[i].area;
                                coin_perimeters[j] += circular_blobs[i].perimeter;
                                break;
                            }
                        }
                    }

                    free(result); // Liberta o CoinResult alocado
                }
            }
        }

        // Liberta memória
        vc_image_free(image_rgb);
        vc_image_free(image_gray);
        vc_image_free(image_bin);
        vc_image_free(image_blur);
        free(blobs);
        free(circular_blobs);

        // Exibe a frame
        cv::Mat frame_blur(video.height, video.width, CV_8UC1, image_blur->data);
        //cv::imshow("Imagem Desfocada", frame_blur);
        cv::imshow("VC - VIDEO", frame);

        // Sai da aplicação, se o utilizador premir a tecla 'q'
        key = cv::waitKey(1);
    }

    /* Para o temporizador e exibe o tempo decorrido */
    vc_timer();

    /* Fecha a janela */
    cv::destroyWindow("VC - VIDEO");
    //cv::destroyWindow("Imagem Desfocada");

    /* Calcula o valor total em euros */
    double total_value = 0.0;
    for (int i = 0; i < 8; i++) {
        total_value += coin_counts[i] * coin_values[i];
    }

    /* Calcula a área total e o perímetro total */
    long long total_area = 0;
    long long total_perimeter = 0;
    for (int i = 0; i < 8; i++) {
        total_area += coin_areas[i];
        total_perimeter += coin_perimeters[i];
    }

    /* Exibe a contagem de moedas por tipo, área total, perímetro total e valor total */
    std::cout << "\nContagem de moedas por tipo:\n";
    for (int i = 0; i < 8; i++) {
        std::cout << coin_names[i] << ": " << coin_counts[i]
            << ", Área Total: " << coin_areas[i] << " píxeis"
            << ", Perímetro Total: " << coin_perimeters[i] << " píxeis" << std::endl;
    }
    std::cout << "\nValor Total: " << std::fixed << std::setprecision(2) << total_value << " Euros" << std::endl;
    std::cout << "Área Total (todas as moedas): " << total_area << " píxeis" << std::endl;
    std::cout << "Perímetro Total (todas as moedas): " << total_perimeter << " píxeis" << std::endl;

    /* Fecha o ficheiro de vídeo */
    capture.release();

    return 0;
}