#pragma once

#include <iostream>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>

/*
    image purification
*/
void applyThreshold(cv::Mat& org, cv::Mat& mod);

/*
    get angle between 3 points (2 crossing vectors)
*/
double getAngle(cv::Point p1, cv::Point p2, cv::Point p3);

/*
    check if the found polygon has the sudoku shape
*/
bool isValidRect(std::vector<cv::Point>& contour);

/*
    find the largest contour (might be the sudoku itself!)
*/
std::vector<cv::Point> findSudokuGrid(cv::Mat& mod);

/*
    get the sudoku's edges (4)
*/
std::vector<cv::Point> getSudokuCorners(std::vector<cv::Point>& grid);

/*
    change the window perspective to match the sudoku grid
*/
std::pair<cv::Mat, cv::Mat> transformPerspective(cv::Mat& org, std::vector<cv::Point>& srcCorners);

/*
    split the given image into m columns x n rows image matrix
*/
std::vector<cv::Mat> split(cv::Mat& org, int m, int n);