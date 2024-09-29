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
A:  o o o x x x x x :lcd rs rw e psb=5v 병렬모드
B:  x x x x x o o o :pwm 3개 a b c 
D:  o o o x x x x x :lcd 컨트롤 스위치 위 아래 확인 EXT INT 0, 1, 2
E:  o o o x o o o x :Rx, Tx, sound, laserRx EXT INT 4, 5, 6
F:  o o o o o o o o :lcd data  //avcc 5v입력 



****************************/

/*
led 모듈쓸때는 자연광 간섭이 너무 심함
센서를 이용 센서 상승엣지 인터럽트로 변경필요
센서 디폴트 0v 레이져 수신시 5v 
디폴트. 서보모터 일어선 상태   3000넘어감 1000이 일어섬 
        ADCSRA = ADCSRA | 0b01000000; // ADSC = 1 변환 시작
        while((ADCSRA & 0x10) == 0); // ADIF=1이 될떄까지
        ad_val = (int)ADCL + ((int)ADCH << 8); // A/D 변환값 읽기   
        
레이저센서 저항달아서 사용 모듈 x
*/


unsigned int cnt = 0;
unsigned int time = 0;
//0 = 일어난상태 1 = 누운상태
unsigned char checklaser[3] = {0};
unsigned char setnum = 0;
unsigned int randomnum = 0;

//gamemode : 0 = off 1 = 연습모드, 2 = 제한시간 
//menumode : 0 = 초기화면 1 = 모드선택 2 = 연습모드 3 = 제한시간
unsigned char gamemode = 0, menumode = 0, cursor = 0;

//서보모터 
void SetUpServo();  //배열 모두 0초기화, 서보모터 일어섬
void SetDownServo(); //배열 모두 0 초기화 서보모터 내려감

//lcd dp용
void RemainSet();
void MainMenu();
void TimeDP();
//서보모터 2500변경

void main(void)
{
	//포트 입출력 정의 
    DDRA = 0xFF;    //lcd rs rw en
    DDRB = 0xE0;    //PB5,6,7 out  pwm, servo 
    DDRD = 0x00; 	//스위치 
    DDRE = 0x00;    //레이져 입력 외부 인터럽트
    DDRF = 0xFF;    //lcd data
     
    //외부인터럽트
    EIMSK = 0b01110111; //외부인터럽트 0,1,2,4,5,6 인에이블 
    EICRA = 0b00111111; //0,1,2 상승엣지, 풀다운스위치 
    EICRB = 0b00101010; //4,5,6 레이져수신 하강엣지 

    
    //타이머 0 초기설정: 제한시간 설정  
    TIMSK = 0x00;   //TOIE0 = 1   오버플로우 인터럽트 
    TCCR0 = 0x04;  // 프리스케일 = ck/64    
    TCNT0 = 6;  //1/16us * 64 * (256 - 6) = 1ms

    
    //타이머 1 초기설정 FAST PWM, servo
    TCCR1A=0xAA; TCCR1B=0x1A; // OCR1A -> OC Clear / OCR1A,B,C 활성화 / Fast PWM TOP = ICR1 / 8분주    
    OCR1A = _UP;       
    delay_ms(20);
    OCR1B = _UP; 
    delay_ms(20);
    OCR1CH = _UP >> 8; 
    OCR1CL = _UP & 0xFF;
    delay_ms(20);
    ICR1=19999; 
    SetDownServo(); //초기부팅진행 확인용 서보모터 
    
    //USART 통신
	USART0_Init(); // 9600/8/n/1/n
	dfplayer_init();     
    
	/*MP3_send_cmd(0x12,0,3); //원하는 곡 실행모드, 0003.mp3파일 실행*/
	
	//GLCD 
	glcd_init();  //그래픽모드 시작
	glcd_fill(0x0000); //빈칸으로 초기화
    draw_han(0,0,0,5,gun);   //0번째칸 0번줄에 5개의 글자 입력(총쏘기게임 5자)
    draw_han(6,32,0,1,img1);   //6칸 32번줄 도트이미지 생성 좌상 
    draw_han(7,32,0,1,img2);   //7칸 32번줄 
    draw_han(6,48,0,1,img3);   //6칸 48번줄  
    draw_han(7,48,0,1,img4);  //7칸 48번줄  
    Myclcd_init(); //캐릭터모드 시작
    delay_ms(1000); 
    /*

    */
    
    //인터럽트 활성화 
    SREG = 0x80;
    
    //대기상태  
    MP3_send_cmd(0x12,0,4); //원하는 곡 실행모드, 0003.mp3파일 실행
    SetUpServo();     //부팅완료 

    
    
    //loop 
    
    while(1)
    {   
		switch(menumode){
			case _SELECTMODE: //select mode
                clcd_inst(0x80);        // 첫줄 첫칸
				clcd_str("Select Mode");
				clcd_inst(0x90);        // 두번째줄 첫칸
				clcd_str("Practice "); if (cursor == 0) clcd_data(0x11); else clcd_data(0x20);
				clcd_inst(0x88);        // 세번째줄 첫칸
				clcd_str("TimeAttack "); if (cursor == 1) clcd_data(0x11); else clcd_data(0x20);  
                delay_ms(50);
			break;
			
			case _DPARCADE:
				clcd_clear();
				clcd_inst(0x80);        // 첫줄 첫칸
				clcd_str("Practice!!!");
				
			break;
			
			case _DPTIMEATTACK:
				clcd_clear();
				clcd_inst(0x80);        // 첫줄 첫칸
				clcd_str("TimeAttack!!!");
			break;
			
			default:
				clcd_inst(0x90);        // 두번째줄 첫칸
		        clcd_str("Press AnyKey");   
		        delay_ms(1000);
		        clcd_inst(0x90);        // 두번째줄 첫칸
		        clcd_str("            ");
		        delay_ms(1000); 
			break;	
		}
		
		/*
		clcd_inst(0x80);        // 첫줄 첫칸
        clcd_inst(0x81);        // 첫줄 세번째칸
        clcd_inst(0x90);        // 두번째줄 첫칸
        clcd_inst(0x88);        // 세번째줄 첫칸
        clcd_inst(0x8b);        // 세번째줄 일곱번째칸
        clcd_inst(0x8f);        // 세번째줄 열다섯째칸
        clcd_inst(0x98);        // 네번째줄 첫칸
        clcd_data(0x01); 
        clcd_str("Hi!");
		*/

        switch (gamemode){
            case _ARCADE: //아케이드모드 
                //5세트    
                MP3_send_cmd(0x12,0,2); //원하는 곡 실행모드, 0002.mp3파일 실행
                setnum = 5;   
                clcd_inst(0x98);        // 네번째줄 첫칸
	        	clcd_str("RemainSet:5");
				while(setnum!=0){ //세트0될때까지 반복 
	                if((checklaser[0] & checklaser[1] & checklaser[2]) == 1){
	                    setnum--;   
	                    SetUpServo(); 	
						RemainSet();
	                } 
            	}
                gamemode = _IDLE; //루프 탈출시 게임 종료 
                menumode = _IDLE;          
                MainMenu();
            break;
			////////////////////////////////
            case _TIMEATTACK: //제한시간모드 
                TIMSK = 0x01; //오버플로우 인에이블 
                cnt = 0; 
                setnum = 0;   
                time = 60;
                MP3_send_cmd(0x12,0,3); //원하는 곡 실행모드, 0002.mp3파일 실행
                while(TIMSK != 0x00){  
                    randomnum = rand() % 100000;   
                    
                    if((randomnum < 3) & (checklaser[0] == 1)){  // 20%로 표적이 내려가있을 떄  
                        checklaser[0] = 0;  //올라간 상태로 변경
                        OCR1A = _UP;        //서보모터 업
                    }   
                    
                    if(((randomnum > 3) & (randomnum < 6)) & (checklaser[1] == 1)){ // 20%로 표적이 내려가있을 떄 
                        checklaser[1] = 0;
                        OCR1B = _UP; 
                    }  
                    
                    if(((randomnum > 6) & (randomnum < 9)) & (checklaser[2] == 1)){  // 20%로 표적이 내려가있을 때
                        checklaser[2] = 0;
                        OCR1CH = _UP >> 8; 
                        OCR1CL = _UP & 0xFF; 
                    }   
                    
                    if(checklaser[0] == checklaser[1] == checklaser[2] == 1){
                        checklaser[0] = 0;  //올라간 상태로 변경
                        OCR1A = _UP;        //서보모터 업  
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

///////////////외부인터럽트 0,1,2/////////////
//외부인터럽트 0,1,2 lcd제어 스위치
//
interrupt [EXT_INT0] void SwUp(void)
{



	//두번째메뉴
	if (menumode == _SELECTMODE) cursor=0;


    //첫메뉴
		if(menumode == 0){ menumode++; clcd_clear();}  

	
}

interrupt [EXT_INT1] void SwDown(void)
{

	//두번째메뉴
	if (menumode == _SELECTMODE) cursor = 1;


	//첫메뉴
    if (menumode == 0) {menumode++;  clcd_clear();} 

}

interrupt [EXT_INT2] void SwOK(void)
{

	//두번째메뉴
	if (menumode == _SELECTMODE) {
		switch (cursor)
		{
		case 0:
			gamemode = _ARCADE;
			menumode = _DPARCADE; 
            clcd_clear();
			clcd_inst(0x80);        // 첫줄 첫칸
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
    //첫메뉴
    if (menumode == 0) {menumode=_SELECTMODE; clcd_clear();}                                 

}

///////////////////////외부인트럽트 4,5,6/////////////////
//외부인터럽트 4,5,6 게임진행 중일때, 표적이 서있을때 수신되면 셋
//레이져1 서버모터1의 상호작용
interrupt [EXT_INT4] void LaserRx1(void)    
{
    if((gamemode == _ARCADE) & (checklaser[0] == 0)){
        checklaser[0] = 1;
        OCR1A = _DOWN;
		clcd_inst(0x8b);        // 두번째줄 첫칸
		clcd_str("Good!");
        delay_ms(500);  
        clcd_inst(0x8b);        // 두번째줄 첫칸
        clcd_str("           "); 
    } 
    
    if((gamemode == _TIMEATTACK) & (checklaser[0] == 0)){
        checklaser[0] = 1;
        OCR1A = _DOWN;
		clcd_inst(0x8b);        // 두번째줄 첫칸
		clcd_str("Good!"); 
        setnum++;
        delay_ms(500);  
        clcd_inst(0x8b);        // 두번째줄 첫칸
        clcd_str("         ");      
        delay_ms(50);  
    } 
}


//레이져2 서버모터2의 상호작용
interrupt [EXT_INT5] void LaserRx2(void)
{
    if((gamemode == _ARCADE) & (checklaser[1] == 0)){
        checklaser[1] = 1;
        OCR1B = _DOWN;  
		clcd_inst(0x8b);        // 두번째줄 첫칸
		clcd_str("Good!");
        delay_ms(500);
        clcd_inst(0x8b);        // 두번째줄 첫칸
        clcd_str("        ");     
    }  
    
    if((gamemode == _TIMEATTACK) & (checklaser[1] == 0)){
        checklaser[1] = 1;
        OCR1B = _DOWN;
		clcd_inst(0x8b);        // 두번째줄 첫칸
		clcd_str("Good!"); 
        setnum++;
        delay_ms(500);  
        clcd_inst(0x8b);        // 두번째줄 첫칸
        clcd_str("       ");     
        delay_ms(50);  
    } 
}


//레이져3 서버모터3의 상호작용
interrupt [EXT_INT6] void LaserRx3(void)
{
    if((gamemode == _ARCADE) & (checklaser[2] == 0)){
        checklaser[2] = 1;
        OCR1CH = _DOWN >> 8; 
        OCR1CL = _DOWN & 0xFF;
		clcd_inst(0x8b);        // 두번째줄 첫칸
		clcd_str("Good!");
        delay_ms(500);  
        clcd_inst(0x8b);        // 두번째줄 첫칸
        clcd_str("         ");   
        
        
    }
    
    if((gamemode == _TIMEATTACK) & (checklaser[2] == 0)){
        checklaser[2] = 1;
        OCR1CH = _DOWN >> 8; 
        OCR1CL = _DOWN & 0xFF;
		clcd_inst(0x8b);        // 두번째줄 첫칸
		clcd_str("Good!");
        setnum++;
        delay_ms(500);  
        clcd_inst(0x8b);        // 두번째줄 첫칸
        clcd_str("         ");      
        delay_ms(50);  
    } 
}
//////////////////////////////////////////////////

interrupt [TIM0_OVF] void timer0(void)
{
    TCNT0 = 6;          //초기값 재설정 
    
    cnt++;  //인터럽트횟수 +1 
    if(cnt % 1000 == 0) {
        time--; 
        TimeDP();      
    }
    if(cnt == 1000 * 60){ //1000ms * 60 = 1min 
        cnt = 0; 
        TIMSK = 0x00;  
        gamemode = _IDLE; //루프 탈출시 게임 종료 
        menumode = _IDLE;          
        MainMenu();
    }
}


void SetUpServo(){ //모두 올리고 수신플래그 0 초기화 
    OCR1A = _UP; 
    OCR1B = _UP; 
    OCR1CH = _UP >> 8; 
    OCR1CL = _UP & 0xFF;   
    delay_ms(500);    
    memset(checklaser, 0, sizeof(checklaser));
}

void SetDownServo(){ //모두 내리고 수신플래그 0 
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
		clcd_inst(0x98);        // 네번째줄 첫칸
		clcd_str("RemainSet:");
		break;
	case 4:
		clcd_inst(0x98);        // 네번째줄 첫칸
		clcd_str("RemainSet:4");
		break;
	case 3:
		clcd_inst(0x98);        // 네번째줄 첫칸
		clcd_str("RemainSet:3");
		break;
	case 2:
		clcd_inst(0x98);        // 네번째줄 첫칸
		clcd_str("RemainSet:2");
		break;
	case 1:
		clcd_inst(0x98);        // 네번째줄 첫칸
		clcd_str("RemainSet:1");
		break;
	}
}


void MainMenu(void){  
    //lcd
    clcd_clear();
	glcd_init();  //그래픽모드 시작
	glcd_fill(0x0000); //빈칸으로 초기화
    draw_han(0,0,0,5,gun);   //0번째칸 0번줄에 5개의 글자 입력(총쏘기게임 5자)
    draw_han(6,32,0,1,img1);   //6칸 32번줄 도트이미지 생성 좌상 
    draw_han(7,32,0,1,img2);   //7칸 32번줄 
    draw_han(6,48,0,1,img3);   //6칸 48번줄  
    draw_han(7,48,0,1,img4);  //7칸 48번줄  
    Myclcd_init(); //캐릭터모드 시작
    delay_ms(1000); 
    
    //대기상태  
    MP3_send_cmd(0x12,0,4); //원하는 곡 실행모드, 0003.mp3파일 실행
    SetUpServo();     
}


void TimeDP(void){
    clcd_inst(0x99);        // 네번째줄 첫칸  
    clcd_str("Time");
    clcd_inst(0x9d);        // 네번째줄 첫칸
    clcd_data((time/10)+48);   
    clcd_inst(0x9e);        // 네번째줄 첫칸
    clcd_data((time%10)+48);    
    clcd_inst(0x90);        // 세번째줄 첫칸
    clcd_data((setnum/10)+48);
    clcd_inst(0x91);        // 세번째줄 첫칸 
    clcd_data((setnum%10)+48);   
    delay_ms(5);
}
