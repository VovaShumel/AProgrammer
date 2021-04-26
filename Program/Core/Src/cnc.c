#include "cnc.h"
#include "math.h"
#include "stdlib.h"

#define sqr(x) ((x) * (x))

#define TIM_CCER_CCxE_MASK ((uint32_t)(TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E))
#define TIM_CCER_CCxNE_MASK ((uint32_t)(TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE))
////////////////////////////////////////////////////////////////////////////////

typedef enum
{ 
    DRIVE_IDLED,
    DRIVE_ACCELERATING,
    DRIVE_RUNNING,
    DRIVE_DECELERATING
} DRIVE_STAGE_TypeDef;

//#define USE_FLOAT /* comment to use int32 math */

#ifdef USE_FLOAT    
    #define MATH_TYPE float
    #define _ABS fabsf
#else
    #define MATH_TYPE int
    #define _ABS abs
#endif


#define RESET_DOUT(PORT,PIN) HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_RESET)
#define SET_DOUT(PORT,PIN)   HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_SET)

#define _TIM_GET_FLAG(__HANDLE__, __FLAG__)             (((__HANDLE__)->SR &(__FLAG__)) == (__FLAG__))
#define _TIM_SET_AUTORELOAD(__HANDLE__, __AUTORELOAD__) (__HANDLE__)->ARR = (__AUTORELOAD__)
#define _TIM_CLEAR_FLAG(__HANDLE__, __FLAG__)           (__HANDLE__)->SR = ~(__FLAG__)
#define _TIM_CLEAR_IT(__HANDLE__, __INTERRUPT__)     ((__HANDLE__)->SR = ~(__INTERRUPT__))
#define _TIM_ENABLE_IT(__HANDLE__, __INTERRUPT__)    ((__HANDLE__)->DIER |= (__INTERRUPT__))
#define _TIM_DISABLE_IT(__HANDLE__, __INTERRUPT__)  ((__HANDLE__)->DIER &= ~(__INTERRUPT__))
#define _TIM_GET_COUNTER(__HANDLE__)                    ((__HANDLE__)->CNT)
#define _TIM_ENABLE(__HANDLE__)                 ((__HANDLE__)->CR1|=(TIM_CR1_CEN))
#define _TIM_DISABLE(__HANDLE__) \
                        do { \
                          if (((__HANDLE__)->CCER & TIM_CCER_CCxE_MASK) == 0) \
                            { \
                            if(((__HANDLE__)->CCER & TIM_CCER_CCxNE_MASK) == 0) \
                            { \
                              (__HANDLE__)->CR1 &= ~(TIM_CR1_CEN); \
                            } \
                          } \
                        } while(0)


typedef struct
{
    TCoord x_max;
    MATH_TYPE  V0;   // start motion speed in pulses / sec
    int    Anom; // acceleration in pulses / sec^2
    int    Vnom/*[CNC_SPEED_LAST]*/; // speeds for different speed modes

    // set by user
    TCoord dest; // point of destenation

    // cur state vars
    DRIVE_STAGE_TypeDef stage;
    int                 TaskFinished; // task is finished
    volatile int        ToStop; // decelerate and stop without return to stop point
    TCoord              x; // cur position in pulses
    MATH_TYPE           v; // cur velocity in pulses / sec
    MATH_TYPE           v_nom; // current nominal speed
    MATH_TYPE           a; // acceleration with sign (direction) in pulses / sec^2
    int                 pulse_width;

    int cycles;
} MOTOR2_TypeDef;

typedef struct
{
    MOTOR2_TypeDef     m[MC]; // 0 - x, 1 - y, 2 - z
    //volatile int       TaskFinished;
    CNC_SPEED_TypeDef  speed;
    int                check_x; // set to 0 for single eliminating checking x coord before change dest
} CNC2_TypeDef;

                          
CNC_TypeDef cnc;

CNC2_TypeDef cnc2;

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// CNC LEVEL routines
//------------------------------------------------------------------------------

void CNC_StartTask_ab(int a, int b)
{
  //cnc2.TaskFinished = 0;
  for (int i=a; i<=b; i++)
  {
	  cnc2.m[i].TaskFinished = 0;
	  cnc2.m[i].stage        = DRIVE_IDLED;
  }
}

void CNC_StartTask()
{
	CNC_StartTask_ab (0, MC-1);
}

void CNC_StartTask_i(int i)
{
	CNC_StartTask_ab (i, i);
}
////////////////////////////////////////////////////////////////////////////////

//int CNC_AtHome(int i)
//{
//  return GET_DIN(cnc.m[i].limit) ? (1^cnc.m[i].limit_inv) : (0^cnc.m[i].limit_inv);
//}
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// USER LEVEL routines
//------------------------------------------------------------------------------

void CNC_Init()
{
    // Init signals
  
    for (int i=0; i<MC; i++)
    {
        RESET_DOUT(cnc.m[i].dir_port,  cnc.m[i].dir_pin);
        RESET_DOUT(cnc.m[i].step_port, cnc.m[i].step_pin);
    }
    
    // Enable drivers
    if (cnc.enable_port)  RESET_DOUT(cnc.enable_port, cnc.enable_pin);

    // Init vars    
    for (int i=0; i<MC; i++)   
    {
      // convert from mm to pulses
      cnc2.m[i].x_max = cnc.m[i].x_max * cnc.m[i].mm / 100;
      cnc2.m[i].Anom  = cnc.m[i].Anom  * cnc.m[i].mm / 100;
      cnc2.m[i].V0    = (MATH_TYPE)sqrt(cnc2.m[i].Anom / 2); // v0 = sqrt(a/2)

//      for (int j=0; j<CNC_SPEED_LAST; j++)
        cnc2.m[i].Vnom/*[j]*/  = cnc.m[i].Vnom/*[j]*/ * cnc.m[i].mm / 100;
           
      cnc2.m[i].x = 0;
      cnc2.m[i].stage = DRIVE_IDLED;
      cnc2.m[i].TaskFinished = 1;
      cnc2.m[i].ToStop = 0;
      
      _TIM_DISABLE(   cnc.m[i].tim);
      _TIM_CLEAR_FLAG(cnc.m[i].tim, TIM_SR_UIF);
      _TIM_ENABLE_IT( cnc.m[i].tim, TIM_DIER_UIE);
    }
    
//    cnc2.check_x = 1;
    cnc2.speed = CNC_SPEED_NORMAL;
}
////////////////////////////////////////////////////////////////////////////////

//int CNC_FindHomes()
//{
//  int inited        [MC];
//  int home_realeased[MC];
//  int home_reached  [MC];
//  int homes = 0;
//
//  Delay(1);
//
//  // init and prepare coords
//  TCoords c;
//  for (int i=0; i<MC; i++)
//  {
//    home_reached  [i] = 0;
//    home_realeased[i] = CNC_AtHome(i) ? 0 : 1;
//
//    TCoord max = cnc2.m[i].x_max;
//
//    if (i == 2) // Z
//    {
//      inited[i] = 1;
//      c[i] = home_realeased[i] ? -(max+max/5) : max / 10;
//    }
//    else
//    {
//      c[i] = CNC_GetCoord(i); // don't move
//      inited[i] = 0;
//    }
//  }
//
//  cnc2.check_x = 0; // skip coord checking
//  CNC_SetSpeed(CNC_SPEED_SLOW);
//  CNC_Start_GoTo(c);
//
//  Delay(1); // delay for home signals settling at power on
//
//  while (!CNC_TaskFinished())
//  {
//    for (int i=0; i<MC; i++)
//    {
//      if ((i==2) || (home_reached[2])) // if Z coord or when Z was at home
//      {
//        if (!inited[i]) // init for other axis (X, Y) when Z was at home
//        {
//          TCoord max = cnc2.m[i].x_max;
//          cnc2.m[i].dest = home_realeased[i] ? -(max+max/5) : max / 10;
//          cnc2.m[i].TaskFinished = 0; // start task (it can be finished)
//          inited[i] = 1;
//        }
//        else if (!home_realeased[i]) // forward moving (from home)
//        {
//          if (!CNC_AtHome(i)) // if outside of home
//          {
//            home_realeased[i] = 1;
////TODO: Up to 1 mm error of Home searching, so should move further away from home.
//
////TODO: Use statemachine, not several vars (home_realeased, home_reached, inited)
//            cnc2.m[i].dest = -cnc2.m[i].x_max;
//            cnc2.m[i].TaskFinished = 0; // start task (it can be finished)
//          }
//          else if (cnc2.m[i].TaskFinished)
//          {
//            return 0; // error: home sensor locked
//          }
//        }
//             // reverse moving
//        else if ((!home_reached[i]) && (CNC_AtHome(i))) // if home reached
//        {
//          home_reached[i] = 1;
//          homes |= 1 << i;
//          cnc2.m[i].x    = -cnc.m[i].home_ofs * cnc.m[i].mm / 100;
//
//if (i==2)
//{
//	int cnt   = -cnc.m[i].home_ofs * 600 / 800;
//	int cnt16 = cnt;
//	__HAL_TIM_SET_COUNTER(&htim2, cnt16);
//}
//          cnc2.m[i].TaskFinished = 0; // start task (it can be finished)
//          cnc2.m[i].dest = 0;
//        }
//      }
//    }
//
//    CNC_Update();
//  }
//
//  return homes ^ ((1 << MC) -1); // return bits of not reached homes
//}
//////////////////////////////////////////////////////////////////////////////////

int CNC_GoTo(TCoords c, int check_homes, int * to_stop)
{
  for (int i=0; i<MC; i++)
  {
    TCoord max = cnc2.m[i].x_max;

    if (cnc2.check_x)
    {
      if      (c[i] > max               ) c[i] = max;
      else if (c[i] < -cnc.m[i].home_ofs) c[i] = -cnc.m[i].home_ofs;
    }

    cnc2.m[i].dest = c[i] * cnc.m[i].mm / 100;
  }
  CNC_StartTask();
//  cnc2.check_x = 1;
  
  while (!CNC_TaskFinished())
  {
    CNC_Update();
    
    if ((to_stop) && (*to_stop)) 
    {
      CNC_Stop(); 
      HAL_Delay(100);
    }
    
//    if (check_homes)
//		for (int i=0; i<MC; i++)
//		  if (CNC_AtHome(i))
//		  {
//			CNC_Stop();
//			Delay(100);
//			return i+1;
//		  }

    if ((to_stop) && (*to_stop)) break;
  }
  
  return 0;
}
////////////////////////////////////////////////////////////////////////////////

void CNC_Start_GoTo_ab(int a, int b, TCoord * c)
{
  for (int i=a; i<=b; i++)
  {
//    TCoord max = cnc.m[i].x_max;
//
//    if (cnc2.check_x)
//    {
//      if      (c[i] > max               ) c[i] = max;
//      else if (c[i] < -cnc.m[i].home_ofs) c[i] = -cnc.m[i].home_ofs;
//    }

    cnc2.m[i].dest = c[i-a] * cnc.m[i].mm / 100;
  }
//  cnc2.check_x = 1;
  CNC_StartTask_ab (a,b);
}

void CNC_Start_GoTo(TCoords c)
{
	CNC_Start_GoTo_ab (0, MC-1, &c[0]);
}

void CNC_Start_GoTo_i(int i, TCoord * c)
{
	CNC_Start_GoTo_ab (i,i, c);
}
////////////////////////////////////////////////////////////////////////////////

//void CNC_SetSpeedSet(CNC_SPEED_TypeDef cnc_speed)
//{
//  cnc2.speed = cnc_speed;
//}
////////////////////////////////////////////////////////////////////////////////

void CNC_SetSpeed_i(int i, int speed)
{
    cnc.m[i].Vnom = speed;
    cnc2.m[i].Vnom/*[cnc_speed]*/  = cnc.m[i].Vnom/*[cnc_speed]*/ * cnc.m[i].mm / 100;
}
////////////////////////////////////////////////////////////////////////////////

void CNC_Stop()
{
	for (int i=0; i<MC; i++)  cnc2.m[i].ToStop = 1;

	while(!CNC_TaskFinished()) CNC_Update();

	for (int i=0; i<MC; i++)  cnc2.m[i].ToStop = 0;
}
////////////////////////////////////////////////////////////////////////////////

void CNC_Stop_i(int i)
{
  cnc2.m[i].ToStop = 1;

  while(!CNC_TaskFinished_i(i)) ; //CNC_Update_i(i);

  cnc2.m[i].ToStop = 0;
}
////////////////////////////////////////////////////////////////////////////////

void CNC_Halt()
{
	for (int i=0; i<MC; i++)
	{
		_TIM_DISABLE(cnc.m[i].tim);
		cnc2.m[i].TaskFinished = 1;
		cnc2.m[i].ToStop = 0;
	}

	//cnc2.TaskFinished = 1;
}
////////////////////////////////////////////////////////////////////////////////

void CNC_StopHere()
{
  for (int i=0; i<MC; i++) 
    cnc2.m[i].dest = cnc2.m[i].x;

  while(!CNC_TaskFinished()) CNC_Update();
}
////////////////////////////////////////////////////////////////////////////////

TCoord CNC_GetCoord(int i)
{
  return cnc2.m[i].x * 100 / cnc.m[i].mm;
}
////////////////////////////////////////////////////////////////////////////////

void CNC_ResetCoord(int i)
{
	cnc2.m[i].x = 0;
}
////////////////////////////////////////////////////////////////////////////////

int CNC_TaskFinished()
{
    // check Task Finishing for all motors
	int finished = 1;
	for (int i=0; i<MC; i++)
		finished &= cnc2.m[i].TaskFinished;

    return finished;
}
////////////////////////////////////////////////////////////////////////////////

int CNC_TaskFinished_i(int i)
{
  return cnc2.m[i].TaskFinished;
}

////////////////////////////////////////////////////////////////////////////////
#define _TIM_ENABLED(__HANDLE__) ((__HANDLE__)->CR1 & (TIM_CR1_CEN))

void CNC_Del()
{
//  volatile int s = 0;
//  for (int i=0; i<cnc.pulse_width; i++) s += i;
//  return s;
	_TIM_ENABLE(cnc.del_tim);
	while (cnc.del_tim->CR1 & TIM_CR1_CEN); // wait until timer is off
}

void CNC_ISR(int i) // duration: 6..14 us
{
  MOTOR_TypeDef  * m  = &cnc.m[i];
  MOTOR2_TypeDef * m2 = &cnc2.m[i];
  
  // temp vars used for speedup ISR
  DRIVE_STAGE_TypeDef stage = m2->stage;
  MATH_TYPE a    = m2->a;
  MATH_TYPE v    = m2->v;
  MATH_TYPE V0   = m2->V0;
  TCoord    x    = m2->x;
  TCoord    dest = m2->dest;
  
  m2->cycles++;

  // calc and send pulse (set STEP pin)	

  // condition for switching to DECELERATING
  if (stage != DRIVE_IDLED) // if moving
  {
      if (cnc2.m[i].ToStop)
      {
        stage = DRIVE_DECELERATING;
      }
      else
      {
        TCoord pts = (TCoord)((sqr(v) - sqr(V0)) / (2*a)); // Pulses_To_Stop (decelerating point) = (v-v0)^2 / a

        if (((a > 0) && (x >= dest - pts)) || // if moving forward  && decelerating point has been reached
            ((a < 0) && (x <= dest - pts)))   // if moving backward && decelerating point has been reached
        {
            stage = DRIVE_DECELERATING;
        }
        else if (stage != DRIVE_RUNNING) 
        {
            // if coordinate has been updated and decelerating point is already not reached...
            // then try to accelerate
            stage = DRIVE_ACCELERATING;
        }
      }
  }
  
  // condition for switching to RUNNING
  if (stage == DRIVE_ACCELERATING)
  {
        if (((v > 0) && (v > m2->v_nom)) ||
            ((v < 0) && (v < m2->v_nom)))
        {
                // speed has been reached
                v = m2->v_nom;
                stage = DRIVE_RUNNING;
        }
  }
  
  // condition for switching to IDLE
  if ((x == dest) && (_ABS(v) < 2*(a / V0))) // if dest point has been reached and speed is low enough (< 2*V0)
  {
        stage = DRIVE_IDLED;
  }
  
  //  calc new speed
  if (stage == DRIVE_ACCELERATING)
  {
#ifdef USE_FLOAT
    v = v + a /_ABS(v); // v = v0 + a*t = v0 + a*(1/v), 1/v - period of last pulse (in seconds)
#else    
    v = v + (a*1000/_ABS(v) + (a>0 ? 500 : -500)) / 1000; // v = v0 + a*t = v0 + a*(1/v), 1/v - period of last pulse (in seconds)
#endif    
  }
  else if (stage == DRIVE_DECELERATING)
  {
      MATH_TYPE prev_v = v;
#ifdef USE_FLOAT
      v = v - a / _ABS(v); // v = v0 - a*t = v0 - a*(1/v), 1/v - period of last pulse (in seconds)
#else    
      v = v - (a*1000/_ABS(v) + (a>0 ? 500 : -500)) / 1000; // v = v0 - a*t = v0 - a*(1/v), 1/v - period of last pulse (in seconds)
#endif    

      // additional condition for switching to IDLE	  
      if ((_ABS(v) <= V0) || // if almost stopped
          (v * prev_v < 0))    // or if speed changes direction (sign) - overshooted
      {
          stage = DRIVE_IDLED;
      }
  }
  
  // condition for switching to ACCELERATING 
  // init section
  if (stage == DRIVE_IDLED)
  {
        if ((x != dest) && (!cnc2.m[i].ToStop)) // if dest point is not reached
        {
            if (dest > x) // forward moving
            {
                    v         = V0;
                    a         = m2->Anom;
                    m2->v_nom = m2->Vnom/*[cnc2.speed]*/;
                    if (!m->dir_inv) RESET_DOUT(m->dir_port, m->dir_pin);
                    else             SET_DOUT  (m->dir_port, m->dir_pin);
            }
            else     // backward moving
            {
                    v         = -V0;
                    a         = -m2->Anom;
                    m2->v_nom = -m2->Vnom/*[cnc2.speed]*/;
                    if (!m->dir_inv) SET_DOUT  (m->dir_port, m->dir_pin);
                    else             RESET_DOUT(m->dir_port, m->dir_pin);
            }

            for (int i=0; i<m->dir_delay; i++) CNC_Del();

            stage = DRIVE_ACCELERATING;
        }
        else
        {
            v = 0;
        }
        
        _TIM_DISABLE(m->tim);
        _TIM_CLEAR_FLAG(m->tim, TIM_SR_UIF);
//        RESET_DOUT(m->step);
//        Delay_us(1); // 200 ns for settling DIR pin (see RM for A4988)
  }

  // generate STEP pulse
  if (stage != DRIVE_IDLED)
  {
    x += (v > 0) ? 1 : -1; // inc/dec coordinate
      
    // calc Autoreload registers value
    int arr = (uint32_t)_ABS(1000000 / v); // in microseconds

    _TIM_SET_AUTORELOAD(m->tim, arr);

    SET_DOUT(m->step_port, m->step_pin);
    CNC_Del();
    RESET_DOUT(m->step_port, m->step_pin);
      
    _TIM_CLEAR_FLAG(m->tim, TIM_SR_UIF);
    
    if (!_TIM_ENABLED(m->tim)) // if timer is already running
      _TIM_ENABLE(m->tim); // start timer
  }
  
  // condition for Task Finishing
  if (stage == DRIVE_IDLED)
  {
    if ((x == dest) || (cnc2.m[i].ToStop))
    {
      _TIM_DISABLE(m->tim);
      _TIM_CLEAR_FLAG(m->tim, TIM_SR_UIF);
      m2->TaskFinished = 1;
      if (cnc2.m[i].ToStop) dest = x;
    }
  }
  
  // save back
  m2->a     = a;
  m2->v     = v;
  m2->V0    = V0;
  m2->x     = x;
  m2->dest  = dest;
  m2->stage = stage;
}
////////////////////////////////////////////////////////////////////////////////

void CNC_Update_ab(int a, int b)
{

  for (int i=a; i<=b; i++)
  {
    MOTOR_TypeDef  * m  = &cnc.m[i];
    MOTOR2_TypeDef * m2 = &cnc2.m[i];

    if ((!_TIM_ENABLED(m->tim)) && (m2->TaskFinished == 0))
      CNC_ISR(i);

    if (m2->TaskFinished)
    		cnc2.m[i].ToStop = 0;
  }
}

void CNC_Update()
{
	CNC_Update_ab(0, MC-1);
}

void CNC_Update_i(int i)
{
	CNC_Update_ab(i, i);
}
////////////////////////////////////////////////////////////////////////////////

