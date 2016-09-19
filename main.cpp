#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;

RNG rng;

const char* keys =
{
	"{help h usage ?| | Utility to generate CAPTCHA images }"
	"{resolution display-resolution dr| 3 | Display Resolution 0=AR_HQVGA, 1=AR_QVGA, 2=AR_WQVGA, 3=AR_HVGA }"
	"{ncharacters characters chars length| 6 | Number of characters in captcha }"
};

static enum DisplayResolution { AR_HQVGA = 0, AR_QVGA, AR_WQVGA, AR_HVGA };

const Size displayResolutionUnits[] = 
{
	{ 240, 160 },	// AR_HQVGA
	{ 320, 240 },	// AR_QVGA
	{ 400, 240 },	// AR_WQVGA
	{ 480, 320 }	// AR_HVGA
};

static Vec3b randBGR();
static Vec3b randBrightBGR();
static Vec3b randPastelBGR();
static void addNoise(Mat& image);
static void addTextFitted(Mat& image, const Rect& textRect, int nChars, const Scalar& colour);
static void warpText(Mat& image, const Rect& textRect);
static void combineMats(const Mat& fore, const Mat& back, Mat& result, const Scalar& targetForeColour);
static void fitTextHoriz(Mat& image, const std::string& txt, const Rect& txtContainer, 
		int fontFace = CV_FONT_HERSHEY_SIMPLEX, int thickness = 1, const Scalar& colour = { 0, 0, 0 });
static void fitTextVert(Mat& image, const std::string& txt, const Rect& txtContainer,
		int fontFace = CV_FONT_HERSHEY_SIMPLEX, int thickness = 1, const Scalar& colour = { 0, 0, 0 });

int main(int argc, char** argv)
{
	rng.state = getTickCount();
	CommandLineParser parser(argc, argv, keys);

	if (parser.has("help")) {
		parser.printMessage();
		return 0;
	}

	int nChars = parser.get<int>("characters");
	int drIndex = parser.get<int>("resolution"); 
	
	Scalar textColour = randBrightBGR();
	Size displayResolution = displayResolutionUnits[(drIndex >= 0 && drIndex < 4) ? drIndex : 0];
	Rect textRect(displayResolution.width * 0.2, displayResolution.height * 0.05, 
		displayResolution.width*0.8, displayResolution.height);

	Mat image(displayResolution, CV_8UC3, Scalar(230, 255, 255));
	Mat textImg(displayResolution, CV_8UC3, Scalar(0,0,0));

	addNoise(image);

	addTextFitted(textImg, textRect, nChars, textColour);

	warpText(textImg, textRect);

	combineMats(textImg, image, image, textColour);

	imshow("Result", image);
	waitKey(0);

	return 0;
}

Vec3b randBGR()
{
	return Vec3b(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
}

Vec3b randBrightBGR()
{
	Mat hsv(1, 1, CV_8UC3);
	cvtColor(hsv, hsv, CV_BGR2HSV);
	hsv.at<Vec3b>(0, 0) = Vec3b(rng.uniform(0, 180), 255, 255);
	cvtColor(hsv, hsv, CV_HSV2BGR);
	return hsv.at<Vec3b>(0, 0);
}

Vec3b randPastelBGR()
{
	Mat hsv(1, 1, CV_8UC3);
	cvtColor(hsv, hsv, CV_BGR2HSV);
	hsv.at<Vec3b>(0, 0) = Vec3b(rng.uniform(0, 180), rng.uniform(50, 75), 255);
	cvtColor(hsv, hsv, CV_HSV2BGR);
	return hsv.at<Vec3b>(0, 0);
}

void addNoise(Mat& image)
{
	int lineTypes[] = { FILLED, LINE_4, LINE_8, LINE_AA };
	int loops = rng.uniform(10, 20);
	
	for (int i = 0; i < loops; i++) 
	{
		Point centre(rng.uniform(0, image.cols), rng.uniform(0, image.rows));
		int radius = rng.uniform(1, int(image.rows*0.4));
		circle(image, centre, radius, randPastelBGR(), rng.uniform(-1, 7), lineTypes[rng.uniform(0, 4)]);
	}

	loops = rng.uniform(10, 20);
	for (int i = 0; i < loops; i++) 
	{
		Point strt(rng.uniform(0, image.cols), rng.uniform(0, image.rows));
		Point end(rng.uniform(0, image.cols), rng.uniform(0, image.rows));
		line(image, strt, end, randPastelBGR(), rng.uniform(1, 7), lineTypes[rng.uniform(1, 4)]);
	}
}

void addTextFitted(Mat& image, const Rect& textRect, int nChars, const Scalar& colour)
{
	std::string text;
	char characters[] = 
	{
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
		'u', 'v', 'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y', 'Z',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
	};

	for (int i = 0; i < nChars; i++)
		text += characters[rng.uniform(0, 62)];

	fitTextHoriz(image, text, textRect, CV_FONT_HERSHEY_PLAIN, 6, colour);
}

void warpText(Mat& image, const Rect& textRect)
{
	Point2f inputQuad[4], outputQuad[4];
	inputQuad[0] = Point2f(textRect.x, textRect.y);
	inputQuad[1] = Point2f(textRect.x + textRect.width, textRect.y);
	inputQuad[2] = Point2f(textRect.x + textRect.width, textRect.y + textRect.height);
	inputQuad[3] = Point2f(textRect.x, textRect.y + textRect.height);

	outputQuad[0] = Point2f(rng.uniform(25, 50), textRect.y);
	outputQuad[1] = Point2f(textRect.x + textRect.width, textRect.y);
	outputQuad[2] = Point2f(textRect.x + textRect.width, textRect.y + textRect.height);
	outputQuad[3] = Point2f(rng.uniform(25, 50) * -1, textRect.y + textRect.height);

	Mat lambda = getPerspectiveTransform(inputQuad, outputQuad);
	warpPerspective(image, image, lambda, image.size());
}

void combineMats(const Mat& fore, const Mat& back, Mat& result, const Scalar& foreTargetColour)
{
	cv::Mat foreTransparent;
	cv::inRange(fore, foreTargetColour, foreTargetColour, foreTransparent);
	fore.copyTo(back, foreTransparent);
}

void fitTextHoriz(Mat& image, const std::string& txt, const Rect& txtContainer,
	int fontFace, int thickness, const Scalar& colour)
{
	int baseLine = 0;
	int testScale = txtContainer.width;
	Size textSize = getTextSize("a", fontFace, testScale, thickness, &baseLine);

	float cWidth = float(textSize.width) / testScale;
	float cHeight = float(textSize.height) / testScale;

	float suggestedScale = txtContainer.width / cWidth / txt.length();
	float estHeight = cHeight * suggestedScale;
	int ty = (txtContainer.height - estHeight) * 0.5 + estHeight + txtContainer.y;
	putText(image, txt, { txtContainer.x, ty }, fontFace, suggestedScale, colour, thickness);
}

void fitTextVert(Mat& image, const std::string& txt, const Rect& txtContainer,
	int fontFace, int thickness, const Scalar& colour)
{
	int baseLine = 0;
	int testScale = 2;
	Size textSize = getTextSize("a", fontFace, testScale, thickness, &baseLine);

	float cWidth = float(textSize.width) / testScale;
	float cHeight = float(textSize.height) / testScale;

	float suggestedScale = txtContainer.height / cHeight;
	float estWidth = cWidth * suggestedScale * txt.length();
	int tx = txtContainer.x;
	int ty = txtContainer.y + txtContainer.height;
	putText(image, txt, { tx, ty }, fontFace, suggestedScale, colour, thickness);
}