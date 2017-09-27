#include <boost/test/minimal.hpp>
#include "powerhub.hpp"
    
powerhub::ADSvector ads_test; 

int AdsTest(std::string tmpstr)
{
	ads_test[tmpstr] = new Adafruit_ADS1115((int)std::stoi(tmpstr,nullptr,0));
	return SUCCESSFUL;
}
     
int test_main( int, char *[ ] ) // note the name! 
{
	powerhub obj;
	//obj.InitPowerHub("rele.config");
	BOOST_CHECK(obj.LoadConfigs("rele.config") == SUCCESSFUL);
	BOOST_CHECK(AdsTest("0x48") == SUCCESSFUL);
	BOOST_CHECK(AdsTest("0x4a") == SUCCESSFUL);
	obj.CheckADC		 		(ads_test.find("0x48"), 0, 1000);
	obj.CheckThermalSensor	 		(ads_test.find("0x48"), 0, 1000, ADC_VALUE_AVG);
	obj.CheckADCGetValue	 		(ads_test.find("0x48"), 0, 1000, ADC_VALUE_AVG);
	obj.CheckHallSensorStatus		(ads_test.find("0x48"), 0, 1000);
	obj.CheckHallSensorStatusWaitOfState	(ads_test.find("0x48"), 0, 1000, 1, HALL_VPS_OFF);
	obj.CheckADC		 		(ads_test.find("0x4a"), 0, 1000);
	obj.CheckThermalSensor	 		(ads_test.find("0x4a"), 0, 1000, ADC_VALUE_AVG);
	obj.CheckADCGetValue	 		(ads_test.find("0x4a"), 0, 1000, ADC_VALUE_AVG);
	obj.CheckHallSensorStatus		(ads_test.find("0x4a"), 0, 1000);
	obj.CheckHallSensorStatusWaitOfState	(ads_test.find("0x4a"), 0, 1000, 1, HALL_VPS_OFF);
	obj.CheckADC		 		(obj.adsmap.find(obj.rele.pin[0].hall_address), 0, 1000);
	obj.CheckThermalSensor	 		(obj.adsmap.find(obj.rele.pin[0].hall_address), 0, 1000, ADC_VALUE_AVG);
	obj.CheckADCGetValue	 		(obj.adsmap.find(obj.rele.pin[0].hall_address), 0, 1000, ADC_VALUE_AVG);
	obj.CheckHallSensorStatus		(obj.adsmap.find(obj.rele.pin[0].hall_address), 0, 1000);
	obj.CheckHallSensorStatusWaitOfState	(obj.adsmap.find(obj.rele.pin[0].hall_address), 0, 1000, 1, HALL_VPS_OFF);
        return 0;
}
