# powerhub_CW commands


```html
help				| help  
exit				| exit  
allon				| sequential activation of relays  
alloff				| sequential shutdown of relays  
rebootall			| sequential reboot of relays  
on		-p  <(0-7)>	| turn on the relay at the specified number  
on		-ep <(0-7)> 	| turn on the relay at the pin number  
off		-p  <(0-7)> 	| turn off the relay at the specified number  
off		-ep <(0-7)> 	| turn off the relay at the pin number  
reboot		-p  <(0-7)> 	| reboot the relay at the specified number  
reboot		-ep <(0-7)> 	| reboot the relay at the pin number  
status            		| get status  
status        	-po         	| dislay pin_out status  
status		-ip		| dislay ip status  
status		-i2c		| dislay i2c status  
status		-fi2cmed	| dislay i2c median status  
status		-fi2cmax	| dislay i2c max status  
status		-fi2cmin	| dislay i2c min status  
status		-fi2cav		| dislay i2c average status  
status		-fi2clist	| dislay i2c list for smth time status  
reloadconfig       		| reload config
```
