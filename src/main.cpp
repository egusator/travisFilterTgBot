#include <algorithm>
#include <cpr/cpr.h>
#include <experimental/random> // for randint function
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <tgbot/tgbot.h>
#include <travisFilter.h>
using namespace std;
using namespace cv;
using namespace TgBot;
#define OUTPUT_PHOTO_PATH "out.jpg"
bool isNumber(string s);
namespace defaults {
enum params { contrast = 90, brightness = 60, gamma = 100, yellow = 20, grain = 50 };
}
namespace borders_min {
enum { contrast = 0, brightness = 0, gamma = 0, yellow = 0, grain = 0 };
}
namespace borders_max {
enum { contrast = 300, brightness = 100, gamma = 500, yellow = 100, grain = 100 };
}

string token, startMessage;

int currentParameterNumber;

bool parameterIsChanging = false;

vector<int> Params{defaults::params::contrast,
                   defaults::params::brightness,
                   defaults::params::gamma,
                   defaults::params::yellow,
                   defaults::params::grain};

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

    bot.getEvents().onCommand("brightness", [&bot](TgBot::Message::Ptr message) {
        currentParameterNumber = travisFilter::paramNumbers::brightness;
        bot.getApi().sendMessage(message->chat->id,
                                 "Enter the value you want (" + to_string(borders_min::brightness)
                                     + " - " + to_string(borders_max::brightness) + ").");
        parameterIsChanging = true;
    });

    bot.getEvents().onCommand("contrast", [&bot](TgBot::Message::Ptr message) {
        currentParameterNumber = travisFilter::paramNumbers::contrast;
        bot.getApi().sendMessage(message->chat->id,
                                 "Enter the value you want (" + to_string(borders_min::contrast)
                                     + " - " + to_string(borders_max::contrast) + ").");
        parameterIsChanging = true;
    });

    bot.getEvents().onCommand("gamma", [&bot](TgBot::Message::Ptr message) {
        currentParameterNumber = travisFilter::paramNumbers::gamma;
        bot.getApi().sendMessage(message->chat->id,
                                 "Enter the value you want (" + to_string(borders_min::gamma)
                                     + " - " + to_string(borders_max::gamma) + ").");
        parameterIsChanging = true;
    });

    bot.getEvents().onCommand("yellow", [&bot](TgBot::Message::Ptr message) {
        currentParameterNumber = travisFilter::paramNumbers::yellow;
        bot.getApi().sendMessage(message->chat->id,
                                 "Enter the value you want (" + to_string(borders_min::yellow)
                                     + " - " + to_string(borders_max::yellow) + ").");
        parameterIsChanging = true;
    });

    bot.getEvents().onCommand("grain", [&bot](TgBot::Message::Ptr message) {
        currentParameterNumber = travisFilter::paramNumbers::grain;
        bot.getApi().sendMessage(message->chat->id,
                                 "Enter the value you want (" + to_string(borders_min::grain)
                                     + " - " + to_string(borders_max::grain) + ").");
        parameterIsChanging = true;
    });

    bot.getEvents().onCommand("return_defaults", [&bot](TgBot::Message::Ptr message) {
        Params[travisFilter::paramNumbers::brightness] = defaults::params::brightness;
        Params[travisFilter::paramNumbers::contrast] = defaults::params::contrast;
        Params[travisFilter::paramNumbers::gamma] = defaults::params::gamma;
        Params[travisFilter::paramNumbers::yellow] = defaults::params::yellow;
        Params[travisFilter::paramNumbers::grain] = defaults::params::grain;
        bot.getApi().sendMessage(message->chat->id, "Values changed. Send photos.");
    });

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        if (parameterIsChanging) {
            if (!isNumber(message->text)) {
                bot.getApi().sendMessage(message->chat->id, "Wrong value. Send integer number.");
            } else {
                int inputParameterValue = stoi(message->text);
                Params[currentParameterNumber] = inputParameterValue;
                parameterIsChanging = false;
                bot.getApi().sendMessage(message->chat->id,
                                         "Value successfully changed. Send photos.");
            }
        } else if (message->photo.size() > 0) {
            File::Ptr sourceImage = bot.getApi().getFile(message->photo.back()->fileId);
            string filePath = sourceImage->filePath;
            cpr::Response r = cpr::Get(
                cpr::Url{"https://api.telegram.org/file/bot" + token + "/" + filePath});

            string encoded_string = r.text;

            vector<uchar> data(encoded_string.begin(), encoded_string.end());

            Mat img = imdecode(data, IMREAD_UNCHANGED);
            Mat filtered_img = travisFilter::filter(&img, Params);

            cout << "Results in " << OUTPUT_PHOTO_PATH << "\n";
            imwrite(OUTPUT_PHOTO_PATH, filtered_img);

            bot.getApi().sendPhoto(message->chat->id,
                                   InputFile::fromFile(OUTPUT_PHOTO_PATH, "image//jpg"));
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
bool isNumber(string s)
{
    for (int i = 0; i < s.length(); i++) {
        if (!isdigit(s[i]))
            return false;
    }
    return true;
}
