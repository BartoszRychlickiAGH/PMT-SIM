#PMT-SIM:
    The project includes implementations of functionalities to run DAC embedded in PMT board.

    DAC datasheet: https://www.alldatasheet.com/datasheet-pdf/view/521541/AD/AD5361BSTZ-REEL1.html
	
	Firmware provides setting selected value of voltage from range (1-01V to 10V) to selected output channel (from 1 to 6)

#How to Run:
    To properly launch project, it only requires correct hardware connection. Loading project on is held via ST-LINK V2.

#Authors:

	- Bartosz Rychlicki
    - Michał Gądek
    - Michał Jurek

#Functonalities:
    1. Setting DAC in correct mode
    2. Reading DAC status
    3. Sending HITs
	4. Receiving selected PMT-SIM output
	5. Receiving selected value of voltage
	6. Setting selected voltage on selected output
	7. Sending firmware reponse to GUI via UART

#UART Commends:
	I. RX: (UART commands, whose stm32 can collect and control PCB outputs)
			- h;<1/2>\r\n 		 	 - triggers HIT1 or HIT2
			- <1/6>;<-10/10>\r\n 	 - sets exact voltage on predefined PMT-SIM's PCB output

	II. TX: (UART commends to send to GUI, to confirm excecuting received command)
			- h;<1/2>;<OK/ER>   	 - returns error or ok status in case execution of HIT1 or HIT2 went succesfully
			- <1/6>;<-10/10>;<OK/ER> - returns error or ok status in case execution, of setting voltage on given channel, went succesfully
			
#Changelog:
    [27.10.2025 5:47 pm]  Github repository was created
	[19.01.2026 12:44 am] Pushed implementation, ready for functional tests