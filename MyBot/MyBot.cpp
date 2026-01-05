#define _CRT_SECURE_NO_WARNINGS

#include <dpp/dpp.h>
#include <iostream>
#include <string>
#include <dotenv/dotenv.h>

using namespace std;

int main() {
	dotenv env(".env");

	dpp::cluster bot(env.get("token", ""));

	bot.on_ready([&bot](const dpp::ready_t& event) {
		cout << "Bot initiazlied" << endl;
		bot.messages_get(1348244337536008193, 0, 1456957892673671244, 0, 30, [](dpp::confirmation_callback_t value) {
			if (value.is_error()) {
				cout << "Something went wrong" << endl;
				cout << value.get_error().human_readable << endl;
				}
				
			auto* msgs = get_if<dpp::message_map>(&value.value);
			if (!msgs) {
				cout << "Messages returned empty";
				return 0;
			}

			for (dpp::message_map::iterator it = msgs->begin(); it != msgs->end(); it++) {
				dpp::snowflake id = it->second.id;
				uint64_t timestamp = (id >> 22) + 1420070400000;
				time_t seconds = timestamp / 1000;
				struct tm buf;				
				localtime_s(&buf, &seconds);			

				cout << std::put_time(&buf, "%Y-%m-%d %H:%M:%S UTC") << endl;
			}
			});

	}
	);

	bot.start(dpp::st_wait);

	return 0;
}