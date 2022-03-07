 
using namespace std;
using namespace cv;
using namespace TgBot;

namespace travisFilter {
enum paramNumbers { contrast = 0, brightness = 1, gamma = 2, yellow = 3, grain = 4 };

void  correction(Mat &img, double alpha_, int beta_,
		double gamma_) {
	Mat res1;
	img.convertTo(res1, -1, alpha_, beta_);
	CV_Assert(gamma_ >= 0);
	Mat lookUpTable(1, 256, CV_8U);
	uchar *p = lookUpTable.ptr();
	for (int i = 0; i < 256; ++i)
		p[i] = saturate_cast<uchar>(pow(i / 255.0, gamma_) * 255.0);
	Mat res = img.clone();
	LUT(res1, lookUpTable, res);
	img = res.clone();
}

void addGrain(Mat &img, int depth)
{
    using namespace std::experimental::fundamentals_v2;

    for (int i = 0; i < img.rows; i++)
        for (int j = 0; j < img.cols; j++) {
            int a = img.at<Vec3b>(i, j)[0] + randint(-1 * depth, depth),

                b = img.at<Vec3b>(i, j)[1] + randint(-1 * depth, depth) / 2,

                c = img.at<Vec3b>(i, j)[2] + randint(-1 * depth, depth) / 1.5;

            if (a < 0)
                a = 0;
            if (b < 0)
                b = 0;
            if (c < 0)
                c = 0;
            if (a > UCHAR_MAX)
                a = UCHAR_MAX;
            if (b > UCHAR_MAX)
                b = UCHAR_MAX;
            if (c > UCHAR_MAX)
                c = UCHAR_MAX;

            img.at<Vec3b>(i, j)[0] = a;
            img.at<Vec3b>(i, j)[1] = b;
            img.at<Vec3b>(i, j)[2] = c;
        }
}

void addYellow(Mat &img, int depth)
{
    for (int i = 0; i < img.rows; i++)
        for (int j = 0; j < img.cols; j++) {
            (-depth, depth);
            int a = img.at<Vec3b>(i, j)[0], b = img.at<Vec3b>(i, j)[1], c = img.at<Vec3b>(i, j)[2];
            int S = (a + b + c) / 3;
            a -= 5;
            b = b + 0.15 * (S + depth);
            c = c + 0.30 * S;

            if (a < 0)
                a = 0;
            if (b < 0)
                b = 0;
            if (c < 0)
                c = 0;
            if (a > UCHAR_MAX)
                a = UCHAR_MAX;
            if (b > UCHAR_MAX)
                b = UCHAR_MAX;
            if (c > UCHAR_MAX)
                c = UCHAR_MAX;

            img.at<Vec3b>(i, j)[0] = a;
            img.at<Vec3b>(i, j)[1] = b;
            img.at<Vec3b>(i, j)[2] = c;
        }
}
 
 
Mat  filter(Mat *img_original, vector<int> p)
{
    try {
        using namespace travisFilter;

        //resize(img_original, img_original, Size(0, 0), 0.5, 0.5, INTER_LINEAR);
        if (img_original->empty()) {
            cout << "Could not open or find the image!\n"
                 << "\n";
        }

        Mat img_corrected = img_original->clone();

        //brightness and contrast filter
        correction(img_corrected,
                   p[paramNumbers::brightness] / 100.0,
                   p[paramNumbers::contrast] - 100,
                   p[paramNumbers::gamma] / 100.0);

        //adding random noises
        addGrain(img_corrected, p[paramNumbers::grain]);

        // adding some yellow tone
        addYellow(img_corrected, p[paramNumbers::yellow]);

        // blur
        GaussianBlur(img_corrected, img_corrected, Size(1, 1), 0, 0);

        return img_corrected;
    } catch (cv::Exception cvException) {
        cout << cvException.what() << "\n";

    } catch (std::exception stdException) {
        cout << stdException.what() << "\n";
    } catch (...) {
        cout << "unknown error\n";
    }
}
}
