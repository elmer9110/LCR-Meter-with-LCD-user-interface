# LCR-Meter-with-LCD-user-interface
Embedded System controlled by the TM4C123GH6PM Texas Instruments microcontroller that is able to measure resistance, capacitance, inductance, ESR, and output voltage going to the microcontroller. The user interface is composed of 6 push buttons to select appropriate functions as well as an 4X20 LCD display with a PCF8574 I/O expander that is communicated with through I2C protocol. The entire Embedded System is powered by a lithuim ion battery. 

The measuring of Inductance, resistance, capacitance, and ESR is accomplished through the sequencing of several transistor switch circuits and using the time it took the comparator in the TM4C123GH6PM to trigger. Once the time, which is stored in a 32-bit on board timer, is measured it used to calculate the value of the component and returned to be displayed on the user interface. Additionally, on each measurement function there is a timeout feature enabled in order to prevent long wait times if a very-large component is used i.e., 10Mohm resistor or an incorrect measurement is made i.e., user called capacitance measurement, but a resistor is put in the circuit to be tested. Schematic shown below:

![image](https://user-images.githubusercontent.com/66042477/118740834-7104e380-b812-11eb-86d2-5d97e987bd2e.png)

## Resistance Measurement:
Resistance is measured by turning on the integrate switch on as well as lowside_r switch in order to deintegrate the 1uF capacitor. Then the lowside_r switch is turned off and the meas_lr switch is turned on. Once this happens the timer is started and we wait till the comparator is triggered. Once triggered, the time is stored and then divided by a linear constant previously derived and the resistance value is returned in Ohms. Note, the comparator triggers one the DUT2 pin on circuit schematic hits a reference voltage of 2.469 Volts. Additionally, the range of resistor values that can be measured is 100Ohms-1Mohms.

## Capacitance measurement:
Capacitance is measured by turning on the meas_c  switch on as well as lowside_r switch in order to deintegrate the capacitor being tested. Then the lowside_r switch is turned off and the highside_r switch is turned on. Once this happens, the capacitor under test begins charging and the timer is started and we wait till the comparator is triggered. Once triggered, the time is stored and then divided by a linear constant previously derived, and the capacitance value is returned in microfarads. Note, the comparator triggers one the DUT2 pin on circuit schematic hits a reference voltage of 2.469 Volts. Additionally, the range of resistor values that can be measured is 1nf-10uF.

## Inductance measurement:
Inductance is measured by turning off all switches in order to make sure the inductor magnetic field collapses and it is discharged. Four Zener diodes are in place to protect against high reverse current spikes due to the change in current. Then the meas_lr  switch is turned on as well as lowside_r switch in order to energize the inductor. Once the comparator triggers we record the time additionally we call the esr function to measure the esr of the inductor. Once this is done the timer values is divided by the clock rate to get it in time units, add esr and 33ohms to get total resistance in circuit and use it to calculate current through the inductor. Once this is done, all three values are entered into the inductor current equation and we solve for the inductor value. Note, the comparator triggers one the DUT2 pin on circuit schematic hits a reference voltage of 2.469 Volts. However, if esr of inductor goes above 6-7 ohms then comparator might never trigger, and the reference voltage might need to be lowered. Additionally, the range of inductor values that can be measured is 1nH-100uH.

                                                                                                    
## ESR measurement:
Esr is measured by turning on the meas_lr  switch  as well as lowside_r switch in order to energize the inductor. Once this is done, we measure the voltage on DUT2 pin. Next, because of the circuit made by turning on the two switches mentioned above, al voltage divider is formed where r1 is the esr of the inductor and r2 is the 33 ohm resistor. This allows us to plug in the voltage and solve for the esr of the inductor. Once the calculation is done, the function returns the value of the esr.

## Voltage measurement:
Voltage is measured by sampling DUT2 through a A/D converter and scaled to a range of 0-3.3V.

## Auto mode:
Auto mode works by first calling the inductance measurement, if the function timeouts then we call the capacitance function, else we check the esr. If the esr value is less than 15 then we know we have an inductor. When calling the capacitance function, we check if a timeout occurred, if not then we know we have a capacitor. If a timeout occurred, then we know we have a resistor since we have tested for inductor and capacitor and both of those tests failed if this point of the algorithm has been reached.

