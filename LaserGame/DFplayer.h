typedef unsigned char INT8;
typedef unsigned int INT16;
//
#define BAUD 9600
#define U2X_S 2     // Set of U2X --> 1 or 2
//플레이어 인스트럭션 명령어 저장용 매크로
#define MP3_NEXT                    0x01
#define MP3_PREVIOUS                0x02
#define MP3_TRAKING_NUM                0x03 // 0..2999
#define MP3_INC_VOLUME                0x04
#define MP3_DEC_VOLUME                0x05
#define MP3_VOLUME                    0x06 // 0..30
#define MP3_EQ                        0x07 // 0-Normal / 1-Pop / 2-Rock / 3-Jazz / 4-Classic / 5-Base
#define MP3_PLAYBACK_MODE            0x08 // 0-Repeat / 1-folder repeat / 2-single repeat / 3-random
#define MP3_PLAYBACK_SOURCE            0x09 // 0-U / 1-TF / 2-AUX / 3-SLEEP / 4-FLASH
#define MP3_STANDBY                    0x0A
#define MP3_NORMAL_WORK                0x0B
#define MP3_RESET                    0x0C
#define MP3_PLAYBACK                0x0D
#define MP3_PAUSE                    0x0E
#define MP3_PLAY_FOLDER_FILE        0x0F // 0..10
#define MP3_VOLUME_ADJUST            0x10
#define MP3_REPEAT                    0x11 // 0-stop play / 1-start repeat play
// 시스템 파라미터 응답 매크로
#define MP3_Q_STAY1                    0x3C
#define MP3_Q_STAY2                    0x3D
#define MP3_Q_STAY3                    0x3E
#define MP3_Q_SEND_PRM                0x3F
#define MP3_Q_ERROR                    0x40
#define MP3_Q_REPLY                    0x41
#define MP3_Q_STATUS                0x42
#define MP3_Q_VALUE                    0x43
#define MP3_Q_EQ                    0x44
#define MP3_Q_PLAYBACK_MODE            0x45
#define MP3_Q_SOFT_VERSION            0x46
#define MP3_Q_TF_CARD_FILES            0x47
#define MP3_Q_U_DISK_CARD_FILES        0x48
#define MP3_Q_FLASH_CARD_FILES        0x49
#define MP3_Q_KEEPON                0x4A
#define MP3_Q_CURRENT_TRACK_TF        0x4B
#define MP3_Q_CURRENT_TRACK_U_DISK    0x4C
#define MP3_Q_CURRENT_TRACK_FLASH    0x4D
////////////////////////////////////////////////////////////////////////////////
//명령 파라미터 정리 추후 헤더파일에 넣을 예정
////////////////////////////////////////////////////////////////////////////////
#define MP3_EQ_Normal                    0
#define MP3_EQ_Pop                        1
#define MP3_EQ_Rock                        2
#define MP3_EQ_Jazz                        3
#define MP3_EQ_Classic                    4
#define MP3_EQ_Base                        5
#define MP3_PLAYBACK_MODE_Repeat        0
#define MP3_PLAYBACK_MODE_folder_repeat    1
#define MP3_PLAYBACK_MODE_single_repeat    2
#define MP3_PLAYBACK_MODE_random        3
#define MP3_PLAYBACK_SOURCE_U            0
#define MP3_PLAYBACK_SOURCE_TF            1
#define MP3_PLAYBACK_SOURCE_AUX            2
#define MP3_PLAYBACK_SOURCE_SLEEP        3
#define MP3_PLAYBACK_SOURCE_FLASH        4
 
//dfplay 시리얼 통신용 배열 시작(0x7E) 종료(0xEF)고정이기 때문에 배열로 정리함
INT8 default_buffer[10] = {0x7E, 0xFF, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF}; 
volatile INT8 mp3_cmd_buf[10] = {0x7E, 0xFF, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
    
//USART0 통신 시작용 함수
void USART0_Init( )
{
    // Set baud rate
    UBRR0H = 0; // X-TAL = 16MHz 일때, BAUD = 9600 
    UBRR0L = 103;

    // U2X 활성
    if(U2X_S == 2)
    UCSR0A = 0x1;
    else
    UCSR0A = 0x0;
    
    // tx rx활성
    UCSR0B = 0b00011000; // 송수신 인에이블 TXEN0 = 1, RXEN0=1

    // 모드설정
    UCSR0C = 0b00000110; // 비동기 데이터 8비트 모드
}


void USART0_Transmit( char data )
{
    // 플래그 될 때까지 
    while ( !( UCSR0A & 0x20 ) )   // (1<<UDRE) 
    ;
    //데이터 입력
    UDR0 = data;
}
 
//패리티 검산용 함수
INT16 MP3_checksum (void)
{
    INT16 sum = 0;
    INT8 i;
    for (i=1; i<7; i++) {
        sum += mp3_cmd_buf[i];
    }
    return -sum;
}

//플레이어 명령어 전달
void MP3_send_cmd (INT8 cmd, INT16 high_arg, INT16 low_arg)
{
    INT8 i;
    INT16 checksum;
    mp3_cmd_buf[3] = cmd; //define된 명령어 입력
    mp3_cmd_buf[5] = high_arg;
    mp3_cmd_buf[6] = low_arg;
    checksum = MP3_checksum();//패리티검산
    mp3_cmd_buf[7] = (INT8) ((checksum >> 8) & 0x00FF);
    mp3_cmd_buf[8] = (INT16) (checksum & 0x00FF);
    for( i=0; i< 10; i++){
        USART0_Transmit(mp3_cmd_buf[i]);
        //putchar(mp3_cmd_buf[i]);
        mp3_cmd_buf[i] = default_buffer[i];
    }
}

//플레이어 초기 시작
void dfplayer_init(void)
{
    MP3_send_cmd(MP3_PLAYBACK_SOURCE,0,MP3_PLAYBACK_SOURCE_TF);delay_ms(10);
    MP3_send_cmd(MP3_VOLUME, 0, 30); delay_ms(10);
}
 
