#include <liblcd.h>
#include <libserial.h>

char setup_lcd(){
    gpio_config();
    i2c_config();
    char address = find_lcd_addr();
    lcd_inic(address);     //Inicializar LCD
    return address;
}

// Escreve string no LCD
void write_string(char address, char* str){
    int index = 0;
    while(str[index] != 0){
        write_char(address, str[index]);
        index++;
    }
}

void write_dec12(char address, unsigned int dec){
    dec &= 0xFFF;
    char current;
    int i;
    for(i=1000; i>0; i/=10){
        current = dec/i;
        write_char(address, current+ '0');
        dec -= current*i;
    }
}

// escreve float com uma casa antes e até três casas depois da virg
void write_float(char address, float flo, char precis){

    if(precis < 1){
        precis = 1;
    }

    if(precis > 3){
        precis = 3;
    }

    flo = flo*1000;
    write_char(address, flo/1000+ '0');
    write_char(address, ',');
    int dec = ((int)flo)%1000;

    int i;
    char current;
    for(i = 100; i > 0; i/=10){
        precis--;
        current = dec/i;
        write_char(address, current+ '0');
        dec -= current*i;

        if(precis==0)
            return;
    }
}

// Escreve caractere ASCII no LCD
void write_char(char address, char ch){
    pcf_write(address, (ch & 0xF0) | 0x09);
    pcf_write(address, (ch & 0xF0) | 0x0D);
    pcf_write(address, (ch & 0xF0) | 0x09);

    pcf_write(address, ((ch & 0x0F) << 4) | 0x09);
    pcf_write(address, ((ch & 0x0F) << 4) | 0x0D);
    pcf_write(address, ((ch & 0x0F) << 4) | 0x09);
}

void write_instruction(char address, char instr){
    pcf_write(address, (instr & 0xF0) | 0x08);
    pcf_write(address, (instr & 0xF0) | 0x0C);
    pcf_write(address, (instr & 0xF0) | 0x08);

    pcf_write(address, ((instr & 0x0F) << 4) | 0x08);
    pcf_write(address, ((instr & 0x0F) << 4) | 0x0C);
    pcf_write(address, ((instr & 0x0F) << 4) | 0x08);
}

// Seta o endereço correto do chip-servo
char find_lcd_addr(void){

    //Para cada endereço válido se o PCF_8574 está respondendo
    if (pcf_teste(PCF_ADR1)==TRUE){
        // o servo com o endereço PCF_ADR1 respondeu
        led_VD();       //Houve ACK, tudo certo
        return (char) PCF_ADR1;
    }

    if(pcf_teste(PCF_ADR2)==TRUE){
        // o servo com o endereço PCF_ADR2 respondeu
        led_VD();       //Houve ACK, tudo certo
        return (char) PCF_ADR2;
    }

    // niguém respondeu: tem algo errado
    led_VM();           //Indicar que não houve ACK
    while(TRUE);        //Travar
}

void retornar(char address){
    write_instruction(address, 0x02);
}

void move_cursor_right(char address){
    write_instruction(address, 0x14);
}

void move_cursor_left(char address){
    write_instruction(address, 0x10);
}

// lin 0-1, col 0-15
void set_cursor(char address, char lin, char col){
    write_instruction(address, 0x80 | lin << 6 | col);
}

void clear_lcd(char address){
    write_instruction(address, 0x01);
}

void light_on(char address){
    pcf_write(address, 0x08);
}

void light_off(char address){
    pcf_write(address, 0x00);
}
