# powerhub_CW
/help                           help
/exit                           exit
/allon                          sequential activation of relays
/alloff                         sequential shutdown of relays
/rebootall                      sequential reboot of relays
/on            	-p  <(0-7)>     turn on the relay at the specified number
               	-ep <(0-7)>     turn on the relay at the pin number
/off           	-p  <(0-7)>     turn off the relay at the specified number
               	-ep <(0-7)>     turn off the relay at the pin number
/reboot        	-p  <(0-7)>     reboot the relay at the specified number
               	-ep <(0-7)>     reboot the relay at the pin number
/status                         get status
               	-pi             dislay pin_in status
               	-po             dislay pin_out status
               	-ip             islay ip status
		-i2c		dislay i2cbus status
/reloadconfig                   reload config
