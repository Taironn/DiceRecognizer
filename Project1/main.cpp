#include <iostream>
#include <chrono>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

using namespace std;

struct DiceData
{
	cv::Rect diceBounds;
	cv::Mat diceRegion;
	int circles;
};

int countCircles(cv::Mat& region)
{
	int newsize = 200;
	//Resize dice image
	cv::resize(region, region, cv::Size(newsize, newsize));

	//Only greyscale is needed
	cvtColor(region, region, CV_BGR2GRAY);

	//Thresholding
	cv::threshold(region, region, 150, 255, cv::THRESH_BINARY | CV_THRESH_OTSU);

	//Floodfill algorithm from every corner
	cv::floodFill(region, cv::Point(0, 0), cv::Scalar(255));
	cv::floodFill(region, cv::Point(0, newsize - 1), cv::Scalar(255));
	cv::floodFill(region, cv::Point(newsize - 1, 0), cv::Scalar(255));
	cv::floodFill(region, cv::Point(newsize - 1, newsize - 1), cv::Scalar(255));

	//Blob detection
	cv::SimpleBlobDetector::Params parameters;

	//Filtering by inertia
	parameters.filterByInertia = true;
	//parameters.filterByColor = true;
	//parameters.blobColor = 0;
	parameters.minInertiaRatio = 0.2;

	//For the keypoints
	vector<cv::KeyPoint> keypoints;

	//New blob detector with the parameters
	cv::Ptr<cv::SimpleBlobDetector> simpleBlobDetector = cv::SimpleBlobDetector::create(parameters);

	//Detecting blobs
	simpleBlobDetector->detect(region, keypoints);

	cv::imshow("DiceWindow", region);
	cout << "Circles on dice: " << keypoints.size() << endl;

	return keypoints.size();
}


int main(int argc, char* argv[])
{
	cv::VideoCapture capture;
	//cv::Mat processed;

	const std::string videoStreamAddress = "http://192.168.137.160:8080/videofeed";

	bool success = true;
	if (argc < 2)
	{
		success = capture.open(videoStreamAddress);
	}
	else
	{
		success = capture.open(argv[1]);
		//capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
		//capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
	}
	if (!success)
	{
		std::cerr << "Unable to open video capture" << std::endl;
		return 0;
	}

	cv::namedWindow("PreviewWindow");

	cv::Mat frame;
	cv::Mat background;
	// ide jon a kepfeldolgozas
	cv::Mat processed = frame;
	//cv::Mat hsv = frame;
	//cv::Mat blue, red;
	vector<vector<cv::Point>> dices;
	vector<cv::Vec4i> diceh;
	vector<DiceData> diceData;
	int lastnum = -500;
	capture.read(background);
	//cv::imshow("Background", background);
	while (capture.read(frame))
	{
		processed = frame - background;
		//processed = detectHoughLines(frame);
		//processed = detectHoughLines_v2(frame);
		//processed = detectHoughCircles(frame);
		//cv::ColorConversionCodes
		//cv::cvtColor(frame, hsv, CV_BGR2HSV, 0);
		//cv::inRange(hsv, cv::Vec3b(100, 100, 0), cv::Vec3b(110, 255, 255), blue);
		//cv::inRange(hsv, cv::Vec3b(0, 100, 0), cv::Vec3b(10, 255, 255), red);
		//cv::cvtColor((blue | red), processed, CV_GRAY2BGR);

		//Change to greyscale
		cv::cvtColor(processed, processed, CV_BGR2GRAY, 0);

		//Threshold
		cv::threshold(processed, processed, 150, 255, cv::THRESH_BINARY | CV_THRESH_OTSU);

		//Using Canny edge detector
		cv::Canny(processed, processed, 2, 2 * 2, 3, false);

		//Finding contours on image - only external countours are needed here
		cv::findContours(processed, dices, diceh, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		diceData.clear();
		//Check all found countours
		for (int i = 0; i < dices.size(); i++) {

			//Count contour area
			double diceContourArea = cv::contourArea(dices[i]);

			//Filter by dice size
			if (diceContourArea > 1000 && diceContourArea < 10000) {

				//Get bounding rectangle
				cv::Rect diceBoundsRect = cv::boundingRect(cv::Mat(dices[i]));

				//Set reason of interest for dice
				cv::Mat diceROI = frame(diceBoundsRect);

				//Count circles
				int numberOfCircles = countCircles(diceROI);

				//Initialize new dice data and set its variables
				DiceData dd;
				dd.diceBounds = diceBoundsRect;
				dd.diceRegion = diceROI;

				//Handle simple recognition error
				//if (lastnum < 0) lastnum = numberOfCircles;
				//else if (lastnum != numberOfCircles) 
				//{
				//	if (abs(lastnum - numberOfCircles) < 3) 
				//	{
				//		numberOfCircles = lastnum;
				//	}
				//}

				dd.circles = numberOfCircles;

				//Add dice to collection
				diceData.push_back(dd);
				//cv::rectangle(frame, diceBoundsRect, cv::Scalar(255, 0, 255));
				cout << diceContourArea << endl;
			}

		}
		cout << diceData.size() << endl;
		
		if (diceData.empty()) lastnum = -500;
		for each (DiceData item in diceData)
		{
			cv::rectangle(frame, item.diceBounds, cv::Scalar(255, 0, 255));
			string text = "Dobasod: " + to_string(item.circles);
			cv::putText(frame,text,cv::Point(item.diceBounds.x - 25,item.diceBounds.y-10),cv::FONT_HERSHEY_SIMPLEX,1, cv::Scalar(255, 0, 255), 2, CV_AA);
		}

		cv::imshow("PreviewWindow", processed);
		cv::imshow("BaseWindow", frame);

		char key = static_cast<char>(cv::waitKey(1));
		if (key == 27) { std::cout << "Exiting..." << std::endl; break; }
		else if (key == 32) { capture.read(background); }
	}

	return 0;
}