## METAR Sectional Map Firmware & Config

Scott Martin, January 2023

This outlines the procedure for gathering and installing the source code for a METAR sectional map indicator on the Wemos D1 mini microcontroller (an ESP8266 chip).


### Download the dependencies

**Git **is a version control software used to synchronize code repositories. We'll use this to get the sectional chart source code - [https://git-scm.com/](https://git-scm.com/) 

**Python** is a language/interpreter that's used to install PlatformIO - [https://python.org](https://python.org)  

**PlatformIO Core** is a set of command line tools to compile and flash embedded devices. 

[https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html#local-download-macos-linux-windows](https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html#local-download-macos-linux-windows)   


### Clone the repository

**Do this only once** so that you have the firmware available to compile. Open a powershell (Start, type "powershell", Enter)

Then `cd` to the location you want to have the repository (`cd Documents`, Enter, `cd sectional_map`, Enter), and run


```
git clone https://github.com/smartin015/sectional_map
```



### Edit the config file

The config file should be located at `data/config.txt` within the repository you downloaded. If the file does not exist, create a new one, then



1. Open the `data/config.txt` file using notepad or some other editor
2. Set SSID to the wifi name
3. Set PASS to the password
4. For all following lines, write the METAR code and LED address in the format `KBPI=2`

    Ensure there are no spaces separating the values. You can append `#XXXXXX` to override METAR and set a 6-character [hex color value](https://www.google.com/search?q=hex+color+picker&rlz=1CAKMJF_enUS867US867&oq=hex+color+picker&aqs=chrome..69i57l2j69i60l5.1230j0j4&sourceid=chrome&ie=UTF-8) for that location.


    (Visit the link, drag the slider, then copy the value in the section marked "HEX" to the file)




When you're done, it should look like this:


```
SSID=<redacted>                                                                                                                                                    
PASS=<redacted>                                                                                                                                                          
9FL1=0#00FF00                                                                                                                                                                  
KF45=1
KPBI=2
KBCT=3
```




5. **Save the file.**


### Flash the config and firmware

Go back to your previous powershell (see "Clone the repository" for how to open the shell if you've closed it) and ensure you're in the `sectional_map` directory. Plug in the Wemos D1 Mini microcontroller via USB to your computer.

**Upload the configuration:**


```
pio run -e d1_mini -t uploadfs
```


**Upload the firmware:**


```
pio run -e d1_mini -t upload
```


**Optionally, read the serial port to ensure the METAR results are fetched correctly:**


```
pio device monitor -b 115200
```


(Press Ctrl+C to quit reading)


### Cleanup & Updating

At this point, you will have flashed the firmware and configuration to the device. You no longer need to connect the microcontroller via USB to your computer, and it can be run standalone using a USB wall wart or other power supply.

If you need to change the config for any reason (e.g. it doesn't connect to the wifi correctly) you can just run the `uploadfs` command (see "Upload the configuration") without having to run the other commands.

**If you need to update the source code**, run the following command in a powershell:


```
git pull origin main
```


This will fetch the new code changes and insert them into the existing source files. After this you will need to run the `upload` command (as above) to push the new firmware to the device. 

