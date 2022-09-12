/*
 * libserial.h
 *
 */

#ifndef LIBSERIAL_H_
#define LIBSERIAL_H_

#define TRUE    1
#define FALSE   0

// Definiçãoo do endereço do PCF_8574
#define PCF_ADR1 0x3F
#define PCF_ADR2 0x27

#define BR_100K    11  //SMCLK/100K = 11
#define BR_50K     21  //SMCLK/50K  = 21
#define BR_10K    105  //SMCLK/10K  = 105

void lcd_inic(char lcd_address);
void lcd_aux(char dado);
int pcf_read(char lcd_address);
void pcf_write(char lcd_address, char dado);
int pcf_teste(char adr);
void led_vd(void);
void led_VD(void);
void led_vm(void);
void led_VM(void);
void i2c_config(void);
void gpio_config(void);
void delay(long limite);

#endif /* LIBSERIAL_H_ */
