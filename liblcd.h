/*
 * liblcd.h
 *
 *  Created on: 9 de set de 2022
 *      Author: RAFAEL BARBOSA
 */

#ifndef LIBLCD_H_
#define LIBLCD_H_

#define TRUE    1
#define FALSE   0

// Definiçãoo do endereço do PCF_8574
#define PCF_ADR1 0x3F
#define PCF_ADR2 0x27

char setup_lcd(void);
char find_lcd_addr(void);
void write_char(char address, char ch);
void write_string(char address, char* str);
void write_dec12(char address, unsigned int dec);
void write_float(char address, float flo, char precis);
void write_instruction(char address, char instr);

void retornar(char address);
void move_cursor_right(char address);
void move_cursor_left(char address);
void set_cursor(char address, char lin, char col);
void clear_lcd(char address);
void light_on(char address);
void light_off(char address);

#endif /* LIBLCD_H_ */
