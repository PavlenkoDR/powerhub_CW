#include "powerhub.hpp"

struct ftdi_context *ftdic;
std::map<std::string, int> commandvoc;
json config;
std::vector<std::pair<std::string, Adafruit_ADS1115*> >ads;
std::string fconfig;

void ftdi_fatal (char *str, int ret)
{
    fprintf (stderr, "%s: %d (%s)\n",
             str, ret, ftdi_get_error_string (ftdic));
    ftdi_free(ftdic);
    exit (1);
}

int LoadConfigs(std::string fin)
{	
	//"еach relay corresponds to the index 0-7"
	//"ip       - server each to be checked"
	//"pin_out  - rele adress"
	//"pin_in   - In the future, this will be the address of the Hall sensors" - not supported
	//"check_ip - check ip address? True - yes, false - no"
	//"rele_on  - Parameters when the relay is turn on"
	//"rele_off - Parameters when the relay is turn off"
	fconfig = fin;
	std::ifstream in;
	in.open(fconfig);
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
	int ret;
	int ip_status;
	std::string rele;
	if (e) rele = "rele_on";
	else   rele = "rele_off";
	std::string ip = config[rele][n]["ip"];
	std::string str = config[rele][n]["pin_out"];
	unsigned char pin = std::stoi(str,nullptr,0), c = 0;
	std::cout << n << ": ";
	_Power(pin, e);
	//if (PingHost(config[rele][n]["ip"], 1)) return 0;
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
	for (int i = 0; i < config["rele_on"].size(); i++)
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
	for (int i = 0; i < config["rele_off"].size(); i++)
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

double CheckHallSensorVPS(std::string i2caddress, int pin, int n)
{
	double med;
/*
	000: AINP = AIN0 и AINN = AIN1 100: AINP = AIN0 и AINN = GND
	001: AINP = AIN0 и AINN = AIN3 101: AINP = AIN1 и AINN = GND
	010: AINP = AIN1 и AINN = AIN3 110: AINP = AIN2 и AINN = GND
	011: AINP = AIN2 и AINN = AIN3 111: AINP = AIN3 и AINN = GND
*/
	std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator adsfi;
	for (std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator j = ads.begin(); j != ads.end(); j++)
		if (j->first == i2caddress) {adsfi = j; break;}
  	adsfi->second->startComparator_SingleEnded(pin, 1000);
	for (int i = 0; i < n; i++)
	{
		med += adsfi->second->getLastConversionResults();
		usleep( 1000 );
	}
	return (med*adsfi->second->VPS)/n;
}

int ExecuteCommand(std::string word, std::vector<std::pair<std::string, std::string>> flag)
{
	int id = commandvoc[word];
	unsigned char c = 0, tmp;
	int ret = 0, ip_status;
	std::string ip, tmpstr;
	bool fpi = false, fpo = false, fip = false, fi2c = false;
	switch(id)
	{
	case EXIT_SUCCESSFUL:
		return EXIT_SUCCESSFUL;
		break;  
	case COMMAND_ALLON:
		if (PowerOnAll(100)  == EXIT_ERROR) std::cout << "error"  << std::endl;
		break;
	case COMMAND_ALLOFF:
		if (PowerOffAll(100) == EXIT_ERROR) std::cout << "error" << std::endl;
		break;
	case COMMAND_REBOOTALL:
		if (RebootAll(100)   == EXIT_ERROR) std::cout << "error"   << std::endl;
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
		break;
	case COMMAND_STATUS:
		if ((ret = ftdi_read_data(ftdic, &c, 1)) < 0) {
			ftdi_fatal((char*)"unable to read from ftdi device", ret);
		}
		std::cout << "read data: " << (int)c << std::endl;
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "pi")
				fpi = true;
			if ((*i).first == "po")
				fpo = true;
			if ((*i).first == "ip")
				fip = true;
			if ((*i).first == "i2c")
				fi2c = true;
		}
		if ((fpo || fpi || fip)||(!(fpo | fpi | fip) && !fi2c))
		{
			std::cout << "PowerOn status" << std::endl;
			for (int i = 0; i < config["rele_on"].size(); i++)
			{
				ip = config["rele_on"][i]["ip"];
				//if ((fpi) || (!(fpo || fip))) 
				//	std::cout << "pin_in: "  << config["rele_on"][i]["pin_in"]  << std::endl;
				if ((fpo) || (!(fpi || fip))) 
				{
					tmpstr = config["rele_on"][i]["pin_out"];
					tmp = std::stoi(tmpstr,nullptr,0);
					std::cout << "pin_out: " << tmpstr << " - " << ((((1 << tmp) & c) == (1 << tmp))?"on!":"off!") << std::endl;
				}
				if ((fip) || (!(fpo || fpi)))
				{
					std::cout << "ip: " << ip << " ";
					ip_status = PingHost(ip, 1);
					if 	(ip_status == PING_HOST_AVAILABLE) 	std::cout << "closed!" << std::endl;
					else if (ip_status == PING_HOST_NOT_AVAILABLE) 	std::cout << "opened!" << std::endl;
					else if (ip_status == PING_HOST_NOT_CHECKED) 	std::cout << "not checked!" << std::endl;
				}
			}
			std::cout << "PowerOff status" << std::endl;
			for (int i = 0; i < config["rele_off"].size(); i++)
			{
				ip = config["rele_off"][i]["ip"];
				//if ((fpi) || (!(fpo || fip))) 
				//	std::cout << "pin_in: "  << config["rele_off"][i]["pin_in"]  << std::endl;
				if ((fpo) || (!(fpi || fip)))
				{
					tmpstr = config["rele_off"][i]["pin_out"];
					tmp = std::stoi(tmpstr,nullptr,0);
					std::cout << "pin_out: " << tmpstr << " - " << ((((1 << tmp) & c) == (1 << tmp))?"on!":"off!") << std::endl;
				}
				if ((fip) || (!(fpo || fpi)))
				{
					std::cout << "ip: " << ip << " ";
					ip_status = PingHost(ip, 1);
					if 	(ip_status == PING_HOST_AVAILABLE) 	std::cout << "closed!" << std::endl;
					else if (ip_status == PING_HOST_NOT_AVAILABLE) 	std::cout << "opened!" << std::endl;
					else if (ip_status == PING_HOST_NOT_CHECKED) 	std::cout << "not checked!" << std::endl;
				}
			}
		}
		if ((fi2c)||(!(fpo || fpi || fip ||fi2c)))
		{
			std::cout << "I2CBus status" << std::endl;
			for (int i = 0; i < config["i2cbus"].size(); i++)
			{
				for (int j = 0; j < config["i2cbus"][i]["pins"].size(); j++)
				{
					std::cout << "\tpin " << config["i2cbus"][i]["pins"][j]["pin"] << ": ";
					std::cout << CheckHallSensorVPS(config["i2cbus"][i]["module_address"], config["i2cbus"][i]["pins"][j]["pin"], 1000) << "V" << std::endl;
				}
			}
		}
		break;
	case COMMAND_HELP:
		std::cout << "/help				help" 						<< std::endl;
		std::cout << "/exit				exit" 						<< std::endl;
		std::cout << "/allon				sequential activation of relays" 		<< std::endl;
		std::cout << "/alloff             		sequential shutdown of relays" 			<< std::endl;
		std::cout << "/rebootall			sequential reboot of relays" 			<< std::endl;
		std::cout << "/on            -p  <(0-7)> 	turn on the relay at the specified number" 	<< std::endl;
		std::cout << "               -ep <(0-7)> 	turn on the relay at the pin number" 		<< std::endl;
		std::cout << "/off           -p  <(0-7)> 	turn off the relay at the specified number"	<< std::endl;
		std::cout << "               -ep <(0-7)> 	turn off the relay at the pin number" 		<< std::endl;
		std::cout << "/reboot        -p  <(0-7)> 	reboot the relay at the specified number" 	<< std::endl;
		std::cout << "               -ep <(0-7)> 	reboot the relay at the pin number" 		<< std::endl;
		std::cout << "/status             		get status" 					<< std::endl;
		//std::cout << "               -pi         	dislay pin_in status" 				<< std::endl;
		std::cout << "               -po         	dislay pin_out status" 				<< std::endl;
		std::cout << "               -ip         	dislay ip status" 				<< std::endl;
		std::cout << "/reloadconfig       		reload config" 					<< std::endl;
		break;
	case COMMAND_RELOADCONFIG:
		LoadConfigs(fconfig);
		std::cout << "configs reloaded"   << std::endl;
		break;
	}
	return SUCCESSFUL;
}

int ParsCommad(std::string in)
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
			else if (*i == '/') {i++; pos = PARS_READ_COMMAND; continue;}
			else { std::cout << "incorrect" << std::endl; ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); return EXIT_ERROR;}
			break;
		case PARS_READ_COMMAND:
			if ((*i >= 'a')&&(*i <= 'z'))
			{
				word += *i;
				i++;
			}
			if (*i == ' ')
			{
				if (word.size() == 0) { std::cout << "incorrect" << std::endl; ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); return EXIT_ERROR;}
				while (*i == ' ')     i++;
				if (*i != '-')        { std::cout << "incorrect" << std::endl; ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); return EXIT_ERROR;}
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
				if (str.size() == 0) { std::cout << "incorrect" << std::endl; ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); return EXIT_ERROR;}
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
				if (str.size() == 0) { std::cout << "incorrect" << std::endl; ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); return EXIT_ERROR;}
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
	commandvoc.insert(std::make_pair("rebootall",	COMMAND_REBOOTALL));
	commandvoc.insert(std::make_pair("on", 		COMMAND_ON));
	commandvoc.insert(std::make_pair("off", 		COMMAND_OFF));
	commandvoc.insert(std::make_pair("reboot", 		COMMAND_REBOOT));
	commandvoc.insert(std::make_pair("status", 		COMMAND_STATUS));
	commandvoc.insert(std::make_pair("help", 		COMMAND_HELP));
	commandvoc.insert(std::make_pair("reloadconfig", 	COMMAND_RELOADCONFIG));
	return SUCCESSFUL;
}

int CommandLine(int argc, char *argv[])
{
	if (argc > 1) //when the program is started with parameters, will be executed once
	{
		char *arg = argv[1];
		arg++;
		std::string word(arg), str1, str2;
		if (argc == 2)
			ExecuteCommand(word, std::vector<std::pair<std::string, std::string>>());
		else
		{
			std::vector<std::pair<std::string, std::string>> flag;
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
			ExecuteCommand(word, flag);
		}
		
	}
	else //when the program is started without parameters
	{
		std::string line;
		while(true)
		{
			std::cout << "\x1b[1;32m>>>\x1b[0m ";
			std::getline(std::cin, line);
			if (ParsCommad(line) == EXIT_SUCCESSFUL) break;
		}
	}
	return SUCCESSFUL;
}

int InitI2CBus()
{
	/*
	Adafruit_ADS1115 ads = Adafruit_ADS1115((int)std::stoi("0x48",nullptr,0));
	ads.setGain(GAIN_ONE);
  	ads.startComparator_SingleEnded(0, 1000);
  	int16_t adc0;
	while(true)
	{
		
  		adc0 = ads.getLastConversionResults();
		std::cout << "AIN0: " << adc0*ads.VPS << std::endl;
	  	usleep(1000);
	}
	*/
	//*
	std::string tmpstr;
	std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator adsfi;
	for (int i = 0; i < config["i2cbus"].size(); i++)
	{
		tmpstr = config["i2cbus"][i]["module_address"];
		//std::cout << "module_address " << tmpstr << " " << (int)std::stoi(tmpstr,nullptr,0) << std::endl;
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
		//std::cout << "volts " << config["i2cbus"][i]["volts"] << std::endl;
		for (std::vector<std::pair<std::string, Adafruit_ADS1115*> >::iterator j = ads.begin(); j != ads.end(); j++)
			if (j->first == tmpstr) {adsfi = j; break;}
		//std::cout << "module_address " << tmpstr << " " << adsfi->first << std::endl;
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
	/*
		000: AINP = AIN0 и AINN = AIN1 100: AINP = AIN0 и AINN = GND
		001: AINP = AIN0 и AINN = AIN3 101: AINP = AIN1 и AINN = GND
		010: AINP = AIN1 и AINN = AIN3 110: AINP = AIN2 и AINN = GND
		011: AINP = AIN2 и AINN = AIN3 111: AINP = AIN3 и AINN = GND
	*/
		//adsfi = ads.find("0x48");
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

int FreeI2CBus(){ads.clear();return SUCCESSFUL;}
