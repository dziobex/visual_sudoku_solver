#include "process.h"

void applyThreshold(cv::Mat& org, cv::Mat& mod) {
    cv::cvtColor(org, mod, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(mod, mod, cv::Size(11, 11), 0);
    cv::adaptiveThreshold(mod, mod, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

    // thicc digits!
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(mod, mod, kernel);
}

double getAngle(cv::Point p1, cv::Point p2, cv::Point p3) {
    double dx1 = (double)p1.x - (double)p2.x;
    double dy1 = (double)p1.y - (double)p2.y;

    double dx2 = (double)p3.x - (double)p2.x;
    double dy2 = (double)p3.y - (double)p2.y;

    return std::atan2(dx1 * dy2 - dy1 * dx2, dx1 * dx2 + dy1 * dy2) * 180.0 / CV_PI;
}

bool isValidRect(std::vector<cv::Point>& contour) {
    std::vector<cv::Point> approxShape;

    cv::approxPolyDP(contour, approxShape, cv::arcLength(contour, true) * 0.02, true);

    if (approxShape.size() != 4)
        return false;

    double sides[4]{};

    // are all angles ~90 deg?
    for (int i = 0; i < 4; ++i) {
        double angle = std::abs(getAngle(approxShape[i], approxShape[(i + 1) % 4], approxShape[(i + 2) % 4]));

        if (angle < 80 || angle > 100)
            return false;

        sides[i] = cv::norm(approxShape[i] - approxShape[(i + 1) % 4]);
    }

    // are all of the sides have similar lengths? check the ratio!
    return *std::max_element(sides, sides + 4) / *std::min_element(sides, sides + 4) <= 1.2;
}

std::vector<cv::Point> findSudokuGrid(cv::Mat& mod) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mod, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxArea = 0;
    std::vector<cv::Point> maxContour;

    for (auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (isValidRect(contour) && area > maxArea) {
            maxArea = area;
            maxContour = contour;
        }
    }

    return maxContour;
}

std::vector<cv::Point> getSudokuCorners(std::vector<cv::Point>& grid) {
    cv::Point topLeft, topRight, bottomLeft, bottomRight;
    topLeft = topRight = bottomLeft = bottomRight = grid[0];

    for (auto& point : grid) {
        if (point.x + point.y < topLeft.x + topLeft.y)
            topLeft = point;
        if (point.x - point.y > topRight.x - topRight.y)
            topRight = point;
        if (point.x + point.y > bottomRight.x + bottomRight.y)
            bottomRight = point;
        if (point.x - point.y < bottomLeft.x - bottomLeft.y)
            bottomLeft = point;
    }

    return std::vector<cv::Point> { topLeft, topRight, bottomRight, bottomLeft };
}

std::pair<cv::Mat, cv::Mat> transformPerspective(cv::Mat& org, std::vector<cv::Point>& srcCorners) {
    std::vector<cv::Point2f> srcPoints;

    for (auto& corn : srcCorners)
        srcPoints.push_back(cv::Point2f(corn.x, corn.y));

    float side = std::max(cv::norm(srcPoints[0] - srcPoints[1]), cv::norm(srcPoints[1] - srcPoints[2]));

    cv::Mat transformMatrix = cv::getPerspectiveTransform(
        srcPoints,
        std::vector<cv::Point2f>{
        cv::Point2f(0, 0),
            cv::Point2f(side - 1, 0),
            cv::Point2f(side - 1, side - 1),
            cv::Point2f(0, side - 1)
    }
    );

    cv::Mat res;
    cv::warpPerspective(org, res, transformMatrix, cv::Size(side, side));

    return std::make_pair(res, transformMatrix);
}

std::vector<cv::Mat> split(cv::Mat& org, int m, int n) {
    int width = org.cols / m;
    int height = org.rows / n;

    std::vector<cv::Mat> res;

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            res.push_back(org(cv::Rect(width * j, height * i, width, height)));

    return res;
}