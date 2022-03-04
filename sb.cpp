#include <experimental/random> // for randint function
#include <iostream>
#include <opencv2/opencv.hpp>
#include <tgbot/tgbot.h>
#include <string>
#include <cpr/cpr.h>
//  NOT "using namespace std;" here, to avoid collisions between the beta variable and std::beta in c++17
using namespace std;
using namespace cv;
using namespace TgBot;

//Default parameters for
#define DEFAULT_ALPHA 90 // contrast
#define DEFAULT_BETA 60 // brightness
#define DEFAULT_GAMMA 100 // gamma-correction
#define ADD_YELLOW_DEPTH 20 // adding yellow tone
#define GRAIN_DEPTH 50 // grain
#define OUTPUT_PHOTO_PATH "out.jpg"

namespace travisFilter {
enum params {
	contrast = 0, brightness = 1, gamma = 2, yellow = 3, grain = 4
};

void correction(Mat &img, double alpha_, int beta_, double gamma_);

void addYellow(Mat &img, int depth);

void addGrain(Mat &img, int depth);

Mat filter(Mat*, vector<int>);
} // namespace travisFilter

string token, startMessage;

int current_param;

bool changing = false;

vector<int> Params { DEFAULT_ALPHA, DEFAULT_BETA, DEFAULT_GAMMA,
ADD_YELLOW_DEPTH, GRAIN_DEPTH };

int main(int argc, char **argv) {

	ifstream tokenFile;
	tokenFile.open("token.txt");
	getline(tokenFile, token);

	std::ifstream startMessageFile("startMessage.txt");
	std::string temp;
	while (std::getline(startMessageFile, temp)) {
		startMessage += temp + "\n";
	}
	Bot bot(token);

	bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
		bot.getApi().sendMessage(message->chat->id, startMessage);
	});

	bot.getEvents().onCommand("brightness",
			[&bot](TgBot::Message::Ptr message) {
				current_param = travisFilter::params::brightness;
				bot.getApi().sendMessage(message->chat->id,
						"Enter the value you want (0-200)");
				changing = true;

			});

	bot.getEvents().onCommand("contrast", [&bot](TgBot::Message::Ptr message) {
		current_param = travisFilter::params::contrast;
		bot.getApi().sendMessage(message->chat->id,"Enter the value you want (0-200)" );
		changing = true;

	});

	bot.getEvents().onCommand("gamma", [&bot](TgBot::Message::Ptr message) {
		current_param = travisFilter::params::gamma;
		bot.getApi().sendMessage(message->chat->id,"Enter the value you want (0-200)" );
		changing = true;

	});

	bot.getEvents().onCommand("yellow", [&bot](TgBot::Message::Ptr message) {
		current_param = travisFilter::params::yellow;
		bot.getApi().sendMessage(message->chat->id,"Enter the value you want (0-200)" );
		changing = true;

	});

	bot.getEvents().onCommand("grain", [&bot](TgBot::Message::Ptr message) {
		current_param = travisFilter::params::grain;
		bot.getApi().sendMessage(message->chat->id,"Enter the value you want (0-200) (current is " + to_string(Params[travisFilter::params::grain]) + ")");
		changing = true;

	});

	bot.getEvents().onCommand("return_defaults",
			[&bot](TgBot::Message::Ptr message) {
				Params[travisFilter::params::brightness] = DEFAULT_ALPHA;
				Params[travisFilter::params::contrast] = DEFAULT_BETA;
				Params[travisFilter::params::gamma] = DEFAULT_GAMMA;
				Params[travisFilter::params::yellow] = ADD_YELLOW_DEPTH;
				Params[travisFilter::params::grain] = GRAIN_DEPTH;
				bot.getApi().sendMessage(message->chat->id,
						"Values changed. Send photos");
			});

	bot.getEvents().onAnyMessage(
			[&bot](TgBot::Message::Ptr message) {
				if (changing) {
					int value = stoi(message->text);
					Params[current_param] = value;
					changing = false;
					bot.getApi().sendMessage(message->chat->id,
							"Value changed. Send photos");
				} else if (message->photo.size() > 0) {
					for (PhotoSize::Ptr p : message->photo) {
						cout << p->width << " " << p->height << "\n";
					}
					File::Ptr sourceImage = bot.getApi().getFile(
							message->photo.back()->fileId);

					string filePath = sourceImage->filePath;
					cpr::Response r = cpr::Get(
							cpr::Url { "https://api.telegram.org/file/bot"
									+ token + "/" + filePath });

					string encoded_string = r.text;

					vector<uchar> data(encoded_string.begin(),
							encoded_string.end());

					Mat img = imdecode(data, IMREAD_UNCHANGED);
					Mat filtered_img = travisFilter::filter(&img, Params);

					cout << "Results in " << OUTPUT_PHOTO_PATH << "\n";
					imwrite(OUTPUT_PHOTO_PATH, filtered_img);

					bot.getApi().sendPhoto(message->chat->id,
							InputFile::fromFile(OUTPUT_PHOTO_PATH,
									"image//jpg"));

				}
			});

	try {
		printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
		TgBot::TgLongPoll longPoll(bot);
		while (true) {
			printf("Long poll started\n");
			longPoll.start();
		}
	} catch (TgBot::TgException &e) {
		printf("error: %s\n", e.what());
	}
	return 0;
}
Mat travisFilter::filter(Mat *img_original, vector<int> p) {
	try {
		using namespace travisFilter;

		//resize(img_original, img_original, Size(0, 0), 0.5, 0.5, INTER_LINEAR);
		if (img_original->empty()) {
			cout << "Could not open or find the image!\n" << "\n";
		}

		Mat img_corrected = img_original->clone();

		//brightness and contrast filter
		correction(img_corrected, p[params::brightness] / 100.0,
				p[params::contrast] - 100, p[params::gamma] / 100.0);

		//adding random noises
		addGrain(img_corrected, p[params::grain]);

		// adding some yellow tone
		addYellow(img_corrected, p[params::yellow]);

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

void travisFilter::correction(Mat &img, double alpha_, int beta_,
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

void travisFilter::addGrain(Mat &img, int depth) {
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

void travisFilter::addYellow(Mat &img, int depth) {
	for (int i = 0; i < img.rows; i++)
		for (int j = 0; j < img.cols; j++) {
			(-depth, depth);
			int a = img.at<Vec3b>(i, j)[0], b = img.at<Vec3b>(i, j)[1], c =
					img.at<Vec3b>(i, j)[2];
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
