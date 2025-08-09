#include "SoftWire.h"

SoftWire LCD_Wire(32, 33); // SDA SCL
char swTxBuffer[16];
#define SLAVE_ADDRESS_LCD 0x27 // change this according to ur setup

void lcd_send_cmd (char cmd)
{
  char data_u, data_l;
	uint8_t data_t[4];
	data_u = (cmd&0xf0);
	data_l = ((cmd<<4)&0xf0);
	data_t[0] = data_u|0x0C;  //en=1, rs=0
	data_t[1] = data_u|0x08;  //en=0, rs=0
	data_t[2] = data_l|0x0C;  //en=1, rs=0
	data_t[3] = data_l|0x08;  //en=0, rs=0
	LCD_Wire.beginTransmission(SLAVE_ADDRESS_LCD);  // Bắt đầu gửi
  LCD_Wire.write(data_t[0]);
  LCD_Wire.write(data_t[1]); 
  LCD_Wire.write(data_t[2]); 
  LCD_Wire.write(data_t[3]);                       // Gửi 4 byte từ mảng data_t                     // Gửi 4 byte từ mảng data_t
  LCD_Wire.endTransmission();  
}

void lcd_send_data (char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data&0xf0);
	data_l = ((data<<4)&0xf0);
	data_t[0] = data_u|0x0D;  //en=1, rs=0
	data_t[1] = data_u|0x09;  //en=0, rs=0
	data_t[2] = data_l|0x0D;  //en=1, rs=0
	data_t[3] = data_l|0x09;  //en=0, rs=0
	LCD_Wire.beginTransmission(SLAVE_ADDRESS_LCD);  // Bắt đầu gửi
  LCD_Wire.write(data_t[0]);
  LCD_Wire.write(data_t[1]); 
  LCD_Wire.write(data_t[2]); 
  LCD_Wire.write(data_t[3]);                       // Gửi 4 byte từ mảng data_t
  LCD_Wire.endTransmission();  
}

void lcd_init (void)
{
  LCD_Wire.setTxBuffer(swTxBuffer, sizeof(swTxBuffer));
  LCD_Wire.begin();
	lcd_send_cmd (0x33); /* set 4-bits interface */
	lcd_send_cmd (0x32);
	delay(50);
	lcd_send_cmd (0x28); /* start to set LCD function */
	delay(50);
	lcd_send_cmd (0x01); /* clear display */
	delay(50);
	lcd_send_cmd (0x06); /* set entry mode */
	delay(50);
	lcd_send_cmd (0x0c); /* set display to on */	
	delay(50);
	lcd_send_cmd (0x02); /* move cursor to home and set data address to 0 */
	delay(50);
	lcd_send_cmd (0x80);
}

void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++);
}

void lcd_clear_display (void)
{
	lcd_send_cmd (0x01); //clear display
}

void lcd_goto_XY(int row, int col) {
  uint8_t pos_Addr = 0x80;  // base address

  switch (row) {
    case 0:
      pos_Addr += 0x00 + col;
      break;
    case 1:
      pos_Addr += 0x40 + col;
      break;
    case 2:
      pos_Addr += 0x14 + col;
      break;
    case 3:
      pos_Addr += 0x54 + col;
      break;
    default:
      pos_Addr += 0x00 + col;  // default dòng 0
      break;
  }

  lcd_send_cmd(pos_Addr);
}
void lcd_print_num(float num, uint8_t decimal_places) {
  char buffer[20];  // đủ để chứa số và dấu phẩy
  int int_part = (int)num;     // phần nguyên
  float frac_part = num - int_part;  // phần thập phân

  // Xử lý phần nguyên
  sprintf(buffer, "%d", int_part);
  lcd_send_string(buffer);

  lcd_send_data('.');  // hiển thị dấu chấm

  // Xử lý phần thập phân
  for (uint8_t i = 0; i < decimal_places; i++) {
    frac_part *= 10;
    int digit = (int)frac_part;
    sprintf(buffer, "%d", digit);
    lcd_send_string(buffer);
    frac_part -= digit;
  }
}