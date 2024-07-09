#include <iostream>
#include <opencv2/opencv.hpp>

/*
*   image purification
*/
void applyThreshold(cv::Mat& org, cv::Mat& mod) {
    cv::cvtColor(org, mod, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(mod, mod, cv::Size(11, 11), 0);
    cv::adaptiveThreshold(mod, mod, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 5, 2);
}

/*
*   find the largest contour (might be the sudoku itself)
*   what if sudoku isn't present???
*/
cv::Rect findSudokuGrid(cv::Mat& mod) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mod, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxArea = 0;
    cv::Rect maxRect;

    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);

        double area = rect.area();

        if (area > maxArea) {
            maxArea = area;
            maxRect = rect;
        }
    }

    return maxRect;
}

/*
* get corners
*/
std::vector<cv::Point2f> getSudokuCorners(cv::Rect& grid) {
    std::vector<cv::Point2f> corners;

    corners.push_back( cv::Point2f( grid.tl().x, grid.tl().y ) );
    corners.push_back( cv::Point2f( grid.tl().x + grid.width, grid.tl().y ) );
    corners.push_back( cv::Point2f( grid.br().x, grid.br().y) );
    corners.push_back( cv::Point2f( grid.tl().x, grid.tl().y + grid.height ) );

    return corners;
}

/*
*   make the sudoku great again!
*/
cv::Mat transformPerspective(cv::Mat& org, std::vector<cv::Point2f>& srcCorners, cv::Rect& grid) {
    std::vector<cv::Point2f> dstCorners;

    dstCorners.push_back( cv::Point2f(0, 0) );
    dstCorners.push_back( cv::Point2f( grid.width, 0 ) );
    dstCorners.push_back( cv::Point2f( grid.width, grid.height ) );
    dstCorners.push_back( cv::Point2f( 0, grid.height ) );

    cv::Mat wrapped = cv::getPerspectiveTransform( srcCorners, dstCorners );

    cv::Mat res;

    cv::warpPerspective( org, res, wrapped, cv::Size( grid.width, grid.height ) );

    return res;
}

int main() {
    cv::VideoCapture camera(0);

    if ( !camera.isOpened() ) {
        std::cerr << "Couldn't open the camera!\n";
        return -1;
    }

    std::cout << "Click 'ESC' to stop the program!\n";

    for (;;) {
        cv::Mat frame, modifiedFrame;

        camera.read(frame);

        if (frame.empty()) {
            std::cerr << "The captured frame is empty!\n";
            break;
        }

        applyThreshold(frame, modifiedFrame);

        cv::Rect sudokuGrid = findSudokuGrid(modifiedFrame);

        if (sudokuGrid.area() > 0) {
            std::vector<cv::Point2f> srcCorners = getSudokuCorners(sudokuGrid);

            cv::Mat transformedGrid = transformPerspective(frame, srcCorners, sudokuGrid);

            cv::rectangle(frame, sudokuGrid, cv::Scalar(0, 255, 0), 2);

            // display the transformed sudokuuu!
            cv::imshow("transformed", transformedGrid);
        }

        cv::imshow("original", frame);
        cv::imshow("mapped", modifiedFrame);

        if (cv::waitKey(27) >= 0)
            break;
    }

    return 0;
}
