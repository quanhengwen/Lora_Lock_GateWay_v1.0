/*
********************************************************************************
*                            中国自主物联网操作系统
*                            Thread Operating System
*
*                              主芯片:STM32F401re
*
********************************************************************************
*文件名     : device_atk_as608.c
*作用       : ATK_AS608驱动
********************************************************************************
*版本     作者            日期            说明
*V0.1   Guolz       2019/4/5      初始版本
********************************************************************************
*/
#include "system.h"

////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用os,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "rtthread.h"					//os 使用	  
#endif


char handshake_status = handshake_pre;
uint32_t AS608_Addr = 0XFFFFFFFF; 		//默认
uint16_t ValidN;
SysPara AS608Para;						//指纹模块AS608参数


const char *EnsureMessage(uint8_t ensure) 
{
	const char *p;
	switch(ensure)
	{
		case  0x00:
			p="OK";break;		
		case  0x01:
			p="数据包接收错误";break;
		case  0x02:
			p="传感器上没有手指";break;
		case  0x03:
			p="录入指纹图像失败";break;
		case  0x04:
			p="指纹图像太干、太淡而生不成特征";break;
		case  0x05:
			p="指纹图像太湿、太糊而生不成特征";break;
		case  0x06:
			p="指纹图像太乱而生不成特征";break;
		case  0x07:
			p="指纹图像正常，但特征点太少（或面积太小）而生不成特征";break;
		case  0x08:
			p="指纹不匹配";break;
		case  0x09:
			p="没搜索到指纹";break;
		case  0x0a:
			p="特征合并失败";break;
		case  0x0b:
			p="访问指纹库时地址序号超出指纹库范围";
		case  0x10:
			p="删除模板失败";break;
		case  0x11:
			p="清空指纹库失败";break;	
		case  0x15:
			p="缓冲区内没有有效原始图而生不成图像";break;
		case  0x18:
			p="读写 FLASH 出错";break;
		case  0x19:
			p="未定义错误";break;
		case  0x1a:
			p="无效寄存器号";break;
		case  0x1b:
			p="寄存器设定内容错误";break;
		case  0x1c:
			p="记事本页码指定错误";break;
		case  0x1f:
			p="指纹库满";break;
		case  0x20:
			p="地址错误";break;
		default :
			p="模块返回确认码有误";break;
	}

	return p;	
}


//发送包头
static void SendHead(void){
    System.Device.Usart2.WriteData(0xEF);
    System.Device.Usart2.WriteData(0x01);
}
//发送地址
static void SendAddr(void)
{
	System.Device.Usart2.WriteData(((AS608Addr>>24)&0xff));
	System.Device.Usart2.WriteData(((AS608Addr>>16)&0xff));
 	System.Device.Usart2.WriteData(((AS608Addr>>8)&0xff));
	System.Device.Usart2.WriteData(AS608Addr&0xff);
}
//发送包标识,
static void SendFlag(uint8_t flag)
{
	System.Device.Usart2.WriteData(flag);
}
//发送包长度
static void SendLength(int length)
{
	System.Device.Usart2.WriteData(length>>8);
	System.Device.Usart2.WriteData(length);
}
//发送指令码
static void Sendcmd(u8 cmd)
{
	System.Device.Usart2.WriteData(cmd);
}
//发送校验和
static void SendCheck(u16 check)
{
	System.Device.Usart2.WriteData(check>>8);
	System.Device.Usart2.WriteData(check);
}



//uint8_t PS_HandShake(uint32_t *PS_Addr)
//{
//	if(handshake_status == handshake_pre){
//		SendHead();
//		SendAddr();
//		System.Device.Usart2.WriteData(0x01);
//		System.Device.Usart2.WriteData(0x00);
//		System.Device.Usart2.WriteData(0x00);	
//		delay_ms(500);
//	}else if(handshake_status == handshake_judging){		
//        handshake_status = handshake_done;
//        *PS_Addr=(rx2Buff[2]<<24) + (rx2Buff[3]<<16) + (rx2Buff[4]<<8) + (rx2Buff[5]);

//		memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf

//		LEDOn();
//		rt_thread_delay(100);
//		LEDOff();
//		rt_thread_delay(100);
//		LEDOn();
//		rt_thread_delay(10);
//		LEDOff();
//		
//        return 0;
//	}
//	
//	return 1;		
//}

static uint32_t PS_HandShake(void)
{
    uint32_t ret = 0xffffffff;
handshake_done:
    SendHead();
    SendAddr();
    System.Device.Usart2.WriteData(0x01);
    System.Device.Usart2.WriteData(0x00);
    System.Device.Usart2.WriteData(0x00);	
	rt_thread_delay(500);
    if(Uart2lenth >= 12){
        Uart2lenth = 0;
        ret = (rx2Buff[2]<<24) + (rx2Buff[3]<<16) + (rx2Buff[4]<<8) + (rx2Buff[5]);
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
        
        LEDOn();
        rt_thread_delay(100);
        LEDOff();
        rt_thread_delay(100);
        LEDOn();
        rt_thread_delay(100);
        LEDOff();
    }else{
        goto handshake_done;
    }
	
    return ret;
}


uint8_t PS_ValidTempleteNum(uint16_t *ValidN)
{
	uint16_t temp;
    uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x1d);
	temp = 0x01+0x03+0x1d;
	SendCheck(temp);
wait_rec_PS_ValidTempleteNum:    
    rt_thread_delay(500);
    if(Uart2lenth >= 14){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
        *ValidN = (rx2Buff[10]<<8) +rx2Buff[11];
        if(ensure==0x00){
            printf("\r\n有效指纹个数=%d个\r\n",(rx2Buff[10]<<8)+rx2Buff[11]);
        }
        else
            printf("\r\n%s",EnsureMessage(ensure));
        
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto wait_rec_PS_ValidTempleteNum;
    }
    	
	return ensure;
}

uint8_t PS_ReadSysPara(SysPara *p)
{
	uint16_t temp;
  	uint8_t ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);		//命令包标识
	SendLength(0x03);
	Sendcmd(0x0F);
	temp = 0x01+0x03+0x0F;
	SendCheck(temp);
wait_rec_PS_ReadSysPara:
    rt_thread_delay(500);
    if(Uart2lenth >= 28){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
		p->PS_max = (rx2Buff[14]<<8)+rx2Buff[15];
		p->PS_level = rx2Buff[17];
		p->PS_addr=(rx2Buff[18]<<24)+(rx2Buff[19]<<16)+(rx2Buff[20]<<8)+rx2Buff[21];
		p->PS_size = rx2Buff[23];
		p->PS_N = rx2Buff[25];
        	
        if(ensure==0x00)
        {
            printf("\r\n模块最大指纹容量=%d",p->PS_max);
            printf("\r\n对比等级=%d",p->PS_level);
            printf("\r\n地址=%x",p->PS_addr);
            printf("\r\n波特率=%d",p->PS_N*9600);
        }
        else 
            printf("\r\n%s",EnsureMessage(ensure));
        
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto wait_rec_PS_ReadSysPara;
    }

	return ensure;
}

static uint8_t FingerDetect(void){
	uint8_t ret = 0;
	
	if(HAL_GPIO_ReadPin(StaPORT,GPIO_PIN_10) == GPIO_PIN_SET){
		LEDOn();
		ret = 1;
	}else{
		LEDOff();
		ret = 0;
	}
	
	return ret;
}

//录入图像 PS_GetImage
//功能:探测手指，探测到后录入指纹图像存于ImageBuffer。 
//模块返回确认字
uint8_t PS_GetImage(void)
{
  	uint16_t temp;
  	uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);		//命令包标识
	SendLength(0x03);
	Sendcmd(0x01);
 	temp = 0x01+0x03+0x01;
	SendCheck(temp);
GetImage:   
    rt_thread_delay(500);
    if(Uart2lenth >= 12){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto GetImage;
    }
    
	return ensure;
}


//生成特征 PS_GenChar
//功能:将ImageBuffer中的原始图像生成指纹特征文件存于CharBuffer1或CharBuffer2			 
//参数:BufferID --> charBuffer1:0x01	charBuffer1:0x02												
//模块返回确认字
uint8_t PS_GenChar(uint8_t BufferID)
{
	uint16_t temp;
  	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x04);
	Sendcmd(0x02);
	System.Device.Usart2.WriteData(BufferID);
	temp = 0x01+0x04+0x02+BufferID;
	SendCheck(temp);

GenChar:   
    rt_thread_delay(500);
    if(Uart2lenth >= 12){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto GenChar;
    }
	
	return ensure;
}


//高速搜索PS_HighSpeedSearch
//功能：以 CharBuffer1或CharBuffer2中的特征文件高速搜索整个或部分指纹库。
//		  若搜索到，则返回页码,该指令对于的确存在于指纹库中 ，且登录时质量
//		  很好的指纹，会很快给出搜索结果。
//参数:  BufferID， StartPage(起始页)，PageNum（页数）
//说明:  模块返回确认字+页码（相配指纹模板）
uint8_t PS_HighSpeedSearch(uint8_t BufferID,uint16_t StartPage,uint16_t PageNum,SearchResult *p)
{
	uint16_t temp;
  	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x08);
	Sendcmd(0x1b);
	System.Device.Usart2.WriteData(BufferID);
	System.Device.Usart2.WriteData(StartPage>>8);
	System.Device.Usart2.WriteData(StartPage);
	System.Device.Usart2.WriteData(PageNum>>8);
	System.Device.Usart2.WriteData(PageNum);
	temp = 0x01+0x08+0x1b+BufferID
	+(StartPage>>8)+(uint8_t)StartPage
	+(PageNum>>8)+(uint8_t)PageNum;
	SendCheck(temp);
	
HighSpeedSearch:   
    rt_thread_delay(500);
    if(Uart2lenth >= 16){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
        p->pageID 	=(rx2Buff[10]<<8) +rx2Buff[11];
		p->mathscore=(rx2Buff[12]<<8) +rx2Buff[13];
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto HighSpeedSearch;
    }
    
	return ensure;
}


//刷指纹
void press_FR(void)
{
	SearchResult seach;
	uint8_t ensure;
	char *str;
	ensure=PS_GetImage();
	if(ensure==0x00)//获取图像成功 
	{	
        printf("获取图像成功 \r\n");
		//打开蜂鸣器	
		ensure=PS_GenChar(CharBuffer1);
		if(ensure==0x00) //生成特征成功
		{		
//			BEEP=0;//关闭蜂鸣器	
            printf("生成特征成功 \r\n");
			ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&seach);
			if(ensure==0x00)//搜索成功
			{				
				printf("刷指纹成功\r\n");				
				printf("确有此人,ID:%d  匹配得分:%d",seach.pageID,seach.mathscore);
			}
			else 
				printf("\r\n%s\r\n",EnsureMessage(ensure));			
	  }else{
			printf("\r\n%s\r\n",EnsureMessage(ensure));	
	  }

	  //关闭蜂鸣器
	 delay_ms(500);
	}
}

//精确比对两枚指纹特征 PS_Match
//功能:精确比对CharBuffer1 与CharBuffer2 中的特征文件 
//模块返回确认字
uint8_t PS_Match(void)
{
	uint16_t temp;
   	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);			//命令包标识
	SendLength(0x03);
	Sendcmd(0x03);
	temp = 0x01+0x03+0x03;
	SendCheck(temp);
ju_Match:    
    rt_thread_delay(500);
    if(Uart2lenth >= 14){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto ju_Match;
    }
    
	return ensure;
}

//合并特征（生成模板）PS_RegModel
//功能:将CharBuffer1与CharBuffer2中的特征文件合并生成 模板,结果存于CharBuffer1与CharBuffer2	
//说明:  模块返回确认字
uint8_t PS_RegModel(void)
{
	uint16_t temp;
    uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x05);
	temp = 0x01+0x03+0x05;
	SendCheck(temp);
RegModel:    
    rt_thread_delay(500);
    if(Uart2lenth >= 12){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto RegModel;
    }

	return ensure;		
}

//储存模板 PS_StoreChar
//功能:将 CharBuffer1 或 CharBuffer2 中的模板文件存到 PageID 号flash数据库位置。			
//参数:  BufferID @ref charBuffer1:0x01	charBuffer1:0x02
//       PageID（指纹库位置号）
//说明:  模块返回确认字
uint8_t PS_StoreChar(u8 BufferID,u16 PageID)
{
	uint16_t temp;
  	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x06);
	Sendcmd(0x06);
	System.Device.Usart2.WriteData(BufferID);
	System.Device.Usart2.WriteData(PageID>>8);
	System.Device.Usart2.WriteData(PageID);
	temp = 0x01+0x06+0x06+BufferID
	+(PageID>>8)+(u8)PageID;
	SendCheck(temp);
StoreChar:    
    rt_thread_delay(500);
    if(Uart2lenth >= 12){
        Uart2lenth = 0;
        ensure = rx2Buff[9];
        memset(rx2Buff,0x0,uart_rx_len);   //清空接收buf
    }else{
        goto StoreChar;
    }
//	data=JudgeStr(2000);
//	if(data)
//		ensure=data[9];
//	else
//		ensure=0xff;
	return ensure;	
}


//录指纹
void Add_FR(void)
{
	uint8_t i,ensure ,processnum=0;
	uint16_t ID;
	while(1)
	{
		switch (processnum)
		{
			case 0:
				i++;
				printf("请按指纹\r\n");
				ensure=PS_GetImage();
				if(ensure==0x00) 
				{
					//蜂鸣器响
					ensure=PS_GenChar(CharBuffer1);//生成特征
					//蜂鸣器停
					if(ensure==0x00)
					{
						printf("指纹正常\r\n");
						i=0;
						processnum=1;//跳到第二步						
					}else printf("\r\n%s\r\n",EnsureMessage(ensure));					
				}else printf("\r\n%s\r\n",EnsureMessage(ensure));						
				break;
			
			case 1:
				i++;
				printf("请按再按一次指纹\r\n");
				ensure=PS_GetImage();
				if(ensure==0x00) 
				{
					//蜂鸣器响
					ensure=PS_GenChar(CharBuffer2);//生成特征
					//蜂鸣器停
					if(ensure==0x00)
					{
						printf("指纹正常\r\n");
						i=0;
						processnum=2;//跳到第三步
					}else printf("\r\n%s\r\n",EnsureMessage(ensure));
				}else printf("\r\n%s\r\n",EnsureMessage(ensure));	
				break;

			case 2:
				printf("对比两次指纹\r\n");
				ensure=PS_Match();
				if(ensure==0x00) 
				{
					printf("对比成功,两次指纹一样\r\n");
					processnum=3;//跳到第四步
				}
				else 
				{
					printf("对比失败，请重新录入指纹\r\n");
					printf("\r\n%s\r\n",EnsureMessage(ensure));	
					i=0;
					processnum=0;//跳回第一步		
				}
				delay_ms(1200);
				break;

			case 3:
				printf("生成指纹模板\r\n");
				ensure=PS_RegModel();
				if(ensure==0x00) 
				{
					printf("生成指纹模板成功\r\n");
					processnum=4;//跳到第五步
				}else {processnum=0;printf("\r\n%s\r\n",EnsureMessage(ensure));}
				delay_ms(1200);
				break;
				
			case 4:	
				printf("请输入储存ID,按Enter保存\r\n");
				printf("0=< ID <=299\r\n");
                printf("ID = 1\r\n");
                ID = 1;
//				do
//					ID=GET_NUM();
//				while(!(ID<AS608Para.PS_max));//输入ID必须小于模块容量最大的数值
				ensure=PS_StoreChar(CharBuffer2,ID);//储存模板
				if(ensure==0x00) 
				{			
					printf("录入指纹成功\r\n");
					PS_ValidTempleteNum(&ValidN);//读库指纹个数
					printf("剩余容量%d\r\n",AS608Para.PS_max-ValidN);
					delay_ms(1500);
					return ;
				}else {processnum=0;printf("\r\n%s\r\n",EnsureMessage(ensure));}					
				break;				
		}
		delay_ms(400);
		if(i==5)//超过5次没有按手指则退出
		{
			printf("退出\r\n");
			break;	
		}				
	}
}

//清空指纹库 PS_Empty
//功能:  删除flash数据库中所有指纹模板
//参数:  无
//说明:  模块返回确认字
uint8_t PS_Empty(void)
{
	uint16_t temp;
  	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x0D);
	temp = 0x01+0x03+0x0D;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	return ensure;
}

//删除模板 PS_DeletChar
//功能:  删除flash数据库中指定ID号开始的N个指纹模板
//参数:  PageID(指纹库模板号)，N删除的模板个数。
//说明:  模块返回确认字
uint8_t PS_DeletChar(u16 PageID,u16 N)
{
	uint16_t temp;
  	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x07);
	Sendcmd(0x0C);
	System.Device.Usart2.WriteData(PageID>>8);
	System.Device.Usart2.WriteData(PageID);
	System.Device.Usart2.WriteData(N>>8);
	System.Device.Usart2.WriteData(N);
	temp = 0x01+0x07+0x0C
	+(PageID>>8)+(u8)PageID
	+(N>>8)+(u8)N;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	
	return ensure;
}


void Del_FR(void)
{
	uint8_t ensure;
	uint16_t num;
	printf("删除指纹\r\n");
	printf("请输入指纹ID按Enter发送\r\n");
	printf("0=< ID <=299\r\n");
	delay_ms(50);
//	num=GET_NUM();//获取返回的数值
	if(num==0xFFFF)
		goto MENU ; //返回主页面
	else if(num==0xFF00)
		ensure=PS_Empty();//清空指纹库
	else 
		ensure=PS_DeletChar(num,1);//删除单个指纹
	if(ensure==0){
		printf("删除指纹成功\r\n");
	}else
		printf("\r\n%s\r\n",EnsureMessage(ensure));
	delay_ms(1200);
	PS_ValidTempleteNum(&ValidN);//读库指纹个数
	printf("剩余容量%d\r\n",AS608Para.PS_max-ValidN);
MENU:	
	delay_ms(50);
}



/* 线程thread_Fingerprint_Detect_entry的入口函数 */
void thread_Fingerprint_Detect_entry(void* parameter)
{
	uint8_t ensure;
	static uint8_t det_flag = 0;
	static uint8_t last_det_flag = 0;
    AS608_Addr = PS_HandShake();
    printf("AS608_Addr = 0x%x\r\n",AS608_Addr);
	ensure = PS_ValidTempleteNum(&ValidN);  //读库指纹个数
	ensure = PS_ReadSysPara(&AS608Para); 	//读参数 
	if(ensure == 0x00){
		printf("库容量:%d     对比等级: %d",AS608Para.PS_max-ValidN,AS608Para.PS_level);
	}
	
	while (1)
	{
        det_flag = FingerDetect();
		if(last_det_flag != det_flag){
			last_det_flag = det_flag;

			if(det_flag == 1){
				press_FR();
                //Add_FR();
			}
		}
        
        

        rt_thread_delay(10);
	}
}



