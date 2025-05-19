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

        // Tempo em segundos.
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
        double nseconds = time_span.count();

        std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
        std::cout << "Pressione qualquer tecla para continuar...\n";
        std::cin.get();
    }
}


int main(void) {
    // V�deo
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

    /* Leitura de v�deo de um ficheiro */
    /* NOTA IMPORTANTE:
    O ficheiro video.avi dever� estar localizado no mesmo direct�rio que o ficheiro de c�digo fonte.
    */
    capture.open(videofile);

    /* Em alternativa, abrir captura de v�deo pela Webcam #0 */
    //capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

    /* Verifica se foi poss�vel abrir o ficheiro de v�deo */
    if (!capture.isOpened())
    {
        std::cerr << "Erro ao abrir o ficheiro de v�deo!\n";
        return 1;
    }

    /* N�mero total de frames no v�deo */
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    /* Frame rate do v�deo */
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    /* Resolu��o do v�deo */
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    /* Cria uma janela para exibir o v�deo */
    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

    /* Inicia o timer */
    vc_timer();

    cv::Mat frame;
    while (key != 'q') {
        /* Leitura de uma frame do v�deo */
        capture.read(frame);

        /* Verifica se conseguiu ler a frame */
        if (frame.empty()) break;

        /* N�mero da frame a processar */
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        /* Exemplo de inser��o texto na frame */
        str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);


        // Fa�a o seu c�digo aqui...

        /*
        // Cria uma nova imagem IVC
        IVC *image = vc_image_new(video.width, video.height, 3, 256);
        // Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
        memcpy(image->data, frame.data, video.width * video.height * 3);
        // Executa uma fun��o da nossa biblioteca vc
        vc_rgb_get_green(image);
        // Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
        memcpy(frame.data, image->data, video.width * video.height * 3);
        // Liberta a mem�ria da imagem IVC que havia sido criada
        vc_image_free(image);
        */

        // Cria imagem IVC (RGB)
        IVC* image_rgb = vc_image_new(video.width, video.height, 3, 255);
        memcpy(image_rgb->data, frame.data, video.width * video.height * 3);

        // Cria imagem grayscale
        IVC* image_gray = vc_image_new(video.width, video.height, 1, 255);
        vc_rgb_to_gray(image_rgb, image_gray->data);

        // Cria imagem binária
        IVC* image_bin = vc_image_new(video.width, video.height, 1, 255);
        vc_gray_to_binary(image_gray, image_bin, 110); // threshold

        // Convert IVC to cv::Mat for Gaussian blur
        cv::Mat mat_bin(video.height, video.width, CV_8UC1, image_bin->data);
        cv::Mat mat_blur;
        cv::GaussianBlur(mat_bin, mat_blur, cv::Size(15, 15), 4.0);

        // Cria imagem para blur e copia os dados do cv::Mat
        IVC* image_blur = vc_image_new(video.width, video.height, 1, 255);
        memcpy(image_blur->data, mat_blur.data, video.width * video.height * 1);
        vc_gray_to_binary(image_blur, image_blur, 110); // threshold

        int nblobs;
        OVC* blobs = vc_detect_blobs(image_bin, &nblobs);



#define MIN_CIRCULARITY 0.52 // Minimum circularity to consider a blob as circular
        int n_circular_blobs = nblobs;
        OVC* circular_blobs = vc_filter_circular_blobs(blobs, &n_circular_blobs, MIN_CIRCULARITY);

        // Draw bounding boxes for all blobs (green)
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

                char text[32];
                snprintf(text, sizeof(text), "%.2f", circular_blobs[i].circularity);
                cv::putText(frame,
                    text,
                    cv::Point(circular_blobs[i].x, circular_blobs[i].y - circular_blobs[i].height / 2 - 10),
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

                    char width_text[32];
                    snprintf(width_text, sizeof(width_text), "Width: %d", circular_blobs[i].width);
                    cv::putText(frame,
                        width_text,
                        cv::Point(circular_blobs[i].x, circular_blobs[i].y - circular_blobs[i].height / 2 - 50),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.5,
                        cv::Scalar(0, 0, 255),
                        1);

                    cv::putText(frame,
                        result->coin_name ? result->coin_name : "Unknown",
                        cv::Point(circular_blobs[i].x, circular_blobs[i].y + circular_blobs[i].height / 2 + 20),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.5,
                        cv::Scalar(0, 0, 255),
                        1);
                    free(result); // Free the allocated CoinResult
                }
            }
        }






        // Libera memória
        vc_image_free(image_rgb);
        vc_image_free(image_gray);
        vc_image_free(image_bin);

        // +++++++++++++++++++++++++

        // Exibe a frame
        cv::Mat frame_blur(video.height, video.width, CV_8UC1, image_blur->data);
        cv::imshow("Imagem Borrada", frame_blur);
        cv::imshow("VC - VIDEO", frame);


        // Sai da aplica��o, se o utilizador premir a tecla 'q'
        key = cv::waitKey(1);


    }

    /* Para o timer e exibe o tempo decorrido */
    vc_timer();

    /* Fecha a janela */
    cv::destroyWindow("VC - VIDEO");

    /* Fecha o ficheiro de v�deo */
    capture.release();

    return 0;
}
