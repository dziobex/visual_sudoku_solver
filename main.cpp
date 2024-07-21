#include <iostream>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>

#include "settings.h"
#include "solve.h"

/*
    image purification
*/
void applyThreshold(cv::Mat& org, cv::Mat& mod) {
    cv::cvtColor(org, mod, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(mod, mod, cv::Size(11, 11), 0);
    cv::adaptiveThreshold(mod, mod, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

    // thicc digits!
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(mod, mod, kernel);
}

/*
    get angle between 3 points (2 crossing vectors)
*/
double getAngle(cv::Point p1, cv::Point p2, cv::Point p3) {
    double dx1 = (double)p1.x - (double)p2.x;
    double dy1 = (double)p1.y - (double)p2.y;

    double dx2 = (double)p3.x - (double)p2.x;
    double dy2 = (double)p3.y - (double)p2.y;

    return std::atan2(dx1 * dy2 - dy1 * dx2, dx1 * dx2 + dy1 * dy2) * 180.0 / CV_PI;
}

/*
    check if the found polygon has the sudoku shape
*/
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

/*
    find the largest contour (might be the sudoku itself!)
*/
std::vector<cv::Point> findSudokuGrid(cv::Mat& mod) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mod, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxArea = 0;
    std::vector<cv::Point> maxContour;

    for ( auto& contour : contours ) {
        double area = cv::contourArea(contour);
        if (isValidRect(contour) && area > maxArea) {
            maxArea = area;
            maxContour = contour;
        }
    }

    return maxContour;
}

/*
    get the sudoku's edges
*/
std::vector<cv::Point> getSudokuCorners( std::vector<cv::Point>& grid ) {
    cv::Point topLeft, topRight, bottomLeft, bottomRight;
    topLeft = topRight = bottomLeft = bottomRight = grid[0];

    for ( auto& point : grid ) {
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

/*
    change the window perspective to match the sudoku grid
*/
cv::Mat transformPerspective(cv::Mat& org, std::vector<cv::Point>& srcCorners) {
    std::vector<cv::Point2f> srcPoints;

    for ( auto& corn : srcCorners )
        srcPoints.push_back( cv::Point2f(corn.x, corn.y) );

    float side = std::max( cv::norm( srcPoints[0] - srcPoints[1] ), cv::norm( srcPoints[1] - srcPoints[2] ) );

    cv::Mat transformMatrix = cv::getPerspectiveTransform(
        srcPoints,
        std::vector<cv::Point2f> {
                cv::Point2f(0, 0),
                cv::Point2f(side - 1, 0),
                cv::Point2f(side - 1, side - 1),
                cv::Point2f(0, side - 1)
        }
    );

    cv::Mat res;

    cv::warpPerspective(org, res, transformMatrix, cv::Size(side, side));

    return res;
}

/*
    split the given image into m columns x n rows image matrix
*/
std::vector<cv::Mat> split(cv::Mat& org, int m, int n) {
    int width = org.cols / m;
    int height = org.rows / n;

    std::vector<cv::Mat> res;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            int x = width * j;
            int y = height * i;
            res.push_back(org(cv::Rect(x, y, width, height)));
        }
    }

    return res;
}

/*
    transform the number format to more readable one
*/
/*
    to do:
    -move the num to the center
    -remove random dots/noise which sometimes is mistaken with number
*/
bool transformNumber(cv::Mat& num) {
    cv::cvtColor(num, num, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(num, num, cv::Size(3, 3), 0);
    cv::adaptiveThreshold(num, num, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

    // remove the cell's border
    int margin = 5;
    cv::Rect innerCell(margin, margin, num.cols - 2 * margin, num.rows - 2 * margin);
    cv::Mat innerNum = num(innerCell);

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
        if ( num.cols < targetSize || num.rows < targetSize ) {
            float scale = std::max(float(targetSize) / num.cols, float(targetSize) / num.rows);
            cv::resize(num, num, cv::Size(), scale, scale, cv::INTER_LINEAR);
        }

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2.5, 2.5));
        cv::dilate(num, num, kernel);

        return true;
    }

    return false;
}

/*
    process the image and do... the work!
*/
void start(tesseract::TessBaseAPI& tess, cv::Mat& frame, cv::Mat& modifiedFrame) {
    applyThreshold(frame, modifiedFrame);

    std::vector<cv::Point> sudokuGrid = findSudokuGrid(modifiedFrame);

    if (sudokuGrid.empty())
        return;

    std::vector<cv::Point> srcCorners = getSudokuCorners(sudokuGrid);

    int grid[9][9] {};

    cv::Mat transformedGrid, transformedGridCopy;

    if (srcCorners.size() == 4) {
        transformedGrid = transformPerspective(frame, srcCorners);
        transformedGridCopy = transformedGrid.clone();

        std::vector<cv::Mat> sudokuNumbers = split(transformedGrid, 9, 9);

        // go through the sudokuNumbers and mark recognized numbers
        int cellWidth = transformedGrid.cols / 9;
        int cellHeight = transformedGrid.rows / 9;

        if (cellWidth > 0 && cellHeight > 0) {
            for (int i = 0; i < sudokuNumbers.size(); ++i) {
                cv::Mat& num = sudokuNumbers[i];

                if (!transformNumber(num))
                    continue;

                tess.SetImage((uchar*)num.data, num.size().width, num.size().height, num.channels(), num.step1());
                tess.Recognize(0);

                if (i >= 54)
                   cv::imshow("cell " + std::to_string(i), num);

                const char* word = tess.GetUTF8Text();
                float conf = tess.MeanTextConf();

                int x = (i % 9) * cellWidth;
                int y = (i / 9) * cellHeight;

                if ( word && conf >= CONF_RATE ) {
                    grid[i / 9][i % 9] = word[0] - '0';

                    //cv::putText(transformedGridCopy, std::string(1, word[0]), cv::Point(x + cellWidth / 4, y + 3 * cellHeight / 4),
                    //    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
                }

                delete[] word;
            }

            //cv::imshow("transformed", transformedGridCopy);

            for (auto& corner : srcCorners) {
                cv::circle(frame, corner, 5, cv::Scalar(0, 0, 255), cv::FILLED);
            }

            cv::rectangle(frame, cv::boundingRect(srcCorners), cv::Scalar(0, 255, 0), 2);
        }
    }

    // print (or not) the found solution

    if (solve(grid, 0, 0)) {
        printf("SOLVE:\n");

        for (int i = 0; i < 9; ++i) {
            printf("[ ");
            for (int j = 0; j < 9; ++j) {
                printf("%d, ", grid[i][j]);
            }
            printf(" ]\n");
        }

        // go through the sudokuNumbers and mark recognized numbers
        int cellWidth = transformedGrid.cols / 9;
        int cellHeight = transformedGrid.rows / 9;

        if (cellWidth > 0 && cellHeight > 0) {

            for (int i = 0; i < 81; ++i) {

                int x = (i % 9) * cellWidth;
                int y = (i / 9) * cellHeight;

                cv::putText(transformedGridCopy, std::string(1, grid[i / 9][i % 9] + '0'), cv::Point(x + cellWidth / 4, y + 3 * cellHeight / 4),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
            }

            cv::imshow("transformed", transformedGridCopy);
        }
    }
    else
        printf("COULND'T FIND ANY SOLUTION :(");

    cv::imshow("original", frame);
    cv::imshow("mapped", modifiedFrame);
}

/*
    read data from the main camera
*/
int activeCamera(tesseract::TessBaseAPI& tess) {
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
int passiveImage(tesseract::TessBaseAPI& tess, std::string path) {
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    cv::Mat modifiedImg;

    if (img.empty())
        return std::cerr << "Couldn't open the file!\n", -1;

    start(tess, img, modifiedImg);

    std::cout << "Click 'ESC' to stop the program!\n";

    while (cv::waitKey(27) < 0) {}

    return 0;
}

int main(int argc, char** argv) {

    // init tesseract engine - and set some data, which will make recognizing digits EASIER
    tesseract::TessBaseAPI tess;
    tess.Init(NULL, "eng", tesseract::OEM_TESSERACT_ONLY);
    tess.SetVariable("tessedit_char_whitelist", "123456789");
    tess.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);

    if (argc == 1)
        return activeCamera(tess);

    // it makes testing EASIER
    else if (argc == 3 ) {
        std::string str(argv[1]);

        std::string path = std::filesystem::absolute(std::filesystem::current_path().parent_path().parent_path()).string() + "\\samples\\";
        std::string file = argv[2] + std::string(".jpg");
        
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
        file.erase(remove_if(file.begin(), file.end(), isspace), file.end());

        if ( str == "samples" || str == "s")
            return passiveImage(tess, path + file );
    }

    return passiveImage(tess, argv[1]);
}