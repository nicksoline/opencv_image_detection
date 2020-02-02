#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>
#include <opencv2/video/background_segm.hpp>

#include <iostream>

using namespace cv;
using namespace std;

Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
void processVideo(char* videoFilename);

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		cout << " Usage: static_detection VideoToLoadAndProcess" << endl;
		return -1;
	}

	namedWindow("Frame");

	processVideo(argv[1]);

	return 0;
}

void processVideo(char* videoFilename) {
	// all frames are matrices of pixels
	Mat frame,						// current frame to be processed
		reference,					// 1st frame of the video with clear scene
		fgMaskMOG2,					// frame with moving objects only
		combine,					// output frame with 4 quadrants of processing frames
		reference_frame,			// reference-frame
		reference_frame_gray,		// reference-frame in gray scale
		reference_frame_blur,		// blurred reference-frame in gray scale
		reference_frame_gray_move,	// blurred moving objects
		ref_cnt,					// new objects minus moving objects
		fgMaskMOG2_contoured;		// silhouettes, see below
	int keyboard; //input from keyboard

	Ptr<BackgroundSubtractor> pMOG2;
	pMOG2 = createBackgroundSubtractorMOG2(1500, 64, false);

	//create the capture object
	VideoCapture capture(videoFilename);
	if (!capture.isOpened()){
		//error in opening the video input
		cerr << "Unable to open video file: " << videoFilename << endl;
		exit(EXIT_FAILURE);
	}
	// get reference frame
	capture >> reference;
	VideoWriter video("out.avi", CV_FOURCC('X', 'V', 'I', 'D'), 30.0, reference.size() * 2);
	combine = Mat::zeros(reference.size() * 2, reference.type());

	Rect rect, _rect(1000, 1000, 0, 0);
	int delay = 0, x = 0, y = 0;
	while ((char)keyboard != 'q' && (char)keyboard != 27 && capture.get(CV_CAP_PROP_POS_FRAMES) < capture.get(CV_CAP_PROP_FRAME_COUNT)){
		// get current frame
		capture >> frame;
		// get difference between current and reference frame: reference-frame in pixelwise manner
		subtract(reference, frame, reference_frame);
		//imshow("Frame", reference_frame); //window 1

		GaussianBlur(reference_frame, reference_frame_blur, Size(9, 9), 0, 0);
		//imshow("Frame", reference_frame_blur); // window 1 with blur

		cvtColor(reference_frame_blur, reference_frame_gray, CV_BGR2GRAY);
		//imshow("Frame", reference_frame_gray); // window 1 from blur to gray

		threshold(reference_frame_gray, reference_frame_gray, 35, 255, THRESH_BINARY);
		//imshow("Frame", reference_frame_gray); //window 1 from gray to binary

		//GaussianBlur(reference_frame_gray, reference_frame_gray, Size(9, 9), 0, 0);
		// accumulate a history of the frames in pMOG2 object to
		// compare its with each others within pMOG2 object
		// i.e. pMOG2 stores a frame with trails of moving objects
		pMOG2->apply(reference, fgMaskMOG2);
		//imshow("Frame", fgMaskMOG2);
		// smoothing of the picture to eliminate camera artefacts
		
		// get all moving objects
		pMOG2->apply(reference_frame_blur, fgMaskMOG2);
		//imshow("Frame", fgMaskMOG2); // window 2 unrefined

		// get binary frame (the mask) of blurred new objects
		

		// get silhouettes (very blurred contours) of moving objects
		GaussianBlur(fgMaskMOG2, fgMaskMOG2, Size(15, 15), 0, 0);
		//imshow("Frame", fgMaskMOG2); // window 2 unrefined with blur

		threshold(fgMaskMOG2, fgMaskMOG2, 1, 255, THRESH_BINARY);
		//imshow("Frame", fgMaskMOG2); // window 2 refined

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		Scalar color(255, 255, 255);
		Scalar color_blue(255, 127, 0);
		Scalar color_red(0, 80, 255);

		// find contours of silhouettes
		fgMaskMOG2.copyTo(fgMaskMOG2_contoured);
		findContours(fgMaskMOG2_contoured, contours, hierarchy, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		// drawing convex masks on unconvex moving contours to
		// hide dark (and therefore not detected as motion) segments of moving objects
		for (int i = 0; i < contours.size(); i++)
			drawContours(fgMaskMOG2, contours, i, color, CV_FILLED);

		// ref_cnt = new objects minus moving objects
		// i.e. ref_cnt = static new objects
		subtract(reference_frame_gray, fgMaskMOG2, ref_cnt);
		//imshow("Frame", ref_cnt); // window 3 unrefined

		// find contours of static new objects
		findContours(ref_cnt, contours, hierarchy, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		// conver color scales into drawable format
		//cvtColor(reference_frame_gray, reference_frame_gray, CV_GRAY2BGR);
		
		cvtColor(fgMaskMOG2, reference_frame_gray_move, CV_GRAY2BGR);
		//imshow("Frame", reference_frame_gray_move); // window 2 to color for window 3 hidden process

		// get difference between new and moving objects
		subtract(reference_frame, reference_frame_gray_move, ref_cnt);
		//imshow("Frame", ref_cnt); // window 3 refined
		// check if object is static
		for (int i = 0; i < contours.size(); i++) {
			if (contourArea(contours[i]) > 200) {
				delay++;
				// check if the object is presented on the video long enough
				if (delay > 80) {
					rect = boundingRect(contours[i]);
					// initialization of bounding rectangle coords
					if (x == 0 && y == 0) {
						x = rect.x;
						y = rect.y;
					}
					// check if object is quite motionless
					if (abs(x - rect.x) + abs(y - rect.y) < 200) {
						// check if bounding box grows
						if (rect.x < _rect.x)
							_rect.x = rect.x;
						if (rect.y < _rect.y)
							_rect.y = rect.y;
						if (rect.width > _rect.width)
							_rect.width = rect.width;
						if (rect.height > _rect.height)
							_rect.height = rect.height;
						// highlight an abandoned object on the frame with non-moving objects
						drawContours(ref_cnt, contours, i, color_blue, 2, LINE_AA);
						// highlight an abandoned object on the original frame
						rectangle(frame, _rect, color_red, 2);
					}
				}
			}
		}

		// drawing all four quadrants:

		// 1. top-left = new objects
		reference_frame.copyTo(combine(Range(0, frame.size().height), Range(0, frame.size().width)));

		// 2. bottom-left = moving objects
		reference_frame_gray_move.copyTo(combine(Range(frame.size().height, frame.size().height * 2), Range(0, frame.size().width)));

		// 3. bottom-right = new but non-moving objects with its contours
		ref_cnt.copyTo(combine(Range(frame.size().height, frame.size().height * 2), Range(frame.size().width, frame.size().width * 2)));

		// 4. top-right = highlighted static objects on original frame
		frame.copyTo(combine(Range(0, frame.size().height), Range(frame.size().width, frame.size().width * 2)));

		imshow("Frame", frame);
		video << frame;
		keyboard = waitKey(30);
	}
	//delete capture object
	video.release();
	capture.release();
}