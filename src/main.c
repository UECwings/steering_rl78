/* main.c */

// SFR定義の使用
#pragma sfr

// 割り込み有効/無効
#pragma DI
#pragma EI

// 割り込みベクタ定義
#pragma interrupt INTTM01 dummy
#pragma interrupt INTAD AD_Interrupter

// include
#include "SoftUART.h"
#include "Servo.h"
#include "UART0.h"
#include "FIFO.h"
#include "Flash.h"
#include "System.h"
#include <stdlib.h>
#include <string.h>

// alias
#define puts SUART_PutStr

// prototype
void dummy(void);
void CTRL_Initialize(void);
void CMDLINE(void);
void CMD_control(int argc, char *argv[]);       // 制御の有効/無効
void CMD_ad(int argc, char *argv[]);            // ADCの手動操作
void CMD_servo(int argc, char *argv[]);         // サーボの手動操作
void CMD_servo_speed(int argc, char *argv[]);   // サーボの速度設定
void CMD_fread(int argc, char *argv[]);         // フラッシュを読み込んで表示
void CMD_fifo(int argc, char *argv[]);          // UARTのFIFOのポインタを表示
void CMD_load(int argc, char *argv[]);          // 舵角設定の読み込み
void CMD_save(int argc, char *argv[]);          // 舵角設定の保存
void CMD_config(int argc, char *argv[]);        // 舵角を設定する

// global
volatile int last_ad[2];
volatile int last_servo[2];

void main(void)
{
    int retcode;

    // 動作周波数を32MHzにする
    HOCODIV = 0x00;
    
    // RxD0, TxD0をP1.6, P1.7に割り振る
    PIOR |= 0x02; // PIOR1 = 1;

    P1.7 = 1;
    PM1.7 = 0; // TxD0 出力
    PM1.6 = 1; // RxD0 入力

    // Initialize Timer-Array-Unit 0
    TAU0EN = 1;        //Enable Timer
    TPS0 = 0x0050;    //Select clock source (CK01 = 1MHz)
    TS0 = 0;        //Stop timer
    TOE0 = 0;        //Set output settings
    TO0 = 0;        //Set output
    TOL0 = 0;        //Set output level
    TOM0 = 0;        //Set output mode

    // Initialize Software-UART
    SUART_Initialize();

    // Enable Interrupt
    EI();

    exec("led 7");

    // リセット要因を表示
    exec("resf");

    // Enable Analog-to-Digital Converter
    ADCEN = 1;

    // Enable Serial-Array-Unit 0
    SAU0EN = 1;

    // Initialize Hardware-UART 0
    UART0_Initialize();

    SUART_PutStr("SAU Enabled.\r\n");

    puts("Loading config...");
    retcode = exec("load 0");
    //retcode = 1;
    if (retcode != 0) { // 読み込み失敗した場合
        puts("Loading default config...");
        exec("default_config");
    }

    // サーボモータ制御の初期化と開始
    CTRL_Initialize();

    exec("led 3");

    SUART_PutStr("RL78/G13 debug console.\r\n");

    // デバッグ用コマンドラインを起動
    CMDLINE();
}

//割り込みベクタテーブル登録用のダミー関数
void dummy(void)
{
}

void CTRL_Initialize(void)
{
    //ADモード設定
    ADM0 = 0x30; //速度設定(2.75us)
    ADM1 = 0xA0; //タイマ01の割り込み(INTTM01)で変換開始, ハードウェアトリガ
    ADM2 = 0x00; 

    ADS = 2; // 2 = vertical, 3 = horizontal
    ADCE = 1;

    ADMK = 0;

    ADCS = 1;

    //タイマ01設定
    TMR01 = 0x40;
    TMMK01 = 0;
    TDR01 = 50000; // 1MHz / 50k = 20Hz

    TS0 = 0x02; //タイマ開始
}

void AD_Interrupter(void)
{
    int an;
    int adc_out;
    int servo_ch = 0;
    
    if (ADS == 2) {servo_ch = SERVO_TAIL_VERTICAL;}
    if (ADS == 3) {servo_ch = SERVO_TAIL_HORIZONTAL;}
    
    adc_out = (int) (ADCR>>6);
    
    // 保存
    last_ad[servo_ch] = adc_out;

    //舵角を送信
    Servo_SetPos((char) servo_ch, adc_out);

    if (ADS == 2) {
        ADS = 3;
    } else {
        ADS = 2;
    }
}

void CMDLINE(void)
{
    char str[128];
    int retcode;

    while(1) {
        SUART_PutStr("> ");

        SUART_GetStr(str, 127);

        SUART_PutStr("\r\n");
        
        // コマンドの実行
        retcode = exec(str);
        
        // コマンドがない場合
        if (retcode == EXEC_COMMAND_NOT_FOUND) {
            puts("exec: command not found.\r\n");
            continue;
        }
        
        // 終了コードが0でない場合
        if (retcode != 0) {
            puts("ret = ");
            SUART_PutByte((unsigned char) retcode);
            puts("\r\n");
        }
    }
}

void CMD_servo_speed(int argc, char *argv[])
{
    int s_id, s_speed;
    
    s_id = (int) atoi(argv[1]);
    s_speed = (int) atoi(argv[2]);
    Servo_SetSpeed((char) s_id, s_speed);

    SUART_PutStr("ServoID 0x");
    SUART_PutByte((unsigned char) s_id & 0x7f);
    SUART_PutStr(" Speed => 0x");
    SUART_PutByte((unsigned char) s_speed & 0x7f);

    return;
}
