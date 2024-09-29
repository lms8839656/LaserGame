#include <mega128.h>
#include <delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "DFplayer.h"
#include "glcd.h"



#define _LASER1, checklaser=1
#define _LASER2, checklaser=1
#define _LASER3, checklaser=1
#define _DOWN 2700
#define _UP 1000

#define _IDLE 0
#define _ARCADE 1
#define _TIMEATTACK 2

#define _SELECTMODE 1
#define _DPARCADE 2
#define _DPTIMEATTACK 3



/*****************************

PIN
    0 1 2 3 4 5 6 7   
A:  o o o x x x x x :lcd rs rw e psb=5v ���ĸ��
B:  x x x x x o o o :pwm 3�� a b c 
D:  o o o x x x x x :lcd ��Ʈ�� ����ġ �� �Ʒ� Ȯ�� EXT INT 0, 1, 2
E:  o o o x o o o x :Rx, Tx, sound, laserRx EXT INT 4, 5, 6
F:  o o o o o o o o :lcd data  //avcc 5v�Է� 



****************************/

/*
led ��⾵���� �ڿ��� ������ �ʹ� ����
������ �̿� ���� ��¿��� ���ͷ�Ʈ�� �����ʿ�
���� ����Ʈ 0v ������ ���Ž� 5v 
����Ʈ. �������� �Ͼ ����   3000�Ѿ 1000�� �Ͼ 
        ADCSRA = ADCSRA | 0b01000000; // ADSC = 1 ��ȯ ����
        while((ADCSRA & 0x10) == 0); // ADIF=1�� �ɋ�����
        ad_val = (int)ADCL + ((int)ADCH << 8); // A/D ��ȯ�� �б�   
        
���������� ���״޾Ƽ� ��� ��� x
*/


unsigned int cnt = 0;
unsigned int time = 0;
//0 = �Ͼ���� 1 = �������
unsigned char checklaser[3] = {0};
unsigned char setnum = 0;
unsigned int randomnum = 0;

//gamemode : 0 = off 1 = �������, 2 = ���ѽð� 
//menumode : 0 = �ʱ�ȭ�� 1 = ��弱�� 2 = ������� 3 = ���ѽð�
unsigned char gamemode = 0, menumode = 0, cursor = 0;

//�������� 
void SetUpServo();  //�迭 ��� 0�ʱ�ȭ, �������� �Ͼ
void SetDownServo(); //�迭 ��� 0 �ʱ�ȭ �������� ������

//lcd dp��
void RemainSet();
void MainMenu();
void TimeDP();
//�������� 2500����

void main(void)
{
	//��Ʈ ����� ���� 
    DDRA = 0xFF;    //lcd rs rw en
    DDRB = 0xE0;    //PB5,6,7 out  pwm, servo 
    DDRD = 0x00; 	//����ġ 
    DDRE = 0x00;    //������ �Է� �ܺ� ���ͷ�Ʈ
    DDRF = 0xFF;    //lcd data
     
    //�ܺ����ͷ�Ʈ
    EIMSK = 0b01110111; //�ܺ����ͷ�Ʈ 0,1,2,4,5,6 �ο��̺� 
    EICRA = 0b00111111; //0,1,2 ��¿���, Ǯ�ٿ��ġ 
    EICRB = 0b00101010; //4,5,6 ���������� �ϰ����� 

    
    //Ÿ�̸� 0 �ʱ⼳��: ���ѽð� ����  
    TIMSK = 0x00;   //TOIE0 = 1   �����÷ο� ���ͷ�Ʈ 
    TCCR0 = 0x04;  // ���������� = ck/64    
    TCNT0 = 6;  //1/16us * 64 * (256 - 6) = 1ms

    
    //Ÿ�̸� 1 �ʱ⼳�� FAST PWM, servo
    TCCR1A=0xAA; TCCR1B=0x1A; // OCR1A -> OC Clear / OCR1A,B,C Ȱ��ȭ / Fast PWM TOP = ICR1 / 8����    
    OCR1A = _UP;       
    delay_ms(20);
    OCR1B = _UP; 
    delay_ms(20);
    OCR1CH = _UP >> 8; 
    OCR1CL = _UP & 0xFF;
    delay_ms(20);
    ICR1=19999; 
    SetDownServo(); //�ʱ�������� Ȯ�ο� �������� 
    
    //USART ���
	USART0_Init(); // 9600/8/n/1/n
	dfplayer_init();     
    
	/*MP3_send_cmd(0x12,0,3); //���ϴ� �� ������, 0003.mp3���� ����*/
	
	//GLCD 
	glcd_init();  //�׷��ȸ�� ����
	glcd_fill(0x0000); //��ĭ���� �ʱ�ȭ
    draw_han(0,0,0,5,gun);   //0��°ĭ 0���ٿ� 5���� ���� �Է�(�ѽ����� 5��)
    draw_han(6,32,0,1,img1);   //6ĭ 32���� ��Ʈ�̹��� ���� �»� 
    draw_han(7,32,0,1,img2);   //7ĭ 32���� 
    draw_han(6,48,0,1,img3);   //6ĭ 48����  
    draw_han(7,48,0,1,img4);  //7ĭ 48����  
    Myclcd_init(); //ĳ���͸�� ����
    delay_ms(1000); 
    /*

    */
    
    //���ͷ�Ʈ Ȱ��ȭ 
    SREG = 0x80;
    
    //������  
    MP3_send_cmd(0x12,0,4); //���ϴ� �� ������, 0003.mp3���� ����
    SetUpServo();     //���ÿϷ� 

    
    
    //loop 
    
    while(1)
    {   
		switch(menumode){
			case _SELECTMODE: //select mode
                clcd_inst(0x80);        // ù�� ùĭ
				clcd_str("Select Mode");
				clcd_inst(0x90);        // �ι�°�� ùĭ
				clcd_str("Practice "); if (cursor == 0) clcd_data(0x11); else clcd_data(0x20);
				clcd_inst(0x88);        // ����°�� ùĭ
				clcd_str("TimeAttack "); if (cursor == 1) clcd_data(0x11); else clcd_data(0x20);  
                delay_ms(50);
			break;
			
			case _DPARCADE:
				clcd_clear();
				clcd_inst(0x80);        // ù�� ùĭ
				clcd_str("Practice!!!");
				
			break;
			
			case _DPTIMEATTACK:
				clcd_clear();
				clcd_inst(0x80);        // ù�� ùĭ
				clcd_str("TimeAttack!!!");
			break;
			
			default:
				clcd_inst(0x90);        // �ι�°�� ùĭ
		        clcd_str("Press AnyKey");   
		        delay_ms(1000);
		        clcd_inst(0x90);        // �ι�°�� ùĭ
		        clcd_str("            ");
		        delay_ms(1000); 
			break;	
		}
		
		/*
		clcd_inst(0x80);        // ù�� ùĭ
        clcd_inst(0x81);        // ù�� ����°ĭ
        clcd_inst(0x90);        // �ι�°�� ùĭ
        clcd_inst(0x88);        // ����°�� ùĭ
        clcd_inst(0x8b);        // ����°�� �ϰ���°ĭ
        clcd_inst(0x8f);        // ����°�� ���ټ�°ĭ
        clcd_inst(0x98);        // �׹�°�� ùĭ
        clcd_data(0x01); 
        clcd_str("Hi!");
		*/

        switch (gamemode){
            case _ARCADE: //�����̵��� 
                //5��Ʈ    
                MP3_send_cmd(0x12,0,2); //���ϴ� �� ������, 0002.mp3���� ����
                setnum = 5;   
                clcd_inst(0x98);        // �׹�°�� ùĭ
	        	clcd_str("RemainSet:5");
				while(setnum!=0){ //��Ʈ0�ɶ����� �ݺ� 
	                if((checklaser[0] & checklaser[1] & checklaser[2]) == 1){
	                    setnum--;   
	                    SetUpServo(); 	
						RemainSet();
	                } 
            	}
                gamemode = _IDLE; //���� Ż��� ���� ���� 
                menumode = _IDLE;          
                MainMenu();
            break;
			////////////////////////////////
            case _TIMEATTACK: //���ѽð���� 
                TIMSK = 0x01; //�����÷ο� �ο��̺� 
                cnt = 0; 
                setnum = 0;   
                time = 60;
                MP3_send_cmd(0x12,0,3); //���ϴ� �� ������, 0002.mp3���� ����
                while(TIMSK != 0x00){  
                    randomnum = rand() % 100000;   
                    
                    if((randomnum < 3) & (checklaser[0] == 1)){  // 20%�� ǥ���� ���������� ��  
                        checklaser[0] = 0;  //�ö� ���·� ����
                        OCR1A = _UP;        //�������� ��
                    }   
                    
                    if(((randomnum > 3) & (randomnum < 6)) & (checklaser[1] == 1)){ // 20%�� ǥ���� ���������� �� 
                        checklaser[1] = 0;
                        OCR1B = _UP; 
                    }  
                    
                    if(((randomnum > 6) & (randomnum < 9)) & (checklaser[2] == 1)){  // 20%�� ǥ���� ���������� ��
                        checklaser[2] = 0;
                        OCR1CH = _UP >> 8; 
                        OCR1CL = _UP & 0xFF; 
                    }   
                    
                    if(checklaser[0] == checklaser[1] == checklaser[2] == 1){
                        checklaser[0] = 0;  //�ö� ���·� ����
                        OCR1A = _UP;        //�������� ��  
                        checklaser[1] = 0;
                        OCR1B = _UP; 
                        checklaser[2] = 0;
                        OCR1CH = _UP >> 8; 
                        OCR1CL = _UP & 0xFF; 
                    }
                }  
            break;
            ////////////////////////////////////
            default:
                SetUpServo();   
            break;
        }
    }
}

///////////////�ܺ����ͷ�Ʈ 0,1,2/////////////
//�ܺ����ͷ�Ʈ 0,1,2 lcd���� ����ġ
//
interrupt [EXT_INT0] void SwUp(void)
{



	//�ι�°�޴�
	if (menumode == _SELECTMODE) cursor=0;


    //ù�޴�
		if(menumode == 0){ menumode++; clcd_clear();}  

	
}

interrupt [EXT_INT1] void SwDown(void)
{

	//�ι�°�޴�
	if (menumode == _SELECTMODE) cursor = 1;


	//ù�޴�
    if (menumode == 0) {menumode++;  clcd_clear();} 

}

interrupt [EXT_INT2] void SwOK(void)
{

	//�ι�°�޴�
	if (menumode == _SELECTMODE) {
		switch (cursor)
		{
		case 0:
			gamemode = _ARCADE;
			menumode = _DPARCADE; 
            clcd_clear();
			clcd_inst(0x80);        // ù�� ùĭ
			clcd_str("Practice!!!");
			break;
		case 1:
			gamemode = _TIMEATTACK;
			menumode = _DPTIMEATTACK;
		default:
			break;
		}
        clcd_clear(); 
	}	 
    //ù�޴�
    if (menumode == 0) {menumode=_SELECTMODE; clcd_clear();}                                 

}

///////////////////////�ܺ���Ʈ��Ʈ 4,5,6/////////////////
//�ܺ����ͷ�Ʈ 4,5,6 �������� ���϶�, ǥ���� �������� ���ŵǸ� ��
//������1 ��������1�� ��ȣ�ۿ�
interrupt [EXT_INT4] void LaserRx1(void)    
{
    if((gamemode == _ARCADE) & (checklaser[0] == 0)){
        checklaser[0] = 1;
        OCR1A = _DOWN;
		clcd_inst(0x8b);        // �ι�°�� ùĭ
		clcd_str("Good!");
        delay_ms(500);  
        clcd_inst(0x8b);        // �ι�°�� ùĭ
        clcd_str("           "); 
    } 
    
    if((gamemode == _TIMEATTACK) & (checklaser[0] == 0)){
        checklaser[0] = 1;
        OCR1A = _DOWN;
		clcd_inst(0x8b);        // �ι�°�� ùĭ
		clcd_str("Good!"); 
        setnum++;
        delay_ms(500);  
        clcd_inst(0x8b);        // �ι�°�� ùĭ
        clcd_str("         ");      
        delay_ms(50);  
    } 
}


//������2 ��������2�� ��ȣ�ۿ�
interrupt [EXT_INT5] void LaserRx2(void)
{
    if((gamemode == _ARCADE) & (checklaser[1] == 0)){
        checklaser[1] = 1;
        OCR1B = _DOWN;  
		clcd_inst(0x8b);        // �ι�°�� ùĭ
		clcd_str("Good!");
        delay_ms(500);
        clcd_inst(0x8b);        // �ι�°�� ùĭ
        clcd_str("        ");     
    }  
    
    if((gamemode == _TIMEATTACK) & (checklaser[1] == 0)){
        checklaser[1] = 1;
        OCR1B = _DOWN;
		clcd_inst(0x8b);        // �ι�°�� ùĭ
		clcd_str("Good!"); 
        setnum++;
        delay_ms(500);  
        clcd_inst(0x8b);        // �ι�°�� ùĭ
        clcd_str("       ");     
        delay_ms(50);  
    } 
}


//������3 ��������3�� ��ȣ�ۿ�
interrupt [EXT_INT6] void LaserRx3(void)
{
    if((gamemode == _ARCADE) & (checklaser[2] == 0)){
        checklaser[2] = 1;
        OCR1CH = _DOWN >> 8; 
        OCR1CL = _DOWN & 0xFF;
		clcd_inst(0x8b);        // �ι�°�� ùĭ
		clcd_str("Good!");
        delay_ms(500);  
        clcd_inst(0x8b);        // �ι�°�� ùĭ
        clcd_str("         ");   
        
        
    }
    
    if((gamemode == _TIMEATTACK) & (checklaser[2] == 0)){
        checklaser[2] = 1;
        OCR1CH = _DOWN >> 8; 
        OCR1CL = _DOWN & 0xFF;
		clcd_inst(0x8b);        // �ι�°�� ùĭ
		clcd_str("Good!");
        setnum++;
        delay_ms(500);  
        clcd_inst(0x8b);        // �ι�°�� ùĭ
        clcd_str("         ");      
        delay_ms(50);  
    } 
}
//////////////////////////////////////////////////

interrupt [TIM0_OVF] void timer0(void)
{
    TCNT0 = 6;          //�ʱⰪ �缳�� 
    
    cnt++;  //���ͷ�ƮȽ�� +1 
    if(cnt % 1000 == 0) {
        time--; 
        TimeDP();      
    }
    if(cnt == 1000 * 60){ //1000ms * 60 = 1min 
        cnt = 0; 
        TIMSK = 0x00;  
        gamemode = _IDLE; //���� Ż��� ���� ���� 
        menumode = _IDLE;          
        MainMenu();
    }
}


void SetUpServo(){ //��� �ø��� �����÷��� 0 �ʱ�ȭ 
    OCR1A = _UP; 
    OCR1B = _UP; 
    OCR1CH = _UP >> 8; 
    OCR1CL = _UP & 0xFF;   
    delay_ms(500);    
    memset(checklaser, 0, sizeof(checklaser));
}

void SetDownServo(){ //��� ������ �����÷��� 0 
    OCR1A = _DOWN; 
    OCR1B = _DOWN; 
    OCR1CH = _DOWN >> 8; 
    OCR1CL = _DOWN & 0xFF;   
    delay_ms(500);    
    memset(checklaser, 0, sizeof(checklaser));
}


void RemainSet() {
	switch (setnum)
	{
	case 5:
		clcd_inst(0x98);        // �׹�°�� ùĭ
		clcd_str("RemainSet:");
		break;
	case 4:
		clcd_inst(0x98);        // �׹�°�� ùĭ
		clcd_str("RemainSet:4");
		break;
	case 3:
		clcd_inst(0x98);        // �׹�°�� ùĭ
		clcd_str("RemainSet:3");
		break;
	case 2:
		clcd_inst(0x98);        // �׹�°�� ùĭ
		clcd_str("RemainSet:2");
		break;
	case 1:
		clcd_inst(0x98);        // �׹�°�� ùĭ
		clcd_str("RemainSet:1");
		break;
	}
}


void MainMenu(void){  
    //lcd
    clcd_clear();
	glcd_init();  //�׷��ȸ�� ����
	glcd_fill(0x0000); //��ĭ���� �ʱ�ȭ
    draw_han(0,0,0,5,gun);   //0��°ĭ 0���ٿ� 5���� ���� �Է�(�ѽ����� 5��)
    draw_han(6,32,0,1,img1);   //6ĭ 32���� ��Ʈ�̹��� ���� �»� 
    draw_han(7,32,0,1,img2);   //7ĭ 32���� 
    draw_han(6,48,0,1,img3);   //6ĭ 48����  
    draw_han(7,48,0,1,img4);  //7ĭ 48����  
    Myclcd_init(); //ĳ���͸�� ����
    delay_ms(1000); 
    
    //������  
    MP3_send_cmd(0x12,0,4); //���ϴ� �� ������, 0003.mp3���� ����
    SetUpServo();     
}


void TimeDP(void){
    clcd_inst(0x99);        // �׹�°�� ùĭ  
    clcd_str("Time");
    clcd_inst(0x9d);        // �׹�°�� ùĭ
    clcd_data((time/10)+48);   
    clcd_inst(0x9e);        // �׹�°�� ùĭ
    clcd_data((time%10)+48);    
    clcd_inst(0x90);        // ����°�� ùĭ
    clcd_data((setnum/10)+48);
    clcd_inst(0x91);        // ����°�� ùĭ 
    clcd_data((setnum%10)+48);   
    delay_ms(5);
}
