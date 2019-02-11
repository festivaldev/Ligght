//
//  LigghtPrograms.h
//  Ligght – Daisy-chained traffic lights controlled via I2C
//
//  Created by Janik Schmidt on 10.02.19.
//  Copyright © 2019 FESTIVAL Development. All rights reserved.
//

#include "Ligght.h"

#ifndef LigghtPrograms_h
#define LigghtPrograms_h

typedef struct {
	LightTransitionState mainState;
	LightTransitionState secondaryState;
	double beginTime;
	double duration;
	bool isFinalStep;
} LightProgramStep;

LightProgramStep maintenance[] = {
	{
		mainState: None,
		secondaryState: Yellow,
		beginTime: 0,
		duration: 1E3,
	},
	{
		mainState: None,
		secondaryState: None,
		beginTime: 1E3,
		duration: 1E3,
		isFinalStep: true
	},
};

LightProgramStep simpleLight[] = {
	{
		mainState: None,
		secondaryState: Green,
		beginTime: 0,
		duration: 20E2
	},
	{
		mainState: None,
		secondaryState: Yellow,
		beginTime: 20E2,
		duration: 3E2
	},
	{
		mainState: None,
		secondaryState: Red,
		beginTime: 23E2,
		duration: 2E2
	},
	
	{
		mainState: None,
		secondaryState: RedYellow,
		beginTime: 49E2,
		duration: 2E2
	},
	{
		mainState: None,
		secondaryState: Green,
		beginTime: 51E2,
		duration: 20E2,
		isFinalStep: true
	},
};

LightProgramStep fourWayJunction[] {
	{
		mainState: Green,
		secondaryState: Red,
		beginTime: 0,
		duration: 20E2
	},
	{
		mainState: Yellow,
		secondaryState: Red,
		beginTime: 20E2,
		duration: 3E2
	},
	{
		mainState: Red,
		secondaryState: Red,
		beginTime: 23E2,
		duration: 2E2
	},
	{
		mainState: Red,
		secondaryState: RedYellow,
		beginTime: 25E2,
		duration: 2E2
	},
	{
		mainState: Red,
		secondaryState: Green,
		beginTime: 27E2,
		duration: 12.5E2
	},
	{
		mainState: Red,
		secondaryState: Yellow,
		beginTime: 39.5E2,
		duration: 3E2,
	},
	{
		mainState: Red,
		secondaryState: Red,
		beginTime: 42.5E2,
		duration: 2E2
	},
	{
		mainState: RedYellow,
		secondaryState: Red,
		beginTime: 44.5E2,
		duration: 2E2,
		isFinalStep: true
	},
};

#endif