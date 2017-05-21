#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <fstream>
#include <string>
#include <unistd.h>	//sleep()
#include <vector>
#include <ftdi.h>
#include <cmath> 	//round()
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

std::vector<int> CheckHallSensor(std::string i2caddress, int pin, int n);

void ftdi_fatal (char *str, int ret);

int ExecuteCommand(std::string word, std::vector<std::pair<std::string, std::string>> flag);
int PingHost(std::string host, int try_count);
int ExecuteArgcArgv(int argc, char *argv[]);
int CommandLine(int argc, char *argv[]);
int Power(int n, int try_count, int e);
int _Power(unsigned char pin, int e);
int PowerOff(int n, int try_count);
int PowerOn(int n, int try_count);
int InitPowerHub(std::string str);
int LoadConfigs(std::string fin);
int ParsCommand(std::string in);
int PowerOffAll(int try_count);
int PowerOnAll(int try_count);
int RebootAll(int try_count);
int InitCommandLine();
int FreePowerHub();
int ReloadFTDI();
int InitI2CBus();
int FreeI2CBus();
int InitFTDI();
int FreeFTDI();

