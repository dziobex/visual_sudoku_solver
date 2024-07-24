#pragma once

#include "start.hpp"

/*
    read data from the main camera
*/
inline int activeCamera(tesseract::TessBaseAPI& tess) {
    cv::VideoCapture camera(0);

    if (!camera.isOpened()) {
        std::cerr << "Couldn't open the camera!\n";
        return -1;
    }

    std::cout << "Click 'ESC' to stop the program!\n";

    cv::Mat frame, modifiedFrame;

    for (;;) {
        camera.read(frame);

        if (frame.empty()) {
            std::cerr << "The captured frame is empty!\n";
            break;
        }

        start(tess, frame, modifiedFrame);

        if (cv::waitKey(27) >= 0)
            break;
    }

    return 0;
}

/*
    read data from the given file
*/
inline int passiveImage(tesseract::TessBaseAPI& tess, std::string path) {
    cv::Mat img = cv::imread( path, cv::IMREAD_COLOR );
    cv::Mat modifiedImg;

    if (img.empty())
        return std::cerr << "Couldn't open the file!\n", -1;

    start(tess, img, modifiedImg);

    std::cout << "Click 'ESC' to stop the program!\n";

    while (cv::waitKey(27) < 0) {}

    return 0;
}