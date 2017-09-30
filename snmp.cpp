#include "snmp.hpp"

std::string snmp::snmpget(std::string mib)
{
	FILE *in;
	char buff[512];
	std::string str = "";
	in = popen(("snmpget -v 1 -c " + community + " " + ip + " " + mib).c_str(), "r");
	if (fgets(buff, sizeof(buff), in) != NULL)
		str = buff;
	pclose(in);
	return str;
}

/*
using namespace std;

int main() {
	cout << "upsBatteryStatus:             " << snmp::upsBatteryStatus() 			<< endl;
	cout << "upsSecondsOnBattery:          " << snmp::upsSecondsOnBattery() 		<< endl;
	cout << "upsEstimatedMinutesRemaining: " << snmp::upsEstimatedMinutesRemaining() 	<< endl;
	cout << "upsEstimatedChargeRemaining:  " << snmp::upsEstimatedChargeRemaining() 	<< endl;
	cout << "upsBatteryVoltage:            " << snmp::upsBatteryVoltage() 			<< endl;
	cout << "upsBatteryCurrent:            " << snmp::upsBatteryCurrent() 			<< endl;
	cout << "upsBatteryTemperature:        " << snmp::upsBatteryTemperature() 		<< endl;
	cout << "upsOutputSource:              " << snmp::upsOutputSource() 			<< endl;
	cout << "upsOutputFrequency:           " << snmp::upsOutputFrequency() 			<< endl;
	cout << "upsOutputVoltage:             " << snmp::upsOutputVoltage() 			<< endl;
	cout << "upsOutputCurrent:             " << snmp::upsOutputCurrent() 			<< endl;
	cout << "upsOutputPower:               " << snmp::upsOutputPower() 			<< endl;
    	return 0; 
}
*/
