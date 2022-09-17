// Ler sinais de um joystick
// Armazenar em ADC12MEM1 e 2
// Conversao em 100 Hz por canal
// Usar TA0.1

#include <msp430.h> 
#include <liblcd.h>
#include <libserial.h>

#define TRUE 1
#define QTD 4 //Nr de amostras por canal

#define MCU_CLOCK                   1048576
#define HALF_MCU_CLOCK              529288
#define A_CLOCK                     32768

#define PWM_SERVO_FREQ              50     // 50Hz
#define PWM_SERVO_PERIOD            (MCU_CLOCK / PWM_SERVO_FREQ)

// PWM timer values to servo with
//  max position of servo in degrees = 180 degrees
//  minimum servo duty cicle - 0.5ms
//  maximum servo duty cicle - 2.5ms
//  using the MCU_CLOCK with 6th output mode

unsigned int degree_to_period[] = { 2621, 2609, 2598, 2586, 2574, 2563, 2551,
                                    2539, 2528, 2516, 2504, 2493, 2481, 2470,
                                    2458, 2446, 2435, 2423, 2411, 2400, 2388,
                                    2376, 2365, 2353, 2341, 2330, 2318, 2306,
                                    2295, 2283, 2272, 2260, 2248, 2237, 2225,
                                    2213, 2202, 2190, 2178, 2167, 2155, 2143,
                                    2132, 2120, 2108, 2097, 2085, 2073, 2062,
                                    2050, 2038, 2027, 2015, 2004, 1992, 1980,
                                    1969, 1957, 1945, 1934, 1922, 1910, 1899,
                                    1887, 1875, 1864, 1852, 1840, 1829, 1817,
                                    1806, 1794, 1782, 1771, 1759, 1747, 1736,
                                    1724, 1712, 1701, 1689, 1677, 1666, 1654,
                                    1642, 1631, 1619, 1607, 1596, 1584, 1572,
                                    1561, 1549, 1538, 1526, 1514, 1503, 1491,
                                    1479, 1468, 1456, 1444, 1433, 1421, 1409,
                                    1398, 1386, 1374, 1363, 1351, 1340, 1328,
                                    1316, 1305, 1293, 1281, 1270, 1258, 1246,
                                    1235, 1223, 1211, 1200, 1188, 1176, 1165,
                                    1153, 1141, 1130, 1118, 1106, 1095, 1083,
                                    1072, 1060, 1048, 1037, 1025, 1013, 1002,
                                    990, 978, 967, 955, 943, 932, 920, 908, 897,
                                    885, 874, 862, 850, 839, 827, 815, 804, 792,
                                    780, 769, 757, 745, 734, 722, 710, 699, 687,
                                    675, 664, 652, 640, 629, 617, 606, 594, 582,
                                    571, 559, 547, 536, 524 };

void ADC_config(void);
void USCI_A1_config(void);
void TA0_config(void);
void GPIO_config(void);
void servo_config(void);

void config_dma_adc(void);
void config_dma_servo(void);
void config_dma_serial(void);

void write_base_lcd(char address);
void write_joyst_lcd(char address, int avg_int, float avg_flt, float max,
                     float min);
void write_string_uart(char string[]);
void write_base_uart(void);
void write_joyst_uart(int avg_int_a1, float avg_flt_a1, int avg_int_a2,
                      float avg_flt_a2);
void write_servo(float measure);

void int_to_string_4_digits(char string[], int number);
void float_to_string(char string[], float flo, char precis);

// Variaveis globais
volatile int v[2*QTD];  //Vetor
                        // pares -> v_x (A1)
                        // impares -> v_y (A2)
volatile int media_x;   // media vetor x
volatile int media_y;   // media vetor y

volatile int chanel = 1;   // canal que esta sendo mostrado

volatile int uart_counter = 0;

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
    USCI_A1_config();
    servo_config();

    config_dma_adc();
    config_dma_servo();
    config_dma_serial();

    char address = setup_lcd();
    write_base_lcd(address);

    write_base_uart();

    __enable_interrupt();

    while (TRUE)
    {
        ADC12CTL0 |= ADC12ENC; //Habilitar ADC

        while (DMAIFG == 0)
            ; //Esperar flag - conversoes

        // calcula media
        media_x = (v[0] + v[2] + v[4] + v[6]);
        media_x = media_x >> 2;
        media_y = (v[1] + v[3] + v[5] + v[7]);
        media_y =  media_y >> 2;

        ADC12CTL0 &= ~ADC12ENC; //Parar as conversoes

        volt_x = (media_x * 3.3) / 4095;
        volt_y = ((4095 - media_y) * 3.3) / 4095;

        if (volt_x < min_x)
            min_x = volt_x;
        if (volt_y < min_y)
            min_y = volt_y;
        if (volt_x > max_x)
            max_x = volt_x;
        if (volt_y > max_y)
            max_y = volt_y;

        if (chanel == 1)
        {
            // atualizar o LCD com o canal A1
            write_joyst_lcd(address, media_x, volt_x, max_x, min_x);
            // atualizar servo com o canal A1
            write_servo(volt_x);
        }

        if (chanel == 2)
        {
            // atualizar o LCD com o canal A2
            write_joyst_lcd(address, media_y, volt_y, max_y, min_y);
            // atualizar servo com o canal A2
            write_servo(volt_y);
        }
        // Atualizar UART
        write_joyst_uart(media_x, volt_x, media_y, volt_y);

        if ((P6IN & BIT3) == 0)
        {
            __delay_cycles(10000); // debouncing
            chanel = (chanel == 1) ? 2 : 1;

            min_x = 3.3;
            max_x = 0;
            min_y = 3.3;
            max_y = 0;
        }
    }
}

void write_base_lcd(char address)
{
    set_cursor(address, 0, 0);
    write_string(address, "An=d.dddV   NNNN");

    set_cursor(address, 1, 0);
    write_string(address, "Mn=d.dd  Mn=d.dd");
}

void write_joyst_lcd(char address, int avg_int, float avg_flt, float max,
                     float min)
{
    set_cursor(address, 0, 1);
    write_char(address, chanel + '0');

    set_cursor(address, 0, 3);
    write_float(address, avg_flt, 3);

    set_cursor(address, 0, 12);
    write_dec12(address, avg_int);

    set_cursor(address, 1, 3);
    write_float(address, min, 2);

    set_cursor(address, 1, 12);
    write_float(address, max, 2);
}

void write_string_uart(char string[])
{
    int i = 0;
    while (string[i] != 0)
    {
        UCA1TXBUF = string[i];
        while ((UCA1IFG & UCTXIFG) == 0)
            ; //Esperar TXIFG=1
        i++;
    }
}

void write_base_uart(void)
{
    char string[] =
            "cont: ---Canal A1---- ---Canal A2----                                  \n";
    write_string_uart(string);
}

void write_joyst_uart(int avg_int_a1, float avg_flt_a1, int avg_int_a2,
                      float avg_flt_a2)
{
    char string_4[] = "....";
    char string_5[] = ".....";

    int_to_string_4_digits(string_4, uart_counter);
    write_string_uart(string_4);
    write_string_uart(": ");

    int_to_string_4_digits(string_4, avg_int_a1);
    write_string_uart(string_4);
    write_string_uart(" ---> ");
    float_to_string(string_5, avg_flt_a1, 3);
    write_string_uart(string_5);
    write_string_uart("V     ");

    int_to_string_4_digits(string_4, avg_int_a2);
    write_string_uart(string_4);
    write_string_uart(" ---> ");
    float_to_string(string_5, avg_flt_a2, 3);
    write_string_uart(string_5);
    write_string_uart("V\n");

    uart_counter++;
    if (uart_counter >= 10000)
        uart_counter = 0;
}

void write_servo(float measure)
{
    int servo_position = (measure * 10 * 180) / 50;
    TA2CCR2 = degree_to_period[servo_position];
}

void ADC_config(void)
{
    ADC12CTL0 &= ~ADC12ENC; //Desabilitar para configurar
    ADC12CTL0 = ADC12ON; //Ligar ADC
    ADC12CTL1 = ADC12CONSEQ_3 | //Modo sequencia de canais
            ADC12SHS_1 | //Selecionar TA0.1
            ADC12CSTARTADD_1 | //Resultado em ADC12MEM1
            ADC12SSEL_3; //ADC12CLK = SMCLK
    ADC12CTL2 = ADC12RES_2; //Modo 12 bits

    ADC12MCTL1 = ADC12SREF_0 | ADC12INCH_1; //Config MEM 1,3,5,7
    ADC12MCTL7 = ADC12MCTL5 = ADC12MCTL3 = ADC12MCTL1;
    ADC12MCTL2 = ADC12SREF_0 | ADC12INCH_2; //Config MEM 2,4,6,8
    ADC12MCTL8 = ADC12MCTL6 = ADC12MCTL4 = ADC12MCTL2;
    ADC12MCTL8 |= ADC12EOS; //MEM8 = ultima ADC12EOS

    P6SEL |= BIT2 | BIT1; // Desligar digital de P6.2,1
    ADC12CTL0 |= ADC12ENC; //Habilitar ADC12
//    ADC12IE |= ADC12IE2; //Hab interrupcao MEM2aa
}

// Configurar USCI_A0
void USCI_A1_config(void)
{
    UCA1CTL1 = UCSWRST; //RST=1 para USCI_A1
    UCA1CTL0 = 0; //sem paridade, 8 bits, 1 stop, modo UART
    UCA1STAT = UCLISTEN; //Loop Back
    UCA1BRW = 3; // Divisor
    UCA1MCTL = UCBRS_3; //Modulador = 3 e UCOS=0
    P4SEL |= BIT3 | BIT0; //Disponibilizar P4.3 e P4.0
    PMAPKEYID = 0X02D52; //Liberar mapeamento de P4
    P4MAP0 = PM_UCA1TXD; //P4.0 = TXD
    P4MAP3 = PM_UCA1RXD; //P4.3 = RXD
    UCA1CTL1 = UCSSEL_1; //RST=0 e Selecionar ACLK
    // UCA1IE = UCRXIE; //Hab. Interrup recepção
}

void TA0_config(void)
{
    TA0CTL = TASSEL_1 | MC_1;
    TA0CCTL1 = OUTMOD_6; //Out = modo 6
    TA0CCR0 = 32767 / 32; //32 Hz (16 Hz por canal)
    TA0CCR1 = TA0CCR0 / 2; //Carga 50%
}

void servo_config(void)
{
    // Timer A2.2 -> moves servo
    TA2CTL = TASSEL_2 | MC_1;           // SMCLK, upmode
    TA2CCTL2 = OUTMOD_6;                // TA2CCR2 toggle/reset
    TA2CCR0 = PWM_SERVO_PERIOD - 1;     // PWM Period - 50Hz
    TA2CCR2 = 0;                        // TA2CCR2 PWM Duty Cycle
    P2DIR |= BIT5;                      // P2.5 = output
    P2SEL |= BIT5;                      // P2.5 = TA2 output
}

void config_dma_adc(void)
{
    DMACTL0 |= DMA0TSEL_24; // Disparo = ADC12IFG
    DMA0CTL = DMADT_2    | //Modo rajada com repeticao
            DMADSTINCR_3 | //Ponteiro destino incrementando
            DMASRCINCR_3 ; //Ponteiro fonte incrementando
            //DMAIE;         //Habilita interrupcao
                           //Destino opera com words
                           //Fonte opera com words
    DMA0SA = &ADC12MEM1;
    DMA0DA = v; //Destino = v
    DMA0SZ = 8; //Quantidade de transferências
    DMA0CTL |= DMAEN; //Habilitar DMA0
}

void config_dma_servo(void){
    // TO DO
}

void config_dma_serial(void){
    // TO DO
}

void GPIO_config(void)
{
    P1DIR |= BIT0;  //Led vermelho
    P1OUT &= ~BIT0;

    P4DIR |= BIT7;  //Led verde
    P4OUT &= ~BIT7;

    P6DIR &= ~BIT3; //P6.3 = SW
    P6REN |= BIT3;
    P6OUT |= BIT3;
}

void int_to_string_4_digits(char string[], int number)
{
    string[0] = '0';
    string[0] = '1';
    string[0] = '2';
    string[0] = '3';

    char current;
    int i, j = 0;
    for (i = 1000; i > 0; i /= 10)
    {
        current = number / i;
        string[++j] = current + '0';
        number -= current * i;
    }
}

void float_to_string(char string[], float flo, char precis)
{
    string[0] = 'd';
    string[1] = 'd';
    string[2] = 'd';
    string[3] = ',';
    string[4] = 'd';

    if (precis < 1)
    {
        precis = 1;
    }

    if (precis > 3)
    {
        precis = 3;
    }

    flo = flo * 1000;
    string[4] = flo / 1000 + '0';
    int dec = ((int) flo) % 1000;

    int i;
    int j = 0;
    char current;
    for (i = 100; i > 0; i /= 10)
    {
        precis--;
        current = dec / i;
        string[++j] = current + '0';
        dec -= current * i;

        if (precis == 0)
            return;
    }

    return;
}
