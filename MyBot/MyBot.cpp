#define _CRT_SECURE_NO_WARNINGS

#include <dpp/dpp.h>
#include <iostream>
#include <string>
#include <dotenv/dotenv.h>
#include <random>
#include <iomanip>
#include <fstream>
#include <ctime>

using namespace std;

time_t idToTime(const dpp::snowflake& id) {
	uint64_t timestamp = (id >> 22) + 1420070400000;
	return (timestamp / 1000);
}

shared_ptr<tm> getTime(const time_t& arg) {
	auto thetime = make_shared<tm>();
	localtime_s(thetime.get(), &arg);
	return thetime;
}

void printMessages(const shared_ptr<map<time_t, dpp::message>>& list) {
	//printing time
	for (const auto& pair : *list) {
		auto timeptr = getTime(pair.first);
		cout << put_time(timeptr.get(), "%Y-%m-%d %H:%M:%S UTC") << endl;
	}
}

void printBadMessages(const vector<dpp::message>& list) {
	cout << endl;
	cout << "Printing bad messages" << endl << endl;	

	//getting current time
	time_t now = time(0);
	char time_string[50];

	strftime(time_string, sizeof(time_string), "%Y-%m-%d_%H-%M-%S", localtime(&now));
	string filename = format("{}.txt", time_string);
	cout << "Saving to file: " << filename << endl;

	//saving to file
	ofstream thefile(filename);

	for (const dpp::message &item : list) {	
		time_t seconds = idToTime(item.id);
		auto timeptr = getTime(seconds);

		string output = format("\"{}\" sent by {}", item.content, item.author.global_name);
		
		cout << output <<  " on " << put_time(timeptr.get(), "%Y-%m-%d %H:%M:%S UTC") << endl;; //printing to terminal
		thefile << output << " on " << put_time(timeptr.get(), "%Y-%m-%d %H:%M:%S UTC") << endl; //printing to output file		
		
	}
	thefile.close();
	cout << endl  << "Finished printing bad messages" << endl;
}



int main() {
	dotenv env(".env");

	dpp::cluster bot(env.get("token", ""), dpp::i_guilds | dpp::i_guild_members);

	bot.on_slashcommand([&bot](const dpp::slashcommand_t event) mutable -> dpp::task<void> {
		if (event.command.get_command_name() == "clean_channel") {			
			auto user = make_shared<dpp::guild_member>();
			auto channel = make_shared<dpp::channel>();

			//retrieving the channel
			*channel = event.command.get_channel();
			dpp::snowflake channel_id = channel->id;

			event.reply("Processing");			
			
			//getting the original response
			dpp::confirmation_callback_t orig_resp = co_await event.co_get_original_response();
			if (orig_resp.is_error()) {
				cout << "[ERROR]" << orig_resp.get_error().human_readable << endl;
				co_return;
			}
			else {
				//deleting message
				dpp::message msg = get<dpp::message>(orig_resp.value);
				dpp::confirmation_callback_t msg_delete = co_await bot.co_message_delete(msg.id, channel_id);

				if (msg_delete.is_error()) {
					cout << "[ERROR] " << msg_delete.get_error().human_readable << endl;
					co_return;
				}
				else {
					cout << "Bot message deleted" << endl;
				}
			}

			

			//retrieving the user
			dpp::confirmation_callback_t user_res = co_await bot.co_guild_get_member(channel->guild_id, bot.me.id);
			if (user_res.is_error()) {
				cout << "[ERROR] " << user_res.get_error().human_readable << endl;
				co_return;
			}
			else {
				*user = user_res.get<dpp::guild_member>();
			}

			//retrieving the perms
			dpp::permission perms = channel->get_user_permissions(*user);
			if (!perms.can(dpp::p_manage_messages)) {
				cout << "Missing permissions to manage messages" << endl;
				co_return;
			}

			auto list = make_shared<map<time_t, dpp::message>>(); //ordered map
			auto bad_list = make_shared< vector<dpp::message>>(); // list for non 6 messages

			//retrieve params
			int size = 50;

			auto fetch = make_shared<function<void(dpp::snowflake)>>();
			*fetch = [channel_id, bad_list, list, size, fetch, &bot](dpp::snowflake bef) {
				bot.messages_get(channel_id, 0, bef, 0, size, [bad_list, list, size, fetch, &bot](dpp::confirmation_callback_t value) {
					if (value.is_error()) { //error
						cout << "Something went wrong" << endl;
						cout << value.get_error().human_readable << endl;
						return;
					}

					auto* msgs = get_if<dpp::message_map>(&value.value);
					if (!msgs) { //no messages were found in channel
						cout << "Messages returned empty" << endl;
						return;
					}

					//adding to the ordered map
					for (dpp::message_map::iterator it = msgs->begin(); it != msgs->end(); it++) {
						if (it->second.content != "6") { //adding non 6 message to the bad list
							bad_list->push_back(it->second);
						}
						time_t seconds = idToTime(it->second.id);

						/*(*list)[seconds] = it->second;*/ //this makes copies
						list->try_emplace(seconds, move(it->second)); // moving instead of making copies
					}
					auto tmptr = getTime(list->begin()->first);
					cout << "Retrieved: " << msgs->size() << " ending on " << put_time(tmptr.get(), "%Y-%m-%d %H:%M:%S UTC") << endl;

					//breaking loop
					if (msgs->size() < size) {
						cout << "Finished Retrieving" << endl;
						cout << endl;
						printMessages(list);//printing
						printBadMessages(*bad_list);

						//deleting bad messages
						for (auto& item : *bad_list) {
							bot.message_delete(item.id, item.channel_id, [](dpp::confirmation_callback_t value) {
								if (value.is_error()) {
									cout << "[ERROR] " << value.get_error().human_readable << endl;
								}
								});
						}

						return;
					}
					else {
						(*fetch)((*list).begin()->second.id); //starting new fetch from the earliest message in the list
					}
					});
				};

			// starting the fetch lambda
			(*fetch)(0);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		//registering the slash commands
		if (dpp::run_once<struct register_bot_commands>()) {
			cout << "Registering commands" << endl;

			vector<dpp::slashcommand> list;
			dpp::slashcommand clean_command("clean_channel", "This command will delete all the non 6 messages from this channel", bot.me.id);
			list.push_back(clean_command);
			bot.global_bulk_command_create(list);

			cout << "Registered commands" << endl << endl;
		}

		cout << "Bot initiazlied" << endl;
	});

	bot.start(dpp::st_wait);

	return 0;
}