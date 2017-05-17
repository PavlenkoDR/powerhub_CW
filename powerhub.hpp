#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>	//sleep()
#include <vector>
#include <ftdi.h>
#include "json.hpp" 	//MIT License
#include "ads1x15c.hpp"	//BSD License. Updated for raspberry pi by me

#define EXIT_SUCCESSFUL			256 
#define EXIT_ERROR			255 //exit with smth error
#define ERROR				254   

//see /help
#define SUCCESSFUL			0   
#define COMMAND_ALLON			1   
#define COMMAND_ALLOFF			2  
#define COMMAND_REBOOTALL		3   
#define COMMAND_ON			4   
#define COMMAND_OFF			5   
#define COMMAND_REBOOT			6   
#define COMMAND_STATUS			7   
#define COMMAND_HELP			8   
#define COMMAND_RELOADCONFIG		9  
 
//command line parsing define
#define PARS_BEFORE_READ_COMMAND	0
#define PARS_READ_COMMAND		1
#define PARS_READ_FLAG			2
#define PARS_READ_FLAG_VALUE		3
#define PARS_END			4

//ping define
#define PING_HOST_AVAILABLE		0
#define PING_HOST_NOT_AVAILABLE		1
#define PING_HOST_NOT_CHECKED		2

using json = nlohmann::json;

void ftdi_fatal (char *str, int ret);
int LoadConfigs(std::string fin);
int InitFTDI();
int FreeFTDI();
int ReloadFTDI();
int PingHost(std::string host, int try_count);
int _Power(unsigned char pin, int e);
int Power(int n, int try_count, int e);
int PowerOn(int n, int try_count);
int PowerOnAll(int try_count);
int PowerOff(int n, int try_count);
int PowerOffAll(int try_count);
int RebootAll(int try_count);
double CheckHallSensorVPS(std::string i2caddress, int pin, int n);
int ExecuteCommand(std::string word, std::vector<std::pair<std::string, std::string>> flag);
int InitCommandLine();
int CommandLine(int argc, char *argv[]);
int InitI2CBus();
int FreeI2CBus();

