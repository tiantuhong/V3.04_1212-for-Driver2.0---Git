/******************************************************************************

                  版权所有 (C), 1994-2016, 杭州九阳欧南多小家电有限公司

 ******************************************************************************
  文 件 名   : Robit_Run.c
  版 本 号   : V1.0
  作    者   : 田宏图
  生成日期   : 2016年3月16日
  最近修改   :
  功能描述   : 扫地机行走控制
  函数列表   :
  修改历史   :
  1.日    期   : 2016年3月16日
    作    者   : 田宏图
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "stm32f10x.h"
#include <string.h>
#include <math.h>
/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/
void Robit_RunLR(uint8_t LeftMotor_Speed,uint8_t LeftMotor_Dir,uint8_t RightMotor_Speed,uint8_t RightMotor_Dir);
void Robit_RunDir(uint8_t Speed,uint8_t Dir);
void Robit_Stop(void);
/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/
int16_t EdageRunAngle(void);
uint16_t CalAngleDif(int16_t Angle1, int16_t Angle2);
/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
 
Motor_TypeDef LeftMotor,RightMotor;
MotorSpeed_TypeDef LeftMotorSpeed,RightMotorSpeed;
FunctionalState AdjustSpeedEn,MotorLinerModeEn, AccCtrlTimeEn;
int16_t Sys_Startangle;
int16_t AnglerateTarg, AnglerateTargInit;

uint8_t SpeedAdjTime;   //轮子速度PID调节时间

///速度档位划分为0~10  11个档位
uint8_t Robit_RunSpeed;

uint32_t TargMileage,LeftCurMileage,RightCurMileage, TargMileageForArc;
FunctionalState RobitRouteModeEn;
FunctionalState MotorStartupEn,MotorStopCtrlEn,RobitModeCtrlEn;
FunctionalState VaccEn, WaccEn; //线加速使能标志 ， 角加速度使能标志
uint8_t VaccInit = 0, WaccInit = 0;

int32_t  LeftMileageAll,RightMileageAll; //左右轮累计里程数，前进增加，后退减小。
int32_t  LeftMileageAllPre,RightMileageAllPre; //左右轮累计里程数，前进增加，后退减小。

uint8_t  TargCirle;
uint16_t TargAngle; // 设定旋转角度
uint16_t RotationAngleCur; //当前旋转角度
uint16_t StepModeCnt;

///扫地机运行状态  参数

uint8_t RobitStatus;  				// 0--停止状态 1-- 工作状态
uint8_t RobitRunningMode;           //扫地机工作模式  0--开环工作方式 1--闭环工作方式
uint8_t RobitRunningClosedLoopMode; //扫地机闭环工作方式  0--速度闭环方式 1--行程闭环工作方式
uint8_t RobitRunbyRouteMode;		//扫地机行程闭环方式  0--直线行驶目标里程数1--轴心转动目标角度  2--沿墙行走模式

FlagStatus LeftMotorSwDirFlag,RightMotorSwDirFlag;//扫地机速度闭环模式中，方向切换标志位
uint16_t   LeftMotorSpeedBak,RightMotorSpeedBak;//扫地机速度闭环模式中，方向切换时的速度备份
MOTOR_RUNDIR_TypeDef    LeftMotorDirBak,RightMotorDirBak;//扫地机速度闭环模式中，方向切换时的方向备份



uint8_t Robit_Dir;    //0---前进  1---后退
int32_t RobotLineSpeed,RobotLineSpeedTarg, RobotAngleRate; //机器人线速度(mm/s)和角速度(毫弧度/s)
int16_t AngularVelocityOdo;                                //里程计计算的角速度

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
uint8_t LeftMotorSpeedCnt,RightMotorSpeedCnt;
uint32_t LeftMotorSpeedSum,RightMotorSpeedSum;
uint8_t MotorLinerModeCnt;

uint16_t LeftMotorInterCnt,RightMotorInterCnt;
uint16_t LeftMotorErrorCnt,RightMotorErrorCnt;

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

const uint16_t Speed_Mileage[9] = 
{
	4500,4242,3969,3674,3354,3000,2598,2121,1500
};

const uint8_t Speed_Angle[9] = 
{
	64, 60, 56, 52, 47, 42, 37, 30, 21
};

const uint8_t Angle_Num[41] = 
{
// 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 , 20 ,
   19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19 , 19 , 19 , 19 , 19 , 19 , 19 , 19 , 19 , 18 , 18 ,	
// 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 , 32 , 33 , 34 , 35 , 36 , 37 , 38 , 39 , 40,
   18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17 , 16 , 16 , 16 , 16 , 16 , 15 , 15 , 15 , 15	
};


/*****************************************************************************
 函 数 名  : RobitMode_Conf
 功能描述  : 扫地机工作模式初始化
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年4月8日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void RobitMode_Conf(void)
{
	RobitStatus = 0;  				//默认扫地机停止状态
	RobitRunningMode = 0;			//默认工作在开环状态
	RobitRunningClosedLoopMode = 0;	//闭环模式默认速度闭环
	RobitRunbyRouteMode = 0;		//行程闭环模式默认直线行驶方式
    MotorLinerModeEn = DISABLE;
    MotorLinerModeCnt = 0;
    SpeedAdjTime = 5;
    
    LeftMotorErrorCnt = 0;
    RightMotorErrorCnt = 0;
    LeftMotorInterCnt = 0;
    RightMotorInterCnt = 0;
    
    LeftMileageAllPre = 0;
	RightMileageAllPre = 0;
    
    RightMotor.SpeedAdjAll = 0;
    LeftMotor.SpeedAdjAll = 0;
    Robit_Dir = 0;
    RobotLineSpeedTarg = 0;
    AnglerateTarg = 0;
}

/*****************************************************************************
 函 数 名  : RobitMode_Ctrl
 功能描述  : 扫地机工作模式处理,判断是否由工作状态进入停止状态
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 

1.判断方式 左右轮的状态都是停止状态认为扫地机已经进入停止转态
2.由停止->工作状态的判定在各个进入工作状态时的位置时给定
3.本函数1ms调用一次

4.速度闭环模式时，方向切换处理
 
 修改历史      :
  1.日    期   : 2016年4月8日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void RobitMode_Ctrl(void)
{
	if(!RobitModeCtrlEn)
		return;
	RobitModeCtrlEn = DISABLE;

	if(LeftMotor.Status == SYS_STOP && RightMotor.Status == SYS_STOP)
	{
		if(RobitStatus != 0)
		{
			RobitStatus = 0;
						
            RobotLineSpeed = 0;
            
            RobotLineSpeedTargPre = 0;
            RobotAngleRatePre = 0;
            
            
            //RightMotorSwDirFlag = 1 或者LeftMotorSwDirFlag = 1时，说明定量模块还没有执行完成，不能清零该标志位
            // 2017 07 27 清零标志位，会导致某些情况下，机器原地旋转
            // 测试
            if(!(RightMotorSwDirFlag || LeftMotorSwDirFlag))
            {
                //此标志位用于指示 定量直线行走/定量角度旋转/定量画弧线/角速度线速度调整 
                RobitRouteModeEn = DISABLE;
            }
            
		}
	}
	
    //在PathPlanMode = 0时，碰撞后退 否则碰撞后只停止
	if(RobitStatus == 0 && BumpFlag == ENABLE)
	{
        if(0 == PathPlanMode)
        {
            BumpFlag = DISABLE;
            SetRobitMileageRun(70, 2, 110);
        }
        else
        {
            // 沿墙模式时，在执行沿墙时会执行特定的后退程序，并且处理标志位，此处不再处理
//            BumpFlag = DISABLE;
//            BumpOverFlag = ENABLE;
            BumpOverForCloseWallFlag = DISABLE;
        }

	}

	if(RobitStatus == 0 && BumpFlag == DISABLE)
	{
		if(DISABLE == BumpOverFlag)
		{
			BumpOverFlag = ENABLE;
            BumpOverForCloseWallFlag = ENABLE;
//            BumpBak = 0;
		}
	}
	
	if(RobitRunningMode == 1 /*&& RobitRunningClosedLoopMode == 0 */&& LeftMotorSwDirFlag == SET)
	{
		if(LeftMotor.Status == SYS_STOP)
	  	{
			LeftMotorEnterRun(LeftMotorDirBak);		
			
			LeftMotor.TargSpeed = LeftMotorSpeedBak;
			
			LeftMotorSwDirFlag = RESET;
            RobitStatus = 1;
            RobitRunningMode = 1;
 //           RobitRunningClosedLoopMode = 0;
	  	}

		
	}

	if(RobitRunningMode == 1 /*&& RobitRunningClosedLoopMode == 0*/ && RightMotorSwDirFlag == SET)
	{
		
		//如果处于停止中，则启动电机
      	if(RightMotor.Status == SYS_STOP)
      	{
			RightMotorEnterRun(RightMotorDirBak);
			
			RightMotor.TargSpeed = RightMotorSpeedBak;

			RightMotorSwDirFlag = RESET;
            RobitStatus = 1;
            RobitRunningMode = 1;
 //           RobitRunningClosedLoopMode = 0;
      	}
	}



//	if(WallSignal[0] == 1 || WallSignal[1] == 1 ||WallSignal[2] == 1 ||WallSignal[3] == 1 ||WallSignal[4] == 1 )
//	{
//		RightMotorStop();
//		LeftMotorStop();	

//		LeftMotorSwDirFlag = RESET;            
//		RightMotorSwDirFlag = RESET;
//	}
}

/*****************************************************************************
 函 数 名  : Motor_Conf
 功能描述  : 电机数据初始化
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年4月1日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void Motor_Conf(void)
{
	RightMotor.Status = SYS_STOP;
	LeftMotor.Status = SYS_STOP;

	RightMotor.PWMVal = 0;
	LeftMotor.PWMVal = 0;

	RightMotor.CurSpeed = 0;
	LeftMotor.CurSpeed = 0;

	RightCurMileage = 0;
	LeftCurMileage = 0;

	LeftMileageAll = 0;
	RightMileageAll = 0;
	LeftMileageAllPre = 0;
	RightMileageAllPre = 0;

	RobitRouteModeEn = DISABLE;
}

/*****************************************************************************
 函 数 名  : RightMotorRunInit
 功能描述  : 右轮电机前初始化
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月18日
    作    者   : 田宏图
    修改内容   : 新生成函数
  2.日    期   : 2017年7月25日
    作    者   : 田宏图
    修改内容   : 修复堵转状态不正常的BUG，堵转状态也可以转变成启动状态

*****************************************************************************/
void RightMotorRunInit(void)
{
	if(RightMotor.Status == SYS_STOP || RightMotor.Status == SYS_BLOCKING)
	{
		RightMotor.RunEnable = ENABLE;
		RightMotor.RunFlag = RESET;
		RightMotor.SpeedRisedFlag = RESET;
		RightMotor.RunCycle = 0;

		RightMotorSpeedSum = 0;
		RightMotorSpeedCnt = 0;

		RightMotorSpeed.OverflowCnt = 0;
		RightMotorSpeed.PreCountValue = TIM_GetCounter(TIM2);
		RightMotorSpeed.PulseCnt = 0;
		RightMotor.Status = SYS_STARTUP;
	}
}


//修复堵转状态不正常的BUG，堵转状态也可以转变成启动状态 2016.07.25
void LeftMotorRunInit(void)
{
	if(LeftMotor.Status == SYS_STOP || LeftMotor.Status == SYS_BLOCKING)
	{
		LeftMotor.RunEnable = ENABLE;
		LeftMotor.RunFlag = RESET;
		LeftMotor.SpeedRisedFlag = RESET;
		LeftMotor.RunCycle = 0;

		LeftMotorSpeedSum = 0;
		LeftMotorSpeedCnt = 0;

		LeftMotorSpeed.OverflowCnt = 0;
		LeftMotorSpeed.PreCountValue = TIM_GetCounter(TIM2);
		LeftMotor.Status = SYS_STARTUP;	
		LeftMotorSpeed.PulseCnt = 0;
	}
}

void CalSpeed(void)
{
    uint16_t MotorSpeedTmp;
	if(RightMotorSpeed.CalEn)
	{
		RightMotorSpeed.CalEn = DISABLE;

		MotorSpeedTmp = (uint16_t)(Motor_K / RightMotorSpeed.CircleTime);

		if(RightMotorSpeedCnt < 16)
		{
			RightMotorSpeedSum += MotorSpeedTmp;
			RightMotor.CurSpeed = MotorSpeedTmp;
			RightMotorSpeedCnt ++;
		}
		else
		{
			RightMotor.CurSpeed = RightMotorSpeedSum >> 4;
			RightMotorSpeedSum += MotorSpeedTmp;
			RightMotorSpeedSum -= RightMotor.CurSpeed;
		}

		RightMotorSpeed.Valid = ENABLE;
        RightMotorSpeed.ValidForAnguar = ENABLE;
	}

	if(LeftMotorSpeed.CalEn)
	{
		LeftMotorSpeed.CalEn = DISABLE;		

		MotorSpeedTmp = (uint16_t)(Motor_K / LeftMotorSpeed.CircleTime);

		if(LeftMotorSpeedCnt < 16)
		{
			LeftMotorSpeedSum += MotorSpeedTmp;
			LeftMotor.CurSpeed = MotorSpeedTmp;
			LeftMotorSpeedCnt ++;
		}
		else
		{
			LeftMotor.CurSpeed = LeftMotorSpeedSum >> 4;
			LeftMotorSpeedSum += MotorSpeedTmp;
			LeftMotorSpeedSum -= LeftMotor.CurSpeed;
		}

		LeftMotorSpeed.Valid = ENABLE;
        LeftMotorSpeed.ValidForAnguar = ENABLE;
	}
    
    
}

//input: 变量:左轮速度、右轮速度  
//       方向:前进为正，后退为负 
//       单位:RPM
//return:变量:角速度 
//       方向:顺时针为正，逆时针为负        
//       单位:毫弧度/s
int16_t CalOdoAngularVelocity(int16_t leftSpeed, int16_t rightSpeed)
{
    int16_t Wheelbase = ROBOT_WHEEL_AXIS;
    
    return ((rightSpeed - leftSpeed ) * RPM2MMPERSEC * 1000 / Wheelbase);
}


/*****************************************************************************
 函 数 名  : LeftMotorEnterRun
 功能描述  : 左轮进入运动
 输入参数  : uint16_t LeftMotor_Speed  
             uint8_t LeftMotor_Dir     
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月31日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void LeftMotorEnterRun(uint8_t LeftMotor_Dir)
{
	if(LeftMotor_Dir > 1)
		LeftMotor_Dir = 1;

	
    LeftMotorRunInit();
	InitVar_PID(1);

	LeftMotor.PWMVal = PWM_STATRUP_VAL;
	LeftMotor.Dir = (MOTOR_RUNDIR_TypeDef)LeftMotor_Dir;
	
	if(LeftMotor_Dir == 0)
	{
//		Motor_L_F_PWM(LeftMotor.PWMVal);
//		Motor_L_B_PWM(0);
		Motor_L_F_Run(LeftMotor.PWMVal);
	}
	else
	{
//		Motor_L_F_PWM(0);
//		Motor_L_B_PWM(LeftMotor.PWMVal);
		Motor_L_B_Run(LeftMotor.PWMVal);
	}
	
}

/*****************************************************************************
 函 数 名  : RightMotorEnterRun
 功能描述  : 右轮进入运行
 输入参数  : uint8_t RightMotor_Dir  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年4月1日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void RightMotorEnterRun(uint8_t RightMotor_Dir)
{
	if(RightMotor_Dir > 1)
		RightMotor_Dir = 1;	
	

    RightMotorRunInit();
	InitVar_PID(0);

	RightMotor.PWMVal = PWM_STATRUP_VAL;
	RightMotor.Dir = (MOTOR_RUNDIR_TypeDef)RightMotor_Dir;
	if(RightMotor_Dir == 0)
	{		
//		Motor_R_F_PWM(RightMotor.PWMVal);
//		Motor_R_B_PWM(0);
		Motor_R_F_Run(RightMotor.PWMVal);
	}
	else
	{
//		Motor_R_F_PWM(0);
//		Motor_R_B_PWM(RightMotor.PWMVal);
		Motor_R_B_Run(RightMotor.PWMVal);
	}
	
}

/*****************************************************************************
 函 数 名  : MotorStartupCtrl
 功能描述  : 电机启动控制
 输入参数  : void    
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 	1.50ms调用一次
 	2.每次调用时,PWM 占空比增加一个启动步长(50)
 修改历史      :
  1.日    期   : 2016年4月7日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void MotorStartupCtrl(void)
{
	if(!MotorStartupEn)
		return;

	if(RobitStatus == 0 || (RobitStatus == 1 && RobitRunningMode != 1))
		return;

	MotorStartupEn = DISABLE;
	
	if(LeftMotor.Status == SYS_STARTUP)
	{
		if(LeftMotor.PWMVal <= PWM_STATRUP_VAL_MAX - PWM_STATRUP_VAL_STEP)
			LeftMotor.PWMVal += PWM_STATRUP_VAL_STEP;
		
		if(LeftMotor.Dir == MOTOR_RUN_DIR_ZW)
		{
//			Motor_L_F_PWM(LeftMotor.PWMVal);
//			Motor_L_B_PWM(0);
			Motor_L_F_Run(LeftMotor.PWMVal);
		}
		else
		{
//			Motor_L_F_PWM(0);
//			Motor_L_B_PWM(LeftMotor.PWMVal);
			Motor_L_B_Run(LeftMotor.PWMVal);
		}
		
	}

	if(RightMotor.Status == SYS_STARTUP)
	{
		if(RightMotor.PWMVal <= PWM_STATRUP_VAL_MAX - PWM_STATRUP_VAL_STEP)
			RightMotor.PWMVal += PWM_STATRUP_VAL_STEP;
		
		if(RightMotor.Dir == MOTOR_RUN_DIR_ZW)
		{		
//			Motor_R_F_PWM(RightMotor.PWMVal);
//			Motor_R_B_PWM(0);
			Motor_R_F_Run(RightMotor.PWMVal);
		}
		else
		{
//			Motor_R_F_PWM(0);
//			Motor_R_B_PWM(RightMotor.PWMVal);
			Motor_R_B_Run(RightMotor.PWMVal);
		}
		
	}
}

/*****************************************************************************
 函 数 名  : AccCtrl
 功能描述  : 加速度控制
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 				1.5ms调用一次
 				2.加速度控制
 修改历史      :
  1.日    期   : 2017年7月17日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void AccCtrl(void)
{
    static uint16_t BaseSpeedBak = 0;
    static int16_t AngleRateTargBak = 0;
    //电机处于运行状态
    if(RightMotor.Status != SYS_RUNNING && LeftMotor.Status != SYS_RUNNING)
        return;
    
    //定时时间是否到()
    if(!AccCtrlTimeEn)
        return;
    AccCtrlTimeEn = DISABLE;
    
    //线加速度使能标志
    if(VaccEn)
    {
        //线加速度参数初始化
        if(!VaccInit)
        {
            VaccInit = 1;
            BaseSpeedBak = RightMotor.BaseSpeed;
            
            if(RightMotor.CurSpeed <= MOT_RPM_SPEED_MIN && LeftMotor.CurSpeed <= MOT_RPM_SPEED_MIN)
            {
                RightMotor.BaseSpeed = MOT_RPM_SPEED_MIN;
                LeftMotor.BaseSpeed = MOT_RPM_SPEED_MIN;
            }
            else if(RightMotor.CurSpeed + LeftMotor.CurSpeed <= RightMotor.BaseSpeed * 2)
            {
                RightMotor.BaseSpeed = (RightMotor.CurSpeed + LeftMotor.CurSpeed) / 2;
                LeftMotor.BaseSpeed = RightMotor.BaseSpeed;
            }
            else
            {
                //basespeed不变
            }
            
            MotorLinerModeCnt = 10;
        }
        
        //加速过程
        if(RightMotor.BaseSpeed + RightMotor.Vacc < BaseSpeedBak)
        {
            RightMotor.BaseSpeed += RightMotor.Vacc;
            LeftMotor.BaseSpeed += LeftMotor.Vacc;
        }
        //加速完成
        else
        {
            RightMotor.BaseSpeed = BaseSpeedBak;
            LeftMotor.BaseSpeed = BaseSpeedBak;
            VaccEn = DISABLE;
            VaccInit = 0;
        }
    }
    
    //角加速度使能标志
    if(WaccEn)
    {
        //角加速度参数初始化
        if(!WaccInit)
        {
            WaccInit = 1;
            AngleRateTargBak = AnglerateTarg;
            MotorLinerModeCnt = 10;
            
            if(AngleRateTargBak > 0)
            {
                if(Sys_Anglerate < ANGLERATE_MIN)
                {
                    AnglerateTarg = ANGLERATE_MIN;                   
                }
                else if(Sys_Anglerate <= AnglerateTarg)
                {
                    AnglerateTarg = Sys_Anglerate;
                }
                else
                {
                    //不变
                }
                
                if(AnglerateTarg < AngleRateTargBak)
                {
                    RightMotor.SpeedAdjAll = AnglerateTarg * RightMotor.SpeedAdjAll / AngleRateTargBak;
                    LeftMotor.SpeedAdjAll = AnglerateTarg * LeftMotor.SpeedAdjAll / AngleRateTargBak;
                }
            }
            else
            {
                if(Sys_Anglerate > -ANGLERATE_MIN)
                {
                    AnglerateTarg = -ANGLERATE_MIN;       
                }
                else if(Sys_Anglerate > AnglerateTarg)
                {
                    AnglerateTarg = Sys_Anglerate;
                }
                else
                {
                    //不变
                }
                
                if(AnglerateTarg > AngleRateTargBak)
                {
                    RightMotor.SpeedAdjAll = AnglerateTarg * RightMotor.SpeedAdjAll / AngleRateTargBak;
                    LeftMotor.SpeedAdjAll = AnglerateTarg * LeftMotor.SpeedAdjAll / AngleRateTargBak;
                }
            }
        }
        
        if(fabs(AnglerateTarg + RightMotor.Wacc) < fabs(AngleRateTargBak))
        {
            AnglerateTarg += RightMotor.Wacc;
        }
        else
        {
            AnglerateTarg = AngleRateTargBak;
            WaccEn = DISABLE;
            WaccInit = 0;
        }
    }
}

/*****************************************************************************
 函 数 名  : MotorRunCtrl
 功能描述  : 左右轮运行控制
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 				1.5ms调用一次
 				2.左右轮调速控制
 修改历史      :
  1.日    期   : 2016年4月1日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void MotorRunCtrl(void)
{
	int16_t PWM_Adj;
	uint16_t speed_tmp;
//	uint16_t PWM_min;

	//扫地机模式判断
	if(RobitStatus == 0 || (RobitStatus == 1 && RobitRunningMode != 1))
		return;    
    
	if(!AdjustSpeedEn)
		return;

	AdjustSpeedEn = DISABLE;
    
    //PID 调节时间调整
    //设定速度大于1500RPM 设为5ms 
    //设定速度小于1300RPM 设为10ms
    //设定速度在1300~1500RPM 之间 保持原来的时间不变 
    //调节时间初始值为5ms
    if(LeftMotor.TargSpeed >= 1500 && RightMotor.TargSpeed >= 1500)
    {
        SpeedAdjTime = 5;
    }
    else if(!(LeftMotor.TargSpeed >= 1300 && RightMotor.TargSpeed >= 1300))
    {
        SpeedAdjTime = 10;
    }
	
	//左轮
	if(LeftMotor.Status == SYS_RUNNING)
	{
		if(LeftMotorSpeed.Valid == ENABLE)
		{
			LeftMotorSpeed.Valid = DISABLE;

			//PID调节
			PWM_Adj = CalculPID(LeftMotor.TargSpeed, LeftMotor.CurSpeed, 1);


			
			if(PWM_Adj > 0)
		  	{
		  		LeftMotor.PWMVal += PWM_Adj;
		  	}
		  	else
		  	{
		  		PWM_Adj = -PWM_Adj;
				
				//PWM 最小值设置
//				if(LeftMotor.TargSpeed >= 1500)
//				{
//					PWM_min = PWM_VAL_MIN;
//				}
//				else
//				{
//					PWM_min = PWM_VAL_MIN_Low;
//				}

				
		  		if(LeftMotor.PWMVal > PWM_Adj + PWM_VAL_MIN)
		  		{
		  			LeftMotor.PWMVal -= PWM_Adj;
		  		}
		  		else
		  		{
		  			LeftMotor.PWMVal = PWM_VAL_MIN;
		  		}
		  	}
		    
		  	if(LeftMotor.PWMVal > PWM_VAL_MAX)
		  	{
		  		LeftMotor.PWMVal = PWM_VAL_MAX;
		  	}

			//PWM设定
			if(LeftMotor.Dir == MOTOR_RUN_DIR_ZW)
			{
//				Motor_L_F_PWM(LeftMotor.PWMVal);
//				Motor_L_B_PWM(0);
				Motor_L_F_Run(LeftMotor.PWMVal);
			}
			else
			{
//				Motor_L_B_PWM(LeftMotor.PWMVal);
//				Motor_L_F_PWM(0);
				Motor_L_B_Run(LeftMotor.PWMVal);
			}
			
			//当前速度大于设定速度的7/8 认为速度上升过程已完成
			speed_tmp = LeftMotor.TargSpeed * 7 ;
			speed_tmp >>= 3; 
			if(LeftMotor.CurSpeed > speed_tmp)
				LeftMotor.SpeedRisedFlag = SET;
			else
				LeftMotor.SpeedRisedFlag = RESET;
		}
	}

	//右轮
	if(RightMotor.Status == SYS_RUNNING)
	{
		if(RightMotorSpeed.Valid == ENABLE)
		{
			RightMotorSpeed.Valid = DISABLE;
			//PID调节
			PWM_Adj = CalculPID(RightMotor.TargSpeed, RightMotor.CurSpeed, 0);

		
			if(PWM_Adj > 0)
		  	{
		  		RightMotor.PWMVal += PWM_Adj;
		  	}
		  	else
		  	{
		  		PWM_Adj = -PWM_Adj;

				//PWM 最小值设置
//				if(RightMotor.TargSpeed >= 1500)
//				{
//					PWM_min = PWM_VAL_MIN;
//				}
//				else
//				{
//					PWM_min = PWM_VAL_MIN_Low;
//				}
				
		  		if(RightMotor.PWMVal > PWM_Adj + PWM_VAL_MIN)
		  		{
		  			RightMotor.PWMVal -= PWM_Adj;
		  		}
		  		else
		  		{
		  			RightMotor.PWMVal = PWM_VAL_MIN;
		  		}
		  	}
		    
		  	if(RightMotor.PWMVal > PWM_VAL_MAX)
		  	{
		  		RightMotor.PWMVal = PWM_VAL_MAX;
		  	}

			//PWM设定
			if(RightMotor.Dir == MOTOR_RUN_DIR_ZW)
			{
//				Motor_R_F_PWM(RightMotor.PWMVal);
//				Motor_R_B_PWM(0);
				Motor_R_F_Run(RightMotor.PWMVal);
			}
			else
			{
//				Motor_R_B_PWM(RightMotor.PWMVal);
//				Motor_R_F_PWM(0);
				Motor_R_B_Run(RightMotor.PWMVal);
			}
			
			//当前速度大于设定速度的7/8 认为速度上升过程已完成
			speed_tmp = RightMotor.TargSpeed * 7 ;
			speed_tmp >>= 3; 
			if(RightMotor.CurSpeed > speed_tmp)
				RightMotor.SpeedRisedFlag = SET;
			else
				RightMotor.SpeedRisedFlag = RESET;
		}		
	}
}

/*****************************************************************************
 函 数 名  : Robit_LinerPathMoving
 功能描述  : 扫地机直线行走控制
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年4月7日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void Robit_RouteMoving(void)
{
//	uint32_t tmpmileage;
//	uint16_t TmpAngledif;
    int16_t TmpSpeed;
    int16_t SpeedAdjust;
//	int16_t tmpangle;
    int16_t tmpspeedl, tmpspeedr;
//	uint8_t tmpnum;
	static uint8_t AngleCnt = 0,CirleHalf = 0, AngleStopCnt = 0;
//	static uint16_t StartSpeed = 0;

    if(!MotorLinerModeEn)
        return;
    MotorLinerModeEn = DISABLE;
    
	if(!(RobitStatus == 1 && RobitRunningMode == 1 && RobitRunningClosedLoopMode == 1))
		return;
	
    if(!RobitRouteModeEn)	
    return;
	
    MotorLinerModeCnt ++;
	if(RightMotor.Status != SYS_STOP && RightMotor.Status != SYS_STOPING)
	{
			if(TargMileage > RightCurMileage + 2 || RobitRunbyRouteMode == 3 || RobitRunbyRouteMode == 1)
			{
				if(0 == RobitRunbyRouteMode || 2 == RobitRunbyRouteMode || 3 == RobitRunbyRouteMode || 1 == RobitRunbyRouteMode)  // 直线里程控制方式
				{
//					tmpmileage = TargMileage - RightCurMileage;		

						//10ms调整一次 角速度调节
                    if(MotorLinerModeCnt >= 10)
                    {
                        MotorLinerModeCnt = 0;
                        AngleCnt ++;
                        
                        if(0 == RobitRunbyRouteMode)
//						if(2 == RobitRunbyRouteMode)
                        {
                            //20ms调整一次 角度调节  目标角度=初始角度
                            if(AngleCnt >= 2)
                            {
                                AngleCnt = 0;
        //						tmpangle = EdageRunAngle();
                                
                                AnglerateTarg = CalculPID(Sys_Startangle,Sys_Angle,3);
        //						AnglerateTarg = CalculPID(tmpangle,Sys_Angle,3);
        
                            }
                        }
                        else if(2 == RobitRunbyRouteMode) // 沿墙行走模式
//						else if(0 == RobitRunbyRouteMode)
                        {
                            //40ms调整一次 角度调节  目标角度=计算输出角度
                            //VL53L0x 一次测距需要33ms ，改为40ms调整一次
                            if(AngleCnt >= 4)
                            {
                                AngleCnt = 0;
                                boundary(1);
//                                AnglerateTarg = CalculPID(Sys_Startangle,Sys_Angle,3);
        
                            }
                        }
                        
                        if(AnglerateTarg > 20000)
                        {
                            AnglerateTarg = 20000;
                        }
                        else if(AnglerateTarg < -20000)
                        {
                            AnglerateTarg = -20000;
                        }
                        
    //					Sys_Anglerate = ;
                        SpeedAdjust = CalculPID(AnglerateTarg, Sys_Anglerate, 2);//(LeftCurMileage - RightCurMileage);//
                        
                        if(!((RightMotor.SpeedAdjAll >= MOT_RPM_SPEED_MAX * 2 && SpeedAdjust > 0) || (RightMotor.SpeedAdjAll <= - MOT_RPM_SPEED_MAX * 2 && SpeedAdjust < 0)))
                        {
                        //()
                            if(Robit_Dir == 1)
                            {
                                RightMotor.SpeedAdjAll += SpeedAdjust;
                                LeftMotor.SpeedAdjAll -= SpeedAdjust;
                            }
                            else
                            {
                                RightMotor.SpeedAdjAll -= SpeedAdjust;
                                LeftMotor.SpeedAdjAll += SpeedAdjust;
                            }
                        }
                        
                        
                        TmpSpeed = (int16_t)RightMotor.BaseSpeed;
                        if(TmpSpeed + RightMotor.SpeedAdjAll > 0)
                        {
                            if(TmpSpeed + RightMotor.SpeedAdjAll <= MOT_RPM_SPEED_MAX_2)
                            {
                                RightMotor.TargSpeed = RightMotor.BaseSpeed + RightMotor.SpeedAdjAll;
                            }
                            else
                            {
                                RightMotor.TargSpeed = MOT_RPM_SPEED_MAX_2;
                                RightMotor.SpeedAdjAll = MOT_RPM_SPEED_MAX_2 - RightMotor.BaseSpeed;
                                
                                LeftMotor.SpeedAdjAll = -RightMotor.SpeedAdjAll;///1
                            }

                            if(Robit_Dir == 0)
                            {
                                RightMotor.TargDir = MOTOR_RUN_DIR_ZW;
                                tmpspeedr = RightMotor.CurSpeed;
                            }
                            else
                            {
                                RightMotor.TargDir = MOTOR_RUN_DIR_FW;
                                tmpspeedr = -RightMotor.CurSpeed;
                            }
                        }
                        else
                        {
                            
                            
                            if(-(RightMotor.BaseSpeed + RightMotor.SpeedAdjAll) <= MOT_RPM_SPEED_MAX_2)
                            {
                                RightMotor.TargSpeed = -(RightMotor.BaseSpeed + RightMotor.SpeedAdjAll);
                            }
                            else
                            {
                                RightMotor.TargSpeed = MOT_RPM_SPEED_MAX_2;
                                RightMotor.SpeedAdjAll = -(int16_t)(MOT_RPM_SPEED_MAX_2 + RightMotor.BaseSpeed);
                                
                                LeftMotor.SpeedAdjAll = -RightMotor.SpeedAdjAll;///1
                            }
                            
                            if(Robit_Dir == 0)
                            {
                                RightMotor.TargDir = MOTOR_RUN_DIR_FW;
                                tmpspeedr = -RightMotor.CurSpeed;
                            }
                            else
                            {
                                RightMotor.TargDir = MOTOR_RUN_DIR_ZW;
                                tmpspeedr = RightMotor.CurSpeed;
                            }
                        }
                        
                        //目标方向与当前方向不一致，需要切换方向
                        if(RightMotor.Dir != RightMotor.TargDir && RightMotor.Status == SYS_RUNNING && RightMotor.TargSpeed >= MOT_RPM_SPEED_MIN)
                        {
                            RightMotorStop();
                            RightMotorSwDirFlag = SET;
                            RightMotorSpeedBak = RightMotor.TargSpeed;
                            RightMotorDirBak = RightMotor.TargDir;
                        }
                        
                        TmpSpeed = (int16_t)LeftMotor.BaseSpeed;
                        if(TmpSpeed + LeftMotor.SpeedAdjAll > 0)
                        {
                            if(TmpSpeed + LeftMotor.SpeedAdjAll <= MOT_RPM_SPEED_MAX_2)
                            {
                                LeftMotor.TargSpeed = LeftMotor.BaseSpeed + LeftMotor.SpeedAdjAll;
                            }
                            else
                            {
                                LeftMotor.TargSpeed = MOT_RPM_SPEED_MAX_2;
                                LeftMotor.SpeedAdjAll = MOT_RPM_SPEED_MAX_2 - LeftMotor.BaseSpeed;
                                
                                RightMotor.SpeedAdjAll = -LeftMotor.SpeedAdjAll;///1
                            }
                            
                            if(Robit_Dir == 0)
                            {
                                LeftMotor.TargDir = MOTOR_RUN_DIR_ZW;
                                tmpspeedl = LeftMotor.CurSpeed;
                            }
                            else
                            {
                                LeftMotor.TargDir = MOTOR_RUN_DIR_FW;
                                tmpspeedl = -LeftMotor.CurSpeed;
                            }
                        }
                        else
                        {
                            if(-(LeftMotor.BaseSpeed + LeftMotor.SpeedAdjAll) <= MOT_RPM_SPEED_MAX_2)
                            {
                                LeftMotor.TargSpeed = -(LeftMotor.BaseSpeed + LeftMotor.SpeedAdjAll);
                            }
                            else
                            {
                                LeftMotor.TargSpeed = MOT_RPM_SPEED_MAX_2;
                                LeftMotor.SpeedAdjAll = -(int16_t)(MOT_RPM_SPEED_MAX_2 + LeftMotor.BaseSpeed);
                                
                                RightMotor.SpeedAdjAll = -LeftMotor.SpeedAdjAll;///1
                            }
                            
                            if(Robit_Dir == 0)
                            {
                                LeftMotor.TargDir = MOTOR_RUN_DIR_FW;
                                tmpspeedl = -LeftMotor.CurSpeed;
                            }
                            else
                            {
                                LeftMotor.TargDir = MOTOR_RUN_DIR_ZW;
                                tmpspeedl = LeftMotor.CurSpeed;
                            }
                        }
                        
                        if(LeftMotor.Dir  != LeftMotor.TargDir && LeftMotor.Status == SYS_RUNNING && LeftMotor.TargSpeed >= MOT_RPM_SPEED_MIN)
                        {
                            LeftMotorStop();	
                            LeftMotorSwDirFlag = SET;
                            LeftMotorSpeedBak = LeftMotor.TargSpeed;
                            LeftMotorDirBak = LeftMotor.TargDir;
                        }
                        
                        //计算线速度
                        RobotLineSpeed = (tmpspeedl + tmpspeedr) / 2;
                        
                        if(RightMotor.TargSpeed > MOT_RPM_SPEED_MAX_2)
                        {
                            RightMotor.TargSpeed = MOT_RPM_SPEED_MAX_2;
                        }
                        else if(RightMotor.TargSpeed < MOT_RPM_SPEED_MIN_2)
                        {
                            RightMotor.TargSpeed = MOT_RPM_SPEED_MIN_2;
                        }
                        
                        if(LeftMotor.TargSpeed > MOT_RPM_SPEED_MAX_2)
                        {
                            LeftMotor.TargSpeed = MOT_RPM_SPEED_MAX_2;
                        }
                        else if(LeftMotor.TargSpeed < MOT_RPM_SPEED_MIN_2)
                        {
                            LeftMotor.TargSpeed = MOT_RPM_SPEED_MIN_2;
                        }
                          
                    }

				}				
			}
			else
			{
				RightMotorStop();
				LeftMotorStop();
                //清楚左右轮换向启动标志 ，否则会导致误启动
                LeftMotorSwDirFlag = RESET;
                RightMotorSwDirFlag = RESET;
			}
		}
		
//			else //if(tmpmileage >= 100)
//			{
//				TmpSpeed = Speed_Mileage[tmpmileage / Motor_Stop_Step];
//				
//				if(RightMotor.TargSpeed > TmpSpeed)
//				{
//					RightMotor.TargSpeed = TmpSpeed;
//				}
//                LeftMotor.TargSpeed = RightMotor.TargSpeed;
//			}
			
		

	if(LeftMotor.Status != SYS_STOP && LeftMotor.Status != SYS_STOPING)
	{
        if(1 == RobitRunbyRouteMode)
        {

        }
        else if(TargMileage > LeftCurMileage + 2 || RobitRunbyRouteMode == 3)
		{
			
		}
		else
		{
			LeftMotorStop();
			RightMotorStop();
            
            //清楚左右轮换向启动标志 ，否则会导致误启动
            LeftMotorSwDirFlag = RESET;
            RightMotorSwDirFlag = RESET;
		}
	}
    
    //旋转角度 step 模式 画弧线结束条件判定
    if(1 == RobitRunbyRouteMode)
    {
        uint16_t TmpAngledif;
        uint32_t TmpLineDis;
        
        //20170717 5S不能完成step动作，停止
//        if(++StepModeCnt >= 5000)
//        {
//            RightMotorStop();
//            LeftMotorStop();
//            AngleStopCnt = 0;
//            CirleHalf = 0;
//            StepModeCnt = 0;
//            
//            //清楚左右轮换向启动标志 ，否则会导致误启动
//            LeftMotorSwDirFlag = RESET;
//            RightMotorSwDirFlag = RESET;
//        }
        
        // 角度判定
        if(Sys_Anglerate > 0)
        {
            TmpAngledif = CalAngleDif(Sys_Angle, Sys_Startangle);					
        }
        else if(Sys_Anglerate < 0)
        {
            TmpAngledif = CalAngleDif(Sys_Startangle, Sys_Angle);
        }
        
        
        if(TmpAngledif > 18000 && TmpAngledif < 35000)
        {
            CirleHalf = 1; 
        }

        
        
        if(TargCirle > 0)
        {
            if(1 == CirleHalf && TmpAngledif <= ANGLE_DELT)
            {
                CirleHalf = 0;
                TargCirle --;
                
            }
        }
        else
        {	
            if(TargAngle >= 35000)
            {
                //旋转角度较大时，需要半圈标志置位，
                if(!CirleHalf)
                {
                    return;
                }
            }
            
            //旋转角度用于小于350的赋值，大于350度的赋值可能是初始的小幅度翻转偏差导致
            if(TargAngle < 35000 && TmpAngledif < 35000)
                RotationAngleCur = TmpAngledif;
            
            //沿边模式时，step模式的结束判定执行旧的版本
            if(3 == PathPlanMode || TargMileageForArc == 0 || 4 == PathPlanMode) 
            {
                if(TmpAngledif + (fabs(AnglerateTarg) / 2) >= TargAngle)
                {               
                    if(++AngleStopCnt >= 10)
                    {
                        AngleStopCnt = 0;
                        if(fabs(AnglerateTarg) >= ANGLERATE_MIN * 3)
                        {
                            AnglerateTarg /= 3;
                        }
                        else if(fabs(AnglerateTarg) > ANGLERATE_MIN)
                        {
                            if(AnglerateTarg > 0)
                            {
                                AnglerateTarg = ANGLERATE_MIN;
                            }
                            else if(AnglerateTarg < 0)
                            {
                                AnglerateTarg = -ANGLERATE_MIN;
                            }
                        }
                    }
                    if(TargAngle >= ANGLE_DELT * 2)
                    {
                        if(TmpAngledif >= TargAngle - ANGLE_DELT && TmpAngledif <= TargAngle + ANGLE_DELT)
                        {
                            RightMotorStop();
                            LeftMotorStop();
                            AngleStopCnt = 0;
                            CirleHalf = 0;
                            
                            //清楚左右轮换向启动标志 ，否则会导致误启动
                            LeftMotorSwDirFlag = RESET;
                            RightMotorSwDirFlag = RESET;
                            
    //                        AnglerateTarg = 0;
                        }
                    }
                    else
                    {
                        if(TmpAngledif >= TargAngle && TmpAngledif <= TargAngle + ANGLE_DELT)
                        {
                            RightMotorStop();
                            LeftMotorStop();
                            AngleStopCnt = 0;
                            CirleHalf = 0;
                            
                            //清楚左右轮换向启动标志 ，否则会导致误启动
                            LeftMotorSwDirFlag = RESET;
                            RightMotorSwDirFlag = RESET;
    //                        AnglerateTarg = 0;
                        }
                    }
                }
                else if(TmpAngledif + (fabs(AnglerateTargInit) / 2) < TargAngle)
                {
                    AnglerateTarg = AnglerateTargInit;
                }
            }
            //其他模式，step模式结束只是设定角速度为0
            else 
            {
                if(TargAngle >= ANGLE_DELT * 2)
                {
                    if(TmpAngledif >= TargAngle - ANGLE_DELT && TmpAngledif <= TargAngle + ANGLE_DELT)
                    {                           
                        if(AnglerateTarg > 1000)
                            AnglerateTarg = 1000;
 
                        if(StepStatus)
                        {
                            RightMotor.SpeedAdjAll = RightMotor.SpeedAdjAll >> 3;
                            LeftMotor.SpeedAdjAll = LeftMotor.SpeedAdjAll >> 3; 
//                            if(LeftMotor.SpeedAdjAll > 1800 && LeftMotor.SpeedAdjAll > 0)
//                            {
//                                RightMotor.SpeedAdjAll = -1800;
//                                LeftMotor.SpeedAdjAll = 1800;
//                            }
//                            else if(LeftMotor.SpeedAdjAll < -1800 && LeftMotor.SpeedAdjAll < 0)
//                            {
//                                RightMotor.SpeedAdjAll = 1800;
//                                LeftMotor.SpeedAdjAll = -1800;
//                            }
                            StepStatus = 0;
                        }
                    }
                }
                else
                {
                    if(TmpAngledif >= TargAngle && TmpAngledif <= TargAngle + ANGLE_DELT)
                    {
                        if(AnglerateTarg > 1000)
                            AnglerateTarg = 1000;
                        
                        if(StepStatus)
                        {
                            RightMotor.SpeedAdjAll = RightMotor.SpeedAdjAll >> 3;
                            LeftMotor.SpeedAdjAll = LeftMotor.SpeedAdjAll >> 3;
//                            if(LeftMotor.SpeedAdjAll > 1800 && LeftMotor.SpeedAdjAll > 0)
//                            {
//                                RightMotor.SpeedAdjAll = 1800;
//                                LeftMotor.SpeedAdjAll = 1800;
//                            }
//                            else if(LeftMotor.SpeedAdjAll < -1800 && LeftMotor.SpeedAdjAll < 0)
//                            {
//                                RightMotor.SpeedAdjAll = -1800;
//                                LeftMotor.SpeedAdjAll = -1800;
//                            }
                            
                            StepStatus = 0;
                        }
                    }
                }
            }                
        }
        
        //距离判定
        //TargMileageForArc = 0时不做距离判定
        if(TargMileageForArc != 0)
        {
            if(RightMotor.Dir == LeftMotor.Dir)
            {
                TmpLineDis = (LeftCurMileage + RightCurMileage) >> 1;
            }
            else
            {
                TmpLineDis = (uint32_t)fabs((int32_t)LeftCurMileage - (int32_t)RightCurMileage);
                TmpLineDis >>= 1;
            }
            if(TmpLineDis >= TargMileageForArc + 2)
            {
                LeftMotorStop();
                RightMotorStop();
                
                //清楚左右轮换向启动标志 ，否则会导致误启动
                LeftMotorSwDirFlag = RESET;
                RightMotorSwDirFlag = RESET;
            }
        }
    }
}

//计算角度差Angle1 - Angle2，返回值在0~36000之间
uint16_t CalAngleDif(int16_t Angle1, int16_t Angle2)
{
	int32_t calresult;
	
	calresult = (int32_t)(Angle1 - Angle2);
	
	if(calresult < 0)
	{
		calresult += 36000;
	}
	else if(calresult > 36000)
	{
		calresult -= 36000;
	}
	return (uint16_t)calresult;
}

/*****************************************************************************
 函 数 名  : Robit_RunLR
 功能描述  : 扫地机左右轮速度控制
 输入参数  : uint8_t LeftMotor_Speed           // 0~10
             uint8_t LeftMotor_Dir			   //0/1
             uint8_t RightMotor_Speed  		   //0~10
             uint8_t RightMotor_Dir			   //0/1
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月16日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void Robit_RunLR(uint8_t LeftMotor_Speed,uint8_t LeftMotor_Dir,uint8_t RightMotor_Speed,uint8_t RightMotor_Dir)
{
    
	if(LeftMotor_Speed > 10)
		LeftMotor_Speed = 10;
	if(RightMotor_Speed > 10)
		RightMotor_Speed = 10;
	if(LeftMotor_Dir > 1)
		LeftMotor_Dir = 1;
	if(RightMotor_Dir > 1)
		RightMotor_Dir = 1;

	if(LeftMotor_Speed > 0)
    {
        LeftMotorRunInit();
    }
    
    if(RightMotor_Speed > 0)
    {
        RightMotorRunInit();
    }


	RightMotorEnterRun(RightMotor_Dir);
	LeftMotorEnterRun(LeftMotor_Dir);

}

/*****************************************************************************
 函 数 名  : LeftMotorStop
 功能描述  : 左轮停止
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月18日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void LeftMotorStop(void)
{
//	Motor_L_F_PWM(0);
//	Motor_L_B_PWM(0);	
//	LeftMotor.RunEnable = 0;
    
    //20170721
    //清楚左轮换向启动标志，防止误启动发生，若需后续换向启动，再吧标志位置位
//    LeftMotorSwDirFlag = RESET;

	if(LeftMotor.Status != SYS_STOP)
		LeftMotor.Status = SYS_STOPING;
}

/*****************************************************************************
 函 数 名  : RightMotorStop
 功能描述  : 右轮停止
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月18日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void RightMotorStop(void)
{
//	Motor_R_F_PWM(0);
//	Motor_R_B_PWM(0);

//	RightMotor.RunEnable = 0;
    
    //20170721
    //清楚右轮换向启动标志，防止误启动发生，若需后续换向启动，再吧标志位置位
//    LeftMotorSwDirFlag = RESET;

	if(RightMotor.Status != SYS_STOP)
	{
		RightMotor.Status = SYS_STOPING;
		
	}
}

/*
2016.7.25 
1.修复BUG，电机堵转后，不能进入运行状态
*/
void MotorBlockCtrl(void)
{
	if(RightMotor.Status == SYS_BLOCKING)
	{
//		RightMotor.Status = SYS_STARTUP;
		
//		RightMotor.Status = SYS_STOP;
		RightMotorEnterRun(RightMotor.Dir);  // 2016.07.25 更改 修补堵转后不能进入运行状态的BUG
	}

	if(LeftMotor.Status == SYS_BLOCKING)
	{
//		LeftMotor.Status = SYS_STARTUP;
		
//		LeftMotor.Status = SYS_STOP;
		LeftMotorEnterRun(LeftMotor.Dir);   //2016.07.25 更改 修复堵转后不能进入运行状态的BUG
	}
}

/*****************************************************************************
 函 数 名  : MotorStopCtrl
 功能描述  : 左右轮停止控制  ,  1ms调用一次
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月21日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void MotorStopCtrl(void)
{
	if(!MotorStopCtrlEn)
		return;
	MotorStopCtrlEn = DISABLE;
	
	if(RightMotor.Status == SYS_STOP)
	{
		RightMotor.RunEnable = DISABLE;
		RightMotor.RunFlag = RESET;
		RightMotor.CurSpeed = 0;
        
        RightMotor.PWMVal = 0;
	}
	else if(RightMotor.Status == SYS_STOPING)
	{
//		Motor_R_F_PWM(2400);
//		Motor_R_B_PWM(2400);
		Motor_R_Brake();
		
		RightMotor.PWMVal = 0;
        RightMotor.PWMVal = TIM4_Period;

		RightMotor.RunEnable = DISABLE;
		if(RightMotor.RunFlag == RESET)
		{
			RightMotor.Status = SYS_STOP;
			RightMotor.CurSpeed = 0;
//			Motor_R_F_PWM(0);
//			Motor_R_B_PWM(0);
			Motor_R_Stop();
            RightMotor.PWMVal = 0;
		}
		else
		{
			RightMotor.StopCnt++;
			if(RightMotor.StopCnt > Motor_Stop_Time)
			{
				RightMotor.RunFlag = RESET;
				RightMotor.StopCnt = 0;
			}
		}
	}

	if(LeftMotor.Status == SYS_STOP)
	{
		LeftMotor.RunEnable = DISABLE;
		LeftMotor.RunFlag = RESET;
		LeftMotor.CurSpeed = 0;
	}
	else if(LeftMotor.Status == SYS_STOPING)
	{
//		Motor_L_F_PWM(2400);
//		Motor_L_B_PWM(2400);
		Motor_L_Brake();
		LeftMotor.PWMVal = 0;
		LeftMotor.RunEnable = DISABLE;

		if(LeftMotor.RunFlag == RESET)
		{
			LeftMotor.Status = SYS_STOP;
			LeftMotor.CurSpeed = 0;
//			Motor_L_F_PWM(0);
//			Motor_L_B_PWM(0);
			Motor_L_Stop();
		}
		else
		{
			LeftMotor.StopCnt++;
			if(LeftMotor.StopCnt > Motor_Stop_Time)
			{
				LeftMotor.RunFlag = RESET;
				LeftMotor.StopCnt = 0;
			}
		}
	}
}

/*****************************************************************************
 函 数 名  : Robit_Stop
 功能描述  : 扫地机停止运动
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月16日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void Robit_Stop(void)
{
    LeftMotorStop();
    RightMotorStop();	
}

//kalman1_state CurKalman;
//uint16_t LeftMotorCur, RightMotorCur;
uint8_t rightIncrease,rightReduce;
int8_t rightCnt;
uint8_t CarpetStatus, CarpetCheckEn;
uint8_t Sta_Gyro, UpCnt, DownCnt;

uint32_t earthSum = 0;
uint16_t earthAvg;
uint8_t  earthWaveCnt = 0, earthCnt = 0, earthAvgOk = 0, earthWaveTimeCnt = 0;
uint16_t earthWaveMax,earthWaveMin;

#define  ANGLE_X_THRESHOLD  120

//地毯检测 主要检测轮子的电流值的变化
//20ms检测一次
void CarpetCheck(void)
{
    static uint8_t init = 0, earthmaxminOK = 0;
    static uint16_t RunningCnt = 0, EarthCheckPre = 0, AngleCnt = 0; //rightCurPre,leftCurPre, 
//    static int16_t Gyro_x_Bak;
    static int32_t AngleSum = 0;
    uint16_t anglexdelt;
    
    if(!CarpetCheckEn)
        return;
    CarpetCheckEn = 0;
    
    //初始化
    if(!init)
    {
        init = 1;
//        kalman1_init(&CurKalman, 0, 20, 1, 100);
//        rightCurPre = RightMotorCur;
//        leftCurPre = LeftMotorCur;
        CarpetStatus = 0;
        rightCnt = 0;
        
        Sta_Gyro = 0;
        UpCnt = 0;
        DownCnt = 0;
        
        earthWaveMax = 0;
        earthWaveMin = 0xffff;
        earthmaxminOK = 0;
    }
    
    
//    RightMotorCur = kalman1_filter(&CurKalman, ADC_ConvertedValue_Val[5]);
//    LeftMotorCur = kalman1_filter(&CurKalman, ADC_ConvertedValue_Val[6]);
    
    if(RightMotor.Status == SYS_RUNNING && LeftMotor.Status == SYS_RUNNING)
    {
        RunningCnt++;
    }
    else
    {
        RunningCnt = 0;
    }
    
    if(RunningCnt < 50)
        return;
    
    //防止溢出
    RunningCnt = 51;
    
    if(RightMotor.Dir != MOTOR_RUN_DIR_ZW || LeftMotor.Dir != MOTOR_RUN_DIR_ZW)
    {
        //每次转弯时 清零地检波动计数
        earthWaveCnt = 0;
        return;
    }
    
    // 5S计时
    if(earthWaveTimeCnt < 250)
    {
        earthWaveTimeCnt++;   
        
        if(earthWaveTimeCnt == 200)
            earthmaxminOK = 1;
    }
    else
    {
        earthWaveTimeCnt = 0;
        earthWaveCnt = 0;
        
        earthWaveMax = 0;
        earthWaveMin = 0xffff;
        earthmaxminOK = 0;
    }
    
    //地检波动在+300~-300范围内
    if(EarthCheckPre < (ADC_ConvertedValue_Val[0] + 500) && (EarthCheckPre + 500) > ADC_ConvertedValue_Val[0])
    {
        //求平均值
        earthSum += ADC_ConvertedValue_Val[0];
        earthCnt++;
        
        if(earthCnt >= 64)
        {
            earthAvg = earthSum >> 6;
            earthCnt = 0;
            earthSum = 0;
            earthAvgOk = 1;
        }       
    }
    //地检波动较大
    else
    {
        earthCnt = 0;
        earthSum = 0;
        //累计计时
        //从第一次波动开始计时，计算5S内的波动数 如大于18则认为有地毯，否则认为没有
        if(earthWaveCnt == 0)
        {
            earthWaveTimeCnt = 0;
        }
        earthWaveCnt++;    

    }
    
    if(earthWaveMax < ADC_ConvertedValue_Val[0])
    {
        earthWaveMax = ADC_ConvertedValue_Val[0];
    }
    
    if(earthWaveMin > ADC_ConvertedValue_Val[0])
    {
        earthWaveMin = ADC_ConvertedValue_Val[0];
    }
    
    EarthCheckPre = ADC_ConvertedValue_Val[0];
    
    if(IcmDataByKalman.Gyro_x > 2000 || IcmDataByKalman.Gyro_x < -2000)
    {
        Sta_Gyro = 1;
        UpCnt = 0;
    }
    
    if(Sta_Gyro)
    {
        UpCnt++;
        if(UpCnt > 30)
        {
            UpCnt = 0;
            Sta_Gyro = 0;
        }
    }
    
    anglexdelt = CalAngleDif(Sys_Angle2, Sys_AnglexInit);
//    test2 = anglexdelt;
    
    //x轴角度小于初始值 120 认为上坡动作 ANGLE_X_THRESHOLD = 120
    if(anglexdelt > 18000 && anglexdelt < 36000 - ANGLE_X_THRESHOLD && Sta_Gyro)
    {
        CarpetStatus = 2;
        earthAvgOk = 0;
        MopPar.MopEn = DISABLE;
    }
    
    //x轴角度大于初始值120 认为下坡动作
    else if(anglexdelt > ANGLE_X_THRESHOLD && anglexdelt < 18000 && Sta_Gyro)
    {
        CarpetStatus = 1;
        
        earthWaveCnt = 0;
        
        MopPar.MopEn = ENABLE;
    }
    else if(Sta_Gyro == 0)
    {       
        AngleSum += Sys_Angle2;
        if(AngleCnt < 256)
        {
            AngleCnt++;
            Sys_AnglexInit = AngleSum / AngleCnt;
        }
        else
        {
            Sys_AnglexInit = AngleSum >> 8;
            AngleSum -= Sys_AnglexInit;
        }
    }
    
    //不在地毯上
    if(1 == CarpetStatus || 0 == CarpetStatus)
    {
        if((earthWaveCnt >= 18 && earthWaveTimeCnt <= 250) || (earthWaveMax > 3000 && earthWaveMax - earthWaveMin > 2000 && earthmaxminOK)) 
        {
            //可能在地毯上 判断其他条件 �
            CarpetStatus = 2;
            earthAvgOk = 0;
            MopPar.MopEn = DISABLE;
            earthmaxminOK = 0;
//            TDMotorOff();
        }
    }
    //在地毯上
    else if(2 == CarpetStatus || 0 == CarpetStatus)
    {
        //平均值小于1500或者稳定持续时间大于3S
        if((earthAvgOk && earthAvg < 1500)||(earthWaveMax < 2000 && earthWaveMax - earthWaveMin < 1500 && earthmaxminOK))
        {
            //可能不在地毯上
            CarpetStatus = 1;
            earthWaveCnt = 0;
            MopPar.MopEn = ENABLE;
            earthmaxminOK = 0;
            
//            TDMotorOn();
        }
    }
    
//    if(fabs(RightMotorCur - rightCurPre) > 20)
//    {
//        //增加
//        if(RightMotorCur > rightCurPre)
//        {
//            rightIncrease++;
//            rightReduce = 0;
//            rightCnt++;
//        }
//        //减小
//        else
//        {
//            rightIncrease = 0;
//            rightReduce++;
//            rightCnt--;
//        }
//        
//        if(rightIncrease >= 3)
//        {
//            CarpetStatus = 1;
//        }
//        else if(rightReduce >= 3)
//        {
//            CarpetStatus = 2;
//        }
//    }
//    else
//    {
//        rightIncrease = 0;
//        rightReduce = 0;
//        rightCnt = 0;
//    }
//    rightCurPre = RightMotorCur;
}

/*
1.基于当前的样机尺寸，超声波中心距119.25mm,计算机身与墙的夹角如下：
    y = atan(x / 119.25mm)
-------------------------------------------------------------------
    前后超声波数据差 (x)       |     夹角(角度，0.01度精度，y)  
-------------------------------------------------------------------
           x < 301			   |      y = (62 * x + 216) >> 3
-------------------------------------------------------------------
    301 <= x < 501             |      y = (49 * x + 3944) >> 3
-------------------------------------------------------------------
    501 <= x < 701             |      y = (38 * x + 9600) >> 3
-------------------------------------------------------------------
           x > 701             |       不计算角度，应调小夹角
-------------------------------------------------------------------

2.计算样机机身左前方角与墙面的距离(y),前超声波数据D1，前后超声波数据差D2，超声波探头与机身边界的距离D4(39mm)
超声波中心距D3(119.25mm),前超声波与机身最前方的距离L（166mm）,机身与墙面的夹角a.
    y = (D3 * (D1 - D4) / D2 - L) * sin(a).
*/
/*
int16_t DistanceCalu(void)
{
	int16_t tmpdelt, tmpangle;
	int32_t tmpdata; //中间变量
	
	// 1. 计算夹角
	tmpdelt = Ultra_RevTimeVlid[2] - Ultra_RevTimeVlid[1];
	
	if(tmpdelt > 0 && tmpdelt <= 701)
	{
		if(tmpdelt < 301)
		{
			tmpdata = tmpdelt * 62 + 216;
			tmpangle = tmpdata >> 3;
		}
		else if(tmpdelt < 501)
		{
			tmpdata = tmpdelt * 49 + 3944;
			tmpangle = tmpdata >> 3;
		}
		else
		{
			tmpdata = tmpdelt * 38 + 9600;
			tmpangle = tmpdata >> 3;
		}
		test2 = tmpangle ;
		
	// 2. 计算距离
		tmpdata = 701 * (Ultra_RevTimeVlid[1] - 230) / tmpdelt - 976;
		tmpdata = (MF_DSIN(tmpangle) * tmpdata) >> 15;	
		
	}
	else
	{
		tmpdata = 0;
	}
	
	return (int16_t)tmpdata;
}*/

int16_t EdageRunAngle(void)
{
	int16_t Wall_Angle,tmpangle;	
	int16_t Timedelt;
	uint16_t tmpdistance;
	static int32_t tmpangle2 = 0;
	
//	Ultra_Jump[1] = DISABLE;
//	Ultra_Jump[2] = DISABLE;
	if(Ultra_RevTimeVlid[1] > Edage_Distance && Ultra_RevTimeVlid[2] > Edage_Distance)
	{
//		Wall_Angle = (Ultra_RevTimeVlid[1] - Ultra_RevTimeVlid[2]) * 100 / 11;
//		tmpangle2 = Ultra_RevTimeVlid[1] * 100; // 15
//		tmpangle2 >>= 4;
//		if(tmpangle2 > 4000)
//			tmpangle2 = 4000;
		
		if(Ultra_RevTimeVlid[1] >= Ultra_RevTimeVlid[2])
		{
			Timedelt = Ultra_RevTimeVlid[1] - Ultra_RevTimeVlid[2];
			if(Timedelt > 700)
			{
				Wall_Angle = 0;	
			}
			else
			{
				Wall_Angle = Timedelt * 100 / 15;
			}
			
			Timedelt = Timedelt * 7;
			Timedelt /= 5;
//			if(Timedelt < 700)
//			{
				if(Timedelt  + Edage_Distance < Ultra_RevTimeVlid[1])
				{
//					tmpangle2 = Ultra_RevTimeVlid[1] - Timedelt - 400;
//					tmpangle2 *= 100;
//					tmpangle2 >>= 6;					
//					tmpangle2 += 200;
					
					tmpdistance = Wall_Angle / 100;//Ultra_RevTimeVlid[1] - 300 - Timedelt;
					if(tmpdistance > 40)
						tmpdistance = 40;
					
					tmpdistance = Angle_Num[tmpdistance];
					
					tmpangle2 = Ultra_RevTimeVlid[1] - Timedelt - Edage_Distance;
					tmpangle2 = tmpangle2 * tmpdistance ;
					tmpangle2 *= 5;
					tmpangle2 >>= 6;
				}
				else
				{
					tmpangle2 = 0;
				}
				
				if(tmpangle2 > 4000)
				{
					tmpangle2 = 4000;
				}
				
//				tmpangle2 = 0;
//			}
//			else
//			{
//				tmpangle2 = 0;
//			}
			
			tmpangle = Sys_Angle + Wall_Angle + tmpangle2;
			if(tmpangle > 18000)
			{
				tmpangle -= 36000;
			}
			
			return tmpangle;
		}
		else
		{
			Timedelt = Ultra_RevTimeVlid[2] - Ultra_RevTimeVlid[1];
			if(Timedelt > 700)
			{
				Wall_Angle = 0;	
			}
			else
			{
				Wall_Angle = Timedelt * 100 / 15;
			}
			
			Timedelt = Timedelt * 7;
			Timedelt /= 5;
			
//			if(Timedelt < 700)
//			{
				if(Timedelt  + Edage_Distance < Ultra_RevTimeVlid[1])
				{
					tmpdistance = Wall_Angle / 100;//Ultra_RevTimeVlid[1] - 300 - Timedelt;
					if(tmpdistance > 40)
						tmpdistance = 40;
					
					tmpdistance = Angle_Num[tmpdistance];
					
					tmpangle2 = Ultra_RevTimeVlid[1] - Timedelt - Edage_Distance;
					tmpangle2 = tmpangle2 * tmpdistance ;
					tmpangle2 *= 5;
					tmpangle2 >>= 6;					
//					tmpangle2 += 200;
				}
				else
				{
					tmpangle2 = 0;
				}
				
				
				if(tmpangle2 > 4000)
				{
					tmpangle2 = 4000;
				}
//				tmpangle2 = 0;
//			}
//			else
//			{
//				tmpangle2 = 0;
//			}
//			
			tmpangle = Sys_Angle - Wall_Angle + tmpangle2;
			if(tmpangle > 18000)
			{
				tmpangle -= 36000;
			}
			else if(tmpangle < -18000)
			{
				tmpangle += 36000;
			}
			
			return tmpangle;	
		}

	}
	else
	{
		Timedelt = Ultra_RevTimeVlid[1] - Ultra_RevTimeVlid[2];
		Wall_Angle = Timedelt * 100 / 14;
		
//		if(Timedelt )
		tmpangle = Sys_Angle + Wall_Angle;
		

		return tmpangle;
	}
}
/*****************************************************************************
 函 数 名  : TIM2_IRQHandler
 功能描述  : TIM2溢出中断，用于左右轮测速
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月17日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2 , TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM2 , TIM_IT_Update);
		
		LeftMotorSpeed.OverflowCnt++;
		RightMotorSpeed.OverflowCnt++;
		
	}	
}


/*****************************************************************************
 函 数 名  : EXTI1_IRQHandler
 功能描述  : 右轮速度检测中断
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月8日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
//void EXTI9_5_IRQHandler(void)
void EXTI1_IRQHandler(void)
{
//	uint32_t tmpcircletime,tmpcurcountvalue;
//	uint8_t tmpoverflowcntbuf;
//    uint16_t tmpmotordata;
//	uint8_t tmpsta;
	static uint32_t tmpcntla = 0;//, tmpcntlavld = 0;
//	static uint8_t tmpfallingcnt = 0;
	uint32_t tmpcnt,tmpcntdel,tmpcntdelmin;
	
	if ( EXTI_GetITStatus(EXTI_Line1) != RESET )
	{
		EXTI_ClearITPendingBit(EXTI_Line1);
		
//		///再次读取中断口电平，判断是否下降沿（低电平）
//		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5) != RESET)
//			return;

		// 脉宽计算，若小于设定值，则认为是一次错误的触发信号//////////
		if(RightMotor.RunEnable == ENABLE && RightMotor.RunFlag != SET)
		{
			tmpcntla = RightMotorSpeed.PreCountValue;
		}
		
		tmpcnt = TIM_GetCounter(TIM2);
		
		if(tmpcnt >= tmpcntla)
		{
			tmpcntdel = tmpcnt - tmpcntla;
		}
		else
		{
			tmpcntdel = 0xffff - tmpcntla + tmpcnt;			
		}
		
		if(RightMotor.SpeedRisedFlag)
		{
			tmpcntdelmin = Motor_InterTimerMin;//RightMotorSpeed.CircleTime >> 2;
//			tmpcntdelmin /= 5;
		}
		else
		{
			tmpcntdelmin = Motor_InterTimerMin;
		}
		
		tmpcntla = tmpcnt;
		
		if(tmpcntdel < tmpcntdelmin)
//		if(tmpcntdel < RightMotorSpeed.CircleTime)
			return;
		//////////////////////////////
		
		
		
		RightMotorSpeed.PulseCnt ++;
		
		if(RightMotorSpeed.PulseCnt >= 2)
		{
			RightMotorSpeed.PulseCnt = 0;
			return;
		}
		
		
		
        
		//右轮启动后第一次进中断
		if(RightMotor.RunEnable == ENABLE && RightMotor.RunFlag != SET)
		{
			RightMotor.RunFlag = SET;  				// 右轮运行标志置位
			
			
			if(RightMotor.Status == SYS_STARTUP)
				RightMotor.Status = SYS_RUNNING;  	//右轮状态切换至运行状态
            
//            TIM_Cmd(TIM7, ENABLE);

		}

//		RightMotor.RunCycle ++;
		
		    
		// 里程计计数，前进增加，后退减少
		if(RightMotor.Dir == MOTOR_RUN_DIR_ZW)
		{
			RightMileageAll ++;	
		}
		else if(RightMotor.Dir == MOTOR_RUN_DIR_FW)
		{
			RightMileageAll --;	
		}

		//里程或角度控制模式时，行走里程数增加
		if(RobitRouteModeEn)
		{
			RightCurMileage ++;
		}

		
		RightMotor.StopCnt = 0;  //停止计数清零
		RightMotor.Block_Cnt = 0;//堵转计数清零


        //一次脉冲计算一次速度
        if(tmpcnt > RightMotorSpeed.PreCountValue)
        {
            RightMotorSpeed.CircleTime = tmpcnt - RightMotorSpeed.PreCountValue;
        }
        else
        {
            RightMotorSpeed.CircleTime = tmpcnt + 0xffff - RightMotorSpeed.PreCountValue;
        }
        
        RightMotorSpeed.CalEn = ENABLE;
        
        RightMotorSpeed.PreCountValue = tmpcnt;

        
	}



//	if ( EXTI_GetITStatus(EXTI_Line8) != RESET )
//	{
//		EXTI_ClearITPendingBit(EXTI_Line8);
//		
//		Ultra_RevID = 1;

//		if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_8) == SET)
//		{
//			Ultra_RevLv[1] = 1;	
//		}
//		else
//		{
//			Ultra_RevLv[1] = 0;	
//		}

//		Ultra_Deal();
//	}

//	if ( EXTI_GetITStatus(EXTI_Line9) != RESET )
//	{
//		EXTI_ClearITPendingBit(EXTI_Line9);
//		
//		Ultra_RevID = 2;

//		if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_9) == SET)
//		{
//			Ultra_RevLv[2] = 1;	
//		}
//		else
//		{
//			Ultra_RevLv[2] = 0;	
//		}

//		Ultra_Deal();
//	}
}



/*****************************************************************************
 函 数 名  : EXTI9_5_IRQHandler
 功能描述  : 左轮速度检测中断
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年3月8日
    作    者   : 田宏图
    修改内容   : 新生成函数

*****************************************************************************/
void EXTI9_5_IRQHandler(void)
//void EXTI1_IRQHandler(void)
{
//	uint32_t tmpcircletime,tmpcurcountvalue;
//	uint8_t tmpoverflowcntbuf;
//    uint16_t tmpmotordata;
	
	static uint32_t tmpcntla = 0;
//	static uint8_t tmpfallingcnt = 0;
	uint32_t tmpcnt,tmpcntdel,tmpcntdelmin;
	
    
    
	if ( EXTI_GetITStatus(EXTI_Line5) != RESET )
	{
		EXTI_ClearITPendingBit(EXTI_Line5);	
		
		
		tmpcnt = TIM_GetCounter(TIM2);
		
		if(LeftMotor.RunEnable == ENABLE && LeftMotor.RunFlag != SET)
		{
			tmpcntla = LeftMotorSpeed.PreCountValue;
		}
		
		if(tmpcnt >= tmpcntla)
		{
			tmpcntdel = tmpcnt - tmpcntla;
		}
		else
		{
			tmpcntdel = 0xffff - tmpcntla + tmpcnt;			
		}
		
		if(LeftMotor.SpeedRisedFlag)
		{
			tmpcntdelmin = Motor_InterTimerMin;//LeftMotorSpeed.CircleTime >> 2;
//			tmpcntdelmin /= 5;
		}
		else
		{
			tmpcntdelmin = Motor_InterTimerMin;
		}
		
		tmpcntla = tmpcnt;
		
		
		
		if(tmpcntdel < tmpcntdelmin)
			return;
		
		
		LeftMotorSpeed.PulseCnt ++;
		
		if(LeftMotorSpeed.PulseCnt == 2)
		{
			LeftMotorSpeed.PulseCnt = 0;
			return;
		}
        
        
            
		//左轮启动后第一次进中断
		if(LeftMotor.RunEnable == ENABLE && LeftMotor.RunFlag != SET)
		{
			LeftMotor.RunFlag = SET;
			
			if(LeftMotor.Status == SYS_STARTUP)
				LeftMotor.Status = SYS_RUNNING;
			
//			if(tmpcnt >= LeftMotorSpeed.PreCountValue)
//			{
//				tmpcntdel = tmpcnt - LeftMotorSpeed.PreCountValue; 
//			}
//			else
//			{
//				tmpcntdel = LeftMotorSpeed.PreCountValue - tmpcnt;
//			}
            
//            TIM_Cmd(TIM1, ENABLE);
		}

		LeftMotor.RunCycle ++;

		if(LeftMotor.Dir == MOTOR_RUN_DIR_ZW)
		{
			LeftMileageAll ++;	
		}
		else if(LeftMotor.Dir == MOTOR_RUN_DIR_FW)
		{
			LeftMileageAll --;	
		}

		if(RobitRouteModeEn)
		{
			LeftCurMileage ++;
		}
		
		LeftMotor.StopCnt = 0;
		LeftMotor.Block_Cnt = 0;

			
        if(tmpcnt > LeftMotorSpeed.PreCountValue)
        {
            LeftMotorSpeed.CircleTime = tmpcnt - LeftMotorSpeed.PreCountValue;
        }
        else
        {
            LeftMotorSpeed.CircleTime = tmpcnt + 0xffff - LeftMotorSpeed.PreCountValue;
        }
        
        LeftMotorSpeed.CalEn = ENABLE;
        
        LeftMotorSpeed.PreCountValue = tmpcnt;

        
	}
    
    //睡眠唤醒
    if ( EXTI_GetITStatus(EXTI_Line8) != RESET )
	{
		EXTI_ClearITPendingBit(EXTI_Line8);	
        
//        if(SystePwrMode)
//        {
//            __set_FAULTMASK(1);// ??????
//            NVIC_SystemReset();// ??
//            Device_Init();
//        }
//        VL53L0X_IRQ();  
        vl53l0x_Mesure_En = 1; 
    }
    
    if ( EXTI_GetITStatus(EXTI_Line9) != RESET )
	{
		EXTI_ClearITPendingBit(EXTI_Line9);	
        
        if(SystePwrMode)
        {
            __set_FAULTMASK(1);// ??????
            NVIC_SystemReset();// ??
            Device_Init();
        }
    }
}

