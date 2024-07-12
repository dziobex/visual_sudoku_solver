#include <iostream>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>

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
    find the largest contour (might be the sudoku itself)
    todo: check if the contour meets the criteria: rectangle, almost the same dims 
*/
std::vector<cv::Point> findSudokuGrid(cv::Mat& mod) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mod, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxArea = 0;
    std::vector<cv::Point> maxContour;

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > maxArea) {
            maxArea = area;
            maxContour = contour;
        }
    }

    return maxContour;
}

/*
    get edges of the sudoku
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
    do perspective transformation
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
            res.push_back(org(cv::Rect(x + 5 , y + 5, width - 5, height - 5)));
        }
    }

    return res;
}

void start(tesseract::TessBaseAPI& tess, cv::Mat& frame, cv::Mat& modifiedFrame) {
    applyThreshold(frame, modifiedFrame);

    std::vector<cv::Point> sudokuGrid = findSudokuGrid(modifiedFrame);

    if ( sudokuGrid.empty() )
        return;

    std::vector<cv::Point> srcCorners = getSudokuCorners(sudokuGrid);

    if (srcCorners.size() == 4) {
        cv::Mat transformedGrid = transformPerspective(frame, srcCorners);
        cv::Mat transformedGridCopy = transformedGrid.clone();

        std::vector<cv::Mat> sudokuNumbers = split(transformedGrid, 9, 9);

        // go through the sudokuNumbers and mark recognized numbers

        int cellWidth = transformedGrid.cols / 9;
        int cellHeight = transformedGrid.rows / 9;

        for (int i = 0; i < sudokuNumbers.size(); ++i) {
            cv::Mat& num = sudokuNumbers[i];

            cv::cvtColor(num, num, cv::COLOR_BGR2GRAY);
            cv::GaussianBlur(num, num, cv::Size(11, 11), 0);
            cv::adaptiveThreshold(num, num, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

            tess.SetImage((uchar*)num.data, num.size().width, num.size().height, 1, num.step1());
            tess.Recognize(0);

            if (19 <= i && i <= 28)
                cv::imshow("cell " + std::to_string(i), num);

            const char* word = tess.GetUTF8Text();
            float conf = tess.MeanTextConf();
            int x = (i % 9) * cellWidth;
            int y = (i / 9) * cellHeight;

            if ( word && conf >= 80 ) {
                cv::putText(transformedGridCopy, word, cv::Point(x + cellWidth / 4, y + 3 * cellHeight / 4),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
            }

            delete[] word;
        }

        cv::imshow("transformed", transformedGridCopy);

        for (auto& corner : srcCorners) {
            cv::circle(frame, corner, 5, cv::Scalar(0, 0, 255), cv::FILLED);
        }

        cv::rectangle(frame, cv::boundingRect(srcCorners), cv::Scalar(0, 255, 0), 2);
    }

    cv::imshow("original", frame);
    cv::imshow("mapped", modifiedFrame);
}

int main() {
    cv::VideoCapture camera(0);

    if (!camera.isOpened()) {
        std::cerr << "Couldn't open the camera!\n";
        return -1;
    }

    std::cout << "Click 'ESC' to stop the program!\n";

    tesseract::TessBaseAPI tess;
    tess.Init(NULL, "eng", tesseract::OEM_DEFAULT);
    tess.SetVariable("tessedit_char_whitelist", "0123456789");

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
