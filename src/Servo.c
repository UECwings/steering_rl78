#include "Servo.h"
#include "UART0.h"
#include "System.h"

extern config_pack current_config;
extern volatile int last_servo[2];

int Servo_SetPos(char id, int ad)
{
	int ret_pos = 0;
    const servo_cfg *sc = &current_config.servo[id];
    long target_deg;
    long target_pos;
    
    target_deg = (long) sc->trim + (long) (ad - 512) * sc->range / 512;
    if (sc->invert == 1) {target_deg = -target_deg;}
    if (target_deg > sc->limit_upper) {target_deg = sc->limit_upper;}
    if (target_deg < sc->limit_lower) {target_deg = sc->limit_lower;}

    target_pos = (long) sc->offset + (long) sc->rot_count * target_deg / 100 / 360;
    
    last_servo[id] = (int) target_pos;

	UART0_PutChar(0x80 | (unsigned char) (id & 0x1f)); //位置設定
	UART0_PutChar((unsigned char) (target_pos>>7) & 0x7f);
	UART0_PutChar((unsigned char) (target_pos   ) & 0x7f);

	return(ret_pos);
}

void Servo_SetSpeed(char id, int speed)
{
	UART0_PutChar(0xC0 | (unsigned char) (id & 0x1f)); //パラメータ書き込み
	UART0_PutChar(0x02); //パラメータ: スピード
	UART0_PutChar((unsigned char) speed & 0x7f);
}

void Servo_WriteID(char id)
{
    UART0_PutChar(0xE0 | (unsigned char) (id & 0x1f));
    UART0_PutChar(0x01);
    UART0_PutChar(0x01);
    UART0_PutChar(0x01);
}
