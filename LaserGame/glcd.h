#include "gdot.h"

//lcd 핀 원하는 포트 입력용 매크로
#define rs PORTA.0
#define rw PORTA.1
#define en PORTA.2

// LCD 128*64 균등분배용
#define XVAL 16  
#define YVAL 32  

//캐릭터모드 lcd 아스키코드 전송
void clcd_data(char x);

//캐릭터모드 시작 인스트럭션
void clcd_init(void);

//그래픽모드 초기화용
void glcd_fill(unsigned int value);

//캐릭터모드 초기화용
void clcd_clear(void);


//lcd 인에이블
void enable(void)
{
    en = 1;
    delay_ms(1);
    en = 0;
}

//lcd 클리어 인스트럭션 제어용 8비트입력
void clcd_inst(char x)
{
    PORTF = x;
    rs = 0;
    rw = 0;
    enable();
    delay_us(1600);
}


//캐릭터모드 lcd 아스키코드 전송
void clcd_data(char x)
{
    PORTF = x;
    rs = 1;
    rw = 0;
    enable();
    delay_us(72); //해당 glcd는 72us 대기시간 필요
}

//캐릭터모드 lcd 문자열 전송용
void clcd_str(char *str)
{
    while(*str)
    {
        clcd_data(*str++);
    }
}


void clcd_init()    //캐릭터모드 lcd 시작 인스트럭션
{
   clcd_inst(0x30);   //8비트 전송 인스트럭션 시작
   clcd_inst(0x0C);   //lcd커서와 커서에 위치한 문자 깜박임 없애기
   clcd_inst(0x06);   //엔트리 모드
   clcd_inst(0x01);   //lcd 클리어
}

void Myclcd_init()    // 캐릭터모드 lcd ㅅㅣ작 인스트럭션 클리어 X
{
   clcd_inst(0x30);   //8비트 전송 인스트럭션 시작
   clcd_inst(0x0C);   //블링크 없애기
   clcd_inst(0x06);   //엔트리모드 
}

void glcd_init(void) //그래픽 lcd 모드 시작 인스트럭션
{
   clcd_inst(0x30);   //8비트 전송
   clcd_inst(0x0C);   //블링크 끄기
   clcd_inst(0x01);   //lcd 초기화
   clcd_inst(0x06);   //엔트리모드 셋
   clcd_inst(0x34);   // 인스트럭션 모드 확장
   clcd_inst(0x36);   // 그래픽모드 시작.
}

void Myglcd_init(void) //그래픽모드 시작 클리어 X
{
   clcd_inst(0x30);   //8비트 전송
   clcd_inst(0x0C);   //블링크 끄기
   clcd_inst(0x06);   //엔트리모드 셋
   clcd_inst(0x34);   // 인스트럭션 모드 확장
   clcd_inst(0x36);   // 그래픽모드 시작.
}



void clcd_clear(void) //lcd 클리어용 명령 함수
{
    clcd_inst(0x01);
}

void gotoxy(char x, char y)    // x=0-7, y=0-63  (128 x 64) 원하는 좌표 이동용 함수
{
      if (y > 31)
      {
          y -= 32;    //그래픽 LCD는 2개의 64*64 LCD모듈이 이어져 있기 때문에 
          x += 8;      //위치 보정이 피요함
      }
    x |= 0x80;    // 7번비트 ->데이터 쓰기모드 셋
    y |= 0x80;
    clcd_inst(y);    // DDRAM에 좌표를 알려줌
    clcd_inst(x);    // 
}

void glcd_fill(unsigned int value)    //그래픽모드일 때 전체 셋 또는 클리어용
{
    unsigned char i, x, y;
   
    clcd_inst(0x34);
    for(i=0;i<9;)
    {
        for(x=0;x<32;x++)
        {
            clcd_inst((0x80+x));    // 수직
            clcd_inst((0x80+i));    // 수평
            for(y=0;y<16;y++)
                clcd_data(value);
        }
        i+=8;
    }
    clcd_inst(0x36);
}

//한글문자 그래픽모드 출력용 영어랑 다르게 크기가 달라 나눴음
void draw_han(char x, char y, char start, char count, const char han[][32])
{
    int h, k;
   
    for(h=start; h<start+count; h++) //배열의 0번째글자부터 배열안에 들은 문자열까지 반복
    {
        for(k=0; k<16; k++) 
        {
            clcd_inst(0x34);
            gotoxy(x+h,y+k);        // y is vertical
            clcd_data(han[h][k*2]);
            clcd_data(han[h][k*2+1]);
        }
    }
    clcd_inst(0x36);
}

//영어 입력용 함수 한글모드랑 매커니즘은 같음
void draw_eng(char x, char y, char start, char count, const char eng[][16])
{
    int h, k;
   
    for(h=start; h<start+count; h+=2)
    {
        for(k=0; k<16; k++)
        {
            clcd_inst(0x34);
            gotoxy(x+h/2,y+k);      // y is vertical
            clcd_data(eng[h][k]);
            clcd_data(eng[h+1][k]);
        }
    clcd_inst(0x36);
    }
}

