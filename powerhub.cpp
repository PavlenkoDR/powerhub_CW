#include "powerhub.hpp"

struct ftdi_context *ftdic;
std::map<std::string, int> commandvoc;
json config;
std::vector<std::pair<std::string, Adafruit_ADS1115*> >ads;
std::string fconfig;
std::streambuf* backup;
std::ofstream logFile;

void ftdi_fatal (char *str, int ret)
{
    	fprintf (stderr, "%s: %d (%s)\n", str, ret, ftdi_get_error_string (ftdic));
    	ftdi_free(ftdic);
    	exit (1);
}

int LoadConfigs(std::string fin)
{	
	fconfig = fin;
	std::ifstream in;
	in.open(fconfig);
	in >> config;
	in.close();
	logFile.open("logs/log.txt", std::ios::app);
	if ((bool)config["logs"]) std::cout.rdbuf(logFile.rdbuf());
	else			  std::cout.rdbuf(backup);
	return SUCCESSFUL;
}

int InitFTDI()
{
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
	return SUCCESSFUL;
}

int PingHost(std::string host, int try_count)
{
	if (!config["check_ip"]) return PING_HOST_NOT_CHECKED;
	std::string str;
	FILE *in;
	char buff[512];
	std::ofstream out;
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
	std::cout << "pin " << (int)pin << " - " << std::endl;
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
	int ip_status;
	std::string rele;
	if (e) rele = "rele_on";
	else   rele = "rele_off";
	std::string ip = config[rele][n]["ip"];
	std::string str = config[rele][n]["pin_out"];
	unsigned char pin = std::stoi(str,nullptr,0);
	std::cout << n << ": ";
	_Power(pin, e);
	for (int i = 0; i < try_count; i++)
	{
		ip_status = PingHost(ip, 1);
		std::cout << "IP " << ip << " ";
		if 	(ip_status == PING_HOST_AVAILABLE) 	std::cout << "closed!"      << std::endl;
		else if (ip_status == PING_HOST_NOT_AVAILABLE) 	std::cout << "opened!"      << std::endl;
		else if (ip_status == PING_HOST_NOT_CHECKED) 	std::cout << "not checked!" << std::endl;
		if ((ip_status == e)||(ip_status == PING_HOST_NOT_CHECKED)) return SUCCESSFUL;
		std::cout << i << ": try again" << std::endl;
	}
	return ERROR;
}

int PowerOn(int n, int try_count)
{
	return Power(n, try_count, 1);
}

int PowerOnAll(int try_count)
{
	for (int i = 0; i < (int)config["rele_on"].size(); i++)
		if (PowerOn(i, try_count) == SUCCESSFUL)
			continue;
		else
		{
			std::cout << "Can't open " << i << std::endl;
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
	for (int i = 0; i < (int)config["rele_off"].size(); i++)
		if (PowerOff(i, try_count) == SUCCESSFUL)
			continue;
		else
		{
			std::cout << "Can't close " << i << std::endl;
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

std::vector<int> CheckHallSensor(std::string i2caddress, int pin, int n)
{
	std::vector<int> res;
	std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator adsfi;
	for (std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator j = ads.begin(); j != ads.end(); j++)
		if (j->first == i2caddress) {adsfi = j; break;}
  	adsfi->second->startComparator_SingleEnded(pin, 1000);
	for (int i = 0; i < n; i++)
	{
		res.push_back(adsfi->second->getLastConversionResults());
		usleep(1000);
	}
	return res;
}

int ExecuteCommand(std::string word, std::vector<std::pair<std::string, std::string>> flag)
{
	int id = commandvoc[word];
	unsigned char c = 0, tmp;
	int ret = 0, ip_status;
	std::string ip, tmpstr;
	bool fpo = false, fip = false, fi2c = false;
	bool fi2cmed = false, fi2cmax = false, fi2cmin = false, fi2cav = false, fi2clist = false;
	switch(id)
	{
	case EXIT_SUCCESSFUL:
		return EXIT_SUCCESSFUL;
		break;  
	case COMMAND_ALLON:
		if (PowerOnAll (100) == EXIT_ERROR) std::cout << "error" << std::endl;
		return COMMAND_ALLON;
		break;
	case COMMAND_ALLOFF:
		if (PowerOffAll(100) == EXIT_ERROR) std::cout << "error" << std::endl;
		return COMMAND_ALLOFF;
		break;
	case COMMAND_REBOOTALL:
		if (RebootAll  (100) == EXIT_ERROR) std::cout << "error" << std::endl;
		return COMMAND_REBOOTALL;
		break;
	case COMMAND_ON:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "ep")
			{
				int res = PowerOn(std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) std::cout << "error" << std::endl;
				else if (res == SUCCESSFUL) std::cout << std::stoi((*i).second) << " opened" 	 << std::endl;
				else if (res == ERROR)      std::cout << std::stoi((*i).second) << " can't open" << std::endl;
			}
			else if ((*i).first == "p")
			{
				_Power(std::stoi((*i).second), 1);
			}
		}
		return COMMAND_ON;
		break;
	case COMMAND_OFF:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "ep")
			{
				int res = PowerOff(std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) std::cout << "error" << std::endl;
				else if (res == SUCCESSFUL) std::cout << std::stoi((*i).second) << " closed" 	  << std::endl;
				else if (res == ERROR)      std::cout << std::stoi((*i).second) << " can't close" << std::endl;
			}
			else if ((*i).first == "p")
			{
				_Power(std::stoi((*i).second), 0);
			}
		}
		return COMMAND_OFF;
		break;
	case COMMAND_REBOOT:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "ep")
			{
				int res = PowerOff(std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) std::cout << "error" << std::endl;
				res     = PowerOn (std::stoi((*i).second), 100);
				if 	(res == EXIT_ERROR) std::cout << "error" << std::endl;
				else if (res == SUCCESSFUL) std::cout << std::stoi((*i).second) << " rebooted" 	   << std::endl;
				else if (res == ERROR)      std::cout << std::stoi((*i).second) << " can't reboot" << std::endl;
			}
			else if ((*i).first == "p")
			{
				_Power(std::stoi((*i).second), 0);
				_Power(std::stoi((*i).second), 1);
			}
		}
		return COMMAND_REBOOT;
		break;
	case COMMAND_STATUS:
		if ((ret = ftdi_read_data(ftdic, &c, 1)) < 0) {
			ftdi_fatal((char*)"unable to read from ftdi device", ret);
		}
		std::cout << "read data: " << (int)c << std::endl;
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			//std::cout << (*i).first << std::endl;
			if ((*i).first == "po")
				fpo = true;
			if ((*i).first == "ip")
				fip = true;
			if ((*i).first == "i2c")
				fi2c = true;
			if ((*i).first == "i2cmed")
				fi2cmed = true;
			if ((*i).first == "i2cmax")
				fi2cmax = true;
			if ((*i).first == "i2cmin")
				fi2cmin = true;
			if ((*i).first == "i2cav")
				fi2cav = true;
			if ((*i).first == "i2clist")
				fi2cav = true;
			fi2c = fi2cmed | fi2cmax | fi2cmin | fi2cav | fi2c | fi2clist;
		}
		if ((fpo || fip)||(!(fpo | fip) && !fi2c))
		{
			std::cout << "PowerOn status" << std::endl;
			for (int i = 0; i < (int)config["rele_on"].size(); i++)
			{
				ip = config["rele_on"][i]["ip"];
				if (fpo || (!fip)) 
				{
					tmpstr = config["rele_on"][i]["pin_out"];
					tmp = std::stoi(tmpstr,nullptr,0);
					std::cout << "pin_out: " << tmpstr << " - " << ((((1 << tmp) & c) == (1 << tmp))?"on!":"off!") << std::endl;
				}
				if (fip || (!fpo))
				{
					std::cout << "ip: " << ip << " ";
					ip_status = PingHost(ip, 1);
					if 	(ip_status == PING_HOST_AVAILABLE) 	std::cout << "closed!" << std::endl;
					else if (ip_status == PING_HOST_NOT_AVAILABLE) 	std::cout << "opened!" << std::endl;
					else if (ip_status == PING_HOST_NOT_CHECKED) 	std::cout << "not checked!" << std::endl;
				}
			}
			std::cout << "PowerOff status" << std::endl;
			for (int i = 0; i < (int)config["rele_off"].size(); i++)
			{
				ip = config["rele_off"][i]["ip"];
				if (fpo || (!fip))
				{
					tmpstr = config["rele_off"][i]["pin_out"];
					tmp = std::stoi(tmpstr,nullptr,0);
					std::cout << "pin_out: " << tmpstr << " - " << ((((1 << tmp) & c) == (1 << tmp))?"on!":"off!") << std::endl;
				}
				if (fip || (!fpo))
				{
					std::cout << "ip: " << ip << " ";
					ip_status = PingHost(ip, 1);
					if 	(ip_status == PING_HOST_AVAILABLE) 	std::cout << "closed!" << std::endl;
					else if (ip_status == PING_HOST_NOT_AVAILABLE) 	std::cout << "opened!" << std::endl;
					else if (ip_status == PING_HOST_NOT_CHECKED) 	std::cout << "not checked!" << std::endl;
				}
			}
		}
		if ((fi2c)||(!(fpo || fip ||fi2c)))
		{
			bool fi2cflag = !(fi2cmed | fi2cmax | fi2cmin | fi2cav | fi2clist);
			std::cout << "I2CBus status" << std::endl;
			for (int i = 0; i < (int)config["i2cbus"].size(); i++)
			{
				for (int j = 0; j < (int)config["i2cbus"][i]["pins"].size(); j++)
				{
					std::vector<int> tmppinvec = CheckHallSensor(config["i2cbus"][i]["module_address"], 
											config["i2cbus"][i]["pins"][j]["pin"], 
											1000);
					std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator adsfi;
					for (std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator j = ads.begin(); j != ads.end(); j++)
						if (j->first == config["i2cbus"][i]["module_address"]) {adsfi = j; break;}
					double sum = 0;
					std::cout << "=============== pin " << config["i2cbus"][i]["pins"][j]["pin"] << "===============" << std::endl;
					std::cout << "N | Hex | Dec | V " << std::endl;
					if (fi2clist || fi2cflag) for (int k = 0; k < (int)tmppinvec.size(); k++)
					{
						std::cout << std::dec << k 								<< " | ";
						std::cout << std::hex << tmppinvec[k] 							<< " | ";
						std::cout << std::dec << tmppinvec[k] 							<< " | ";
						std::cout << std::dec << tmppinvec[k]*adsfi->second->VPS 				<< std::endl;
					}
					std::cout << std::endl;
					std::sort (tmppinvec.begin(), tmppinvec.end());
					if (fi2cmin || fi2cflag)
					{
						std::cout << "Min: " << "-" 								<< " | ";
						std::cout << 		std::hex << tmppinvec[0] 					<< " | ";
						std::cout << 		std::dec << tmppinvec[0] 					<< " | ";
						std::cout << 		std::dec << tmppinvec[0]*adsfi->second->VPS 			<< std::endl;
					}
					if (fi2cmax || fi2cflag)
					{
						std::cout << "Max: " << "-" 								<< " | ";
						std::cout << 		std::hex << tmppinvec[tmppinvec.size()-1] 			<< " | ";
						std::cout << 		std::dec << tmppinvec[tmppinvec.size()-1] 			<< " | ";
						std::cout << 		std::dec << tmppinvec[tmppinvec.size()-1]*adsfi->second->VPS	<< std::endl;
					}
					if (fi2cmed || fi2cflag)
					{
						std::cout << "Med: " << "-" 								<< " | ";
						std::cout << 		std::hex << tmppinvec[tmppinvec.size()-1] 			<< " | ";
						std::cout << 		std::dec << tmppinvec[tmppinvec.size()-1] 			<< " | ";
						std::cout << 		std::dec << tmppinvec[tmppinvec.size()-1]*adsfi->second->VPS	<< std::endl;
					}
					if (fi2cav || fi2cflag)
					{
						for (int k = 0; k < (int)tmppinvec.size(); k++)
							sum+=tmppinvec[k];
						sum/=tmppinvec.size();
						std::cout << "Average: " << "-" 							<< " | ";
						std::cout << 		std::hex << (int)round(sum) 					<< " | ";
						std::cout << 		std::dec << (int)round(sum)  					<< " | ";
						std::cout << 		std::dec << sum*adsfi->second->VPS 				<< std::endl;
					}
				}
			}
		}
		return COMMAND_STATUS;
		break;
	case COMMAND_HELP:
		std::cout << "help				| help" 					<< std::endl;
		std::cout << "exit				| exit" 					<< std::endl;
		std::cout << "allon				| sequential activation of relays" 		<< std::endl;
		std::cout << "alloff             		| sequential shutdown of relays" 		<< std::endl;
		std::cout << "rebootall				| sequential reboot of relays" 			<< std::endl;
		std::cout << "on		-p  <(0-7)> 	| turn on the relay at the specified number" 	<< std::endl;
		std::cout << "              	-ep <(0-7)> 	| turn on the relay at the pin number" 		<< std::endl;
		std::cout << "off           	-p  <(0-7)> 	| turn off the relay at the specified number"	<< std::endl;
		std::cout << "              	-ep <(0-7)> 	| turn off the relay at the pin number" 	<< std::endl;
		std::cout << "reboot        	-p  <(0-7)> 	| reboot the relay at the specified number" 	<< std::endl;
		std::cout << "               	-ep <(0-7)> 	| reboot the relay at the pin number" 		<< std::endl;
		std::cout << "status             		| get status" 					<< std::endl;
		std::cout << "               	-po         	| dislay pin_out status" 			<< std::endl;
		std::cout << "               	-ip         	| dislay ip status" 				<< std::endl;
		std::cout << "               	-i2c         	| dislay i2c status" 				<< std::endl;
		std::cout << "               	-fi2cmed	| dislay i2c median status" 			<< std::endl;
		std::cout << "               	-fi2cmax        | dislay i2c max status" 			<< std::endl;
		std::cout << "               	-fi2cmin        | dislay i2c min status" 			<< std::endl;
		std::cout << "               	-fi2cav         | dislay i2c average status" 			<< std::endl;
		std::cout << "               	-fi2clist       | dislay i2c list for smth time status" 	<< std::endl;
		std::cout << "reloadconfig       		| reload config" 				<< std::endl;
		return COMMAND_HELP;
		break;
	case COMMAND_RELOADCONFIG:
		LoadConfigs(fconfig);
		std::cout << "configs reloaded"   << std::endl;
		return COMMAND_RELOADCONFIG;
		break;
	}
	return SUCCESSFUL;
}

int ParsCommand(std::string in)
{
	std::string::iterator i = in.begin();
	std::string word = "", str = "", tmp;
	std::vector<std::pair<std::string, std::string>> flag;
	int pos = PARS_BEFORE_READ_COMMAND;
	while(i < in.end())
	{
		switch(pos)
		{
		case PARS_BEFORE_READ_COMMAND:
			if (*i == ' ')      {i++; continue;}
			else if (*i != ' ') {pos = PARS_READ_COMMAND; continue;}
			break;
		case PARS_READ_COMMAND:
			if (((*i >= 'a')&&(*i <= 'z'))||((*i >= '0')&&(*i <= '9')))
			{
				word += *i;
				i++;
			}
			if (*i == ' ')
			{
				if (word.size() == 0) 
				{ 
					std::cout << "incorrect" << std::endl; 
					ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
					return EXIT_ERROR;
				}
				while (*i == ' ') i++;
				if (*i != '-')        
				{ 
					std::cout << "incorrect" << std::endl; 
					ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
					return EXIT_ERROR;
				}
			}
			if (*i == '-') {i++; pos = PARS_READ_FLAG;}
			break;
		case PARS_READ_FLAG:
			if (((*i >= 'a')&&(*i <= 'z'))||((*i >= '0')&&(*i <= '9')))
			{
				str += *i;
				i++;
			}
			if ((*i == ' ')||(i == in.end()))
			{
				if (str.size() == 0) 
				{ 
					std::cout << "incorrect" << std::endl; 
					ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
					return EXIT_ERROR;
				}
				while   (*i == ' ')     i++;
				if 	(*i == '-')     {pos = PARS_READ_FLAG; 		flag.push_back(std::make_pair(str, "")); str = ""; i++; continue;}
				else if (i == in.end()) {pos = PARS_END; 		flag.push_back(std::make_pair(str, "")); str = "";      continue;}
				else if (*i != ' ')     {pos = PARS_READ_FLAG_VALUE; 	flag.push_back(std::make_pair(str, "")); str = "";      continue;}
			}
			if ((i == in.end())||(pos == PARS_END)) {			flag.push_back(std::make_pair(str, "")); str = "";      continue;}
			break;
		case PARS_READ_FLAG_VALUE:
			if ((*i >= '0')&&(*i <= '9'))
			{
				str += *i;
				i++;
			}
			if ((*i == ' ')||(i == in.end()))
			{
				if (str.size() == 0) 
				{ 
					std::cout << "incorrect" << std::endl; 
					ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
					return EXIT_ERROR;
				}
				while   (*i == ' ') i++;
				if 	(*i == '-') 
				{
					pos = PARS_READ_FLAG; 
					tmp = flag.back().first;
					flag.pop_back();
					flag.push_back(std::make_pair(tmp, str));
					str = ""; i++; continue;
				}
				else if ((i == in.end())||(*i != ' ')) 
				{
					pos = PARS_END; 
					tmp = flag.back().first;
					flag.pop_back();
					flag.push_back(std::make_pair(tmp, str));
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
	commandvoc.insert(std::make_pair("exit", 		EXIT_SUCCESSFUL));
	commandvoc.insert(std::make_pair("allon",		COMMAND_ALLON));
	commandvoc.insert(std::make_pair("alloff", 		COMMAND_ALLOFF));
	commandvoc.insert(std::make_pair("rebootall",		COMMAND_REBOOTALL));
	commandvoc.insert(std::make_pair("on", 			COMMAND_ON));
	commandvoc.insert(std::make_pair("off", 		COMMAND_OFF));
	commandvoc.insert(std::make_pair("reboot", 		COMMAND_REBOOT));
	commandvoc.insert(std::make_pair("status", 		COMMAND_STATUS));
	commandvoc.insert(std::make_pair("help", 		COMMAND_HELP));
	commandvoc.insert(std::make_pair("reloadconfig", 	COMMAND_RELOADCONFIG));
	return SUCCESSFUL;
}

int ExecuteArgcArgv(int argc, char *argv[])
{
	char *arg = argv[1];
	arg++;
	std::string word(arg), str1, str2;
	std::vector<std::pair<std::string, std::string>> flag;
	if (argc > 2)
	{
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
			flag.push_back(std::make_pair(str1, str2));
		}
	}
	return ExecuteCommand(word, flag);
}

int CommandLine(int argc, char *argv[])
{
	if (argc > 1) //when the program is started with parameters, will be executed once
	{
		ExecuteArgcArgv(argc, argv);
	}
	else //when the program is started without parameters
	{
		std::string line;
		while(true)
		{
			printf("\x1b[1;32m>>>\x1b[0m ");
			std::getline(std::cin, line);
			if (ParsCommand(line) == EXIT_SUCCESSFUL) break;
			std::cout << std::endl;
		}
	}
	return SUCCESSFUL;
}

int InitI2CBus()
{
	std::string tmpstr;
	std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator adsfi;
	for (int i = 0; i < (int)config["i2cbus"].size(); i++)
	{
		tmpstr = config["i2cbus"][i]["module_address"];
		ads.push_back(std::make_pair(tmpstr, new Adafruit_ADS1115((int)std::stoi(tmpstr,nullptr,0))));
		/*
		"****************************************************************************",
		"GAIN_TWOTHIRDS - 2/3x gain   +/- 6.144V  1 bit = 3mV      0.1875mV (default)",
		"GAIN_ONE       -   1x gain   +/- 4.096V  1 bit = 2mV      0.125mV"           ,
		"GAIN_TWO       -   2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV"          ,
		"GAIN_FOUR      -   4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV"         ,
		"GAIN_EIGHT     -   8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV"        ,
		"GAIN_SIXTEEN   -   16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV"       ,
		"****************************************************************************",
		*/
		for (std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator j = ads.begin(); j != ads.end(); j++)
			if (j->first == tmpstr) {adsfi = j; break;}
		if      (config["i2cbus"][i]["volts"] == "GAIN_TWOTHIRDS")
  			adsfi->second->setGain(GAIN_TWOTHIRDS);
		else if (config["i2cbus"][i]["volts"] == "GAIN_ONE")
  			adsfi->second->setGain(GAIN_ONE);
		else if (config["i2cbus"][i]["volts"] == "GAIN_TWO")
  			adsfi->second->setGain(GAIN_TWO);
		else if (config["i2cbus"][i]["volts"] == "GAIN_FOUR")
  			adsfi->second->setGain(GAIN_FOUR);
		else if (config["i2cbus"][i]["volts"] == "GAIN_EIGHT")
  			adsfi->second->setGain(GAIN_EIGHT);
		else if (config["i2cbus"][i]["volts"] == "GAIN_SIXTEEN")
  			adsfi->second->setGain(GAIN_SIXTEEN);
		//*/
		double med;
	  	adsfi->second->startComparator_SingleEnded(0, 1000);
		for (int i = 0; i < 1000; i++)
		{
			med += adsfi->second->getLastConversionResults();
			usleep( 1000 );
		}
		med = (med*adsfi->second->VPS)/1000;
	}
	return SUCCESSFUL;
}

int InitPowerHub(std::string str)
{
	backup=std::cout.rdbuf();
	InitFTDI();
	LoadConfigs(str);
	InitCommandLine();
	InitI2CBus();
	return SUCCESSFUL;
}

int FreePowerHub()
{
	FreeFTDI();
	FreeI2CBus();
	return SUCCESSFUL;
}

int FreeI2CBus(){ads.clear();return SUCCESSFUL;}

