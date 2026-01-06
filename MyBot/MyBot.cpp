#define _CRT_SECURE_NO_WARNINGS

#include <dpp/dpp.h>
#include <iostream>
#include <string>
#include <dotenv/dotenv.h>
#include <random>
#include <iomanip>

using namespace std;

int main() {
	dotenv env(".env");

	dpp::cluster bot(env.get("token", ""));

	bot.on_ready([&bot](const dpp::ready_t& event) {
		cout << "Bot initiazlied" << endl;
		auto list = make_shared<map<time_t, dpp::message>>(); //ordered map

		//retrieve params
		int size = 50;

		auto fetch = make_shared<function<void(dpp::snowflake)>>();
		*fetch = [list, size, fetch, &bot](dpp::snowflake bef) {
			bot.messages_get(1457638344883437669, 0, bef, 0, size, [list, size, fetch](dpp::confirmation_callback_t value) {
				if (value.is_error()) {
					cout << "Something went wrong" << endl;
					cout << value.get_error().human_readable << endl;
					return;
				}

				auto* msgs = get_if<dpp::message_map>(&value.value);
				if (!msgs) {
					cout << "Messages returned empty" << endl;
					return;
				}

				for (dpp::message_map::iterator it = msgs->begin(); it != msgs->end(); it++) {
					dpp::snowflake id = it->second.id;
					uint64_t timestamp = (id >> 22) + 1420070400000;
					time_t seconds = timestamp / 1000;

					(*list)[seconds] = it->second; //adding time to ordered map
				}

				cout << "Retrieved: " << msgs->size() << endl;

				//breaking loop
				if (msgs->size() < size) {
					cout << "Finished Retrieving" << endl;
					return;
				}
				else {
					dpp::snowflake id = (*list).begin()->second.id;
					cout << "Snowflake id: " << id << endl;
					(*fetch)(id);
				}
			});
		};

		// starting the fetch lambda
		(*fetch)(0);
	});

	bot.start(dpp::st_wait);

	return 0;
}

void printMessages(auto& list) {
	//printing time
	for (const auto& pair : list) {
		tm buf;
		localtime_s(&buf, &pair.first);
		cout << std::put_time(&buf, "%Y-%m-%d %H:%M:%S UTC") << endl;
	}
}