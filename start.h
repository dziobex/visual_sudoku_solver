#pragma once

#include "recognize.h"
#include "solve.h"

/*
    process the image and do... the work!
*/
void start(tesseract::TessBaseAPI& tess, cv::Mat& frame, cv::Mat& modifiedFrame);