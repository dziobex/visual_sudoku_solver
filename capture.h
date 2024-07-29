#pragma once

#include "start.h"

/*
    read data from the main camera
*/
int activeCamera(tesseract::TessBaseAPI& tess);

/*
    read data from the given file
*/
int passiveImage(tesseract::TessBaseAPI& tess, std::string path);