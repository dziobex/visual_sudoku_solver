#include <iostream>
#include <opencv2/opencv.hpp>

/*
*   image purification
*/
void applyThreshold(cv::Mat& org, cv::Mat& mod) {
    cv::cvtColor(org, mod, cv::COLOR_BGR2GRAY);
    // cv::GaussianBlur(mod, mod, cv::Size(15, 15), 0);
    cv::adaptiveThreshold(mod, mod, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 41, 2);
}

/*
* todo:
* contourArea - what is it
* then transform B）
*/

cv::Rect findSudokuGrid(cv::Mat& org, cv::Mat& mod) {

    std::vector<std::vector<cv::Point>> contours;

    cv::findContours(mod, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxArea = 0;
    cv::Rect maxRect;

    for ( const auto& contour : contours ) {
        cv::Rect rect = cv::boundingRect(contour);

        double area = rect.area();

        // cv::rectangle(org, rect, cv::Scalar(0, 0, 255), 2);

        if ( area > maxArea) {
            maxArea = area;
            maxRect = rect;
        }
    }

    return maxRect;
}

int main() {
    cv::VideoCapture camera(0);

    if (!camera.isOpened()) {
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

        cv::Rect sudokuGrid = findSudokuGrid( frame, modifiedFrame );

        if (sudokuGrid.area() > 0) {
            cv::rectangle(frame, sudokuGrid, cv::Scalar(0, 255, 0), 2);
        }

        cv::imshow("original", frame);
        cv::imshow("modified", modifiedFrame);

        if (cv::waitKey(27) >= 0)
            break;
    }

    return 0;
}
