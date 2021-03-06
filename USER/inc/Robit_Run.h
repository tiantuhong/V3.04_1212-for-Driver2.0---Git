#ifndef _ROBIT_RUN_H
#define _ROBIT_RUN_H

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/


//角度控制模式中，一个循环能够增加的角度，单位0.01du
//目前陀螺仪最大测量角速度为200度/s,而机器实际最大角速度在150度/s左右
//目前的陀螺仪采集的频率为100Hz,采样时间为10ms,单次角度最大变化量200
#define ANGLE_DELT  200

#if defined MCU_STM32
#define PWM_K	9  / 8
#elif defined MCU_GD32
#define PWM_K	27 / 16
#endif

#define		Motor_L_F_PWM(_Num_)	TIM_SetCompare1(TIM4,_Num_*PWM_K)
#define		Motor_L_B_PWM(_Num_)	TIM_SetCompare2(TIM4,_Num_*PWM_K)
#define		Motor_R_F_PWM(_Num_)	TIM_SetCompare3(TIM4,_Num_*PWM_K)
#define		Motor_R_B_PWM(_Num_)	TIM_SetCompare4(TIM4,_Num_*PWM_K)

//#define		Motor_L_F_DownOn()		GPIO_WriteBit(GPIOE,GPIO_Pin_0,Bit_RESET)
//#define		Motor_L_B_DownOn()		GPIO_WriteBit(GPIOE,GPIO_Pin_1,Bit_RESET)
//#define		Motor_R_F_DownOn()		GPIO_WriteBit(GPIOE,GPIO_Pin_3,Bit_RESET)
//#define		Motor_R_B_DownOn()		GPIO_WriteBit(GPIOE,GPIO_Pin_2,Bit_RESET)

//#define		Motor_L_F_DownOff()		GPIO_WriteBit(GPIOE,GPIO_Pin_0,Bit_SET)
//#define		Motor_L_B_DownOff()		GPIO_WriteBit(GPIOE,GPIO_Pin_1,Bit_SET)
//#define		Motor_R_F_DownOff()		GPIO_WriteBit(GPIOE,GPIO_Pin_3,Bit_SET)
//#define		Motor_R_B_DownOff()		GPIO_WriteBit(GPIOE,GPIO_Pin_2,Bit_SET)



#define		Motor_L_B_Run(_Pwm_)	Motor_L_B_PWM(0);Motor_L_F_PWM(_Pwm_);
#define		Motor_L_F_Run(_Pwm_)	Motor_L_F_PWM(0);Motor_L_B_PWM(_Pwm_);
#define		Motor_R_F_Run(_Pwm_)	Motor_R_B_PWM(0);Motor_R_F_PWM(_Pwm_);
#define		Motor_R_B_Run(_Pwm_)	Motor_R_F_PWM(0);Motor_R_B_PWM(_Pwm_);
#define		Motor_L_Brake()			Motor_L_B_PWM(TIM4_Period);Motor_L_F_PWM(TIM4_Period);
#define		Motor_R_Brake()			Motor_R_B_PWM(TIM4_Period);Motor_R_F_PWM(TIM4_Period);
#define		Motor_L_Stop()			Motor_L_B_PWM(0);Motor_L_F_PWM(0);
#define		Motor_R_Stop()			Motor_R_B_PWM(0);Motor_R_F_PWM(0);

#define		Motor_Circle_PlusNum	16
#define		Motor_K		60000000L / Motor_Circle_PlusNum
#define		Motor_InterTimerMin		150  //100

#define		Motor_Stop_Time		50      //单位ms
#define		MOTOR_BLOCK_TIME	50		//单位ms
#define 	PWM_STATRUP_VAL_STEP 50
#define 	PWM_STATRUP_VAL_MAX  4400
#define 	MOT_RPM_SPEED_MIN_2 650  
#define 	MOT_RPM_SPEED_MIN   700     
#define     MOT_RPM_SPEED_MAX_2 6500
#define 	MOT_RPM_SPEED_MAX   6000 
#define 	PWM_STATRUP_VAL     2800
#define 	PWM_VAL_MAX   		4480//TIM4_Period  //4480
#define 	PWM_VAL_MIN  	 	500
#define		PWM_VAL_MIN_Low		900
#define     ANGLERATE_MIN       1800

#define		Motor_Stop_Step		50

#define		Edage_Distance		300 //280  // 5cm

typedef enum{
  SYS_STOP,       //电机已停止 
  SYS_STARTUP,    //电机启动中
  SYS_RUNNING,    //电机正在运行中
  SYS_STOPING,    //电机正在停止中
  SYS_BLOCKING,   //电机堵转状态
  SYS_CHECK,      //电机启动前检查
  SYS_ERROR,	  //故障状态
}SystemSta;

typedef enum 
{ 
	MOTOR_RUN_DIR_ZW    = (uint8_t)0x00,  /* 电机正转 */
	
    MOTOR_RUN_DIR_FW   = (uint8_t)0x01, /* 电机反转 */
    
} MOTOR_RUNDIR_TypeDef;


typedef struct
{
  uint16_t RunCycle;		

  uint16_t RunCircle;
  
  uint16_t CurSpeed;

  uint16_t TargSpeed;
  
  uint16_t BaseSpeed;
    
  int32_t Vacc;
    
  int32_t Wacc;
    
  int16_t  SpeedAdjAll;
  
  uint16_t Block_Cnt;
  
  uint16_t StopCnt;

  uint16_t PWMVal;

  FunctionalState RunEnable;

  FlagStatus RunFlag;
	
  FlagStatus SpeedRisedFlag;

  SystemSta  Status;

  MOTOR_RUNDIR_TypeDef Dir;
  
  MOTOR_RUNDIR_TypeDef TargDir;

} Motor_TypeDef;

typedef struct
{
  uint16_t PreCountValue;   //上一次计数值
  
  uint8_t OverflowCnt;     //定时器溢出次数

  uint32_t CircleTime;      //电机运行一圈时间  单位1us

  FunctionalState CalEn;	//计算速度使能
  
  FunctionalState ValidForAnguar; //速度有效，用于计算角速度

  FunctionalState Valid;    //速度有效，用于速度PID调节

  uint8_t  PulseCnt;

} MotorSpeed_TypeDef;



extern Motor_TypeDef LeftMotor;
extern Motor_TypeDef RightMotor;
extern MotorSpeed_TypeDef LeftMotorSpeed,RightMotorSpeed;
extern FunctionalState AdjustSpeedEn,MotorLinerModeEn,MotorStopCtrlEn,RobitModeCtrlEn,AccCtrlTimeEn;
extern uint32_t TargMileage,LeftCurMileage,RightCurMileage, TargMileageForArc;
extern FunctionalState RobitRouteModeEn;
extern FunctionalState MotorStartupEn;
extern uint8_t RobitStatus;  				// 0--停止状态 1-- 工作状态
extern uint8_t RobitRunningMode;           //扫地机工作模式  0--开环工作方式 1--闭环工作方式
extern uint8_t RobitRunningClosedLoopMode; //扫地机闭环工作方式  0--速度闭环方式 1--行程闭环工作方式
extern uint8_t RobitRunbyRouteMode;		//扫地机行程闭环方式  0--直线行驶目标里程数1--轴心转动目标角度
extern int32_t  LeftMileageAll,RightMileageAll; //左右轮累计里程数，前进增加，后退减小。
extern int32_t  LeftMileageAllPre,RightMileageAllPre;
extern FlagStatus LeftMotorSwDirFlag,RightMotorSwDirFlag;//扫地机速度闭环模式中，方向切换标志位
extern uint16_t   LeftMotorSpeedBak,RightMotorSpeedBak;//扫地机速度闭环模式中，方向切换时的速度备份
extern MOTOR_RUNDIR_TypeDef    LeftMotorDirBak,RightMotorDirBak;//扫地机速度闭环模式中，方向切换时的方向备份
extern uint8_t SpeedAdjTime;   //轮子速度PID调节时间
extern uint16_t LeftMotorErrorCnt,RightMotorErrorCnt;
extern int16_t Sys_Startangle,AnglerateTarg, AnglerateTargInit;
extern uint8_t  TargCirle;
extern uint16_t  TargAngle, RotationAngleCur;
extern uint16_t StepModeCnt;
extern uint8_t Robit_Dir;    //0---前进  1---后�
extern int32_t RobotLineSpeed, RobotLineSpeedTarg, RobotAngleRate; //机器人线速度(mm/s)和角速度(毫弧度/s)
extern FunctionalState VaccEn, WaccEn; //线加速使能标志 ， 角加速度使能标志
extern uint8_t VaccInit, WaccInit;
extern int16_t AngularVelocityOdo;   
extern uint8_t CarpetStatus, CarpetCheckEn;
extern uint8_t Sta_Gyro, UpCnt, DownCnt;
extern uint8_t earthWaveCnt, earthAvgOk;
extern uint16_t earthAvg;
extern uint16_t earthWaveMax,earthWaveMin;

extern void Robit_RunLR(uint8_t LeftMotor_Speed, uint8_t LeftMotor_Dir, uint8_t RightMotor_Speed, uint8_t RightMotor_Dir);
extern void Robit_RunDir(uint8_t Speed, uint8_t Dir);
extern void Robit_Stop(void);
extern void MotorStopCtrl(void);
extern void CalSpeed(void);
extern void LeftMotorEnterRun(uint8_t LeftMotor_Dir);
extern void RightMotorEnterRun(uint8_t RightMotor_Dir);
extern void RightMotorStop(void);
extern void LeftMotorStop(void);
extern void RightMotorRunInit(void);
extern void MotorRunCtrl(void);
extern void Robit_RouteMoving(void);
extern void Motor_Conf(void);
extern void RobitMode_Conf(void);
extern void MotorStartupCtrl(void);
extern void MotorBlockCtrl(void);
extern void AccCtrl(void);
extern void RobitMode_Ctrl(void);
extern uint16_t CalAngleDif(int16_t Angle1, int16_t Angle2);
//extern int16_t DistanceCalu(void);
extern int16_t CalOdoAngularVelocity(int16_t leftSpeed, int16_t rightSpeed);
extern void CarpetCheck(void);

#endif

