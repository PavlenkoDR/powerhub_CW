#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <fstream>
#include <fstream>
#include <string>
#include <unistd.h>	//sleep()
#include <vector>
#include <ftdi.h>
#include <termios.h>
#include <cmath> 	//round()
#include "json.hpp" 	//MIT License
#include "ads1x15c.hpp"	//BSD License. Updated for raspberry pi by me
#include <sstream>      // std::stringstream
#include <map>

#define EXIT_SUCCESSFUL			256 
#define HARD_ERROR			255 //exit with smth error
#define SOFT_ERROR			254   

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
#define PING_HOST_STATUS_CONFIRMED	3

//logs
#define LOGS_EMPTY_PARAM		0
#define LOGS_DEC			1
#define LOGS_HEX			2
#define LOGS_NO_LOG			3

//Hall
#define HALL_VPS_ON			0
#define HALL_VPS_OFF			1
#define HALL_VPS_NOT_CHECKED		2
#define HALL_VPS_UNKNOWN		3
#define HALL_STATUS_CONFIRMED		4

//ADS
#define ADS_VALUE_MIN 			0
#define ADS_VALUE_MAX 			1
#define ADS_VALUE_AVG 			2
#define ADS_VALUE_MED 			3

#define private public

using json = nlohmann::json;

class powerhub{
private:
	class releClass
	{
	private:
		class relepin
		{
		public:
			bool 	    rele_pin_init = false;
			bool 	    ssh_control   = false;
			bool 	    check_ads     = false;
			bool 	    check_ip      = false;
			bool 	    temp_control  = false;
			std::string hall_address;
			std::string pin_hall;
			std::string term_address;
			std::string pin_term;
			std::string pin_out;
			std::string user;
			std::string ip;
			int 	    port;
		};
	public:
		int pinSize = 8, rele_offSize = 8, rele_onSize = 8;
		relepin pin[8];
		int rele_off[8], rele_on[8];
		bool init = false;
	} rele;
	struct ftdi_context *ftdic;
	std::map<std::string, int> commandvoc;
	json config;
	std::string fconfig;
	std::string sshpassword;
	std::ofstream logFile;
	bool logsMutex = false;
	bool clearCommandLine = false;
	
	bool FTDIstatus;// 0 - off, 1 - on
	bool logsStatus;// 0 - off, 1 - on
	bool sshControl;// 0 - off, 1 - on
	bool checkHall;  // 0 - off, 1 - on
	bool checkIP;   // 0 - off, 1 - on
	bool tempControl;   // 0 - off, 1 - on
	bool work_capacity;
	
	adsGain_t gain;
	double VPS;
	int ADS_min;
	int ADS_max;
	double term_min;
	double term_max;
	double term_normal_min;
	double term_normal_max;
	double term_critical_min;
	double term_critical_max;
	double term_dangerous_min;
	double term_dangerous_max;
	int check_sleep;
	std::string email;
	bool emailRead;
	
	std::string GetDataTime();
	int InitCommandLine();
	int FreePowerHub();
	
public:
	powerhub();
	powerhub(std::string str);
	~powerhub();
	std::vector<int> CheckADS(std::string i2caddress, int pin, int number_metering);
	double CheckThermalSensor(std::string i2caddress, int pin, int number_metering, int value);
	void ThreadCheckTerm();
	void ThreadCheckWorkingCapacity();
	void _InitThreads();
	int CheckADSGetValue(std::string i2caddress, int pin, int number_metering, int value);
	int CheckHallSensorStatus(std::string i2caddress, int pin, int number_metering);
	int CheckHallSensorStatusWaitOfState(std::string i2caddress, int pin, int number_metering, int try_count, int state);
	int _Power(unsigned char pin, int e);
	int _Reboot(unsigned char pin);
	int CommandLine(int argc, char *argv[]);
	int ExecuteArgcArgv(int argc, char *argv[]);
	int ExecuteCommand(std::string word, std::vector<std::pair<std::string, std::string>> flag);
	int FreeFTDI();
	int ftdi_fatal (std::string str, int ret);
	int InitFTDI();
	int InitPowerHub(std::string str);
	int InitSSH();
	int InitThreads();
	int LoadConfigs(std::string fin);
	int Logs(int msg, std::string str);
	int ParsCommand(std::string in);
	int PingHost(std::string host, int try_count);
	int PingHostWaitOfState(std::string host, int try_count, int state);
	int PowerOff(int n, int try_count);
	int PowerOffAll(int try_count);
	int PowerOn(int n, int try_count);
	int PowerOnAll(int try_count);
	int Reboot(int n, int try_count);
	int RebootAll(int try_count);
	int ReloadFTDI();
	int SSHPowerOff(int n, int try_count);
	int SSHPowerOn(int n, int try_count);
};

