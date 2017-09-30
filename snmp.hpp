#include <iostream> 
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

class snmp
{
private:
	std::string snmpget(std::string mib);
	inline int snmpgetToInt(std::string str)
	{
		str.erase (0, str.find("INTEGER: ") + sizeof("INTEGER: ") - 1);
		return std::stoi(str.c_str(), nullptr, 0);
	}
public:
	std::string community;
	std::string ip;
	bool community_init;
	bool ip_init;
	bool init;
	snmp(std::string _community, std::string _ip)
	{
		community = _community;
		ip = _ip;
		community_init = true;
		ip_init = true;
		init = true;
	}
	snmp()
	{
		community_init = false;
		ip_init = false;
		init = false;
	}
	inline int upsBatteryStatus()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.2.1.0"));
	}
	inline int upsSecondsOnBattery()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.2.2.0"));
	}
	inline int upsEstimatedMinutesRemaining()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.2.3.0"));
	}
	inline int upsEstimatedChargeRemaining()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.2.4.0"));
	}
	inline int upsBatteryVoltage()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.2.5.0"));
	}
	inline int upsBatteryCurrent()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.2.6.0"));
	}
	inline int upsBatteryTemperature()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.2.7.0"));
	}
	inline int upsOutputSource()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.4.1.0"));
	}
	inline int upsOutputFrequency()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.4.2.0"));
	}
	inline int upsOutputVoltage()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.4.4.1.2.1"));
	}
	inline int upsOutputCurrent()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.4.4.1.3.1"));
	}
	inline int upsOutputPower()
	{
		return snmpgetToInt(snmpget("1.3.6.1.2.1.33.1.4.4.1.4.1"));
	}
};
