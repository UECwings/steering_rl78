#pragma sfr

#include "SoftUART.h"
#include "System.h"
#include "Flash.h"
#include "Servo.h"
#include <stdlib.h>
#include <string.h>

#define puts SUART_PutStr
#define MAX_ARGC 8

command command_table[] = {
    //関数名, コマンド名, 概要
    {help, "help", "show command list"},
    {clear, "clear", "clear terminal"},
    {reset, "reset", "software reset (by WDT)"},
    {resf, "resf", "show reset factor"},
    {load, "load", "load config"},
    {save, "save", "save config"},
    {default_config, "default_config", "load default config"},
    {led, "led", "set led status"},
    {ad, "ad", "show last ad value"},
    {control, "control", "start/stop servo control"},
    {servo_pos, "servo", "send servo pos"},
    {servo_set, "set", "set servo config"},
    {servo_writeid, "write_id", "set servo id"},
    {show, "show", "show config"},
    {NULL, NULL, NULL} // footer
};

// 設定
config_pack current_config;

//
extern volatile int last_ad[2];
extern volatile int last_servo[2];

// コマンド実行
int exec(const char *str)
{
    char cmdline[128];
    char *argv[MAX_ARGC];
    int argc;
    int retcode;
    int i;
    
    strncpy(cmdline, str, 127);
    cmdline[127] = '\0';

    // コマンド名
    argv[0] = strtok(cmdline, " ");
    if (argv[0] == NULL) {return;}

    // 引数
    for (argc = 1; argc < MAX_ARGC; argc++) {
        argv[argc] = strtok(NULL, " ");
        if (argv[argc] == NULL) {
            break;
        }
    }

    // 検索
    for (i = 0; command_table[i].func != NULL; i++) {
        if (strcmp(command_table[i].name, argv[0]) == 0) {
            // 実行
            retcode = command_table[i].func(argc, argv);
            return retcode;
        }
    }

    // 関数がない
    return EXEC_COMMAND_NOT_FOUND;
}

// 以下，コマンド

int help(int argc, char *argv[])
{
    int i, p;
    int maxlen;
    int len;
    
    // 最大長を調べる
    maxlen = 0;
    for (i = 0; command_table[i].func != NULL; i++) {
        len = strlen(command_table[i].name);
        if (maxlen < len) {maxlen = len;}
    }
    
    // 表示
    for (i = 0; command_table[i].func != NULL; i++) {
        // コマンド名
        puts(command_table[i].name);
        // パディング
        len = strlen(command_table[i].name);
        for (p = len; p < maxlen + 1; p++) {
            puts(" ");
        }
        // コマンドの説明
        puts(command_table[i].desc);
        puts("\r\n");
    }

    return 0;
}

int clear(int argc, char *argv[])
{
    SUART_PutStr("\x1b[2J"); //画面クリア
    SUART_PutStr("\x1b[0;0H"); //カーソル位置を(0, 0)へ移動

    return 0;
}

int reset(int argc, char *argv[])
{
    //WDTへの不正な書き込み
    WDTE = 0;
    while(1) {/* NOP */}
    
    return 0;
}

int resf(int argc, char *argv[])
{
    const char resf = RESF; //リセット要因
    
    if (resf & 0x80) {puts("Reset factor: Execute a illegal instruction.\r\n");}
    if (resf & 0x10) {puts("Reset factor: Watch-Dog-Timer.\r\n");}
    if (resf & 0x04) {puts("Reset factor: RAM parity error.\r\n");}
    if (resf & 0x02) {puts("Reset factor: Illegal memory access.\r\n");}
    if (resf & 0x01) {puts("Reset factor: Low-Voltage-Detect.\r\n");}
    
    return 0;
}

int load(int argc, char *argv[])
{
    int slot = 0; // 読み込みブロック番号
    unsigned char checksum;
    unsigned char *p;
    int i;
    int ret;
    config_pack cfg;

    if (argc > 1) {
        slot = atoi(argv[1]);
    }
    
    // 読み込み
    ret = Flash_Read(slot, sizeof(config_pack), (unsigned char *) &cfg);

    if (ret != 0) {
        puts("Failed to read.\r\n");
        return 1;
    }

    // ヘッダのチェック
    if (strncmp(cfg.signature, CONFIG_SIGNATURE, CONFIG_SIGNATURE_LENGTH) != 0) {
        puts("Wrong signature.\r\n");
        return 2;
    }

    if (cfg.size != sizeof(config_pack)) {
        puts("Data size mismatch.\r\n");
        return 3;
    }
    
    // チェックサムの計算
    checksum = 0;
    p = (unsigned char *) &cfg;
    for (i = 0; i < sizeof(config_pack) - 2; i++) {
        checksum += p[i];
    }

    if (cfg.checksum != checksum) {
        puts("Checksum mismatch.\r\n");
        return 4;
    }

    // データのコピー
    memcpy(&current_config, &cfg, sizeof(config_pack));

    puts("ok.\r\n");
    return 0;
}

int save(int argc, char *argv[])
{
    int slot = 0; // 書き込みブロック番号
    unsigned char checksum;
    unsigned char *p;
    int i;
    int ret;
    config_pack cfg;

    if (argc > 1) {
        slot = atoi(argv[1]);
    }
    
    // ヘッダの構築
    cfg = current_config;
    memcpy(cfg.signature, CONFIG_SIGNATURE, CONFIG_SIGNATURE_LENGTH);
    cfg.size = sizeof(config_pack);

    // チェックサムの計算
    checksum = 0;
    p = (unsigned char *) &cfg;
    for (i = 0; i < sizeof(config_pack) - 2; i++) {
        checksum += p[i];
    }
    cfg.checksum = checksum;

    // 書き込み
    ret = Flash_Write(slot, sizeof(config_pack), (unsigned char *) &cfg);

    // 結果表示
    if (ret == 0) {
        puts("ok.\r\n");
    } else {
        puts("Failed.\r\n");
    }

    return 0;
}

int default_config(int argc, char *argv[])
{
    int i;
    
    // VERTICAL
    // サーボの設定
    current_config.servo[SERVO_TAIL_VERTICAL].offset = 7500; // 中央の位置 (近藤科学 ICS 3.5は7500)
    current_config.servo[SERVO_TAIL_VERTICAL].rot_count = 10666; // 1回転あたりのカウント (近藤科学 ICS 3.5は10666)
    current_config.servo[SERVO_TAIL_VERTICAL].invert = 0;
    // 舵の設定
    current_config.servo[SERVO_TAIL_VERTICAL].trim = SERVO_DEG(-10.00);
    current_config.servo[SERVO_TAIL_VERTICAL].range = SERVO_DEG(120.00);
    current_config.servo[SERVO_TAIL_VERTICAL].limit_upper = SERVO_DEG( 90.00);
    current_config.servo[SERVO_TAIL_VERTICAL].limit_lower = SERVO_DEG(-90.00);
    
    // HORIZONTAL
    current_config.servo[SERVO_TAIL_HORIZONTAL].offset = 7500;
    current_config.servo[SERVO_TAIL_HORIZONTAL].rot_count = 10666; // 1回転あたりのカウント (近藤科学 ICS 3.5は10666)
    current_config.servo[SERVO_TAIL_HORIZONTAL].invert = 1;
    // 舵の設定
    current_config.servo[SERVO_TAIL_HORIZONTAL].trim = SERVO_DEG(12.00);
    current_config.servo[SERVO_TAIL_HORIZONTAL].range = SERVO_DEG(120.00);
    current_config.servo[SERVO_TAIL_HORIZONTAL].limit_upper = SERVO_DEG( 110.00);
    current_config.servo[SERVO_TAIL_HORIZONTAL].limit_lower = SERVO_DEG(-110.00);

    puts("ok.\r\n");
    
    return 0;
}

int led(int argc, char *argv[])
{
    int pattern;

    if (argc < 2) {
        puts("Usage: led <0-7>\r\n");
        return 1;
    }
    
    pattern = atoi(argv[1]);
    
    PM3.0 = 0; //output
    PM5.0 = 0;
    PM5.1 = 0;
    
    P3.0 = (pattern>>2) & 0x01;
    P5.0 = (pattern>>1) & 0x01;
    P5.1 = (pattern>>0) & 0x01;
    
    return 0;
}

void hash_bar(int val, int max)
{
    int i;
    const int cnt = (int) ((long) val * 64 / max);
    
    puts(" |");
    for (i = 0; i < cnt; i++) {
        puts("#");
    }
    for (; i < 65; i++) {
        puts(" ");
    }
    puts("|");
}

int ad(int argc, char *argv[])
{
    int ad;

    puts("AD[0] = ");
    ad = last_ad[0];
    SUART_PutByte((unsigned char)(ad>>8));
    SUART_PutByte((unsigned char) ad);
    hash_bar(ad, 1024);
    puts("\r\n");

    puts("sv[0] = ");
    ad = last_servo[0];
    SUART_PutByte((unsigned char)(ad>>8));
    SUART_PutByte((unsigned char) ad);
    hash_bar(ad, 16384);
    puts("\r\n");
    
    puts("AD[1] = ");
    ad = last_ad[1];
    SUART_PutByte((unsigned char)(ad>>8));
    SUART_PutByte((unsigned char) ad);
    hash_bar(ad, 1024);
    puts("\r\n");
    
    puts("sv[1] = ");
    ad = last_servo[1];
    SUART_PutByte((unsigned char)(ad>>8));
    SUART_PutByte((unsigned char) ad);
    hash_bar(ad, 16384);
    puts("\r\n");
    
    return 0;
}

int control(int argc, char *argv[])
{
    int stat;
    
    if (argc != 2) {
        SUART_PutStr("control: error: invalid arguments.\r\n");
        return 1;
    }
    
    stat = (int) atoi(argv[1]);
    
    if (stat) {
        TS0 = 0x02;
        SUART_PutStr("control: start.\r\n");
    } else {
        TT0 = 0x02;
        SUART_PutStr("control: stop.\r\n");
    }
    
    return 0;
}

int servo_pos(int argc, char *argv[])
{
    int ch;
    int ad;

    if (argc < 3) {
        puts("invaild arguiments.\r\n");
        return 1;
    }
    
    ch = atoi(argv[1]);
    ad = atoi(argv[2]);
    
    Servo_SetPos((char) ch, ad);

    return 0;
}

int servo_set(int argc, char *argv[])
{
    int ch;
    int val;
    servo_cfg *sc;

    if (argc != 4) {
        puts("Usage: ");
        puts(argv[0]);
        puts(" <id> <keyword> <value>\r\n");
        puts(" keyword = offset|rot_count|invert|trim|range|limit_upper|limit_lower\r\n");
        return 1;
    }

    ch = atoi(argv[1]);
    sc = &current_config.servo[ch];
    
    val = atoi(argv[3]);

    if (strcmp(argv[2], "offset") == 0) {
        sc->offset = val;
    } else if (strcmp(argv[2], "rot_count") == 0) {
        sc->rot_count = val;
    } else if (strcmp(argv[2], "invert") == 0) {
        sc->invert = val;
    } else if (strcmp(argv[2], "trim") == 0) {
        sc->trim = val;
    } else if (strcmp(argv[2], "range") == 0) {
        sc->range = val;
    } else if (strcmp(argv[2], "limit_upper") == 0) {
        sc->limit_upper = val;
    } else if (strcmp(argv[2], "limit_lower") == 0) {
        sc->limit_lower = val;
    } else {
        puts("unknown keyword.\r\n");
        return 2;
    }
    
    return 0;
}

int servo_writeid(int argc, char *argv[])
{
    int ch;

    if (argc != 2) {
        puts("Usage: ");
        puts(argv[0]);
        puts("<id>\r\n");
        return 1;
    }
    
    ch = atoi(argv[1]);
    
    Servo_WriteID((char) ch);

    return 0;
}

int show(int argc, char *argv[])
{
    int ch;
    
    for (ch = 0; ch < SERVO_CH; ch++) {
        puts("=== servo ch: "); SUART_PutInt(ch); puts(" ===\r\n");
        puts("offset     : "); SUART_PutInt(current_config.servo[ch].offset     ); puts("\r\n");
        puts("rot_count  : "); SUART_PutInt(current_config.servo[ch].rot_count  ); puts("\r\n");
        puts("invert     : "); SUART_PutInt(current_config.servo[ch].invert     ); puts("\r\n");
        puts("trim       : "); SUART_PutInt(current_config.servo[ch].trim       ); puts("\r\n");
        puts("range      : "); SUART_PutInt(current_config.servo[ch].range      ); puts("\r\n");
        puts("limit_upper: "); SUART_PutInt(current_config.servo[ch].limit_upper); puts("\r\n");
        puts("limit_lower: "); SUART_PutInt(current_config.servo[ch].limit_lower); puts("\r\n");
    }
    
    return 0;
}