#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>	//sleep()
#include <vector>
#include <ftdi.h>
#include "json.hpp" 	//MIT License

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

using namespace std;

struct ftdi_context *ftdic;
map<string, int> commandvoc;
json config;

void ftdi_fatal (char *str, int ret)
{
    fprintf (stderr, "%s: %d (%s)\n",
             str, ret, ftdi_get_error_string (ftdic));
    ftdi_free(ftdic);
    exit (1);
}

int LoadConfigs()
{	
	//"еach relay corresponds to the index 0-7"
	//"ip       - server each to be checked"
	//"pin_out  - rele adress"
	//"pin_in   - In the future, this will be the address of the Hall sensors"
	//"check_ip - check ip address? True - yes, false - no"
	//"rele_on  - Parameters when the relay is turn on"
	//"rele_off - Parameters when the relay is turn off"

	std::ifstream in;
	in.open("rele.config");
	in >> config;
	in.close();
	return SUCCESSFUL;
}

int InitFTDI()
{
	unsigned char c = 0;
	int ret;
	if ((ftdic = ftdi_new()) == 0) {
		fprintf(stderr, "ftdi_new() failed\n");
		return EXIT_FAILURE;
	}
	if ((ret = ftdi_usb_open(ftdic, 0x0403, 0x6001)) < 0) {
		ftdi_fatal((char*)"unable to open ftdi device", ret);
	}
	if ((ret = ftdi_set_bitmode(ftdic, 0xFF, BITMODE_BITBANG)) < 0) {
		ftdi_fatal((char*)"unable to open ftdi device", ret);
	}
	return SUCCESSFUL;
}

int FreeFTDI()
{
    	ftdi_usb_close(ftdic);
    	ftdi_free(ftdic);
	return SUCCESSFUL;
}

int ReloadFTDI()
{
	FreeFTDI();
	InitFTDI();
}

int PingHost(string host, int try_count)
{
	if (!config["check_ip"]) return PING_HOST_NOT_CHECKED;
	string str;
	FILE *in;
	char buff[512];
	ofstream out;
	int flag_host_ok;
	for (int i = 0; i < try_count; i++)
	{
		flag_host_ok = PING_HOST_NOT_AVAILABLE;
		str = "ping -c 1 "+ host;
		buff[0] = 0;
		try
		{
			if(!(in = popen(str.c_str(), "r")))
			{
				return PING_HOST_NOT_CHECKED;
			}
		}
		catch(...)
		{
			flag_host_ok = PING_HOST_AVAILABLE;
		}
		while(fgets(buff, sizeof(buff), in)!=NULL)
		{
			str = buff;
			if ((str.find("Destination Host Unreachable") != std::string::npos) or
			    (str.find("Network is unreachable")       != std::string::npos) or
			    (str.find("received, 100%")               != std::string::npos))
			{
				flag_host_ok = PING_HOST_AVAILABLE;
				break;
			}
		}
		pclose(in);
		if (buff[0] == 0)
			flag_host_ok = PING_HOST_AVAILABLE;
		if (flag_host_ok == PING_HOST_NOT_AVAILABLE) break;
		sleep(2);
	}
	return flag_host_ok;
}
int _Power(unsigned char pin, int e)
{
	int ret;
	unsigned char c = 0;
	cout << "pin " << (int)pin << " - " << endl;
	if ((ret = ftdi_read_data(ftdic, &c, 1)) < 0) {
		ftdi_fatal((char*)"unable to read from ftdi device", ret);
	}
	c = ( c & (~(1 << pin)) ) | ( e << pin );
	if ((ret = ftdi_write_data(ftdic, &c, 1)) < 0) {
		ftdi_fatal((char*)"unable to write into ftdi device", ret);
	}
	ReloadFTDI();
	if (c & ( 1 << pin )) 	std::cout << "on!"  << std::endl;
	else 			std::cout << "off!" << std::endl;
	return SUCCESSFUL;
}

int Power(int n, int try_count, int e)
{
	int ret;
	int ip_status;
	string rele;
	if (e) rele = "rele_on";
	else   rele = "rele_off";
	string ip = config[rele][n]["ip"];
	string str = config[rele][n]["pin_out"];
	unsigned char pin = std::stoi(str,nullptr,0), c = 0;
	cout << n << ": ";
	_Power(pin, e);
	//if (PingHost(config[rele][n]["ip"], 1)) return 0;
	for (int i = 0; i < try_count; i++)
	{
		ip_status = PingHost(ip, 1);
		cout << "IP " << ip << " ";
		if 	(ip_status == PING_HOST_AVAILABLE) 	cout << "closed!"      << endl;
		else if (ip_status == PING_HOST_NOT_AVAILABLE) 	cout << "opened!"      << endl;
		else if (ip_status == PING_HOST_NOT_CHECKED) 	cout << "not checked!" << endl;
		if ((ip_status == e)||(ip_status == PING_HOST_NOT_CHECKED)) return SUCCESSFUL;
		cout << i << ": try again" << endl;
	}
	return ERROR;
}

int PowerOn(int n, int try_count)
{
	return Power(n, try_count, 1);
}

int PowerOnAll(int try_count)
{
	for (int i = 0; i < config["rele_on"].size(); i++)
		if (PowerOn(i, try_count) == SUCCESSFUL)
			continue;
		else
		{
			cout << "Can't open " << i << endl;
			return EXIT_ERROR;
		}
	return SUCCESSFUL;
}

int PowerOff(int n, int try_count)
{
	return Power(n, try_count, 0);
}


int PowerOffAll(int try_count)
{
	for (int i = 0; i < config["rele_off"].size(); i++)
		if (PowerOff(i, try_count) == SUCCESSFUL)
			continue;
		else
		{
			cout << "Can't close " << i << endl;
			return EXIT_ERROR;
		}
	return SUCCESSFUL;
}

int RebootAll(int try_count)
{
	if (PowerOffAll(100) == EXIT_ERROR) return EXIT_ERROR;
	if (PowerOnAll(100)  == EXIT_ERROR) return EXIT_ERROR;
	return SUCCESSFUL;
}

int ExecuteCommand(string word, vector<pair<string, string>> flag)
{
	int id = commandvoc[word];
	unsigned char c = 0, tmp;
	int ret = 0, ip_status;
	string ip, tmpstr;
	bool fpi = false, fpo = false, fip = false;
	switch(id)
	{
	case EXIT_SUCCESSFUL:
		return EXIT_SUCCESSFUL;
		break;  
	case COMMAND_ALLON:
		if (PowerOnAll(100)  == EXIT_ERROR) cout << "error"  << endl;
		break;
	case COMMAND_ALLOFF:
		if (PowerOffAll(100) == EXIT_ERROR) cout << "error" << endl;
		break;
	case COMMAND_REBOOTALL:
		if (RebootAll(100)   == EXIT_ERROR) cout << "error"   << endl;
		break;
	case COMMAND_ON:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "ep")
			{
				int res = PowerOn(std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) cout << "error" << endl;
				else if (res == SUCCESSFUL) cout << std::stoi((*i).second) << " opened" 	<< endl;
				else if (res == ERROR)      cout << std::stoi((*i).second) << " can't open" 	<< endl;
			}
			else if ((*i).first == "p")
			{
				_Power(std::stoi((*i).second), 1);
			}
		}
		break;
	case COMMAND_OFF:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "ep")
			{
				int res = PowerOff(std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) cout << "error" << endl;
				else if (res == SUCCESSFUL) cout << std::stoi((*i).second) << " closed" 	<< endl;
				else if (res == ERROR)      cout << std::stoi((*i).second) << " can't close" 	<< endl;
			}
			else if ((*i).first == "p")
			{
				_Power(std::stoi((*i).second), 0);
			}
		}
		break;
	case COMMAND_REBOOT:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "ep")
			{
				int res = PowerOff(std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) cout << "error" << endl;
				res     = PowerOn (std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) cout << "error" << endl;
				else if (res == SUCCESSFUL) cout << std::stoi((*i).second) << " rebooted" 	<< endl;
				else if (res == ERROR)      cout << std::stoi((*i).second) << " can't reboot" 	<< endl;
			}
			else if ((*i).first == "p")
			{
				_Power(std::stoi((*i).second), 0);
				_Power(std::stoi((*i).second), 1);
			}
		}
		break;
	case COMMAND_STATUS:
		if ((ret = ftdi_read_data(ftdic, &c, 1)) < 0) {
			ftdi_fatal((char*)"unable to read from ftdi device", ret);
		}
		cout << "read data: " << (int)c << endl;
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "pi")
				fpi = true;
			if ((*i).first == "po")
				fpo = true;
			if ((*i).first == "ip")
				fip = true;
		}
		cout << "PowerOn status" << endl;
		for (int i = 0; i < config["rele_on"].size(); i++)
		{
			ip = config["rele_on"][i]["ip"];
			if ((fpi) || (!(fpo || fip))) 
				cout << "pin_in: "  << config["rele_on"][i]["pin_in"]  << endl;
			if ((fpo) || (!(fpi || fip))) 
			{
				tmpstr = config["rele_on"][i]["pin_out"];
				tmp = std::stoi(tmpstr,nullptr,0);
				cout << "pin_out: " << tmpstr << " - " << ((((1 << tmp) & c) == (1 << tmp))?"on!":"off!") << endl;
			}
			if ((fip) || (!(fpo || fpi)))
			{
				cout << "ip: " << ip << " ";
				ip_status = PingHost(ip, 1);
				if 	(ip_status == PING_HOST_AVAILABLE) 	cout << "closed!" << endl;
				else if (ip_status == PING_HOST_NOT_AVAILABLE) 	cout << "opened!" << endl;
				else if (ip_status == PING_HOST_NOT_CHECKED) 	cout << "not checked!" << endl;
			}
		}
		cout << "PowerOff status" << endl;
		for (int i = 0; i < config["rele_off"].size(); i++)
		{
			ip = config["rele_off"][i]["ip"];
			if ((fpi) || (!(fpo || fip))) 
				cout << "pin_in: "  << config["rele_off"][i]["pin_in"]  << endl;
			if ((fpo) || (!(fpi || fip)))
			{
				tmpstr = config["rele_off"][i]["pin_out"];
				tmp = std::stoi(tmpstr,nullptr,0);
				cout << "pin_out: " << tmpstr << " - " << ((((1 << tmp) & c) == (1 << tmp))?"on!":"off!") << endl;
			}
			if ((fip) || (!(fpo || fpi)))
			{
				cout << "ip: " << ip << " ";
				ip_status = PingHost(ip, 1);
				if 	(ip_status == PING_HOST_AVAILABLE) 	cout << "closed!" << endl;
				else if (ip_status == PING_HOST_NOT_AVAILABLE) 	cout << "opened!" << endl;
				else if (ip_status == PING_HOST_NOT_CHECKED) 	cout << "not checked!" << endl;
			}
		}
		break;
	case COMMAND_HELP:
		cout << "/help				help" 						<< endl;
		cout << "/exit				exit" 						<< endl;
		cout << "/allon				sequential activation of relays" 		<< endl;
		cout << "/alloff             		sequential shutdown of relays" 			<< endl;
		cout << "/rebootall			sequential reboot of relays" 			<< endl;
		cout << "/on            -p  <(0-7)> 	turn on the relay at the specified number" 	<< endl;
		cout << "               -ep <(0-7)> 	turn on the relay at the pin number" 		<< endl;
		cout << "/off           -p  <(0-7)> 	turn off the relay at the specified number"	<< endl;
		cout << "               -ep <(0-7)> 	turn off the relay at the pin number" 		<< endl;
		cout << "/reboot        -p  <(0-7)> 	reboot the relay at the specified number" 	<< endl;
		cout << "               -ep <(0-7)> 	reboot the relay at the pin number" 		<< endl;
		cout << "/status             		get status" 					<< endl;
		cout << "               -pi         	dislay pin_in status" 				<< endl;
		cout << "               -po         	dislay pin_out status" 				<< endl;
		cout << "               -ip         	dislay ip status" 				<< endl;
		cout << "/reloadconfig       		reload config" 					<< endl;
		break;
	case COMMAND_RELOADCONFIG:
		LoadConfigs();
		cout << "configs reloaded"   << endl;
		break;
	}
	return SUCCESSFUL;
}

int ParsCommad(string in)
{
	string::iterator i = in.begin();
	string word = "", str = "", tmp;
	vector<pair<string, string>> flag;
	int pos = PARS_BEFORE_READ_COMMAND;
	while(i < in.end())
	{
		switch(pos)
		{
		case PARS_BEFORE_READ_COMMAND:
			if (*i == ' ')      {i++; continue;}
			else if (*i == '/') {i++; pos = PARS_READ_COMMAND; continue;}
			else { cout << "incorrect" << endl; ExecuteCommand("help", vector<pair<string, string>>()); return EXIT_ERROR;}
			break;
		case PARS_READ_COMMAND:
			if ((*i >= 'a')&&(*i <= 'z'))
			{
				word += *i;
				i++;
			}
			if (*i == ' ')
			{
				if (word.size() == 0) { cout << "incorrect" << endl; ExecuteCommand("help", vector<pair<string, string>>()); return EXIT_ERROR;}
				while (*i == ' ')     i++;
				if (*i != '-')        { cout << "incorrect" << endl; ExecuteCommand("help", vector<pair<string, string>>()); return EXIT_ERROR;}
			}
			if (*i == '-') {i++; pos = PARS_READ_FLAG;}
			break;
		case PARS_READ_FLAG:
			if ((*i >= 'a')&&(*i <= 'z'))
			{
				str += *i;
				i++;
			}
			if ((*i == ' ')||(i == in.end()))
			{
				if (str.size() == 0) { cout << "incorrect" << endl; ExecuteCommand("help", vector<pair<string, string>>()); return EXIT_ERROR;}
				while   (*i == ' ')     i++;
				if 	(*i == '-')     {pos = PARS_READ_FLAG; 		flag.push_back(make_pair(str, "")); str = ""; i++; continue;}
				else if (i == in.end()) {pos = PARS_END; 		flag.push_back(make_pair(str, "")); str = "";      continue;}
				else if (*i != ' ')     {pos = PARS_READ_FLAG_VALUE; 	flag.push_back(make_pair(str, "")); str = "";      continue;}
			}
			if ((i == in.end())||(pos == PARS_END)) {			flag.push_back(make_pair(str, "")); str = "";      continue;}
			break;
		case PARS_READ_FLAG_VALUE:
			if ((*i >= '0')&&(*i <= '9'))
			{
				str += *i;
				i++;
			}
			if ((*i == ' ')||(i == in.end()))
			{
				if (str.size() == 0) { cout << "incorrect" << endl; ExecuteCommand("help", vector<pair<string, string>>()); return EXIT_ERROR;}
				while   (*i == ' ') i++;
				if 	(*i == '-') 
				{
					pos = PARS_READ_FLAG; 
					tmp = flag.back().first;
					flag.pop_back();
					flag.push_back(make_pair(tmp, str));
					str = ""; i++; continue;
				}
				else if ((i == in.end())||(*i != ' ')) 
				{
					pos = PARS_END; 
					tmp = flag.back().first;
					flag.pop_back();
					flag.push_back(make_pair(tmp, str));
					str = ""; continue;
				}
			}
			break;
		}
		if (pos == PARS_END) break;
		
	}
	return ExecuteCommand(word, flag);
}

int InitCommandLine()
{ 
	commandvoc.insert(make_pair("exit", 		EXIT_SUCCESSFUL));
	commandvoc.insert(make_pair("allon",		COMMAND_ALLON));
	commandvoc.insert(make_pair("alloff", 		COMMAND_ALLOFF));
	commandvoc.insert(make_pair("rebootall",	COMMAND_REBOOTALL));
	commandvoc.insert(make_pair("on", 		COMMAND_ON));
	commandvoc.insert(make_pair("off", 		COMMAND_OFF));
	commandvoc.insert(make_pair("reboot", 		COMMAND_REBOOT));
	commandvoc.insert(make_pair("status", 		COMMAND_STATUS));
	commandvoc.insert(make_pair("help", 		COMMAND_HELP));
	commandvoc.insert(make_pair("reloadconfig", 	COMMAND_RELOADCONFIG));
	return SUCCESSFUL;
}

int CommandLine(int argc, char *argv[])
{
	if (argc > 1) //when the program is started with parameters, will be executed once
	{
		char *arg = argv[1];
		arg++;
		string word(arg), str1, str2;
		if (argc == 2)
			ExecuteCommand(word, vector<pair<string, string>>());
		else
		{
			vector<pair<string, string>> flag;
			int i = 2;
			while (i < argc)
			{
				str1 = ""; str2 = "";
				if (argv[i][0] == '-')
				{
					arg = argv[i];
					arg++;
					str1 = arg;
				}
				i++;
				if ((i < argc)&&(argv[i][0] != '-'))
				{
					str2 = argv[i];
					i++;
				}
				flag.push_back(make_pair(str1, str2));
			}
			ExecuteCommand(word, flag);
		}
		
	}
	else //when the program is started without parameters
	{
		string line;
		while(true)
		{
			cout << "\x1b[1;32m>>>\x1b[0m ";
			std::getline(std::cin, line);
			if (ParsCommad(line) == EXIT_SUCCESSFUL) break;
		}
	}
	return SUCCESSFUL;
}

int main(int argc, char *argv[])
{
	InitFTDI();
	LoadConfigs();
	InitCommandLine();
	CommandLine(argc, argv);
	FreeFTDI();
	cout << "Done!" << endl;
	return SUCCESSFUL;
}
