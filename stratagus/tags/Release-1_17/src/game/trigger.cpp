//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name trigger.c	-	The trigger handling. */
//
//	(c) Copyright 2002 by Lutz Sammer
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freecraft.h"

#include "ccl.h"
#include "unittype.h"
#include "player.h"
#include "trigger.h"
#include "campaign.h"
#include "interface.h"

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

    /// Get unit-type.
extern UnitType* CclGetUnitType(SCM ptr);


#define ANY_UNIT	((const UnitType*)0)
#define ALL_UNITS	((const UnitType*)-1)
#define ALL_FOODUNITS	((const UnitType*)-2)
#define ALL_BUILDINGS	((const UnitType*)-3)

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

local SCM Trigger;			/// Current trigger

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Get player number.
**
**	@param player	The player
**
**	@return		The player number, -1 matches any.
*/
local int TriggerGetPlayer(SCM player)
{
    int ret;

    if (gh_exact_p(player)) {
	ret = gh_scm2int(player);
	if (ret < 0 || ret > PlayerMax) {
	    errl("bad player", player);
	}
	return ret;
    }
    if (gh_eq_p(player, gh_symbol2scm("any"))) {
	return -1;
    } else if (gh_eq_p(player, gh_symbol2scm("this"))) {
	return ThisPlayer->Player;
    }
    errl("bad player", player);

    return 0;
}

/**
**	Get the unit-type.
**
**	@param unit	The unit type.
**
**	@return		The unit-type pointer.
*/
local const UnitType* TriggerGetUnitType(SCM unit)
{
    if (gh_eq_p(unit, gh_symbol2scm("any"))) {
	return ANY_UNIT;
    } else if (gh_eq_p(unit, gh_symbol2scm("all"))) {
	return ALL_UNITS;
    } else if (gh_eq_p(unit, gh_symbol2scm("units"))) {
	return ALL_FOODUNITS;
    } else if (gh_eq_p(unit, gh_symbol2scm("buildings"))) {
	return ALL_BUILDINGS;
    }

    return CclGetUnitType(unit);
}

// --------------------------------------------------------------------------
//	Conditions

local int CompareEq(int a,int b)
{
    return a==b;
}
local int CompareNEq(int a,int b)
{
    return a!=b;
}
local int CompareGrEq(int a,int b)
{
    return a>=b;
}
local int CompareGr(int a,int b)
{
    return a>b;
}
local int CompareLeEq(int a,int b)
{
    return a<=b;
}
local int CompareLe(int a,int b)
{
    return a<b;
}

typedef int (*CompareFunction)(int,int);

/**
**	Returns a function pointer to the comparison function
**
**	@param op	The operation
**
**	@return		Function pointer to the compare function
*/
local CompareFunction GetCompareFunction(const char* op)
{
    if( op[0]=='=' ) {
	if( (op[1]=='=' && op[2]=='\0') || (op[1]=='\0') ) {
	    return &CompareEq;
	}
    }
    else if( op[0]=='>' ) {
	if( op[1]=='=' && op[2]=='\0' ) {
	    return &CompareGrEq;
	}
	else if( op[1]=='\0' ) {
	    return &CompareGr;
	}
    }
    else if( op[0]=='<' ) {
	if( op[1]=='=' && op[2]=='\0' ) {
	    return &CompareLeEq;
	}
	else if( op[1]=='\0' ) {
	    return &CompareLe;
	}
    }
    else if( op[0]=='!' && op[1]=='=' && op[2]=='\0' ) {
	return &CompareNEq;
    }
    return NULL;
}

/**
**	Player has the quantity of unit-type.
*/
local SCM CclIfUnit(SCM player,SCM operation,SCM quantity,SCM unit)
{
    int plynr;
    int q;
    int pn;
    const UnitType* unittype;
    const char* op;
    CompareFunction Compare;

    plynr=TriggerGetPlayer(player);
    op=get_c_string(operation);
    q=gh_scm2int(quantity);
    unittype=TriggerGetUnitType(unit);

    Compare=GetCompareFunction(op);
    if( !Compare ) {
	fprintf(stderr,"Illegal comparison operation in if-unit: %s\n",op);
	Exit(1);
    }

    if( plynr==-1 ) {
	plynr=0;
	pn=PlayerMax;
    } else {
	pn=plynr+1;
    }

    if( unittype==ANY_UNIT ) {
	for( ; plynr<pn; ++plynr ) {
	    int j;

	    for( j=0; j<NumUnitTypes; ++j ) {
		if( Compare(Players[plynr].UnitTypesCount[j],q) ) {
		    return SCM_BOOL_T;
		}
	    }
	}
    } else if( unittype==ALL_UNITS ) {
	for( ; plynr<pn; ++plynr ) {
	    if( Compare(Players[plynr].TotalNumUnits,q) ) {
		return SCM_BOOL_T;
	    }
	}
    } else if( unittype==ALL_FOODUNITS ) {
	for( ; plynr<pn; ++plynr ) {
	    if( Compare(Players[plynr].NumFoodUnits,q) ) {
		return SCM_BOOL_T;
	    }
	}
    } else if( unittype==ALL_BUILDINGS ) {
	for( ; plynr<pn; ++plynr ) {
	    if( Compare(Players[plynr].NumBuildings,q) ) {
		return SCM_BOOL_T;
	    }
	}
    } else {
	for( ; plynr<pn; ++plynr ) {
	    DebugLevel3Fn("Player%d, %d == %s\n",plynr,q,unittype->Ident);
	    if( Compare(Players[plynr].UnitTypesCount[unittype->Type],q) ) {
		return SCM_BOOL_T;
	    }
	}
    }

    return SCM_BOOL_F;
}

/**
**	Player has the quantity of unit-type near to unit-type.
*/
local SCM CclIfNearUnit(SCM player,SCM operation,SCM quantity,SCM unit,
                        SCM nearunit)
{
    int plynr;
    int q;
    int n;
    int i;
    const UnitType* unittype;
    const UnitType* ut2;
    const char* op;
    Unit* table[UnitMax];
    CompareFunction Compare;

    plynr=TriggerGetPlayer(player);
    op=get_c_string(operation);
    q=gh_scm2int(quantity);
    unittype=TriggerGetUnitType(unit);
    ut2=CclGetUnitType(nearunit);

    Compare=GetCompareFunction(op);
    if( !Compare ) {
	fprintf(stderr,"Illegal comparison operation in if-near-unit: %s\n",op);
	Exit(1);
    }

    //
    //	Get all unit types 'near'.
    //
    n=FindUnitsByType(ut2,table);
    DebugLevel3Fn("%s: %d\n",ut2->Ident,n);
    for( i=0; i<n; ++i ) {
	Unit* unit;
	Unit* around[UnitMax];
	int an;
	int j;
	int s;

	unit=table[i];

#ifdef UNIT_ON_MAP
	// FIXME: could be done faster?
#endif
	// FIXME: I hope SelectUnits checks bounds?
	// FIXME: Yes, but caller should check.
	// NOTE: +1 right,bottom isn't inclusive :(
	if( unit->Type->UnitType==UnitTypeLand ) {
	    an=SelectUnits(
		unit->X-1,unit->Y-1,
		unit->X+unit->Type->TileWidth+1,
		unit->Y+unit->Type->TileHeight+1,around);
	} else {
	    an=SelectUnits(
		unit->X-2,unit->Y-2,
		unit->X+unit->Type->TileWidth+2,
		unit->Y+unit->Type->TileHeight+2,around);
	}
	DebugLevel3Fn("Units around %d: %d\n",UnitNumber(unit),an);
	//
	//	Count the requested units
	//
	for( j=s=0; j<an; ++j ) {
	    unit=around[j];
	    //
	    //	Check unit type
	    //
	    if( (unittype==ANY_UNIT && unittype==ALL_UNITS)
		    || (unittype==ALL_FOODUNITS && !unit->Type->Building)
		    || (unittype==ALL_BUILDINGS && unit->Type->Building)
		    || (unittype==unit->Type) ) {
		//
		//	Check the player
		//
		if( plynr==-1 || plynr==unit->Player->Player ) {
		    ++s;
		}
	    }
	}
	if( Compare(s,q) ) {
	    return SCM_BOOL_T;
	}
    }

    return SCM_BOOL_F;
}

/**
**	Player has the quantity of rescued unit-type near to unit-type.
*/
local SCM CclIfRescuedNearUnit(SCM player,SCM operation,SCM quantity,SCM unit,
                               SCM nearunit)
{
    int plynr;
    int q;
    int n;
    int i;
    const UnitType* unittype;
    const UnitType* ut2;
    const char* op;
    Unit* table[UnitMax];
    CompareFunction Compare;

    plynr=TriggerGetPlayer(player);
    op=get_c_string(operation);
    q=gh_scm2int(quantity);
    unittype=TriggerGetUnitType(unit);
    ut2=CclGetUnitType(nearunit);

    Compare=GetCompareFunction(op);
    if( !Compare ) {
	fprintf(stderr,"Illegal comparison operation in if-rescued-near-unit: %s\n",op);
	Exit(1);
    }

    //
    //	Get all unit types 'near'.
    //
    n=FindUnitsByType(ut2,table);
    DebugLevel3Fn("%s: %d\n",ut2->Ident,n);
    for( i=0; i<n; ++i ) {
	Unit* unit;
	Unit* around[UnitMax];
	int an;
	int j;
	int s;

	unit=table[i];

#ifdef UNIT_ON_MAP
	// FIXME: could be done faster?
#endif
	// FIXME: I hope SelectUnits checks bounds?
	// FIXME: Yes, but caller should check.
	// NOTE: +1 right,bottom isn't inclusive :(
	if( unit->Type->UnitType==UnitTypeLand ) {
	    an=SelectUnits(
		unit->X-1,unit->Y-1,
		unit->X+unit->Type->TileWidth+1,
		unit->Y+unit->Type->TileHeight+1,around);
	} else {
	    an=SelectUnits(
		unit->X-2,unit->Y-2,
		unit->X+unit->Type->TileWidth+2,
		unit->Y+unit->Type->TileHeight+2,around);
	}
	DebugLevel3Fn("Units around %d: %d\n",UnitNumber(unit),an);
	//
	//	Count the requested units
	//
	for( j=s=0; j<an; ++j ) {
	    unit=around[j];
	    if( unit->Rescued ) {	// only rescued units
		//
		//	Check unit type
		//
		if( (unittype==ANY_UNIT && unittype==ALL_UNITS)
			|| (unittype==ALL_FOODUNITS && !unit->Type->Building)
			|| (unittype==ALL_BUILDINGS && unit->Type->Building)
			|| (unittype==unit->Type) ) {
		    //
		    //	Check the player
		    //
		    if( plynr==-1 || plynr==unit->Player->Player ) {
			++s;
		    }
		}
	    }
	}
	if( Compare(s,q) ) {
	    return SCM_BOOL_T;
	}
    }

    return SCM_BOOL_F;
}

/**
**	Player has n opponents left.
*/
local SCM CclIfOpponents(SCM player,SCM operation,SCM quantity)
{
    int plynr;
    int q;
    int pn;
    int n;
    const char* op;
    CompareFunction Compare;

    plynr=TriggerGetPlayer(player);
    op=get_c_string(operation);
    q=gh_scm2int(quantity);

    Compare=GetCompareFunction(op);
    if( !Compare ) {
	fprintf(stderr,"Illegal comparison operation in if-opponents: %s\n",op);
	Exit(1);
    }

    if( plynr==-1 ) {
	plynr=0;
	pn=PlayerMax;
    } else {
	pn=plynr+1;
    }

    //
    //	Check the player opponents
    //
    for( n=0; plynr<pn; ++plynr ) {
	int i;

	for( i=0; i<PlayerMax; ++i ) {
	    //
	    //	This player is our enemy and has units left.
	    //
	    if( (Players[i].Enemy&(1<<plynr)) && Players[i].TotalNumUnits ) {
		++n;
	    }
	}
	DebugLevel3Fn("Opponents of %d = %d\n",plynr,n);
	if( Compare(n,q) ) {
	    return SCM_BOOL_T;
	}
    }

    return SCM_BOOL_F;
}

// --------------------------------------------------------------------------
//	Actions

/**
**	Action condition player wins.
*/
local SCM CclActionVictory(void)
{
    GameResult=GameVictory;
    GamePaused=1;
    GameRunning=0;
    return SCM_UNSPECIFIED;
}

/**
**	Action condition player lose.
*/
local SCM CclActionDefeat(void)
{
    GameResult=GameDefeat;
    GamePaused=1;
    GameRunning=0;
    return SCM_UNSPECIFIED;
}

/**
**	Action condition player draw.
*/
local SCM CclActionDraw(void)
{
    GameResult=GameDraw;
    GamePaused=1;
    GameRunning=0;
    return SCM_UNSPECIFIED;
}

/**
**	Add a trigger.
*/
local SCM CclAddTrigger(SCM condition,SCM action)
{
    SCM var;

    //
    //	Make a list of all triggers.
    //		A trigger is a pair of condition and action
    //
    var=gh_symbol2scm("*triggers*");
    setvar(var,cons(cons(condition,action),symbol_value(var,NIL)),NIL);

    return SCM_UNSPECIFIED;
}

/**
**	Check trigger each game cycle.
*/
global void TriggersEachCycle(void)
{
    SCM pair;
    SCM val;

    if( !Trigger ) {
	Trigger=symbol_value(gh_symbol2scm("*triggers*"),NIL);
    }

    if( !gh_null_p(Trigger) ) {		// Next trigger
	pair=gh_car(Trigger);
	Trigger=gh_cdr(Trigger);
	if( !gh_null_p(val=gh_apply(car(pair),NIL)) ) {
	    val=gh_apply(cdr(pair),NIL);
	    if( gh_null_p(val) ) {
		DebugLevel0Fn("FIXME: should remove trigger\n");
	    }
	}
    } else {
	Trigger=NULL;
    }
}

/**
**	Register CCL features for triggers.
*/
global void TriggerCclRegister(void)
{
    gh_new_procedure2_0("add-trigger",CclAddTrigger);
    // Conditions
    gh_new_procedure4_0("if-unit",CclIfUnit);
    gh_new_procedure5_0("if-near-unit",CclIfNearUnit);
    gh_new_procedure5_0("if-rescued-near-unit",CclIfRescuedNearUnit);
    gh_new_procedure3_0("if-opponents",CclIfOpponents);
    // Actions
    gh_new_procedure0_0("action-victory",CclActionVictory);
    gh_new_procedure0_0("action-defeat",CclActionDefeat);
    gh_new_procedure0_0("action-draw",CclActionDraw);

    gh_define("*triggers*",NIL);
}

/**
**	Save the trigger module.
*/
global void SaveTriggers(FILE* file)
{
    fprintf(file,"\n;;; -----------------------------------------\n");
    fprintf(file,";;; MODULE: trigger $Id$\n\n");
    fprintf(file,";;; FIXME: Save not written\n\n");
}

/**
**	Initialize the trigger module.
*/
global void InitTriggers(void)
{
    //
    //	Setup default triggers
    //
    if( gh_null_p(symbol_value(gh_symbol2scm("*triggers*"),NIL)) ) {
	DebugLevel0Fn("Default triggers\n");
	gh_apply(symbol_value(gh_symbol2scm("single-player-triggers"),NIL),NIL);
    }
}

/**
**	Clean up the trigger module.
*/
global void CleanTriggers(void)
{
    SCM var;

    DebugLevel0Fn("FIXME: Cleaning trigger not written\n");

    var=gh_symbol2scm("*triggers*");
    setvar(var,NIL,NIL);

    Trigger=NULL;
}

//@}