#include "gdot.h"

//lcd �� ���ϴ� ��Ʈ �Է¿� ��ũ��
#define rs PORTA.0
#define rw PORTA.1
#define en PORTA.2

// LCD 128*64 �յ�й��
#define XVAL 16  
#define YVAL 32  

//ĳ���͸�� lcd �ƽ�Ű�ڵ� ����
void clcd_data(char x);

//ĳ���͸�� ���� �ν�Ʈ����
void clcd_init(void);

//�׷��ȸ�� �ʱ�ȭ��
void glcd_fill(unsigned int value);

//ĳ���͸�� �ʱ�ȭ��
void clcd_clear(void);


//lcd �ο��̺�
void enable(void)
{
    en = 1;
    delay_ms(1);
    en = 0;
}

//lcd Ŭ���� �ν�Ʈ���� ����� 8��Ʈ�Է�
void clcd_inst(char x)
{
    PORTF = x;
    rs = 0;
    rw = 0;
    enable();
    delay_us(1600);
}


//ĳ���͸�� lcd �ƽ�Ű�ڵ� ����
void clcd_data(char x)
{
    PORTF = x;
    rs = 1;
    rw = 0;
    enable();
    delay_us(72); //�ش� glcd�� 72us ���ð� �ʿ�
}

//ĳ���͸�� lcd ���ڿ� ���ۿ�
void clcd_str(char *str)
{
    while(*str)
    {
        clcd_data(*str++);
    }
}


void clcd_init()    //ĳ���͸�� lcd ���� �ν�Ʈ����
{
   clcd_inst(0x30);   //8��Ʈ ���� �ν�Ʈ���� ����
   clcd_inst(0x0C);   //lcdĿ���� Ŀ���� ��ġ�� ���� ������ ���ֱ�
   clcd_inst(0x06);   //��Ʈ�� ���
   clcd_inst(0x01);   //lcd Ŭ����
}

void Myclcd_init()    // ĳ���͸�� lcd ������ �ν�Ʈ���� Ŭ���� X
{
   clcd_inst(0x30);   //8��Ʈ ���� �ν�Ʈ���� ����
   clcd_inst(0x0C);   //��ũ ���ֱ�
   clcd_inst(0x06);   //��Ʈ����� 
}

void glcd_init(void) //�׷��� lcd ��� ���� �ν�Ʈ����
{
   clcd_inst(0x30);   //8��Ʈ ����
   clcd_inst(0x0C);   //��ũ ����
   clcd_inst(0x01);   //lcd �ʱ�ȭ
   clcd_inst(0x06);   //��Ʈ����� ��
   clcd_inst(0x34);   // �ν�Ʈ���� ��� Ȯ��
   clcd_inst(0x36);   // �׷��ȸ�� ����.
}

void Myglcd_init(void) //�׷��ȸ�� ���� Ŭ���� X
{
   clcd_inst(0x30);   //8��Ʈ ����
   clcd_inst(0x0C);   //��ũ ����
   clcd_inst(0x06);   //��Ʈ����� ��
   clcd_inst(0x34);   // �ν�Ʈ���� ��� Ȯ��
   clcd_inst(0x36);   // �׷��ȸ�� ����.
}



void clcd_clear(void) //lcd Ŭ����� ��� �Լ�
{
    clcd_inst(0x01);
}

void gotoxy(char x, char y)    // x=0-7, y=0-63  (128 x 64) ���ϴ� ��ǥ �̵��� �Լ�
{
      if (y > 31)
      {
          y -= 32;    //�׷��� LCD�� 2���� 64*64 LCD����� �̾��� �ֱ� ������ 
          x += 8;      //��ġ ������ �ǿ���
      }
    x |= 0x80;    // 7����Ʈ ->������ ������ ��
    y |= 0x80;
    clcd_inst(y);    // DDRAM�� ��ǥ�� �˷���
    clcd_inst(x);    // 
}

void glcd_fill(unsigned int value)    //�׷��ȸ���� �� ��ü �� �Ǵ� Ŭ�����
{
    unsigned char i, x, y;
   
    clcd_inst(0x34);
    for(i=0;i<9;)
    {
        for(x=0;x<32;x++)
        {
            clcd_inst((0x80+x));    // ����
            clcd_inst((0x80+i));    // ����
            for(y=0;y<16;y++)
                clcd_data(value);
        }
        i+=8;
    }
    clcd_inst(0x36);
}

//�ѱ۹��� �׷��ȸ�� ��¿� ����� �ٸ��� ũ�Ⱑ �޶� ������
void draw_han(char x, char y, char start, char count, const char han[][32])
{
    int h, k;
   
    for(h=start; h<start+count; h++) //�迭�� 0��°���ں��� �迭�ȿ� ���� ���ڿ����� �ݺ�
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

//���� �Է¿� �Լ� �ѱ۸��� ��Ŀ������ ����
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

