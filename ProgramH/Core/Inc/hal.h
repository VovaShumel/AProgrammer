#ifndef INC_HAL_H_
#define INC_HAL_H_

#include "inc.h"

//#define HW_IsBtnPressed() (TBool)(GPIOC->IDR &   (1 << 13)) // TODO почему-то с HW_IsBtnPressed ругается "'GPIOE' undeclared (first use in this function)"
#define HW_TurnOnLED()			  GPIOE->ODR |=  (1 <<  3)
#define HW_TurnOffLED()		  	  GPIOE->ODR &= ~(1 <<  3)

#endif /* INC_HAL_H_ */
