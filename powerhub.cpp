//TODO uninterruptable power source API
//TODO SNMP

#include "powerhub.hpp"

	std::string powerhub::GetDataTime()
	{
		char buffer[80];
		time_t seconds = time(NULL);
		tm* timeinfo = localtime(&seconds);
		char* format = (char*)"%A, %B %d, %Y %I:%M:%S";
		strftime(buffer, 80, format, timeinfo);
		return buffer;
	}
	powerhub::powerhub()
	{
		system("mkdir logs 2> /dev/null");
		logFile.open("logs/log.txt", std::ios::app);
		//InitPowerHub("rele.config");
	}
	powerhub::powerhub(std::string str)
	{		
		system("mkdir logs 2> /dev/null");
		logFile.open("logs/log.txt", std::ios::app);
		//InitPowerHub(str);
	}
	powerhub::~powerhub()
	{
		if ((logsStatus) && (logFile.is_open())) logFile << "Done!\n" << std::endl;
		FreePowerHub();
		logFile.close();
	}

int powerhub::Logs(int msg, std::string str)
{
	while(logsMutex) sleep(1);
	logsMutex = true;
	switch(msg)
	{
	case LOGS_EMPTY_PARAM:
		std::cout << str;
		if ((logsStatus) && (logFile.is_open())) logFile << str << std::flush;
		break;
	case LOGS_DEC:
		std::cout << std::dec << str;
		if ((logsStatus) && (logFile.is_open())) logFile << std::dec << str << std::flush;
		break;
	case LOGS_HEX:
		std::cout << std::hex << str;
		if ((logsStatus) && (logFile.is_open())) logFile << std::hex << str << std::flush;
		break;
	case LOGS_NO_LOG:
		std::cout << str << std::flush;
		break;
	}
	logsMutex = false;
	return SUCCESSFUL;
}

int powerhub::ftdi_fatal (std::string str, int ret)
{
		if (ftdic != 0)
		{
			str += ": " + std::to_string(ret) + "(" + ftdi_get_error_string (ftdic) + ")\n";
			Logs(LOGS_EMPTY_PARAM, str);
			ftdi_free(ftdic);
		}
		FTDIstatus = false;
		return EXIT_FAILURE;
}

int powerhub::LoadConfigs(std::string fin)
{
	fconfig = fin;
	std::ifstream in;
	std::stringstream tmpmessage;\
	in.open(fconfig);
	in >> config;
	in.close();
	logsStatus = false;
	sshControl = false;
	checkHall   = false;
	checkIP    = false;
	work_capacity = false;
	tempControl = false;
	// =========================================== FLAGS ================================================
	try
	{
		if ((bool)config["logs"]) logsStatus = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. Logs are disabled! Set by default: false.\n");
		std::cout << "For setting, specify the parameter \"logs\": false/true" << std::endl;
	}
	if ((logsStatus) && (logFile.is_open())) logFile << GetDataTime() << ": ";
	try
	{
		if ((bool)config["ssh_control"]) sshControl = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. SSH control are disabled! Set by default: false.\n");
		std::cout << "For setting, specify the parameter \"ssh_control\": false/true" << std::endl;
	}
	try
	{
		if ((bool)config["check_ads"])   checkHall = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. ADC control are disabled! Set by default: false.\n");
		std::cout << "For setting, specify the parameter \"check_ads\": false/true" << std::endl;
	}
	try
	{
		if ((bool)config["check_ip"])   checkIP = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. Check IP are disabled! Set by default: false.\n");
		std::cout << "For setting, specify the parameter \"check_ip\": false/true" << std::endl;
	}
	try
	{
		if ((bool)config["temp_control"])   tempControl = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. Temp control are disabled! Set by default: false.\n");
		std::cout << "For setting, specify the parameter \"temp_control\": false/true" << std::endl;
	}
	try
	{
		if ((bool)config["work_capacity"])   work_capacity = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. work_capacity are disabled! Set by default: false.\n");
		std::cout << "For setting, specify the parameter \"work_capacity\": false/true" << std::endl;
	}
	
	int tmp = 0;
	
	// =========================================== RELE ================================================
	
	try
	{
		tmp = config["rele"].size();
		if (tmp == 0) throw SOFT_ERROR;
		rele.init = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. Rele settings not found!\n");
		std::cout << "For setting, specify the parameter:" 	<< std::endl;
		std::cout << "\"rele\":" 				<< std::endl;
		std::cout << "\t[" 					<< std::endl;
		std::cout << "\t{" 					<< std::endl;
		std::cout << "\t\t\"user\"\t\t: \"example\"," 		<< std::endl;
		std::cout << "\t\t\"port\"\t\t: 1234," 			<< std::endl;
		std::cout << "\t\t\"ip\"\t\t: \"12.345.67.890\"," 	<< std::endl;
		std::cout << "\t\t\"pin_out\"\t: \"0x00\"," 		<< std::endl;
		std::cout << "\t\t\"hall_address\"\t: \"0x00\"," 	<< std::endl;
		std::cout << "\t\t\"pin_hall\"\t: \"0x00\"," 		<< std::endl;
		std::cout << "\t\t\"term_address\"\t: \"0x00\"," 	<< std::endl;
		std::cout << "\t\t\"pin_term\"\t: \"0x00\"," 		<< std::endl;
		std::cout << "\t}," 					<< std::endl;
		std::cout << "\t..." 					<< std::endl;
		std::cout << "\t]" 					<< std::endl;
	}
	rele.pinSize = tmp;
	for (int i = 0; i < tmp; i++)
	{
		
		// ================== Rele Pin ==================
		try
		{
			rele.pin[i].pin_out = config["rele"][i]["pin_out"];
			rele.pin[i].rele_pin_init = true;
		}
		catch(...)
		{
			tmpmessage.str("");
			tmpmessage << "Settings are not available. Rele pin address in rele["<<i<<"] settings not found!\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage.str());
			std::cout << "For setting, specify the parameter:" 	<< std::endl;
			std::cout << "\"rele\":" 				<< std::endl;
			std::cout << "\t[" 					<< std::endl;
			std::cout << "\t{" 					<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
			std::cout << "\t\t\"pin_out\"\t: \"0x00\"," 		<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
			continue;
		}
		// ================== SSH ==================
		try
		{
			rele.pin[i].user = config["rele"][i]["user"];
			rele.pin[i].port = config["rele"][i]["port"];
			rele.pin[i].ssh_control = true;
		}
		catch(...)
		{
			tmpmessage.str("");
			tmpmessage << "Settings are not available. User or port in rele["<<i<<"] settings not found!\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage.str());
			std::cout << "For setting, specify the parameter:" 	<< std::endl;
			std::cout << "\"rele\":" 				<< std::endl;
			std::cout << "\t[" 					<< std::endl;
			std::cout << "\t{" 					<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
			std::cout << "\t\t\"user\"\t: \"example\"," 		<< std::endl;
			std::cout << "\t\t\"port\"\t: 1234," 			<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
		}
		
		// ================== IP ==================
		try
		{
			rele.pin[i].ip = config["rele"][i]["ip"];
			rele.pin[i].check_ip = true;
		}
		catch(...)
		{
			tmpmessage.str("");
			tmpmessage << "Settings are not available. IP in rele["<<i<<"] settings not found!\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage.str());
			std::cout << "For setting, specify the parameter:" 	<< std::endl;
			std::cout << "\"rele\":" 				<< std::endl;
			std::cout << "\t[" 					<< std::endl;
			std::cout << "\t{" 					<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
			std::cout << "\t\t\"ip\"\t: \"12.345.67.890\"," 	<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
		}
		
		// ================== HALL ==================
		try
		{
			rele.pin[i].hall_address = config["rele"][i]["hall_address"];
			rele.pin[i].pin_hall = config["rele"][i]["pin_hall"];
			rele.pin[i].check_ads = true;
		}
		catch(...)
		{
			tmpmessage.str("");
			tmpmessage << "Settings are not available. Pin or hall address in rele["<<i<<"] settings not found!\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage.str());
			std::cout << "For setting, specify the parameter:" 	<< std::endl;
			std::cout << "\"rele\":" 				<< std::endl;
			std::cout << "\t[" 					<< std::endl;
			std::cout << "\t{" 					<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
			std::cout << "\t\t\"hall_address\"\t: \"0x00\"," 	<< std::endl;
			std::cout << "\t\t\"pin_hall\"\t: \"0x00\"," 		<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
		}
		
		// ================== TERM ==================
		try
		{
			rele.pin[i].term_address = config["rele"][i]["term_address"];
			rele.pin[i].pin_term = config["rele"][i]["pin_term"];
			rele.pin[i].temp_control = true;
		}
		catch(...)
		{
			tmpmessage.str("");
			tmpmessage << "Settings are not available. Pin or term address in rele["<<i<<"] settings not found!\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage.str());
			std::cout << "For setting, specify the parameter:" 	<< std::endl;
			std::cout << "\"rele\":" 				<< std::endl;
			std::cout << "\t[" 					<< std::endl;
			std::cout << "\t{" 					<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
			std::cout << "\t\t\"term_address\"\t: \"0x00\"," 	<< std::endl;
			std::cout << "\t\t\"pin_term\"\t: \"0x00\"," 		<< std::endl;
			std::cout << "\t\t..." 					<< std::endl;
		}
	}
	tmp = 0;
	try
	{
		tmp = config["rele_on"].size();
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. rele_on settings not found!\n");
		std::cout << "For setting, specify the parameter:" 	<< std::endl;
		std::cout << "\"rele_on\": [0, 1, 2, 3, 4, 5, 6, 7]" 	<< std::endl;
		rele.init = false;
	}
	rele.rele_onSize = tmp;
	for (int i = 0; i < tmp; i++)
	{
		try
		{
			rele.rele_on[i] = config["rele_on"][i];
		}
		catch(...)
		{
			Logs(LOGS_EMPTY_PARAM, "Settings are not available. Invalid input!\n");
			std::cout << "Input data must be integer" 		<< std::endl;
			std::cout << "For setting, specify the parameter:" 	<< std::endl;
			std::cout << "\"rele_on\": [0, 1, 2, 3, 4, 5, 6, 7]" 	<< std::endl;
			rele.init = false;
			break;
		}
	}
	tmp = 0;
	try
	{
		tmp = config["rele_off"].size();
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. rele_off settings not found!\n");
		std::cout << "For setting, specify the parameter:" 	<< std::endl;
		std::cout << "\"rele_off\": [0, 1, 2, 3, 4, 5, 6, 7]" 	<< std::endl;
		rele.init = false;
	}
	rele.rele_offSize = tmp;
	for (int i = 0; i < tmp; i++)
	{
		try
		{
			rele.rele_off[i] = config["rele_off"][i];
		}
		catch(...)
		{
			Logs(LOGS_EMPTY_PARAM, "Settings are not available. Invalid input!\n");
			std::cout << "Input data must be integer" 		<< std::endl;
			std::cout << "For setting, specify the parameter:" 	<< std::endl;
			std::cout << "\"rele_off\": [0, 1, 2, 3, 4, 5, 6, 7]" 	<< std::endl;
			rele.init = false;
			break;
		}
	}
	
	// =========================================== ADS ================================================
	try
	{
		double step = 32768.0;
		std::string tmpstr = config["gain"];
		if 	(tmpstr == "GAIN_TWOTHIRDS") 	{gain = GAIN_TWOTHIRDS; VPS = 6.144 / step;}
		else if (tmpstr == "GAIN_ONE") 		{gain = GAIN_ONE; 	VPS = 4.096 / step;}
		else if (tmpstr == "GAIN_TWO") 		{gain = GAIN_TWO; 	VPS = 2.048 / step;}
		else if (tmpstr == "GAIN_FOUR") 	{gain = GAIN_FOUR; 	VPS = 1.024 / step;}
		else if (tmpstr == "GAIN_EIGHT") 	{gain = GAIN_EIGHT; 	VPS = 0.512 / step;}
		else if (tmpstr == "GAIN_SIXTEEN") 	{gain = GAIN_SIXTEEN; 	VPS = 0.256 / step;}
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. Gain settings not found! Set by default: GAIN_ONE.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"gain\": \"GAIN_ONE\"" 				<< std::endl;
		gain = GAIN_ONE;
	}
	
	// =========================================== TERM_SENSOR ================================================
	try
	{
		ADS_min = config["term_sensor"]["ADS_min"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. ADS_min settings not found! Set by default: 0.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"ADS_min\": 0" 					<< std::endl;
		ADS_min = 0;
	}
	try
	{
		ADS_max = config["term_sensor"]["ADS_max"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. ADS_max settings not found! Set by default: 65535.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"ADS_max\": 65535" 				<< std::endl;
		ADS_max = 65535;
	}
	try
	{
		term_min = config["term_sensor"]["term_min"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_min settings not found! Set by default: -20.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_min\": -20" 				<< std::endl;
		term_min = -20;
	}
	try
	{
		term_max = config["term_sensor"]["term_max"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_max settings not found! Set by default: 100.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_max\": 100" 				<< std::endl;
		term_max = 100;
	}
	try
	{
		term_normal_min = config["term_sensor"]["term_normal_min"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_normal_min settings not found! Set by default: 0.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_normal_min\": 10" 				<< std::endl;
		term_normal_min = 10;
	}
	try
	{
		term_normal_max = config["term_sensor"]["term_normal_max"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_normal_max settings not found! Set by default: 70.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_normal_max\": 70" 				<< std::endl;
		term_normal_max = 70;
	}
	try
	{
		term_critical_min = config["term_sensor"]["term_critical_min"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_critical_min settings not found! Set by default: 0.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_critical_min\": 0" 			<< std::endl;
		term_critical_min = 0;
	}
	try
	{
		term_critical_max = config["term_sensor"]["term_critical_max"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_critical_max settings not found! Set by default: 70.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_critical_max\": 70" 			<< std::endl;
		term_critical_max = 70;
	}
	try
	{
		term_dangerous_min = config["term_sensor"]["term_dangerous_min"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_dangerous_min settings not found! Set by default: 0.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_dangerous_min\": -10" 			<< std::endl;
		term_dangerous_min = -10;
	}
	try
	{
		term_dangerous_max = config["term_sensor"]["term_dangerous_max"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. term_dangerous_max settings not found! Set by default: 70.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"term_dangerous_max\": 70" 			<< std::endl;
		term_dangerous_max = 70;
	}
	try
	{
		check_sleep = config["check_sleep"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. check_sleep settings not found! Set by default: 5.\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"check_sleep\": 5" 				<< std::endl;
		check_sleep = 5;
	}
	try
	{
		email = config["email"];
		emailRead = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. email settings not found!\n");
		std::cout << "For setting, specify the parameter:" 		<< std::endl;
		std::cout << "\"email\": \"email@host.com\"" 			<< std::endl;
		emailRead = false;
	}
	// =========================================== SNMP ================================================
	try
	{
		snmpObj.community = config["snmp"]["snr-ups"]["community"];
		snmpObj.community_init = true;
		snmpObj.init = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. SNMP community settings not found!\n");
		std::cout << "For setting, specify the parameter \"snmp\":{\"snr-ups\":{\"community\" = \"public\" ..." << std::endl;
		snmpObj.init = false;
	}
	try
	{
		snmpObj.ip = config["snmp"]["snr-ups"]["ip"];
		snmpObj.ip_init = true;
		snmpObj.init = true;
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. SNMP ip settings not found!\n");
		std::cout << "For setting, specify the parameter \"snmp\":{\"snr-ups\":{\"ip\" = \"123.123.123.123\" ..." << std::endl;
		snmpObj.init = false;
	}
	try
	{
		pow_critical_min = config["snmp"]["snr-ups"]["pow_critical_min"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. pow_critical_min settings not found! Set by default: 30.\n");
		std::cout << "For setting, specify the parameter \"snmp\":{\"snr-ups\":{\"pow_critical_min\": 30 ..." << std::endl;
		pow_critical_min = 30;
	}
	try
	{
		pow_dangerous_min = config["snmp"]["snr-ups"]["pow_dangerous_min"];
	}
	catch(...)
	{
		Logs(LOGS_EMPTY_PARAM, "Settings are not available. pow_dangerous_min settings not found! Set by default: 15.\n");
		std::cout << "For setting, specify the parameter \"snmp\":{\"snr-ups\":{\"pow_dangerous_min\": 15 ..." << std::endl;
		pow_dangerous_min = 15;
	}
	
	return SUCCESSFUL;
}

int powerhub::InitFTDI()
{
	int ret;
	if ((ftdic = ftdi_new()) == 0) {
		return ftdi_fatal("ftdi_new() failed", 0);
	}
	if ((ret = ftdi_usb_open(ftdic, 0x0403, 0x6001)) < 0) {
		return ftdi_fatal("unable to open ftdi device", ret);
	}
	if ((ret = ftdi_set_bitmode(ftdic, 0xFF, BITMODE_BITBANG)) < 0) {
		return ftdi_fatal("unable to open ftdi device", ret);
	}
	FTDIstatus = true;
	return SUCCESSFUL;
}

int powerhub::InitSSH()
{
	if (sshControl)
	{
		std::string passbuff = "1", repeatpassbuff = "2";
		while (true)
		{
			std::cout << "password for SSH: ";
			std::getline(std::cin, passbuff);// проблемы с кириллицей, WTW?! TODO: исправить эту хуйню
			std::cout << "repeat password for SSH: ";
			std::getline(std::cin, repeatpassbuff);// проблемы с кириллицей, WTW?! TODO: исправить эту хуйню
			if (passbuff == repeatpassbuff) break;
			else
			{
				std::cout << "passwords do not match!" << std::endl;
				passbuff = "1"; repeatpassbuff = "2";
			}
		}
		sshpassword = passbuff;
		std::cout << "\"" << passbuff << "\"" << std::endl;
	}
	return SUCCESSFUL;
}

int powerhub::FreeFTDI()
{
	if (FTDIstatus)
	{
		ftdi_usb_close(ftdic);
		ftdi_free(ftdic);
	}
	return SUCCESSFUL;
}

int powerhub::ReloadFTDI()
{
	if (FTDIstatus)
	{
		FreeFTDI();
		InitFTDI();
	}
	return SUCCESSFUL;
}

int powerhub::PingHost(std::string host, int try_count)
{
	if (!checkIP) return PING_HOST_NOT_CHECKED;
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

int powerhub::PingHostWaitOfState(std::string host, int try_count, int state)
{
	int ip_status;
	for (int i = 0; i < try_count; i++)
	{
		ip_status = PingHost(host, 1);
		Logs(LOGS_EMPTY_PARAM, "IP " + host);
		if 	(ip_status == PING_HOST_AVAILABLE) 	Logs(LOGS_EMPTY_PARAM, " opened!\n"); 
		else if (ip_status == PING_HOST_NOT_AVAILABLE) 	Logs(LOGS_EMPTY_PARAM, " closed!\n"); 	
		else if (ip_status == PING_HOST_NOT_CHECKED) 	Logs(LOGS_EMPTY_PARAM, " not checked!\n");
		if ((ip_status == state)||(ip_status == PING_HOST_NOT_CHECKED)) return PING_HOST_STATUS_CONFIRMED;
		Logs(LOGS_EMPTY_PARAM, "try " + std::to_string(i+1));
		Logs(LOGS_EMPTY_PARAM, ": ");
	}
	return SOFT_ERROR;
}

int powerhub::_Power(unsigned char pin, int e)
{
	if (!FTDIstatus) return HARD_ERROR; 
	int ret;
	unsigned char c = 0;
	Logs(LOGS_EMPTY_PARAM, "pin " + std::to_string((int)pin) + " - ");
	if ((ret = ftdi_read_data(ftdic, &c, 1)) < 0) {
		ftdi_fatal("unable to read from ftdi device", ret);
	}
	c = ( c & (~(1 << pin)) ) | ( e << pin );
	if ((ret = ftdi_write_data(ftdic, &c, 1)) < 0) {
		ftdi_fatal("unable to write into ftdi device", ret);
	}
	ReloadFTDI();
	if (c & ( 1 << pin )) 	Logs(LOGS_EMPTY_PARAM, "on!\n");
	else 			Logs(LOGS_EMPTY_PARAM, "off!\n");
	return SUCCESSFUL;
}

int powerhub::_Reboot(unsigned char pin)
{
	if (!FTDIstatus) return HARD_ERROR; 
	int res;
	res = _Power(pin, 0);
	if (res == SOFT_ERROR) return SOFT_ERROR;
	if (res == HARD_ERROR) return HARD_ERROR;
	res = _Power(pin, 1);
	if (res == SOFT_ERROR) return SOFT_ERROR;
	if (res == HARD_ERROR) return HARD_ERROR;
	return SUCCESSFUL;
}

int powerhub::PowerOn(int n, int try_count)
{
	if (!FTDIstatus) return HARD_ERROR; 
	std::string ip  = rele.pin[n].ip;
	std::string str = rele.pin[n].pin_out;
	unsigned char pin_out = std::stoi(str,nullptr,0);
	if (_Power(pin_out, 1) == HARD_ERROR) return HARD_ERROR;
	if (rele.pin[n].check_ads)
	{
		if (CheckHallSensorStatusWaitOfState(rele.pin[n].hall_address,
						     std::stoi(rele.pin[n].pin_hall,nullptr,0),
						     1000,
						     100,
						     HALL_VPS_ON) == SOFT_ERROR)
			return SOFT_ERROR;
	} 
	else Logs(LOGS_EMPTY_PARAM, " ADC not checked! Rele hall_address or pin_hall invalid!\n");
	
	if (rele.pin[n].check_ip)
	{
		if (PingHostWaitOfState(ip, try_count, PING_HOST_AVAILABLE) == SOFT_ERROR)
			return SOFT_ERROR;
	}
	else Logs(LOGS_EMPTY_PARAM, " IP not checked! Rele IP invalid!\n");
	
	return SUCCESSFUL;
}

int powerhub::PowerOff(int n, int try_count)
{
	if (!FTDIstatus) return HARD_ERROR; 
	std::string ip  = rele.pin[n].ip;
	std::string str = rele.pin[n].pin_out;
	unsigned char pin_out = std::stoi(str,nullptr,0);
	if ((sshControl)&&(rele.pin[n].ssh_control))
	{
		int port = rele.pin[n].port;
		std::string user = rele.pin[n].user;
		system(("sshpass -p " + sshpassword + " ssh -p " + std::to_string(port) + " " + user + "@" + ip + " sudo poweroff").c_str());
	}
	else
	{
		Logs(LOGS_EMPTY_PARAM, "SSH commands are disabled or rele ssh_control initialization invalid!\n");
	}
	
	if (rele.pin[n].check_ip)
	{
		if (PingHostWaitOfState(ip, try_count, PING_HOST_NOT_AVAILABLE) == SOFT_ERROR)
			return SOFT_ERROR;
	}
	else Logs(LOGS_EMPTY_PARAM, "IP not checked! Rele IP invalid!\n");
	
	if (rele.pin[n].check_ads)
	{
		if (CheckHallSensorStatusWaitOfState(rele.pin[n].hall_address,
						     std::stoi(rele.pin[n].pin_hall,nullptr,0),
						     1000,
						     100,
						     HALL_VPS_OFF) == SOFT_ERROR)
			return SOFT_ERROR;
	} 
	else Logs(LOGS_EMPTY_PARAM, "ADC not checked! Rele hall_address or pin_hall initialization invalid!\n");

	if (_Power(pin_out, 0) == HARD_ERROR) return HARD_ERROR;
	return SUCCESSFUL;
}

int powerhub::Reboot(int n, int try_count)
{
	if (!FTDIstatus) return HARD_ERROR; 
	std::string ip  = rele.pin[n].ip;
	std::string str = rele.pin[n].pin_out;
	Logs(LOGS_HEX, n + ": ");
	if ((sshControl)&&(rele.pin[n].ssh_control))
	{
		int port = rele.pin[n].port;
		std::string user = rele.pin[n].user;
		system(("sshpass -p " + sshpassword + " ssh -p " + std::to_string(port) + " " + user + "@" + ip + " sudo reboot").c_str());
	}
	else
	{
		Logs(LOGS_EMPTY_PARAM, "SSH commands are disabled or rele ssh_control initialization invalid!\n");
	}
	
	if (rele.pin[n].check_ip)
	{
		if (PingHostWaitOfState(ip, try_count, PING_HOST_NOT_AVAILABLE) == SOFT_ERROR)
			return SOFT_ERROR;
	}
	else Logs(LOGS_EMPTY_PARAM, " IP not checked! Rele IP invalid!\n");
	
	if (rele.pin[n].check_ads)
	{
		if (CheckHallSensorStatusWaitOfState(rele.pin[n].hall_address,
						     std::stoi(rele.pin[n].pin_hall,nullptr,0),
						     1000,
						     100,
						     HALL_VPS_OFF) == SOFT_ERROR)
			return SOFT_ERROR;
	} 
	else Logs(LOGS_EMPTY_PARAM, " ADC not checked! Rele hall_address or pin_hall initialization invalid!\n");
	
	if (rele.pin[n].check_ads)
	{
		if (CheckHallSensorStatusWaitOfState(rele.pin[n].hall_address,
						     std::stoi(rele.pin[n].pin_hall,nullptr,0),
						     1000,
						     100,
						     HALL_VPS_ON) == SOFT_ERROR)
			return SOFT_ERROR;
	} 
	else Logs(LOGS_EMPTY_PARAM, " ADC not checked! Rele hall_address or pin_hall initialization invalid!\n");
		
	if (rele.pin[n].check_ip)
	{
		if (PingHostWaitOfState(ip, try_count, PING_HOST_AVAILABLE) == SOFT_ERROR)
			return SOFT_ERROR;
	}
	else Logs(LOGS_EMPTY_PARAM, " IP not checked! Rele IP invalid!\n");
	return SUCCESSFUL;
}

int powerhub::PowerOnAll(int try_count)
{
	if (!FTDIstatus) return HARD_ERROR; 
	for (int i = 0; i < rele.rele_onSize; i++)
		if (PowerOn(i, try_count) == SUCCESSFUL)
			Logs(LOGS_EMPTY_PARAM, "\n");
		else
		{
			std::string tmpmessage;
			tmpmessage = "Can't open " + std::to_string(i) + "\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage);
			return SOFT_ERROR;
		}
	return SUCCESSFUL;
}

int powerhub::PowerOffAll(int try_count)
{
	if (!FTDIstatus) return HARD_ERROR; 
	for (int i = 0; i < rele.rele_offSize; i++)
		if (PowerOff(i, try_count) == SUCCESSFUL)
			Logs(LOGS_EMPTY_PARAM, "\n");
		else
		{
			std::string tmpmessage;
			tmpmessage = "Can't close " + std::to_string(i) + "\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage);
			return SOFT_ERROR;
		}
	return SUCCESSFUL;
}

int powerhub::RebootAll(int try_count)
{
	if (!FTDIstatus) return HARD_ERROR; 
	int res;
	res = PowerOffAll(100);
	if (res == SOFT_ERROR) return SOFT_ERROR;
	if (res == HARD_ERROR) return HARD_ERROR;
	res = PowerOnAll(100);
	if (res == SOFT_ERROR) return SOFT_ERROR;
	if (res == HARD_ERROR) return HARD_ERROR;
	return SUCCESSFUL;
}

std::vector<int> powerhub::CheckADS(std::string i2caddress, int pin, int number_metering)
{
	std::vector<int> res;
	Adafruit_ADS1115 ads((int)std::stoi(i2caddress,nullptr,0), gain);
  	ads.startComparator_SingleEnded(pin, 1000);
	for (int i = 0; i < number_metering; i++)
	{
		res.push_back(ads.getLastConversionResults());
		usleep(1000);
	}
	return res;
}

int powerhub::CheckADSGetValue(std::string i2caddress, int pin, int number_metering, int value)
{
	std::vector<int> tmppinvec = CheckADS(i2caddress, pin, number_metering);
	int res = 0;
	switch(value)
	{
	case ADS_VALUE_MIN:
		res = tmppinvec[0];
		for (int i = 1; i < (int)tmppinvec.size(); i++)
			res = tmppinvec[i]<res?tmppinvec[i]:res;
		break;
	case ADS_VALUE_MAX:
		res = tmppinvec[0];
		for (int i = 1; i < (int)tmppinvec.size(); i++)
			res = tmppinvec[i]>res?tmppinvec[i]:res;
		break;
	case ADS_VALUE_AVG:
		for (int i = 0; i < (int)tmppinvec.size(); i++)
			res += tmppinvec[i];
		res /= tmppinvec.size();
		break;
	case ADS_VALUE_MED:
		std::sort (tmppinvec.begin(), tmppinvec.end());
		res = tmppinvec[tmppinvec.size()];
		break;
	}
	return res;
}

double powerhub::CheckThermalSensor(std::string i2caddress, int pin, int number_metering, int value)
{
	double res = CheckADSGetValue(i2caddress, pin, number_metering, value);
	return res/(ADS_max-ADS_min)*(term_max-term_min)-term_min;
}

int powerhub::CheckHallSensorStatus(std::string i2caddress, int pin, int number_metering)
{
	if (!checkHall) return HALL_VPS_NOT_CHECKED;
	std::vector<int> tmppinvec;
	try
	{
		tmppinvec = CheckADS(i2caddress, pin, number_metering);
	}
	catch(...)
	{
		return SOFT_ERROR;
	}
	double sum = 0;
	for (int k = 0; k < (int)tmppinvec.size(); k++) sum+=tmppinvec[k];
	sum/=tmppinvec.size();
	sum*=VPS;
	if (abs(sum - (double)config["hall_sensor"]["VPS_off"]) <= (double)config["hall_sensor"]["eps"]) return HALL_VPS_OFF;
	//if (abs(sum - (double)config["hall_sensor"]["VPS_on"])  <= (double)config["hall_sensor"]["eps"]) return HALL_VPS_ON;
	//return HALL_VPS_UNKNOWN;
	return HALL_VPS_ON;
}


int powerhub::CheckHallSensorStatusWaitOfState(std::string i2caddress, int pin, int number_metering, int try_count, int state)
{
	int hall_status;
	for (int i = 0; i < try_count; i++)
	{
		hall_status = CheckHallSensorStatus(i2caddress, pin, number_metering);
		Logs(LOGS_HEX, i2caddress + " address, "); 
		Logs(LOGS_HEX, std::to_string(pin) + " pin_hall: "); 
		if 	(hall_status == HALL_VPS_ON) 		Logs(LOGS_EMPTY_PARAM, " VPS ON!\n"); 
		else if (hall_status == HALL_VPS_OFF) 		Logs(LOGS_EMPTY_PARAM, " VPS OFF!\n"); 	
		else if (hall_status == HALL_VPS_NOT_CHECKED) 	Logs(LOGS_EMPTY_PARAM, " VPS not checked!\n");
		else if (hall_status == HALL_VPS_UNKNOWN) 	Logs(LOGS_EMPTY_PARAM, " VPS unknown!\n");
		else if (hall_status == SOFT_ERROR) 		return SOFT_ERROR;
		if ((hall_status == state)||(hall_status == HALL_VPS_NOT_CHECKED)) return HALL_STATUS_CONFIRMED;
		Logs(LOGS_EMPTY_PARAM, "try " + std::to_string(i+1));
		Logs(LOGS_EMPTY_PARAM, ": ");
	}
	
	return SOFT_ERROR;
}

int powerhub::ExecuteCommand(std::string word, std::vector<std::pair<std::string, std::string>> flag)
{
				
	std::ofstream tmpfile;
	int id = commandvoc[word];
	unsigned char c = 0, tmp;
	int ret = 0, ip_status, res;
	std::string ip, tmpstr;
	std::stringstream strstream;
	bool fpo = false, fip = false, fi2c = false;
	bool fi2cmed = false, fi2cmax = false, fi2cmin = false, fi2cav = false, fi2clist = false;
	bool ft = false, fnoany = false;
	switch(id)
	{
	case EXIT_SUCCESSFUL:
		return EXIT_SUCCESSFUL;
		break;  
	case COMMAND_ALLON:
		res = PowerOnAll (100);
		switch(res)
		{
		case SUCCESSFUL:
			Logs(LOGS_EMPTY_PARAM, "All has been opened\n");
			break;
		case SOFT_ERROR:
			Logs(LOGS_EMPTY_PARAM, "COMMAND_ALLON SOFT_ERROR\n");
			return SOFT_ERROR;
			break;
		case HARD_ERROR:
			Logs(LOGS_EMPTY_PARAM, "COMMAND_ALLON HARD_ERROR\n");
			if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
			return HARD_ERROR;
			break;
		}
		return COMMAND_ALLON;
		break;
	case COMMAND_ALLOFF:
		res = PowerOffAll (100);
		switch(res)
		{
		case SUCCESSFUL:
			Logs(LOGS_EMPTY_PARAM, "All has been closed\n");
			break;
		case SOFT_ERROR:
			Logs(LOGS_EMPTY_PARAM, "COMMAND_ALLOFF SOFT_ERROR\n");
			return SOFT_ERROR;
			break;
		case HARD_ERROR:
			Logs(LOGS_EMPTY_PARAM, "COMMAND_ALLOFF HARD_ERROR\n");
			if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
			return HARD_ERROR;
			break;
		}
		return COMMAND_ALLOFF;
		break;
	case COMMAND_REBOOTALL:
		res = RebootAll (100);
		switch(res)
		{
		case SUCCESSFUL:
			Logs(LOGS_EMPTY_PARAM, "All has been rebooted\n");
			break;
		case SOFT_ERROR:
			Logs(LOGS_EMPTY_PARAM, "COMMAND_REBOOTALL SOFT_ERROR\n");
			return SOFT_ERROR;
			break;
		case HARD_ERROR:
			Logs(LOGS_EMPTY_PARAM, "COMMAND_REBOOTALL HARD_ERROR\n");
			if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
			return HARD_ERROR;
			break;
		}
		return COMMAND_REBOOTALL;
		break;
	case COMMAND_ON:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "ep")
			{
				res = PowerOn(rele.rele_on[std::stoi((*i).second)], 100);
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " opened\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_ON SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -ep\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_ON HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
			}
			else if ((*i).first == "p")
			{
				res = _Power(std::stoi((*i).second), 1);
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " opened\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_ON SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -p\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_ON HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
			}
			else if ((*i).first == "rp")
			{
				res = PowerOn(std::stoi((*i).second), 100);
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " opened\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_ON SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -ep\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_ON HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
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
				res = PowerOff(rele.rele_off[std::stoi((*i).second)], 100);
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " closed\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_OFF SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -ep\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_OFF HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
			}
			else if ((*i).first == "p")
			{
				res = _Power(std::stoi((*i).second), 0);
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " closed\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_OFF SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -p\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_OFF HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
			}
			else if ((*i).first == "rp")
			{
				res = PowerOff(std::stoi((*i).second), 100);
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " closed\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_OFF SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -ep\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_OFF HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
			}
		}
		return COMMAND_OFF;
		break;
	case COMMAND_REBOOT:
		if (!flag.empty())
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if (((*i).first == "ep")||((*i).first == "rp"))
			{
				res = Reboot(std::stoi((*i).second), 100);
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " rebooted\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_REBOOT SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -ep\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_REBOOT HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
			}
			else if ((*i).first == "p")
			{
				res = _Reboot(std::stoi((*i).second));
				switch(res)
				{
				case SUCCESSFUL:
					Logs(LOGS_EMPTY_PARAM, std::to_string(std::stoi((*i).second)) + " rebooted\n"); 
					break;
				case SOFT_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_REBOOT SOFT_ERROR: " + std::to_string(std::stoi((*i).second)) + " pin. Flag -p\n");
					return SOFT_ERROR;
					break;
				case HARD_ERROR:
					Logs(LOGS_EMPTY_PARAM, "COMMAND_REBOOT HARD_ERROR\n");
					if (!FTDIstatus) Logs(LOGS_EMPTY_PARAM, "FTDI device not found!\n");
					return HARD_ERROR;
					break;
				}
			}
		}
		return COMMAND_REBOOT;
		break;
	case COMMAND_STATUS:
		for (auto i = flag.begin(); i != flag.end(); i++)
		{
			if ((*i).first == "t")
				ft = true;
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
		fnoany = fi2c | fpo | fip | fi2c | ft;
		if ((FTDIstatus)&&(rele.init))
		{
			if ((ret = ftdi_read_data(ftdic, &c, 1)) < 0) {
				ftdi_fatal("unable to read from ftdi device", ret);
			}
			std::string tmpmessage = "read data: " + std::to_string((int)c) + "\n";
			Logs(LOGS_EMPTY_PARAM, tmpmessage);
			if ((fpo || fip)||(!fnoany))
			{
				Logs(LOGS_EMPTY_PARAM, "========== Pin status ==========\n");
				for (int i = 0; i < rele.pinSize; i++)
				{
					ip = rele.pin[i].ip;
					if (fpo || (!fip)) 
					{
						tmpstr = rele.pin[i].pin_out;
						tmp = std::stoi(tmpstr,nullptr,0);
						Logs(LOGS_EMPTY_PARAM, "pin_out: " + tmpstr + " - " + ((((1 << tmp) & c) == (1 << tmp))?"on!\n":"off!\n"));
					}
					if (fip || (!fpo))
					{
						if (!rele.pin[i].check_ip) {Logs(LOGS_EMPTY_PARAM, " IP not checked! Rele IP invalid!\n"); continue;}
						Logs(LOGS_EMPTY_PARAM, "ip: " + ip + " ");
						ip_status = PingHost(ip, 1);
						if 	(ip_status == PING_HOST_AVAILABLE) 	Logs(LOGS_EMPTY_PARAM, "opened!\n"); 
						else if (ip_status == PING_HOST_NOT_AVAILABLE) 	Logs(LOGS_EMPTY_PARAM, "closed!\n"); 
						else if (ip_status == PING_HOST_NOT_CHECKED) 	Logs(LOGS_EMPTY_PARAM, "not checked!\n"); 
					}
				}
			}
		}
		else
		{
			Logs(LOGS_EMPTY_PARAM, "FTDI device not found or Rele invalid initialization!\n");
		}

		if ((fi2c)||(!fnoany))
		{
			Logs(LOGS_EMPTY_PARAM, "========== I2CBus status ==========\n");
			for (int i = 0; i < rele.pinSize; i++)
			{
				std::vector<int> tmppinvec;
				try
				{
					tmppinvec = CheckADS(rele.pin[i].hall_address, (int)std::stoi(rele.pin[i].pin_hall,nullptr,0), 1000);
				}
				catch(...)
				{
					Logs(LOGS_EMPTY_PARAM, rele.pin[i].hall_address + ".");
					Logs(LOGS_EMPTY_PARAM, rele.pin[i].pin_hall + " broke\n");
					continue;
				}
				double sum = 0;
				strstream.str("");
				strstream << "=== rele pin: " << i 
					  << ", ADS address:" << rele.pin[i].hall_address 
					  << ", ADS pin:"     << rele.pin[i].pin_hall 
					  << " === \n" << "N | Hex | Dec | V\n";
				Logs(LOGS_EMPTY_PARAM, strstream.str());
				system("mkdir hall 2> /dev/null");
				tmpfile.open("hall/"+rele.pin[i].hall_address+"_"+rele.pin[i].pin_hall+".txt");
				if ((fi2clist || (!fnoany))&&(tmpfile.is_open())) 
					for (int k = 0; k < (int)tmppinvec.size(); k++)
					{
						tmpfile << k << " " << std::hex << tmppinvec[k] 
							     << " " << std::dec << tmppinvec[k] 
							     << " " << tmppinvec[k]*VPS << std::endl;
					}
				tmpfile.close();
				std::sort (tmppinvec.begin(), tmppinvec.end());
				if (fi2cmin || (!fnoany))
				{
					strstream.str("");
					strstream << "Min: - | " << std::hex << tmppinvec[0] 
						        << " | " << std::dec << tmppinvec[0] 
						        << " | " << tmppinvec[0]*VPS << "\n";
					Logs(LOGS_EMPTY_PARAM, strstream.str()); 
				}
				if (fi2cmax || (!fnoany))
				{
					strstream.str("");
					strstream << "Max: - | " << std::hex << tmppinvec[tmppinvec.size()-1] 
						        << " | " << std::dec << tmppinvec[tmppinvec.size()-1] 
						        << " | " << tmppinvec[tmppinvec.size()-1]*VPS << "\n";
					Logs(LOGS_EMPTY_PARAM, strstream.str()); 
				}
				if (fi2cmed || (!fnoany))
				{
					strstream.str("");
					strstream << "Med: - | " << std::hex << tmppinvec[tmppinvec.size()/2] 
						        << " | " << std::dec << tmppinvec[tmppinvec.size()/2] 
						        << " | " << tmppinvec[tmppinvec.size()/2]*VPS << "\n";
					Logs(LOGS_EMPTY_PARAM, strstream.str()); 
				}
				if (fi2cav || (!fnoany))
				{
					for (int k = 0; k < (int)tmppinvec.size(); k++)
						sum+=tmppinvec[k];
					sum/=tmppinvec.size();
					strstream.str("");
					strstream << "Average: - | " << std::hex << (int)round(sum)
							    << " | " << std::dec << (int)round(sum)
						            << " | " << sum*VPS << "\n";
					Logs(LOGS_EMPTY_PARAM, strstream.str()); 
				}
			}
		}

		if ((ft)||(!fnoany))
		{
			double temper = 0;
		    	tmpstr = "========== Term status: ==========\n";
			for (int i = 0; i < rele.pinSize; i++)
			{
				try
				{
					temper = CheckThermalSensor(rele.pin[i].term_address, (int)std::stoi(rele.pin[i].pin_term,nullptr,0), 1000, ADS_VALUE_AVG);
					tmpstr += rele.pin[i].term_address + "." + rele.pin[i].pin_term + ": " + std::to_string(temper) + "\n";
					if ((temper < term_critical_min) || (term_critical_max < temper))
					{
						tmpstr += " - DANGEROUS!";
					}
					if ((temper < term_dangerous_min) || (term_dangerous_max < temper))
					{
						tmpstr += " - EMERGENCY EXIT!!!\n";
					}
				}
				catch(...)
				{
					tmpstr += rele.pin[i].term_address + "." + rele.pin[i].pin_term + ": not found!\n";
				}
			}
			Logs(LOGS_EMPTY_PARAM, tmpstr);
		}
		return COMMAND_STATUS;
		break;
	case COMMAND_HELP:
		std::cout << "+--------------+-------------+--------------------------------------------+" << std::endl;
		std::cout << "| help         |             | help                                       |" << std::endl;
		std::cout << "| exit         |             | exit                                       |" << std::endl;
		std::cout << "| allon        |             | sequential activation of relays            |" << std::endl;
		std::cout << "| alloff       |             | sequential shutdown of relays              |" << std::endl;
		std::cout << "| rebootall    |             | sequential reboot of relays                |" << std::endl;
		std::cout << "| on           | -p  <(0-7)> | turn on the relay at the pin number  	|" << std::endl;
		std::cout << "|              | -ep <(0-7)> | turn on the relay at the specified number  |" << std::endl;
		std::cout << "| off          | -p  <(0-7)> | turn off the relay at the pin number 	|" << std::endl;
		std::cout << "|              | -ep <(0-7)> | turn off the relay at the specified number |" << std::endl;
		std::cout << "|              |             | and check smth params                      |" << std::endl;
		std::cout << "|              | -rp <(0-7)> | reboot the relay at the pin number         |" << std::endl;
		std::cout << "|              |             | and check smth params                      |" << std::endl;
		std::cout << "| reboot       | -p  <(0-7)> | reboot the relay at the pin number         |" << std::endl;
		std::cout << "|              | -ep <(0-7)> | reboot the relay at the specified number   |" << std::endl;
		std::cout << "|              |             | and check smth params                      |" << std::endl;
		std::cout << "|              | -rp <(0-7)> | reboot the relay at the pin number         |" << std::endl;
		std::cout << "|              |             | and check smth params                      |" << std::endl;
		std::cout << "| status       |             | get status                                 |" << std::endl;
		std::cout << "|              | -po         | dislay pin_out status                      |" << std::endl;
		std::cout << "|              | -ip         | dislay ip status                           |" << std::endl;
		std::cout << "|              | -i2c        | dislay i2c status                          |" << std::endl;
		std::cout << "|              | -fi2cmed    | dislay i2c median status                   |" << std::endl;
		std::cout << "|              | -fi2cmax    | dislay i2c max status                      |" << std::endl;
		std::cout << "|              | -fi2cmin    | dislay i2c min status                      |" << std::endl;
		std::cout << "|              | -fi2cav     | dislay i2c average status                  |" << std::endl;
		std::cout << "|              | -fi2clist   | dislay i2c list for smth time status       |" << std::endl;
		std::cout << "| reloadconfig |             | reload config                              |" << std::endl;
		std::cout << "+--------------+-------------+--------------------------------------------+" << std::endl;
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

int powerhub::ParsCommand(std::string in)
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
				break;
			}
			if (*i == ' ')
			{
				if (word.size() == 0) 
				{ 
					std::cout << "incorrect" << std::endl; 
					ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
					return SOFT_ERROR;
				}
				while (*i == ' ') i++;
				if (*i != '-')        
				{ 
					std::cout << "incorrect" << std::endl; 
					ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
					return SOFT_ERROR;
				}
				break;
			}
			if (*i == '-') {i++; pos = PARS_READ_FLAG;break;}
			std::cout << "incorrect: \"" << in << "\"" << std::endl; 
			ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
			return SOFT_ERROR;
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
					return SOFT_ERROR;
				}
				while (*i == ' ') i++;
				if (*i == '-')
				{
					pos = PARS_READ_FLAG;
					flag.push_back(std::make_pair(str, ""));
					str = ""; i++; continue;
				}
				else if (i == in.end())
				{
					pos = PARS_END;
					flag.push_back(std::make_pair(str, ""));
					str = "";
					continue;
				}
				else if (*i != ' ')     
				{
					pos = PARS_READ_FLAG_VALUE;
					flag.push_back(std::make_pair(str, ""));
					str = "";
					continue;
				}
			}
			if ((i == in.end())||(pos == PARS_END)) 
			{
				flag.push_back(std::make_pair(str, ""));
				str = "";
				continue;
			}
			//std::cout << "incorrect" << std::endl; 
			//ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
			//return SOFT_ERROR;
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
					return SOFT_ERROR;
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
			//std::cout << "incorrect" << std::endl; 
			//ExecuteCommand("help", std::vector<std::pair<std::string, std::string>>()); 
			//return SOFT_ERROR;
			break;
		}
		if (pos == PARS_END) break;
		
	}
	return ExecuteCommand(word, flag);
}

int powerhub::InitCommandLine()
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

int powerhub::ExecuteArgcArgv(int argc, char *argv[])
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

int powerhub::CommandLine(int argc, char *argv[])
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
			std::getline(std::cin, line);// проблемы с кириллицей, WTW?! TODO: исправить эту хуйню
			if (ParsCommand(line) == EXIT_SUCCESSFUL) break;
			std::cout << std::endl;
		}
	}
	return SUCCESSFUL;
}

void powerhub::ThreadCheckTerm()
{
	double res = 0;
	bool EMERGENCY_EXIT[rele.pinSize], EMERGENCY_EXIT_SUM;
	bool EMERGENCY_TEMPERATURE = false;
	for (int i = 0; i < rele.pinSize; i++)
	{
		EMERGENCY_EXIT[i] = false;
	}
	std::string message;
	while(tempControl)
	{
		EMERGENCY_TEMPERATURE = false;
		message = "========== Term status: ==========\n";
		for (int i = 0; i < rele.pinSize; i++)
		{
			try
			{
				res = CheckThermalSensor(rele.pin[i].term_address, (int)std::stoi(rele.pin[i].pin_term,nullptr,0), 1000, ADS_VALUE_AVG);
				message += rele.pin[i].term_address + "." + rele.pin[i].pin_term + ": " + std::to_string(res) + "\n";
				if ((res < term_critical_min) || (term_critical_max < res))
				{
					EMERGENCY_TEMPERATURE = true;
					message += " - DANGEROUS!";
				}
				if (((res < term_dangerous_min) || (term_dangerous_max < res))&&(!EMERGENCY_EXIT[i]))
				{
					message += " - EMERGENCY EXIT!!!\n";
    					std::cout << "\r";
					Logs(LOGS_EMPTY_PARAM, "EMERGENCY EXIT!!!\n");
					EMERGENCY_EXIT[i] = true;
					ExecuteCommand("alloff", std::vector<std::pair<std::string, std::string>>()); 
				}
				else if (EMERGENCY_EXIT[i])
				{
					if ((res >= term_normal_min) && (term_normal_max >= res))
					{
						EMERGENCY_EXIT[i] = false;
						message += " - Temperature is normalized!\n";
    						std::cout << "\r";
						Logs(LOGS_EMPTY_PARAM, "Temperature is normalized!\n");
						ExecuteCommand("allon", std::vector<std::pair<std::string, std::string>>()); 
					}
					else
					{
						message += " - Expectation of temperature normalization!\n";
    						std::cout << "\r";
						Logs(LOGS_EMPTY_PARAM, "Expectation of temperature normalization!\n");
					}
				}
			}
			catch(...)
			{
				message += rele.pin[i].term_address + "." + rele.pin[i].pin_term + ": not found!\n";
			}
		}
    		std::cout << "\r";
		Logs(LOGS_EMPTY_PARAM, message);
		EMERGENCY_EXIT_SUM = false;
		for (int i = 0; i < rele.pinSize; i++)
		{
			EMERGENCY_EXIT_SUM |= EMERGENCY_EXIT[i];
		}
		if ((emailRead)&&((EMERGENCY_TEMPERATURE)||(EMERGENCY_EXIT_SUM)))
			system(("echo \"" + message + "\" | mail -v -s \"EMERGENCY TEMPERATURE!\" " + email + " 2> /dev/null").c_str());
    		std::cout << "\r";
		std::cout << "\x1b[1;32m>>>\x1b[0m " << std::flush;
		sleep(check_sleep);
	}
}

void powerhub::ThreadCheckWorkingCapacity()
{
	int hall_status, ret, ip_status;
	std::string message, tmpstr;
	unsigned char tmp, c;
	bool EMERGENCY;
	while(work_capacity)
	{
		EMERGENCY = false;
    		message = "========== Working capacity: ==========\n";
		if (FTDIstatus)
			if ((ret = ftdi_read_data(ftdic, &c, 1)) < 0) {
				ftdi_fatal("unable to read from ftdi device", ret);
			}
		for (int i = 0; i < rele.pinSize; i++)
		{
			if (!FTDIstatus) 
			{
				EMERGENCY = true;
				tmpstr = rele.pin[i].pin_out;
				message += "pin:                " + tmpstr + "FTDI device not found!\n";
			}
			else
			{
				tmpstr = rele.pin[i].pin_out;
				tmp = std::stoi(tmpstr,nullptr,0);
				EMERGENCY = (((1 << tmp) & c) == (1 << tmp))?false:true;
				message += "pin:                " + tmpstr + ((((1 << tmp) & c) == (1 << tmp))?" on!\n":" off!\n");
			}
		
			if ((rele.pin[i].check_ip)&&(checkIP))
			{
				message += "ip:                 " + rele.pin[i].ip + " ";
				ip_status = PingHost(rele.pin[i].ip, 1);
				if 	(ip_status == PING_HOST_AVAILABLE) 	message += "opened!\n"; 
				else if (ip_status == PING_HOST_NOT_AVAILABLE) 	{message+= "closed!\n"; EMERGENCY = true;}
				else if (ip_status == PING_HOST_NOT_CHECKED) 	message += "not checked!\n"; 
			}
			else
			{
				message += "IP not checked! Rele IP invalid or IP check disabled!\n";
			}
			if (checkHall)
			{
				hall_status = CheckHallSensorStatus(rele.pin[i].hall_address, std::stoi(rele.pin[i].pin_hall,nullptr,0), 1000);
				message += rele.pin[i].hall_address + "." + rele.pin[i].pin_hall + " pin_hall: ";
				if 	(hall_status == HALL_VPS_ON) 		message += "VPS ON!\n"; 
				else if (hall_status == HALL_VPS_OFF) 		{message+= "VPS OFF!\n"; EMERGENCY = true;}	
				else if (hall_status == HALL_VPS_NOT_CHECKED) 	message += "VPS not checked!\n";
				else if (hall_status == HALL_VPS_UNKNOWN) 	message += "VPS unknown!\n";
				else if (hall_status == SOFT_ERROR) 		message += "not found!\n";
			}
			else
			{
				message += "ADS not checked! Rele IP invalid or ADS check disabled!\n";
			}
			message += "\n";
		}
    		std::cout << "\r";
		Logs(LOGS_EMPTY_PARAM, message);
		std::cout << "\x1b[1;32m>>>\x1b[0m " << std::flush;
		if ((emailRead)&&(EMERGENCY))
			system(("echo \"" + message + "\" | mail -v -s \"PROBLEMS WITH WORKING CAPACITY!\" " + email + " 2> /dev/null").c_str());
		sleep(check_sleep);
	}
	
}

void powerhub::ThreadCheckSNRUPS()
{
	int res = 0;
	bool CRITICAL = false;
	bool DANGEROUS = false;
	bool EMERGENCY_EXIT = false;
	std::string message;
	while(tempControl)
	{
		message = "========== SNR-UPS status: ==========\n";
		try
		{
			res = snmpObj.upsEstimatedChargeRemaining();
			message += "upsBatteryStatus:             " + std::to_string(snmpObj.upsBatteryStatus()) 		+ "\n";
			message += "upsSecondsOnBattery:          " + std::to_string(snmpObj.upsSecondsOnBattery()) 		+ "\n";
			message += "upsEstimatedMinutesRemaining: " + std::to_string(snmpObj.upsEstimatedMinutesRemaining()) 	+ "\n";
			message += "upsEstimatedChargeRemaining:  " + std::to_string(res);
			if (res < pow_critical_min)
			{
				CRITICAL = true;
				message += " - CRITICAL!";
			}
			if (res < pow_dangerous_min)
			{
				DANGEROUS = true;
				message += " - DANGEROUS!";
			}
			message += "\n";
			message += "upsBatteryVoltage:            " + std::to_string(snmpObj.upsBatteryVoltage()) 		+ "\n";
			message += "upsBatteryCurrent:            " + std::to_string(snmpObj.upsBatteryCurrent()) 		+ "\n";
			message += "upsBatteryTemperature:        " + std::to_string(snmpObj.upsBatteryTemperature()) 		+ "\n";
			message += "upsOutputSource:              " + std::to_string(snmpObj.upsOutputSource()) 		+ "\n";
			message += "upsOutputFrequency:           " + std::to_string(snmpObj.upsOutputFrequency()) 		+ "\n";
			message += "upsOutputVoltage:             " + std::to_string(snmpObj.upsOutputVoltage()) 		+ "\n";
			message += "upsOutputCurrent:             " + std::to_string(snmpObj.upsOutputCurrent()) 		+ "\n";
			message += "upsOutputPower:               " + std::to_string(snmpObj.upsOutputPower()) 		+ "\n";
			if ((DANGEROUS)&&(!EMERGENCY_EXIT))
			{
				message += "EMERGENCY EXIT!!!\n";
    				std::cout << "\r";
				Logs(LOGS_EMPTY_PARAM, "EMERGENCY EXIT!!!\n");
				ExecuteCommand("alloff", std::vector<std::pair<std::string, std::string>>()); 
				EMERGENCY_EXIT = true;
			}
			else if (EMERGENCY_EXIT)
			{
				if (res >= ((int)(pow_critical_min*1.25))%100)
				{
					EMERGENCY_EXIT = false;
					message += "Battery charge is normalized!\n";
    					std::cout << "\r";
					Logs(LOGS_EMPTY_PARAM, "Battery charge is normalized!\n");
					ExecuteCommand("allon", std::vector<std::pair<std::string, std::string>>()); 
				}
				else
				{
					message += " - Expectation of battery charge normalization!\n";
    					std::cout << "\r";
					Logs(LOGS_EMPTY_PARAM, "Expectation of battery charge normalization!\n");
				}
			}
		}
		catch(...)
		{
			message += "ERROR!\n";
		}
    		std::cout << "\r";
		Logs(LOGS_EMPTY_PARAM, message);
		if ((emailRead)&&((DANGEROUS)||(CRITICAL)))
			system(("echo \"" + message + "\" | mail -v -s \"EMERGENCY BATTERY CHARGE!\" " + email + " 2> /dev/null").c_str());
    		std::cout << "\r";
		std::cout << "\x1b[1;32m>>>\x1b[0m " << std::flush;
		sleep(check_sleep);
	}
}

void powerhub::_InitThreads()
{
    	std::thread func_CheckWorkingCapacity(&powerhub::ThreadCheckWorkingCapacity, this);
    	if (func_CheckWorkingCapacity.joinable()) func_CheckWorkingCapacity.detach(); 
    	//sleep(15);
    	std::thread func_CheckTerm(&powerhub::ThreadCheckTerm, this); 
    	if (func_CheckTerm.joinable()) func_CheckTerm.detach(); 
    	std::thread func_CheckSNRUPS(&powerhub::ThreadCheckSNRUPS, this); 
    	if (func_CheckSNRUPS.joinable()) func_CheckSNRUPS.detach(); 
}

int powerhub::InitThreads()
{
    	std::thread initThreads(&powerhub::_InitThreads, this); 
    	if (initThreads.joinable()) initThreads.detach(); 
	return SUCCESSFUL; 
}

int powerhub::InitPowerHub(std::string str)
{
	FTDIstatus = false;
	LoadConfigs(str);
	InitSSH();
	InitFTDI();
	InitThreads();
	InitCommandLine();
	return SUCCESSFUL;
}

int powerhub::FreePowerHub()
{
	FTDIstatus = false;
	FreeFTDI();
	return SUCCESSFUL;
}

