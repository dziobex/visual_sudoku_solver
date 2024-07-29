#include "recognize.h"

bool transformNumber(cv::Mat& num) {
    cv::cvtColor(num, num, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(num, num, cv::Size(3, 3), 0);
    cv::adaptiveThreshold(num, num, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

    cv::medianBlur(num, num, 3);
    cv::threshold(num, num, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // remove the cell's border
    int margin = 4;
    cv::Rect innerCell(margin, margin, num.cols - 2 * margin, num.rows - 2 * margin);
    cv::Mat innerNum = num(innerCell);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(innerNum, innerNum, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(innerNum, innerNum, cv::MORPH_CLOSE, kernel);

    kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(2, 2));
    cv::morphologyEx(innerNum, innerNum, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(innerNum, innerNum, cv::MORPH_CLOSE, kernel);

    cv::Mat filteredNum;
    cv::bilateralFilter(innerNum, filteredNum, 9, 75, 75);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(innerNum, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxArea = 0;
    std::vector<cv::Point> maxContour;

    for (auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > maxArea) {
            maxArea = area;
            maxContour = contour;
        }
    }

    if (!maxContour.empty()) {
        cv::Rect newContour = cv::boundingRect(maxContour);
        newContour.x += margin;
        newContour.y += margin;

        num = num(newContour);

        int targetSize = 40;
        if (num.cols < targetSize || num.rows < targetSize) {
            float scale = std::max(float(targetSize) / num.cols, float(targetSize) / num.rows);
            cv::resize(num, num, cv::Size(), scale, scale, cv::INTER_LINEAR);
        }

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2.5, 2.5));
        cv::dilate(num, num, kernel);

        return true;
    }

    return false;
}