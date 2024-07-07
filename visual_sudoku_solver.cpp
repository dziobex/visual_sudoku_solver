#include <iostream>
#include <opencv2/opencv.hpp>

void applyThreshold(cv::Mat& org, cv::Mat& mod) {
	cv::cvtColor(org, mod, cv::COLOR_BGR2GRAY);
	//cv::threshold(mod, mod, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
	cv::adaptiveThreshold(mod, mod, 200, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 3, 2);
}

int main() {

	cv::VideoCapture camera(0);

	if (!camera.isOpened()) {
		std::cerr << "Couldn't open the camera!\n";
		return -1;
	}

	for (;;) {
		cv::Mat frame, modifiedFrame;

		camera.read(frame);
		applyThreshold(frame, modifiedFrame);

		cv::imshow("TO TY", frame);
		cv::imshow("DO DY", modifiedFrame);

		if (cv::waitKey(27) >= 0)
			break;
	}

	return 0;
}