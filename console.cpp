#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <windows.h>
#include "console.h"

using namespace std;

Console::Console() : initialized(false), tickThread(nullptr), shouldRunTicks(false) {}

Console::~Console() {
	cout << "Shutting down console..." << endl;
}

void Console::Initialize() {
    scheduler.Initialize("config.txt");
    initialized = true;

    shouldRunTicks = true;
    tickThread = new std::thread(&Console::TickLoop, this);

    std::cout << "Console initialized successfully." << std::endl;
}

void Console::TickLoop() {
	cout << "Starting tick loop..." << endl;
}

void Console::CreateScreen(const std::string& processName) {
	cout << "Creating screen for process: " << processName << endl;
}

void Console::SearchScreen(const std::string& processName) {
	cout << "Searching for process: " << processName << std::endl;
}

void Console::DisplayProcessScreen(Process* proc) {
	cout << "Display Process Screen" << std::endl;
}

void Console::ListScreens() {
	cout << "Listing all screens..." << endl;
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