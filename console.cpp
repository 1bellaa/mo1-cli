#include "console.h"

using namespace std;

void Console::Initialize() {
	if (initialized) {
		cout << "System already initialized." << endl;
		return;
	}
	// loadconfig thingy here

	initialized = true;
	cout << "System initialized successfully." << endl;
}

bool Console::IsInitialized() {
	return initialized;
}

void Console::CreateScreen(const string& name) {
	cout << "Creating screen: " << name << endl;
}

void Console::ListScreens() {
	cout << "Listing all screens:" << endl;
}

void Console::SearchScreen(const string& name) {
	cout << "Searching for screen: " << name << endl;
}

void Console::SchedulerStart() {
	cout << "Starting scheduler..." << endl;
}

void Console::SchedulerStop() {
	cout << "Stopping scheduler..." << endl;
}

void Console::ReportUtil() {
	cout << "Reporting CPU utilization..." << endl;
}