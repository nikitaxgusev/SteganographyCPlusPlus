#include "tbb/task_group.h"
#include "tbb/tick_count.h"
#include "opencv/cv.hpp"
#include "opencv2/opencv.hpp"
#include <omp.h>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>

using namespace cv;
using namespace tbb;


void help()
{
	std::cout << "Help note:\nUsage for encrypting: [imageName].[png/.jpg/.bmp] -r [fileName].txt"<<std::endl;
	std::cout << "For decrypting: [imageName].[png/.jpg/.bmp]"<<std::endl;
}
bool checkFormat(std::string imageName)
{
	std::vector<std::string> VecOfFormats { ".png",".jpg",".bmp" };
	for (int i = 0; i < VecOfFormats.size(); i++)
	{
		std::size_t found = imageName.find(VecOfFormats[i]);
		if (found != std::string::npos)
			return true;
	}
	return false;

}
int changeLastChar(int value, int newEnd) {
	// change last decimal of an integer
	if (newEnd > 9) return 1;
	value /= 10;
	value *= 10;
	value += newEnd;
	return value;
}

//TEST function
void printPixels(Mat image, int pixelsToPrint) {
	// get RGB value of some pixels. for debugging.

	int readPixels = 0;
	if (pixelsToPrint > (image.cols*image.rows))
		std::cout << "oops" << std::endl;
	else {
		for (int r = 0; r < image.rows; r++) {
			for (int c = 0; c < image.cols; c++) {
				if (readPixels++ < pixelsToPrint) {
					//image.at<Vec3b>(r, c)[2]
					std::cout << (int)image.at<Vec3b>(r, c)[2] << ","		//red
						<< (int)image.at<Vec3b>(r, c)[1] << ","		//green
						<< (int)image.at<Vec3b>(r, c)[0] << "\n";	//blue
				}
			}
		}
	}
}

void ZeroTextToImage(Mat& image, int rr, int rr1, int cc, int cc1) {
	// change all pixels RGB values to end with 0
	//#pragma omp parallel //Because it's too much threads here. Check if the image is so big... CHECKED! NO PROFIT!
	for (int r = rr; r < rr1; r++) {
	//	#pragma omp for nowait			
		for (int c = cc; c < cc1; c++) {
			image.at<Vec3b>(r, c)[2] = changeLastChar(image.at<Vec3b>(r, c)[2], 0);	// red
			image.at<Vec3b>(r, c)[1] = changeLastChar(image.at<Vec3b>(r, c)[1], 0);	// green
			image.at<Vec3b>(r, c)[0] = changeLastChar(image.at<Vec3b>(r, c)[0], 0);	// blue
		}
	}
}




void writeTextToImage(Mat& image, std::vector<char> v, int count) {

#pragma omp parallel
	{
		#pragma omp  for
		for (int i = 0; i < v.size(); i++)
		{

			int row = (count + i) / image.cols;
			int	 col = (count + i) % image.cols;
			int ascii = v[i];

			image.at<Vec3b>(row, col)[0] = changeLastChar(image.at<Vec3b>(row, col)[0], ascii % 10);	// blue

			ascii /= 10;
			image.at<Vec3b>(row, col)[1] = changeLastChar(image.at<Vec3b>(row, col)[1], ascii % 10);	// green
			

			ascii /= 10;
			image.at<Vec3b>(row, col)[2] = changeLastChar(image.at<Vec3b>(row, col)[2], ascii % 10);	// red	
		}
		
	}

}

std::string readTextFromImage(std::string imageName) {

	std::string result = "";
	int red;
	int green;
	int blue;
	int ascii;
	char ch;

	Mat image = imread(imageName);

	if (!image.data) {
		std::cerr << "Could not get image data.\n";
		help();
		exit(-1);
	}

	// check so that first 3 pixels end with 0
	for (int testCol = 0; testCol < 3; testCol++) {
		red = (image.at<Vec3b>(0, testCol)[2] + 1 - 1) % 10;
		green = (image.at<Vec3b>(0, testCol)[1] + 1 - 1) % 10;
		blue = (image.at<Vec3b>(0, testCol)[0] + 1 - 1) % 10;
		if (red != 0 || green != 0 || blue != 0) {
			std::cout << "\nNo encrypted message found.\n";
			return "";
		}
	}

	for (int r = 0; r < image.rows; r++) {
		for (int c = 0; c < image.cols; c++) {
			if (r == 0 && c < 3) continue;	// skip 3 first

			red = (image.at<Vec3b>(r, c)[2] + 1 - 1) % 10;
			green = (image.at<Vec3b>(r, c)[1] + 1 - 1) % 10;
			blue = (image.at<Vec3b>(r, c)[0] + 1 - 1) % 10;

			// 0.0.0 = no more data to read
			if (red == 0 && green == 0 && blue == 0)
				return result;

			ascii = red * 100 + green * 10 + blue;
			ch = ascii;
			
			result += ch;
		}
	}
	return result;
}


void writeInsideInPicture(std::string imageName,std::string fileName)
{
	Mat image = imread(imageName);

	if (!image.data) {
		std::cerr << "Could not get image data.\n";
		help();
		exit(-1);
	}



	int MyCols = image.cols / 2;
	int MyCols1 = image.cols;

	int MyRows = image.rows / 2;
	int MyRows1 = image.rows;



	if (image.rows % 10 % 2 != 0){
		MyRows = (image.rows / 2) + 1;
		MyRows1 = image.rows;
	}

	if (image.cols % 10 % 2 != 0){
		MyCols = (image.cols / 2) + 1;
		MyCols1 = image.cols;
	}



		std::string message;

		std::ifstream fileTakeMessage;
		fileTakeMessage.open(fileName);

		std::string s;

		if (fileTakeMessage.is_open()) {
			while (std::getline(fileTakeMessage, s))
				message += s;
		}

		std::vector<char> v;
		for (char ch : message){
			v.push_back(ch);
		}


		std::cout << "The quantity of characters in your txt file: " << v.size() << " symbols" << std::endl;

		if (message.length() + 3 > (image.rows * image.cols)) {
			std::cerr << "Too small image for that message\n";
			exit(-1);
		}

		double start = omp_get_wtime();

		tbb::task_group g;
		g.run([&] {ZeroTextToImage(image, 0, MyRows, 0, MyCols); });
		g.run([&] {ZeroTextToImage(image, MyRows, MyRows1, MyCols, MyCols1); });
		g.wait();

		writeTextToImage(image, v, 3);

		double end = omp_get_wtime();
		std::cout << end - start << " s." << std::endl;
		imwrite("save.png", image);
		std::cout << "Successful saving. You have got a file for result: save.png" << std::endl;
}

int main(int argc, char** argv)
{
	
	if (argc == 1)
	{
		std::cerr << "No arguments." << std::endl;
		help();
		return -1;
	}

	std::string imageName = argv[1];

	
	if (checkFormat(imageName))
	{
		if (argc == 2)
		{
			std::string res;

			double start = omp_get_wtime();
			res=readTextFromImage(imageName);
			double end = omp_get_wtime();
			std::cout <<"Time work: "<<end - start << " s." << std::endl;
			std::cout << "Check the result file: After_decrypting.txt " << std::endl;

			std::ofstream fileOutResultMessage("After_decrypting.txt");
			fileOutResultMessage << res;

		}

		if (argc == 4)
		{
			if (strcmp(argv[2], "-r") == 0)
			{
				std::string fileName = argv[3];
				std::size_t found = fileName.find(".txt");

				if (found != std::string::npos)
					writeInsideInPicture(imageName, fileName);
				else
					help();
			}
			else
			{
				std::cout << "Wrong the second argument. Only for using: [-r]" << std::endl;
				help();
			}
		}
	}
	else
	{
		std::cout << "You have an issue of the first argument format. Only for using: (name).png, (name).jpg, (name).bmp " << std::endl;
		help();
	}
	return 0;
}

