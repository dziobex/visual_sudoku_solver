#pragma once 

#include "recognize.hpp"
#include "solve.hpp"

/*
    process the image and do... the work!
*/
inline void start(tesseract::TessBaseAPI& tess, cv::Mat& frame, cv::Mat& modifiedFrame) {
    applyThreshold(frame, modifiedFrame);

    std::vector<cv::Point> sudokuGrid = findSudokuGrid(modifiedFrame);

    if (sudokuGrid.empty())
        return;

    std::vector<cv::Point> srcCorners = getSudokuCorners(sudokuGrid);

    int grid[9][9]{};
    bool isInitialFilled[9][9]{};

    cv::Mat transformedGrid, transformedGridCopy;
    cv::Mat transformMatrix, inverseTransformMatrix;

    if (srcCorners.size() == 4) {
        std::tie(transformedGrid, transformMatrix) = transformPerspective(frame, srcCorners);
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

                const char* word = tess.GetUTF8Text();
                float conf = tess.MeanTextConf();

                int x = (i % 9) * cellWidth;
                int y = (i / 9) * cellHeight;

                if (word && conf >= 50) {
                    grid[i / 9][i % 9] = word[0] - '0';
                    isInitialFilled[i / 9][i % 9] = true;

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

        cv::invert(transformMatrix, inverseTransformMatrix);

        // go through the sudokuNumbers and mark recognized numbers
        int cellWidth = transformedGrid.cols / 9;
        int cellHeight = transformedGrid.rows / 9;

        if (cellWidth > 0 && cellHeight > 0) {
            for (int i = 0; i < 81; ++i) {
                if ( !isInitialFilled[i / 9][i % 9] ) {

                    int x = (i % 9) * cellWidth;
                    int y = (i / 9) * cellHeight;

                    cv::Mat point(3, 1, CV_64F);

                    point.at<double>(0, 0) = x + cellWidth / 2;
                    point.at<double>(1, 0) = y + cellHeight / 2;
                    point.at<double>(2, 0) = 1.0;

                    cv::Mat transformedPoint = inverseTransformMatrix * point;

                    double tx = transformedPoint.at<double>(0, 0) / transformedPoint.at<double>(2, 0);
                    double ty = transformedPoint.at<double>(1, 0) / transformedPoint.at<double>(2, 0);

                    std::string text = std::to_string(grid[i / 9][i % 9]);
                    int baseline = 0;
                    double fontScale = 1.0;

                    // Calculate the appropriate font size
                    cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontScale, 2, &baseline);
                    fontScale = 0.8 * std::min(cellWidth / (double)textSize.width, cellHeight / (double)textSize.height);

                    // Recalculate the text size with the new font scale
                    textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontScale, 2, &baseline);

                    // Calculate the bottom-left corner of the text so it is centered
                    cv::Point textOrg(tx - textSize.width / 2, ty + textSize.height / 2);
                    
                    if (!isInitialFilled[i / 9][i % 9]) {
                        cv::Scalar color = cv::Scalar(0, 255, 0);

                        cv::putText(frame, text, textOrg, cv::FONT_HERSHEY_SIMPLEX, fontScale, color, 2);
                    }
                }
                else {

                }
            }

            cv::imshow("transformed", transformedGridCopy);
        }
    }
    else {
        printf("COULDN'T FIND ANY SOLUTION :(");
    }

    cv::imshow("original", frame);
    cv::imshow("mapped", modifiedFrame);
}