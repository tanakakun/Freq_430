/**********************************************************************
           The Apache License 2.0

   Copyright {2015} {Yeonji}

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/


/*Please adjust the following macros according to actual*/

/*Frequency of crystal*/
/*This will be used for caculating*/
#define FOSC (8000000)

/*The clock cycle for Timer*/
/*This may different from FOSC accroding to divider*/
#define FCLK (FOSC)

/*The IO used for interrupt*/
#define FREQ_INT   BIT3

#include <msp430.h>

/*Wave counter*/
unsigned int Period_CNT=0;

/*Gate time, 10 wave cycle for default*/
         int P_CNTGATE=10;
         int CNT_GATE=10;

/*use a variable to expend timer bits*/
unsigned int TH=0;

/*No signal or Frequence too low*/
char No_Signal=1;

union LongChar
{
    unsigned long int Long;
    struct Byte4
    {
        unsigned int ByteL;
        unsigned int ByteH;
    } Bytes;
} Period;


void Freq_Init()
{
    /*TimerA Init*/

    /*Set system clock as 8MHz*/
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;

    /*Set mode for TimerA*/
    TACTL = TASSEL_2 + ID_0 + MC_1 + TACLR;
        /*TASSEL_2:Set clock source as SMCLK*/
        /*ID_3:Set no input divider*/
        /*MC_1:Set mode as count up to TACCR0*/

    /*Set count up level*/
    TACCR0 = 65536-1;

    /*Enable capture interrupt*/
    TACCTL0 |= CCIE;

    /*IO initialization*/

    /*Set as input*/
    P1DIR &= ~FREQ_INT;

    /*Enable pull resistor*/
    P1REN |= FREQ_INT;

    /*Set resistor as pull up*/
    P1OUT |= FREQ_INT;

    /*Set positive edge will toggle interrupt*/
    P1IES |= FREQ_INT;

    /*Enable Port1 interrupt*/
    P1IE  |= FREQ_INT;

    _EINT();
}

/*PORT1 Interrupt Service*/
void __attribute__((interrupt(PORT1_VECTOR))) EINT_ISR(void)
{
    /*Count number of wave cycles*/
    Period_CNT++;

    /*Caculate cycle when wave numbers up to gate*/
    if(Period_CNT>=CNT_GATE)
    {
        /*There maybe some logic error but now it also can use*/
        /*I hope some one will help me to fix it*/

        TACCR0 = 0;

        /*Get the Timer Value*/
        /*Total 4bit*/
        /*Using union will speed up this*/
        Period.Bytes.ByteL=TAR;
        Period.Bytes.ByteH=TH;

        /*Clear Timer*/
        TAR=0;
        TH=0;

        /*Remuse Timer*/
        TACCR0 = 65536-1;

        /*Clear no signal flag*/
        No_Signal=0;

        /*Clear pulse count*/
        Period_CNT=0;

        /*Save pulse number for caculating*/
        /*Adjust for a more rational number*/
        P_CNTGATE=CNT_GATE;

        if(Period.Bytes.ByteH<4)  CNT_GATE+=3;
        if(Period.Bytes.ByteH>8)  CNT_GATE-=3;

        /*Limit for count number*/
        if(CNT_GATE<1) CNT_GATE=1;
        if(CNT_GATE>1024) CNT_GATE=1024;
    }
    /*Clear Interrupr Flag*/
    P1IFG &= ~FREQ_INT;
}

/*Timer1 Interrupt Service*/
void __attribute__((interrupt(TIMER0_A0_VECTOR))) Timer_ISR(void)
{
    /*Use TH to expand Timer to 32bit*/
    TH++;

    /*No signal for more than 0.32s*/
    /*Time=(65536*40)/FCLK*/
    if (TH>=40)
    {
        /*Set no signal flag*/
        No_Signal=1;

        /*Frequence too low may cause this too*/
        /*Set gate for 1 wave to fit low frequency*/
        P_CNTGATE=CNT_GATE;
        CNT_GATE=1;

        /*Clear Timer*/
        TH=0;
    }
}

float Freq_GetVal()
{
    /*Buffer for period time*/
    unsigned long int T;

    /*Buffer for count*/
    int N_CNTGATE;

    /*Buffer for frequency*/
    /*For less memory may change this to a fixed*/
    float Freq;

    /*If No_Signal flagged, return 0*/
    if(No_Signal) return(0);

    /*Disable interrupt to pretect Period.Long*/
    _DINT();

    /*Buffer Period time*/
    T=Period.Long;

    _EINT();

    /*Get count number*/
    N_CNTGATE=P_CNTGATE;

    /*Frequence=N*(1/T)*/
    Freq=(float)FCLK*N_CNTGATE/T;
    
    return(Freq);
}
