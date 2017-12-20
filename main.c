#include <msp430.h> 

unsigned int adc_value1 = 0;
unsigned int adc_value2 = 0;
unsigned int adc_value3 = 0;
unsigned int adc_value4 = 0;
unsigned int in = 0;

unsigned int border_threshold = 0x0C00;
unsigned int bot_threshold = 0x000; //need more testing to confirm this value. Will be changed later.

int left_motor = 999;
int right_motor = 999;

void ADCInitialize(void)
{
    ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT02; // Turn on ADC12, set sampling time
    ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;       // Use sampling timer, single sequence
    ADC12MCTL0 = ADC12INCH_0;                 // ref+=AVcc, channel = A0
    ADC12MCTL1 = ADC12INCH_1;                 // ref+=AVcc, channel = A1
    ADC12MCTL2 = ADC12INCH_2;                 // ref+=AVcc, channel = A2
    ADC12MCTL3 = ADC12INCH_3;                 // ref+=AVcc, channel = A3
    ADC12MCTL4 = ADC12INCH_4 + ADC12EOS;        // ref+=AVcc, channel = A4, end seq.
    ADC12IE = 0x10;                           // Enable ADC12IFG.3
    ADC12CTL0 |= ADC12ENC;                    // Enable conversions
    /*Sets ADC Pin to NOT GPIO*/
    P6SEL |= BIT1 + BIT2 + BIT3 + BIT4;                            // P6.0 ADC option select
    P6DIR &= ~(BIT1 + BIT2 + BIT3 + BIT4);
    P6REN |= BIT1 + BIT2 + BIT3 + BIT4;
    P6OUT &= ~(BIT1 + BIT2 + BIT3 + BIT4);
}

void PWMInitialize(void){
    TA0CCR0 = 1000 - 1;                          // PWM Period
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCR1 = 0;                            // CCR1 PWM duty cycle
    TA0CCTL2 = OUTMOD_7;                      // CCR2 reset/set
    TA0CCR2 = 0;                            // CCR2 PWM duty cycle
    TA0CTL = TASSEL_2 + MC_1 + TACLR;         // SMCLK, up mode, clear TAR
}

void initializeUART(void)       // from Lab 1 example code
{
    P4SEL |= BIT4;          // UART TX
    P4SEL |= BIT5;          // UART RX
    UCA1CTL1 |= UCSWRST;    // Resets state machine
    UCA1CTL1 |= UCSSEL_2;   // SMCLK
    UCA1BR0 = 6;            // 9600 Baud Rate
    UCA1BR1 = 0;            // 9600 Baud Rate
    UCA1MCTL |= UCBRS_0;    // Modulation
    UCA1MCTL |= UCBRF_13;   // Modulation
    UCA1MCTL |= UCOS16;     // Modulation
    UCA1CTL1 &= ~UCSWRST;   // Initializes the state machine
    UCA1IE |= UCRXIE;       // Enables USCI_A0 RX Interrupt

}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    ADCInitialize();            // Initializes ADC function
    PWMInitialize();           // Initializes PWM function
    initializeUART();               // Enable USCI_A0 RX interrupt
    __bis_SR_register(GIE);

    P1DIR |= BIT4 + BIT5;      // Set pins to output (for motors) P1.4 and P1.5
    P2DIR |= BIT0 + BIT2;      // Set pints to output (for motors) P2.0 and P2.2

    P1DIR |= BIT2+BIT3;        // P1.2 and P1.3 output select for motors
    P1SEL |= BIT2+BIT3;        // P1.2 and P1.3 options select for motors


    __delay_cycles(5400000);   // Five second initial delay
    while (1)
    {
        __delay_cycles(10000);       //To avoid errors of ADC updating too quick
         ADC12CTL0 |= ADC12ENC;      //Start sampling/conversion
         ADC12CTL0 |= ADC12SC;
        __bis_SR_register(GIE);      //Enables interrupt


            if(adc_value1 <= border_threshold && adc_value2 <= border_threshold){  //initial condition, no white detected begin looking for bot
                    if(adc_value3 <= bot_threshold && adc_value4 <= bot_threshold){ //does'nt detect bot, spin at half speed until bot is detected
                        P2OUT &= ~BIT0;
                        P2OUT |= BIT2; // Sets direction for H-bridge
                        TA0CCR2 = (left_motor * -1)/2;//Speed of motor CCR/1000
                        P1OUT &= ~BIT5;
                        P1OUT |= BIT4; // Sets direction for H-bridge
                        TA0CCR1 = right_motor/2;
                    }
                    else { //either sensor detects other bot, move forward 100%
                        P2OUT |= BIT0;
                        P2OUT &= ~BIT2;// Sets direction for H-bridge
                        TA0CCR2 = left_motor;//Speed of motor CCR/1000
                        P1OUT &= ~BIT5;
                        P1OUT |= BIT4; // Sets direction for H-bridge
                        TA0CCR1 = right_motor;
                    }

                    }
            else if(adc_value1 > border_threshold && adc_value2 <= border_threshold){ //if the right sensor detects white
                P2OUT &= ~BIT0;
                P2OUT |= BIT2; // Sets direction for H-bridge
                TA0CCR2 = (left_motor * -1);//Speed of motor CCR/1000
                P1OUT &= ~BIT5;
                P1OUT |= BIT4; // Sets direction for H-bridge
                TA0CCR1 = right_motor;
                __delay_cycles(1500000); //delay to give time for bot to fully turn around
            }
            else if(adc_value2 > border_threshold && adc_value1 <= border_threshold){ //if the left sensor detects white
                P2OUT |= BIT0;
                P2OUT &= ~BIT2; // Sets direction for H-bridge
                TA0CCR2 = left_motor % 1000;//Speed of motor CCR/1000
                P1OUT |= BIT5;
                P1OUT &= ~BIT4; // Sets direction for H-bridge
                TA0CCR1 = (right_motor * -1);
                __delay_cycles(1500000); //delay to give time for bot to fully turn around
            }
           else if(adc_value1 > border_threshold && adc_value2 > border_threshold){ //condition if both sensors detect white
               P2OUT &= ~BIT0;
               P2OUT |= BIT2;// Sets direction for H-bridge
               TA0CCR2 = left_motor;//Speed of motor CCR/1000
               P1OUT |= BIT5;
               P1OUT &= ~BIT4; // Sets direction for H-bridge
               TA0CCR1 = right_motor;
                              }
           }


}

/*ADC Interrupt*/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
switch(__even_in_range(ADC12IV,34))
    {
    case  0: break;                           // Vector  0:  No interrupt
    case  2: break;                           // Vector  2:  ADC overflow
    case  4: break;                           // Vector  4:  ADC timing overflow
    case  6: break;                                  // Vector  6:  ADC12IFG0
    case  8: break;                           // Vector  8:  ADC12IFG1
    case 10: break;                           // Vector 10:  ADC12IFG2
    case 12: break;                           // Vector 12:  ADC12IFG3
    case 14:
        adc_value1 = ADC12MEM1; //changes duty cycle
        adc_value2 = ADC12MEM2; //changes duty cycle
        adc_value3 = ADC12MEM3; //changes duty cycle
        adc_value4 = ADC12MEM4; //changes duty cycle
        __bic_SR_register_on_exit(LPM0_bits);
        break;                           // Vector 14:  ADC12IFG4
    case 16: break;                           // Vector 16:  ADC12IFG5
    case 18: break;                           // Vector 18:  ADC12IFG6
    case 20: break;                           // Vector 20:  ADC12IFG7
    case 22: break;                           // Vector 22:  ADC12IFG8
    case 24: break;                           // Vector 24:  ADC12IFG9
    case 26: break;                           // Vector 26:  ADC12IFG10
    case 28: break;                           // Vector 28:  ADC12IFG11
    case 30: break;                           // Vector 30:  ADC12IFG12
    case 32: break;                           // Vector 32:  ADC12IFG13
    case 34: break;                           // Vector 34:  ADC12IFG14
    default: break;
    }
}


#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch(__even_in_range(UCA0IV,USCI_UCTXIFG)){
    case 0: break; //if vector is zero, there is no interrupt
    case 1:
    if(UCA0RXBUF == 117)  // Input 'u' from keyboard to get data
        in = 0;
    else if(UCA0RXBUF == 100)  // Input 'd' from keyboard to get data
        in = 1;
    else if(UCA0RXBUF == 108)  // Input 'l' from keyboard to get data
        in = 2;
    else if(UCA0RXBUF == 114)  // Input 'r' from keyboard to get data
        in = 3;
    else
        UCA0TXBUF = 1; // Show that the wrong button is pressed

        switch(in){
        case 0: // up/forward
            P2OUT |= BIT0;
            P2OUT &= ~BIT2;// Sets direction for H-bridge
            TA0CCR2 = left_motor;//Speed of motor CCR/1000
            P1OUT &= ~BIT5;
            P1OUT |= BIT4; // Sets direction for H-bridge
            TA0CCR1 = right_motor;
            break;
        case 1: // down/backwards
            P2OUT &= ~BIT0;
            P2OUT |= BIT2;// Sets direction for H-bridge
            TA0CCR2 = left_motor;//Speed of motor CCR/1000
            P1OUT |= BIT5;
            P1OUT &= ~BIT4; // Sets direction for H-bridge
            TA0CCR1 = right_motor;
            break;
        case 2: // left
            P2OUT &= ~BIT0;
            P2OUT |= BIT2; // Sets direction for H-bridge
            TA0CCR2 = (left_motor * -1)/2;//Speed of motor CCR/1000
            P1OUT &= ~BIT5;
            P1OUT |= BIT4; // Sets direction for H-bridge
            TA0CCR1 = right_motor/2;
            break;
        case 3: // right
            P2OUT &= ~BIT0;
            P2OUT |= BIT2; // Sets direction for H-bridge
            TA0CCR2 = left_motor/2;//Speed of motor CCR/1000
            P1OUT &= ~BIT5;
            P1OUT |= BIT4; // Sets direction for H-bridge
            TA0CCR1 = (right_motor * -1)/2;
            break;
        default: UCA0TXBUF = UCA0RXBUF;
            break;
        }
        break;
    }
}
