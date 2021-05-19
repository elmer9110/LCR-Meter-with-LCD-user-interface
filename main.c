

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "wait.h"
#include "uart0.h"
#include "adc0.h"
#include "i2c0_lcd.h"


//Pin Definitions
#define MEAS_LR PORTA,6
#define HIGHSIDE_R PORTA,7
#define MEAS_C PORTD,6
#define LOWSIDE_R PORTA,4
#define INTEGRATE PORTE,1
#define COMPARATOR PORTC,7
#define AIN4 PORTD,3

#define RESISTOR_B PORTA,3
#define CAPACITOR_B PORTE,0
#define INDUCTOR_B PORTC,6
#define ESR_B PORTC,5
#define VOLTAGE_B PORTE,3
#define AUTO_B PORTE,2


#define MAX_CHARS 80
#define MAX_FIELDS 5
char FieldString[MAX_CHARS];
char string[80];
uint32_t time_auto=0;



typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;



void initHw()
{
    //Initialize system clock
    initSystemClockTo40Mhz();

    //Enable Analog Comparator Clock
    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    _delay_cycles(3);

    //Enable Relevant Ports
    enablePort(PORTA);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTC);

    //Configure Port A pins
    selectPinPushPullOutput(MEAS_LR);
    selectPinPushPullOutput(HIGHSIDE_R);
    selectPinPushPullOutput(LOWSIDE_R);

    //Configure Port C pins
    selectPinAnalogInput(COMPARATOR);

    //Configure Port D pins
    selectPinPushPullOutput(MEAS_C);
    selectPinAnalogInput(AIN4);

    //Configure Port E pins
    selectPinPushPullOutput(INTEGRATE);

    //Configure pushbuttons and enable pull-up resistors
    selectPinDigitalInput(RESISTOR_B);
    enablePinPullup(RESISTOR_B);
    selectPinDigitalInput(CAPACITOR_B);
    enablePinPullup(CAPACITOR_B);
    selectPinDigitalInput(INDUCTOR_B);
    enablePinPullup(INDUCTOR_B);
    selectPinDigitalInput(VOLTAGE_B);
    enablePinPullup(VOLTAGE_B);
    selectPinDigitalInput(ESR_B);
    enablePinPullup(ESR_B);
    selectPinDigitalInput(AUTO_B);
    enablePinPullup(AUTO_B);

    //Comparator Configuration
    COMP_ACREFCTL_R |= COMP_ACREFCTL_VREF_M | COMP_ACREFCTL_EN;
    //COMP_ACREFCTL_R |= 0xC | COMP_ACREFCTL_EN;
    COMP_ACREFCTL_R &= ~COMP_ACREFCTL_RNG;
    COMP_ACCTL0_R = 0x0000040A;
    waitMicrosecond(10);
    COMP_ACSTAT0_R |= COMP_ACSTAT0_OVAL;

    //Timer 1 Configuration
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
    TIMER1_TAMR_R |=TIMER_TAMR_TACDIR;

}



void getsUart0(USER_DATA* data)
{
    int count=0;
    char c;
    while(1)
    {
        c=getcUart0();
        if((c==8 || c==127) && count>0)
        {
            count--;
        }

        else if(c==13)
        {
            data->buffer[count]='\0';
            return;
        }

        else if(c>=32)
        {
            data->buffer[count]=c;
            count++;
            if(count==MAX_CHARS)
            {
                data->buffer[count]='\0';
                return ;
            }
        }
  }
}

int stringCompare(const char *str1, const char *str2)
{
    int i = 0;
    while(str1[i] == str2[i])
    {
        if(str1[i] == '\0' && str2[i] == '\0')
            break;
            i++;
    }
    return str1[i] - str2[i];
}

int atoi(char* str)
{
    int res = 0; // Initialize result
    int i;
    // Iterate through all characters of input string and
    // update result
    for (i = 0; str[i] != '\0'; i++)
        res = res * 10 + str[i] - '0';

    // return result.
    return res;
}

void parseFields(USER_DATA* data)
{
       int counter = 0;
       int field_count = 0;
       int length=strlen(data->buffer);
       char delStatus='d';
       while(counter!=length)
       {
           if(field_count==MAX_FIELDS)
           {
               if(((data->buffer[counter] >= 65) && (data->buffer[counter] <= 90)) || ((data->buffer[counter] >= 97) && (data->buffer[counter] <= 122)) || (data->buffer[counter] >= 48) && (data->buffer[counter] <= 57))
               {
                   continue;
               }
               else
               {
                   data->fieldCount = field_count;
                   return;
               }
           }
           if(((data->buffer[counter] >= 65) && (data->buffer[counter] <= 90)) || ((data->buffer[counter] >= 97) && (data->buffer[counter] <= 122))&& delStatus=='d')
           {
               data->fieldType[field_count]='a';
               data->fieldPosition[field_count] = counter;
               delStatus='a';
               field_count++;
           }
           if((data->buffer[counter] >= 48) && (data->buffer[counter] <= 57)&& delStatus=='d')
           {
               data->fieldType[field_count]='n';
               data->fieldPosition[field_count] = counter;
               delStatus='n';
               field_count++;
           }
           if(!((data->buffer[counter] >= 65) && (data->buffer[counter] <= 90)) && !((data->buffer[counter] >= 97) && (data->buffer[counter] <= 122))&& !((data->buffer[counter] >= 48) && (data->buffer[counter] <= 57)) && delStatus!='d')
           {
               data->buffer[counter]='\0';
               delStatus='d';
           }
           data->fieldCount = field_count;
           counter++;

       }

}


char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
   int counter=0;
   int counter2= data->fieldPosition[fieldNumber];
   if(fieldNumber <= MAX_FIELDS)
   {
       while(data->buffer[counter2] != '\0')
       {
           FieldString[counter]=data->buffer[counter2];
           counter++;
           counter2++;

       }
       FieldString[counter]= '\0';
       return FieldString;
   }
   else
   {
     FieldString[0]='\0';
     return FieldString;
   }


}

uint32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    uint32_t integer_pointer;
    int counter=0;
    int counter2= data->fieldPosition[fieldNumber];
    if(fieldNumber <= MAX_FIELDS)
       {
           while(data->buffer[counter2]!= '\0')
           {
               FieldString[counter]=data->buffer[counter2];
               counter++;
               counter2++;
           }
           FieldString[counter]= '\0';
           integer_pointer= atoi(FieldString[counter]);
           return integer_pointer;
       }
       else
       {
         integer_pointer=0;
         return integer_pointer;
       }

}

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    char* firstField= '\0';
    firstField = getFieldString(data,0);
    if(stringCompare(firstField,strCommand)==0 && data->fieldCount >= minArguments)
    {
        return true;
    }

    return false;
}



void setAllPinsToZero()
{
    setPinValue(MEAS_LR,0);
    setPinValue(HIGHSIDE_R,0);
    setPinValue(MEAS_C,0);
    setPinValue(LOWSIDE_R,0);
    setPinValue(INTEGRATE,0);
}

float getDUT2Voltage()
{
    float voltage=0;
    setAdc0Ss3Mux(4);
    setAdc0Ss3Log2AverageCount(4);
    voltage = readAdc0Ss3();
    voltage= ((voltage+0.5) / 4096 * 3.3);
    return voltage;
}

uint32_t getResistance()
{
   uint32_t time=0;
   setAllPinsToZero();
   setPinValue(INTEGRATE,1);
   setPinValue(LOWSIDE_R,1);
   waitMicrosecond(100000);

   TIMER1_TAV_R=0;

   TIMER1_CTL_R |= TIMER_CTL_TAEN;
   setPinValue(LOWSIDE_R,0);
   setPinValue(MEAS_LR,1);
   while(COMP_ACSTAT0_R == 0x0);
   time=TIMER1_TAV_R;
   time= time/56.08;
   TIMER1_TAV_R=0;
   TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
   return time;
}

float getCapacitance()
{
    float cap=0;
    TIMER1_TAV_R=0;

    setAllPinsToZero();
    waitMicrosecond(1000);
    setPinValue(MEAS_C,1);
    setPinValue(LOWSIDE_R,1);
    waitMicrosecond(100000);

    setPinValue(LOWSIDE_R,0);
    setPinValue(HIGHSIDE_R,1);
    TIMER1_CTL_R |= TIMER_CTL_TAEN;
    while(COMP_ACSTAT0_R == 0x0)
    {
        if(TIMER1_TAV_R>=1000000000)
        {
            time_auto=1000000001;
            return 0;
        }
    }
    cap=TIMER1_TAV_R;
    cap= cap/6000000;
    TIMER1_TAV_R=0;
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    return cap;

}

float getEsr()
{
    float test_esr=0;
    float test_dut2_voltage=0;
    setAllPinsToZero();
    waitMicrosecond(1000000);

    setPinValue(MEAS_LR,1);
    setPinValue(LOWSIDE_R,1);
    waitMicrosecond(1000000);

    test_dut2_voltage= getDUT2Voltage();
    test_esr= (33*((3.3-test_dut2_voltage)/test_dut2_voltage));
    test_esr=test_esr-1.732;
    return test_esr;
}

float getInductance()
{
    float ind_esr=0;
    float ind=0;
    float current=0;
    float total_esr=0;
    float time=0;
    TIMER1_TAV_R=0;

    ind_esr=getEsr();
    total_esr=ind_esr+33;

    setAllPinsToZero();
    waitMicrosecond(1000000);


    setPinValue(MEAS_LR,1);
    TIMER1_CTL_R |= TIMER_CTL_TAEN;
    setPinValue(LOWSIDE_R,1);

    while(COMP_ACSTAT0_R == 0x0)
    {
        if(TIMER1_TAV_R>=1000000)
        {
            time_auto=1100000;
            return 0;
        }
    }
    time=TIMER1_TAV_R;
    time=time/40000000;
    current= 2.469;
    current=current/total_esr;

    ind= -(time*total_esr)/(log((1-(current*total_esr)/3.3)));
    ind=ind*1000000;

    TIMER1_TAV_R=0;
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    return ind;

}

void auto_mode()
{
    float inductance_auto=0;
    float cap_auto=0;
    uint32_t res_auto=0;
    time_auto=0;
    lcd_clear();
    int voidValue=getInductance();

    if(time_auto<1000000)
    {
        int esr_auto=getEsr();
        if(esr_auto<15)
        {
            //inductance_auto=getInductance();
            //sprintf(string,"Inductance = %.2f microHenry\n\r",inductance_auto);
            //putsUart0(string);
            putsLcd(0,0,"Inductance=");
            inductance_auto=getInductance();
            sprintf(string,"%.2f microHenry",inductance_auto);
            putsLcd(1,0,string);
            waitMicrosecond(3000000);
            lcd_clear();
            return;
        }

    }
    waitMicrosecond(10000);
    voidValue=getCapacitance();
    if(time_auto<1000000000)
    {
        //cap_auto=getCapacitance();
        //sprintf(string,"Capacitance = %.2f microFarads\n\r",cap_auto);
        //putsUart0(string);
        putsLcd(0,0,"Capacitance=");
        cap_auto= getCapacitance();
        sprintf(string,"%.2f microFarads",cap_auto);
        putsLcd(1,0,string);
        waitMicrosecond(3000000);
        lcd_clear();
        return;
    }
    else
    {
       waitMicrosecond(10000);
       res_auto= getResistance();
       putsLcd(0,0,"Resistance=");
       sprintf(string,"%u Ohms",res_auto);
       putsLcd(1,0,string);
       waitMicrosecond(3000000);
       lcd_clear();
       //res_auto=getResistance();
       //sprintf(string,"Resistance = %u Ohms\n\r",res_auto);
       //putsUart0(string);
       return;

    }



}



int main(void)
{
    USER_DATA data;
    //char string[80];
    uint32_t resistance=0;
    float capacitance=0;
    float dut2_voltage=0;
    float esr=0;
    float inductance=0;
    initHw();
    initUart0();
    initAdc0Ss3();
    initLcd();
    setUart0BaudRate(115200, 40e6);
    setAllPinsToZero();

    lcd_clear();
    waitMicrosecond(3000000);
    putsLcd(0,0,"Initializing LCR");
    putsLcd(1,0,"Meter...............");
    waitMicrosecond(4000000);
    lcd_clear();

    putsLcd(0,0,"Execute function by");
    putsLcd(1,0,"pressing appropriate");
    putsLcd(2,0,"button.");
    waitMicrosecond(3000000);
    lcd_clear();
    waitMicrosecond(3000000);

    while(true)
    {
        putsLcd(0,0,"Button order:");
        putsLcd(1,0,"Resist,Cap,Induct");
        putsLcd(2,0,"ESR,Voltage,auto.");
        while(getPinValue(RESISTOR_B) && getPinValue(CAPACITOR_B) && getPinValue(INDUCTOR_B) && getPinValue(ESR_B) && getPinValue(VOLTAGE_B) && getPinValue(AUTO_B));
        lcd_clear();
        if(!getPinValue(RESISTOR_B)==1)
        {
            resistance= getResistance();
            putsLcd(0,0,"Resistance=");
            sprintf(string,"%u Ohms",resistance);
            putsLcd(1,0,string);
            waitMicrosecond(3000000);
            lcd_clear();
        }
        else if(!getPinValue(CAPACITOR_B)==1)
        {
            putsLcd(0,0,"Capacitance=");
            capacitance= getCapacitance();
            sprintf(string,"%.2f microFarads",capacitance);
            putsLcd(1,0,string);
            waitMicrosecond(3000000);
            lcd_clear();
        }
        else if(!getPinValue(INDUCTOR_B)==1)
        {
            putsLcd(0,0,"Inductance=");
            inductance=getInductance();
            sprintf(string,"%.2f microHenry",inductance);
            putsLcd(1,0,string);
            waitMicrosecond(3000000);
            lcd_clear();
        }
        else if(!getPinValue(ESR_B)==1)
        {
            putsLcd(0,0,"ESR=");
            esr=getEsr();
            sprintf(string,"%.2f Ohms",esr);
            putsLcd(1,0,string);
            waitMicrosecond(3000000);
            lcd_clear();
        }
        else if(!getPinValue(VOLTAGE_B)==1)
        {
            putsLcd(0,0,"Voltage=");
            dut2_voltage=getDUT2Voltage();
            sprintf(string,"%.3f volts",dut2_voltage);
            putsLcd(1,0,string);
            waitMicrosecond(3000000);
            lcd_clear();
        }
        else
        {
            putsLcd(0,0,"auto_mode begun....");
            waitMicrosecond(3000000);
            auto_mode();
            lcd_clear();
        }


    }




   /*
    while(true)
    {
       getsUart0(&data);
       putsUart0(data.buffer);
       putcUart0('\n');
       putcUart0('\r');

       parseFields(&data);

       uint8_t i;
       for (i = 0; i < data.fieldCount; i++)
       {
           putcUart0(data.fieldType[i]);
           putcUart0('\t');
           putsUart0(&data.buffer[data.fieldPosition[i]]);
           putsUart0("\n\r");
       }

       bool valid = false;
       if (isCommand(&data, "r", 0))
       {
           resistance= getResistance();
           sprintf(string,"Resistance = %u Ohms\n\r",resistance);
           putsUart0(string);
           valid = true;
       }

       if (isCommand(&data, "c", 0))
       {
           capacitance= getCapacitance();
           sprintf(string,"Capacitance = %.2f microFarads\n\r",capacitance);
           putsUart0(string);
           valid = true;
       }

       if(isCommand(&data,"voltage",0))
       {
           dut2_voltage=getDUT2Voltage();
           sprintf(string,"Voltage = %.3f volts\n\r",dut2_voltage);
           putsUart0(string);
           valid = true;
       }

       if (isCommand(&data, "esr", 0))
       {
           esr=getEsr();
           sprintf(string,"ESR = %.2f Ohms\n\r",esr);
           putsUart0(string);
           valid = true;
       }

       if (isCommand(&data, "ind", 0))
       {
           inductance=getInductance();
           sprintf(string,"Inductance = %.2f microHenry\n\r",inductance);
           putsUart0(string);
           valid = true;
       }

       if(isCommand(&data, "auto",0))
       {
           auto_mode();
           valid=true;

       }

       if (!valid)
       {
           putsUart0("Invalid command\n\r");
       }

    }
    */

}
