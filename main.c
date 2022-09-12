// Ler sinais de um joystick
// Armazenar em ADC12MEM1 e 2
// Conversão em 100 Hz por canal
// Usar TA0.1

#include <msp430.h> 
#include <liblcd.h>
#include <libserial.h>

#define TRUE 1
#define QTD 4 //Nr de amostras por canal

void ADC_config(void);
void TA0_config(void);
void GPIO_config(void);

void write_joyst_lcd(char address, int avg_int, float avg_flt,
                     float max, float min);

// Variáveis globais
volatile int vrx[QTD];  //Vetor x
volatile int vry[QTD];  //Vetor y
volatile int media_x;   // media vetor x
volatile int media_y;   // media vetor y
volatile int index;

volatile int chanel = 1;// canal que está sendo mostrado

volatile int flag;      // flag

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer

    float volt_x = 0;
    float volt_y = 0;

    float min_x = 3.3;
    float max_x = 0;
    float min_y = 3.3;
    float max_y = 0;

    GPIO_config();
    TA0_config();
    ADC_config();
    char address = setup_lcd();
    __enable_interrupt();
    while (TRUE)
    {
        ADC12CTL0 |= ADC12ENC; //Habilitar ADC

        while (flag == 0)
            ; //Esperar flag - calculo da media

        ADC12CTL0 &= ~ADC12ENC; //Parar as conversões

        flag = 0;
        index = 0; //Zerar indexador

        volt_x = (media_x * 3.3)/4095;
        volt_y = ((1024 - media_y) * 3.3)/1024;

        if(volt_x < min_x)
            min_x = volt_x;
        if(volt_y < min_y)
            min_y = volt_y;
        if(volt_x > max_x)
            max_x = volt_x;
        if(volt_y > max_y)
            max_y = volt_y;

        if(chanel==1){
            // atualizar o LCD com o canal A1
            write_joyst_lcd(address, media_x, volt_x,
                                max_x, min_x);
            // atualizar UART  com o canal A1
            // atualizar servo com o canal A1
        }

        if(chanel==2){
            // atualizar o LCD com o canal A2
            write_joyst_lcd(address, media_y, volt_y,
                                max_y, min_y);
            // atualizar UART  com o canal A2
            // atualizar servo com o canal A2
        }

        if((P6IN&BIT3) == 0){
            __delay_cycles(10000); // debouncing
            chanel = (chanel == 1)? 2 : 1;

            min_x = 3.3;
            max_x = 0;
            min_y = 3.3;
            max_y = 0;
        }
    }
}

//#pragma vector = 54
#pragma vector = ADC12_VECTOR
__interrupt void adc_int(void)
{
    vrx[index] = ADC12MEM1;
    vry[index] = ADC12MEM2;
    index++;

    if(index == QTD){
        flag = 1;

        // calcula média
        media_x = (vrx[0]+vrx[1]+vrx[2]+vrx[3]);
        media_x /= 4;
        media_y = (vry[0]+vry[1]+vry[2]+vry[3])/4;
        media_y /= 4;
    }
}

void write_joyst_lcd(char address, int avg_int, float avg_flt,
                     float max, float min){
    clear_lcd(address);
    retornar(address);
    write_char(address, 'A');
    write_char(address, chanel+ '0');
    write_char(address, '=');
    write_float(address, avg_flt, 3);
    set_cursor(address, 0, 12);
    write_dec12(address, avg_int);

    set_cursor(address, 1, 0);
    write_string(address, "Mn=");
    write_float(address, min, 2);

    set_cursor(address, 1, 9);
    write_string(address, "Mx=");
    write_float(address, max, 2);
}

void ADC_config(void)
{
    ADC12CTL0 &= ~ADC12ENC; //Desabilitar para configurar
    ADC12CTL0 = ADC12ON; //Ligar ADC
    ADC12CTL1 = ADC12CONSEQ_3 | //Modo sequência de canais
            ADC12SHS_1 | //Selecionar TA0.1
            ADC12CSTARTADD_1 | //Resultado em ADC12MEM1
            ADC12SSEL_3; //ADC12CLK = SMCLK
    ADC12CTL2 = ADC12RES_2; //Modo 12 bits
    ADC12MCTL1 = ADC12SREF_0 | ADC12INCH_1; //Config MEM1
    ADC12MCTL2 = ADC12EOS | ADC12SREF_0 | ADC12INCH_2; //MEM2 = última
    P6SEL |= BIT2 | BIT1; // Desligar digital de P6.2,1
    ADC12CTL0 |= ADC12ENC; //Habilitar ADC12
    ADC12IE |= ADC12IE2; //Hab interrupção MEM2
}

void TA0_config(void)
{
    TA0CTL = TASSEL_1 | MC_1;
    TA0CCTL1 = OUTMOD_6; //Out = modo 6
    TA0CCR0 = 32767 / 32; //32 Hz (16 Hz por canal)
    TA0CCR1 = TA0CCR0 / 2; //Carga 50%
}

void GPIO_config(void)
{
    P1DIR |= BIT0;  //Led vermelho
    P1OUT &= ~BIT0;
    P6DIR &= ~BIT3; //P6.3 = SW
    P6REN |= BIT3;
    P6OUT |= BIT3;
}
