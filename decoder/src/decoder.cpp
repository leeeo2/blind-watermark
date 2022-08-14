#include <opencv2/opencv.hpp>

#define CHAR_WIDTH 21
using namespace cv;

Vec3b markColor = {0, 0, 255};
bool accept(Vec3b temp)
{
	int filter = 5;
	int count = 0;
	for (int i = 0; i < 3; i++)
	{
		if ((temp[i] > 0 && temp[i] <= filter))
		{
			count++;
		}
	}
	return (count == 3);
}

Mat decode(Mat src)
{
	Vec3b last = src.at<Vec3b>(0, 0);
	int count = 0;
	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			Vec3b now = src.at<Vec3b>(i, j);
			Vec3b temp = now - last;
			if (accept(temp))
			{
				count++;
			}
			else
			{
				if (count > 0 && count <= CHAR_WIDTH)
				{
					for (int k = j - count; k < j; k++)
					{
						src.at<Vec3b>(i, k) = markColor;
					}
				}
				count = 0;
				last = now;
			}
		}
	}
	count = 0;
	for (int i = src.rows - 1; i >= 0; i--)
	{
		for (int j = src.cols - 1; j >= 0; j--)
		{
			Vec3b now = src.at<Vec3b>(i, j);
			Vec3b temp = now - last;
			if (accept(temp))
			{
				count++;
			}
			else
			{
				if (count > 0 && count <= CHAR_WIDTH)
				{
					for (int k = j + count; k > j; k--)
					{
						src.at<Vec3b>(i, k) = markColor;
					}
				}
				count = 0;
				last = now;
			}
		}
	}
	return src;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		printf("      require 2 argument...\n");
		printf("      Usage: %s [input] [output]\n", argv[0]);
		return -1;
	}

	std::string inputFileName = argv[1];
	std::string outputFileName = argv[2];

	Mat src = imread(inputFileName);
	if (!src.empty())
	{
		imwrite(outputFileName, decode(src));
		return 0;
	}
	return 0;
}
