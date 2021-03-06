/*
 *  TestUGens.cpp
 *  Plugins
 *  Copyright (c) 2007 Scott Wilson <i@scottwilson.ca>. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 *  Created by Scott Wilson on 22/06/2007.
 *  Modified by James Harkins on 28/07/2007.
 *
 *
 */

#include "SC_PlugIn.h"
#include <cstdio>

//////////////////////////////////////////////////////////////////////////////////////////////////


inline int sc_fpclassify(float x)
{
	return std::fpclassify(x);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static InterfaceTable *ft;

struct CheckBadValues : public Unit
{
	long	sameCount;
	int		prevclass;
};

// declare unit generator functions
extern "C"
{
	void CheckBadValues_Ctor(CheckBadValues* unit);
	void CheckBadValues_next(CheckBadValues* unit, int inNumSamples);
};

static const char *CheckBadValues_fpclassString(int fpclass);
inline int CheckBadValues_fold_fpclasses(int fpclass);

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBadValues_Ctor(CheckBadValues* unit)
{
	unit->prevclass = FP_NORMAL;
	unit->sameCount = 0;
	SETCALC(CheckBadValues_next);
	CheckBadValues_next(unit, 1);
}


void CheckBadValues_next(CheckBadValues* unit, int inNumSamples)
{
	float *in = ZIN(0);
	float *out = ZOUT(0);
	float id = ZIN0(1);
	int post = (int) ZIN0(2);

	float samp;
	int classification;

	switch(post) {
	case 1:		// post a line on every bad value
		LOOP1(inNumSamples,
			samp = ZXP(in);
			classification = sc_fpclassify(samp);
			switch (classification)
			{
				case FP_INFINITE:
					Print("Infinite number found in Synth %d, ID: %d\n", unit->mParent->mNode.mID, (int)id);
					ZXP(out) = 2;
					break;
				case FP_NAN:
					Print("NaN found in Synth %d, ID: %d\n", unit->mParent->mNode.mID, (int)id);
					ZXP(out) = 1;
					break;
				case FP_SUBNORMAL:
					Print("Denormal found in Synth %d, ID: %d\n", unit->mParent->mNode.mID, (int)id);
					ZXP(out) = 3;
					break;
				default:
					ZXP(out) = 0;
			};
		);
		break;
	case 2:
		LOOP1(inNumSamples,
			samp = ZXP(in);
			classification = CheckBadValues_fold_fpclasses(sc_fpclassify(samp));
			if(classification != unit->prevclass) {
				if(unit->sameCount == 0) {
					Print("CheckBadValues: %s found in Synth %d, ID %d\n",
						CheckBadValues_fpclassString(classification), unit->mParent->mNode.mID, (int)id);
				} else {
					Print("CheckBadValues: %s found in Synth %d, ID %d (previous %d values were %s)\n",
						CheckBadValues_fpclassString(classification), unit->mParent->mNode.mID, (int)id,
						(int)unit->sameCount, CheckBadValues_fpclassString(unit->prevclass)
					);
				};
				unit->sameCount = 0;
			};
			switch (classification)
			{
			case FP_INFINITE:
				ZXP(out) = 2;
				break;
			case FP_NAN:
				ZXP(out) = 1;
				break;
			case FP_SUBNORMAL:
				ZXP(out) = 3;
				break;
			default:
				ZXP(out) = 0;
			};
			unit->sameCount++;
			unit->prevclass = classification;
		);
		break;
	default:		// no post
		LOOP1(inNumSamples,
			samp = ZXP(in);
			classification = sc_fpclassify(samp);
			switch (classification)
			{
			case FP_INFINITE:
				ZXP(out) = 2;
				break;
			case FP_NAN:
				ZXP(out) = 1;
				break;
			case FP_SUBNORMAL:
				ZXP(out) = 3;
				break;
			default:
				ZXP(out) = 0;
			};
		);
		break;
	}
}

const char *CheckBadValues_fpclassString(int fpclass)
{
	switch(fpclass) {
		case FP_NORMAL:       return "normal";
		case FP_NAN:          return "NaN";
		case FP_INFINITE:     return "infinity";
#ifndef _MSC_VER
		case FP_ZERO:         return "zero";
#endif
		case FP_SUBNORMAL:    return "denormal";
		default:              return "unknown";
	}
}

#ifndef _MSC_VER
inline int CheckBadValues_fold_fpclasses(int fpclass)
{
	switch(fpclass) {
		case FP_ZERO:   return FP_NORMAL; // a bit hacky. we mean "zero is fine too".
		default:        return fpclass;
	}
}
#else
inline int CheckBadValues_fold_fpclasses(int fpclass)
{
	return fpclass;
}
#endif

////////////////////////////////////////////////////////////////////

// the load function is called by the host when the plug-in is loaded
PluginLoad(Test)
{
	ft = inTable;
	DefineSimpleUnit(CheckBadValues);
}
