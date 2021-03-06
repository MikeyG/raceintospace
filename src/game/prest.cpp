/*
    Copyright (C) 2005 Michael K. McCarty & Fritz Bronner

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/** \file prest.c Handles all the prestige related code.
 */

#include "prest.h"
#include "Buzz_inc.h"
#include "game_main.h"
#include "mc.h"
#include "pace.h"
#include "mis_c.h"
#include "mis_m.h"

char tYr, tMo, tIDX, bIDX;

void Set_Dock(char plr, char total);
void Set_LM(char plr, char total);
int Check_Photo(void);
int Check_Dock(int limit);
int PrestCheck(char plr);
char Did_Fail(void);
char PosGoal(char *PVal);
char NegGoal(char *PVal);
char SupGoal(char *PVal);
char PosGoal_Check(char *PVal);


void Set_Dock(char plr, char total)
{
    int i;

    for (i = 0; i < total; i++) {
        if (Mev[i].loc == 8 && Mev[i].StepInfo == 1) {
            Data->Prestige[Prestige_MannedDocking].Goal[plr]++;
            break;
        }
    }
}

void Set_LM(char plr, char total)
{
    int i;

    for (i = 0; i < total; i++) {
        if (Mev[i].loc == 26 && Mev[i].StepInfo == 1) {
            Data->P[plr].LMpts++;
        }
    }

    return;
}

int Check_Photo(void)
{
    int i;

    for (i = 0; i < STEPnum; i++) {
        if (Mev[i].loc == 20 && Mev[i].StepInfo == 1) {
            return 1;
        }

        if (Mev[i].loc == 22 && i == 4 && Mev[i].StepInfo == 1) {
            return 1;
        }

        if (Mev[i].loc == 20 && Mev[i].StepInfo > 1) {
            return 2;
        }
    }

    return 0;
}

int Check_Lab(void)
{
    int i;

    for (i = 0; i < STEPnum; i++) {
        if (Mev[i].loc == 28 && Mev[i].StepInfo == 1) {
            return 1;
        }
    }

    return 0;
}

int Check_Dock(int limit)
{
    int i;

    for (i = 0; i < STEPnum; i++) {
        if (Mev[i].loc == 8 && Mev[i].StepInfo == 0) {
            return 0;
        }

        if (Mev[i].loc == 8 && Mev[i].StepInfo <= limit) {
            return 2;
        }

        if (Mev[i].loc == 8 && Mev[i].StepInfo > limit) {
            return 1;
        }
    }

    return 0;
}

/**
 * Map prestige category list (mStr::PCat) to milestones.
 */
int
PrestMap(int val)
{
    switch (val) {
    case Prestige_OrbitalSatellite:
        return Milestone_OrbitalSatellite;

    case Prestige_MannedSpaceMission:
        return Milestone_ManInSpace;

    case Prestige_MannedOrbital:
        return Milestone_EarthOrbit;

    case Prestige_LunarFlyby:
        return Milestone_LunarFlyby;

    case Prestige_LunarProbeLanding:
        return Milestone_LunarPlanetary;

    case Prestige_MannedLunarPass:
        return Milestone_LunarPass;

    case Prestige_MannedLunarOrbit:
        return Milestone_LunarOrbit;

    case Prestige_MannedLunarLanding:
        return Milestone_LunarLanding;

    default:
        return -1;
    }
}

/**
 * Calculate mission penalty due to missed milestones and duration.
 *
 * \param plr current player
 * \return penalty
 *
 * \note Call only when Mis is valid
 */
char
PrestMin(char plr)
{
    int i, j, Neg = 0;

    Neg = 0;
    j = 0;

    if (Mis.Index == 0) {
        return 0;
    }

    for (i = 0; i < 5; i++) {
        j = MAX(j, PrestMap(Mis.PCat[i]));
    }

    /* walk all milestones lower than maximum required for mission */
    for (i = 0; i <= j; ++i) {
        /* if milestone not met, then add penalty */
        if (Data->Mile[plr][i] == 0) {
            Neg += 3;
        }
    }

    Neg = Neg + (plr ? Data->Def.Lev2 : Data->Def.Lev1) - 2;
    // Neg -= (2 - ((plr == 0) ? Data->Def.Lev1 : Data->Def.Lev2));
    Neg = MAX(Neg, 0);

    /* Index 2 = Manned suborbital
     * Index 4 = Manned orbital
     * Index 6 = Manned orbital EVA
     */
    if (Mis.Index != 2
        && Mis.Index != 4
        && Mis.Index != 6
        && (Mis.Days - Data->P[plr].DurationLevel) > 1) { // Raised this from "> 0" to disable broken Duration penalty system -Leon
        Neg += 5 * (Mis.Days - Data->P[plr].DurationLevel);
    }


    return Neg;
}


/** Returns the amount of prestige added
 *
 * \note Assumes that the Mis Structure is Loaded
 */
int PrestCheck(char plr)
{
    int i, total = 0;
    char prg, tm;

    prg = Mis.mEq;

    for (i = 0; i < 5; i++) { // Sum all first/second Nation Bonuses
        tm = Mis.PCat[i];

        if (tm != -1 && Data->Prestige[tm].Goal[plr] == 0) { // First Mission Bonus
            if (Data->Prestige[tm].Goal[other(plr)] == 0 && tm < 27) {
                total += Data->Prestige[tm].Add[0];    // you're first
            } else {
                total += Data->Prestige[tm].Add[1];    // you're second
            }
        }
    }

    if (Mis.Doc == 1 && Data->Prestige[Prestige_MannedDocking].Goal[plr] == 0) {
        if (Data->Prestige[Prestige_MannedDocking].Goal[other(plr)] == 0) {
            total += Data->Prestige[Prestige_MannedDocking].Add[0];    // you're first
        } else {
            total += Data->Prestige[Prestige_MannedDocking].Add[1];    // you're second
        }
    }

    if (Mis.EVA == 1 && Data->Prestige[Prestige_Spacewalk].Goal[plr] == 0) {
        if (Data->Prestige[Prestige_Spacewalk].Goal[other(plr)] == 0) {
            total += Data->Prestige[Prestige_Spacewalk].Add[0];    // you're first
        } else {
            total += Data->Prestige[Prestige_Spacewalk].Add[1];    // you're second
        }
    }

    if (Mis.Days > 1 && Data->P[plr].DurationLevel < Mis.Days) {
        if (Mis.Days == 6 && Data->Prestige[Prestige_Duration_Calc - Mis.Days].Goal[plr] == 0) {
            total += Data->Prestige[Prestige_Duration_Calc - Mis.Days].Add[0];
        } else if (Mis.Days == 5 && Data->Prestige[Prestige_Duration_Calc - Mis.Days].Goal[plr] == 0) {
            total += Data->Prestige[Prestige_Duration_Calc - Mis.Days].Add[0];
        } else if (Mis.Days == 4 && Data->Prestige[Prestige_Duration_Calc - Mis.Days].Goal[plr] == 0) {
            total += Data->Prestige[Prestige_Duration_Calc - Mis.Days].Add[0];
        } else if (Mis.Days == 3 && Data->Prestige[Prestige_Duration_Calc - Mis.Days].Goal[plr] == 0) {
            total += Data->Prestige[Prestige_Duration_Calc - Mis.Days].Add[0];
        } else if (Mis.Days == 2 && Data->Prestige[Prestige_Duration_Calc - Mis.Days].Goal[plr] == 0) {
            total += Data->Prestige[Prestige_Duration_Calc - Mis.Days].Add[0];
        }
    }

    // Hardware Checks
    if (Mis.Days > 1 && Data->Prestige[Prestige_Duration_B + prg].Goal[plr] == 0) {
        if (Data->Prestige[Prestige_Duration_B + prg].Goal[other(plr)] == 0) {
            total += Data->Prestige[Prestige_Duration_B + prg].Add[0];    // you're first
        } else {
            total += Data->Prestige[Prestige_Duration_B + prg].Add[1];    // you're second
        }
    }

    if (total != 0) {
        return total;
    }

    // Other mission bonus

    // Sum all additional Mission Bonuses
    for (i = 0; i < 5; i++)
        if (Mis.PCat[i] != -1) {
            total += Data->Prestige[Mis.PCat[i]].Add[2];
        }

    if (Mis.Doc == 1 && Data->Prestige[Prestige_MannedDocking].Goal[plr] == 0) {
        total += Data->Prestige[Prestige_MannedDocking].Add[2];
    }

    if (Mis.EVA == 1 && Data->Prestige[Prestige_Spacewalk].Goal[plr] == 0) {
        total += Data->Prestige[Prestige_Spacewalk].Add[2];
    }

    return total;
}

char HeroCheck(int which)
{
    switch (which) {
    case Prestige_MannedSpaceMission:
        return 0x01;// RECOVERY

    case Prestige_MannedOrbital:
        return 0x01;  // OIB

    case Prestige_MannedLunarPass:
        return 0x01;    // LIRA

    case Prestige_MannedLunarLanding:
        return 0x01;    // Prestige_MannedLunarLanding

    case Prestige_Spacewalk:
        return 0x02;    // E EVA

    case Prestige_LunarMoonwalk:
        return 0x02;    // L EVA

    default:
        return 0;
    }
}

/** Sets Goal Values and Sums into Data Structure
 * control should be always called with a value of 0
 * Successful Steps Only
 * Requires MEV to be packed
 */
char Set_Goal(char plr, char which, char control)
{
    char sum, pd, qt;

    sum = 0;

    if (control != 3) {
        if (MaxFail() > 1999) {
            return -1;
        }
    } else {
        control = 0;
    }

    if (control == 1 || which >= 0) { // Means successful to this part

        if (Data->Prestige[which].Place == -1) {
            switch (which) { // flag milestones
            case Prestige_OrbitalSatellite:
                isMile(plr, Milestone_OrbitalSatellite) = 1;
                break;

            case Prestige_MannedSpaceMission:
                isMile(plr, Milestone_ManInSpace) = 1;
                break;

            case Prestige_MannedOrbital:
                isMile(plr, Milestone_EarthOrbit) = 1;
                break;

            case Prestige_LunarFlyby:
                isMile(plr, Milestone_LunarFlyby) = 1;
                break;

            case Prestige_LunarProbeLanding:
                isMile(plr, Milestone_LunarPlanetary) = 1;
                break;

            case Prestige_MannedLunarPass:
                isMile(plr, Milestone_LunarPass) = 1;
                break;

            case Prestige_MannedLunarOrbit:
                isMile(plr, Milestone_LunarOrbit) = 1;
                break;

            case Prestige_MannedLunarLanding:
                isMile(plr, Milestone_LunarLanding) = 1;
                break;
            }

            if (control == 0) {
                Data->P[plr].MissionCatastrophicFailureOnTurn |= 4; // for astros


                if (MAIL == 0) {
                    pd = Mev[0].pad;
                    qt = Data->P[0].Udp[pd].Qty;
                    Data->P[0].Udp[pd].HInd = Data->P[0].PastMissionCount;
                    Data->P[0].Udp[pd].Poss[qt] = which;
                    Data->P[0].Udp[pd].PossVal[qt] = 0;
                    Data->P[0].Udp[pd].Mnth = tMo;
                    ++Data->P[0].Udp[pd].Qty;
                } else {
                    Data->Prestige[which].Place = plr;
                    Data->Prestige[which].Index = Data->P[plr].PastMissionCount;
                    Data->Prestige[which].Year = tYr;
                    Data->Prestige[which].Month = tMo;
                    Data->Prestige[which].Goal[plr]++;  // increment count
                    Data->Prestige[which].Points[plr] += Data->Prestige[which].Add[0];
                    sum += Data->Prestige[which].Add[0];
                }

                hero |= HeroCheck(which);
            } else if (control == 1) {
                switch (which) {
                case Prestige_Duration_B:
                case Prestige_Duration_C:
                case Prestige_Duration_D:
                case Prestige_Duration_E:
                case Prestige_Duration_F:
                    if (MAIL == 0) {
                        pd = Mev[0].pad;
                        qt = Data->P[0].Udp[pd].Qty;
                        Data->P[0].Udp[pd].HInd = Data->P[0].PastMissionCount;
                        Data->P[0].Udp[pd].Poss[qt] = which;
                        Data->P[0].Udp[pd].PossVal[qt] = 0;
                        Data->P[0].Udp[pd].Mnth = tMo;
                        ++Data->P[0].Udp[pd].Qty;
                    } else {
                        Data->Prestige[which].Place = plr;
                        Data->Prestige[which].Index = Data->P[plr].PastMissionCount;
                        Data->Prestige[which].Year = tYr;
                        Data->Prestige[which].Month = tMo;
                    }

                default:
                    break;
                }
            }
        } else if (Data->Prestige[which].mPlace == -1 && Data->Prestige[which].Place != plr) {
            Data->P[plr].MissionCatastrophicFailureOnTurn |= 4; // for astros


            Data->Prestige[which].mPlace = plr;

            switch (which) { // flag milestones
            case Prestige_OrbitalSatellite:
                isMile(plr, Milestone_OrbitalSatellite) = 1;
                break;

            case Prestige_MannedSpaceMission:
                isMile(plr, Milestone_ManInSpace) = 1;
                break;

            case Prestige_MannedOrbital:
                isMile(plr, Milestone_EarthOrbit) = 1;
                break;

            case Prestige_LunarFlyby:
                isMile(plr, Milestone_LunarFlyby) = 1;
                break;

            case Prestige_LunarProbeLanding:
                isMile(plr, Milestone_LunarPlanetary) = 1;
                break;

            case Prestige_MannedLunarPass:
                isMile(plr, Milestone_LunarPass) = 1;
                break;

            case Prestige_MannedLunarOrbit:
                isMile(plr, Milestone_LunarOrbit) = 1;
                break;

            case Prestige_MannedLunarLanding:
                isMile(plr, Milestone_LunarLanding) = 1;
                break;
            }

            if (control == 0) {
                Data->Prestige[which].Goal[plr]++;  // increment count
                sum += Data->Prestige[which].Add[1];
                Data->Prestige[which].Points[plr] += Data->Prestige[which].Add[1];

                hero |= HeroCheck(which);
            }
        } else if (sum < 3) { // Other
            if (control == 0) {
                Data->Prestige[which].Goal[plr]++;  // increment count
                sum += Data->Prestige[which].Add[2];
                Data->Prestige[which].Points[plr] += Data->Prestige[which].Add[2];
            }
        }
    }

    //----------------------------------------
    //Specs: Lunar Landing klugge (Duration D)
    //----------------------------------------
    if (which == Prestige_MannedLunarLanding || Data->Prestige[Prestige_MannedLunarLanding].Place == plr) {
        Data->P[plr].History[Data->P[plr].PastMissionCount].Duration = 4;
    }

    switch (which) {
    case Prestige_OrbitalSatellite:
        return(sum);

    case Prestige_MannedSpaceMission:
        return(sum);

    case Prestige_MannedOrbital:
        return(sum + Set_Goal(plr, Prestige_OrbitalSatellite, 1));

    case Prestige_LunarFlyby:
        return(sum + Set_Goal(plr, Prestige_MannedOrbital, 1));

    case Prestige_LunarProbeLanding:
        return(sum + Set_Goal(plr, Prestige_LunarFlyby, 1));

    case Prestige_MannedLunarPass:
        return(sum + Set_Goal(plr, Prestige_LunarProbeLanding, 1));

    case Prestige_MannedLunarOrbit:
        return(sum + Set_Goal(plr, Prestige_MannedLunarPass, 1));

    case Prestige_MannedLunarLanding:
        return(sum + Set_Goal(plr, Prestige_MannedLunarOrbit, 1));

    case Prestige_Duration_A:
        return(sum);

    case Prestige_Duration_B:
        return(sum);

    case Prestige_Duration_C:
        return(sum + Set_Goal(plr, Prestige_Duration_B, 1));

    case Prestige_Duration_D:
        return(sum + Set_Goal(plr, Prestige_Duration_C, 1));

    case Prestige_Duration_E:
        return(sum + Set_Goal(plr, Prestige_Duration_D, 1));

    case Prestige_Duration_F:
        return(sum + Set_Goal(plr, Prestige_Duration_E, 1));

    case Prestige_OnePerson:
        return(sum);

    case Prestige_TwoPerson:
        return(sum + Set_Goal(plr, Prestige_OnePerson, 1));

    case Prestige_ThreePerson:
        return(sum + Set_Goal(plr, Prestige_TwoPerson, 1));

    case Prestige_Minishuttle:
        return(sum + Set_Goal(plr, Prestige_ThreePerson, 1));

    case Prestige_FourPerson:
        return(sum + Set_Goal(plr, Prestige_Minishuttle, 1));

    case Prestige_MercuryFlyby:
    case Prestige_VenusFlyby:
    case Prestige_MarsFlyby:
    case Prestige_JupiterFlyby:
    case Prestige_SaturnFlyby:
    case Prestige_OrbitingLab:
    case Prestige_Spacewalk:
    case Prestige_MannedDocking:
    case Prestige_WomanInSpace:
        return (sum);

    default:
        return 0;
    }
}

/** Only sets negative for highest failed goal step
 *
 * checks if entire mission was a failure
 */
char Did_Fail(void)
{
    char i, bra;
    unsigned int fail;

    fail = 0;
    bra = 0;
    i = 0;

    while (i != 0x7f) {
        if (Mev[i].StepInfo >= 5000) {
            fail += 1000;
        }

        if (Mev[i].StepInfo >= 4000 && Mev[i].StepInfo <= 4999) {
            fail += 1000;
        }

        if (Mev[i].StepInfo >= 3000 && Mev[i].StepInfo <= 3999) {
            fail += 100;
        }

        if (Mev[i].StepInfo >= 2000 && Mev[i].StepInfo <= 2999) {
            fail += 10;
        }

        if (Mev[i].StepInfo >= 1000 && Mev[i].StepInfo <= 1999) {
            fail += 1;
        }

        if (Mev[i].trace != (i + 1)) {
            bra++;
        }

        i = Mev[i].trace;
    }

    if (fail < 90 && bra == 0) {
        return 1;
    } else {
        return -1;
    }
}

int MaxFail(void)
{
    int i = 0, t = 0, bra = 0, count = 0;

    while (i != 0x7f && count < 54) {
        if (Mev[i].StepInfo == 0) {
            Mev[i].StepInfo = 1003;
        }

        t = MAX(Mev[i].StepInfo, t);

        if (Mev[i].trace != (i + 1)) {
            bra++;
        }

        i = Mev[i].trace;
        ++count;
    }

    if (count >= 54) {
        return 1;
    } else {
        return t;
    }
}

#define PSTS(a)  (PVal[a]==1 || PVal[a]==2)
#define NSTS(a)  (PVal[a]==4)
#define SSTS(a)  (PVal[a]==3)
#define STSp(a)  (PVal[a]==1 || PVal[a]==2)
#define STSn(a)  (PVal[a]==4)

char PosGoal(char *PVal)
{
    if (PSTS(Prestige_MannedLunarLanding)) {
        return Prestige_MannedLunarLanding;
    } else if (PSTS(Prestige_MannedLunarOrbit)) {
        return Prestige_MannedLunarOrbit;
    } else if (PSTS(Prestige_MannedLunarPass)) {
        return Prestige_MannedLunarPass;
    } else if (PSTS(Prestige_MannedOrbital)) {
        return Prestige_MannedOrbital;
    } else if (PSTS(Prestige_MannedSpaceMission)) {
        return Prestige_MannedSpaceMission;
    } else {
        return -1;
    }
}

char NegGoal(char *PVal)
{
    if (NSTS(Prestige_MannedLunarLanding)) {
        return Prestige_MannedLunarLanding;
    } else if (NSTS(Prestige_MannedLunarOrbit)) {
        return Prestige_MannedLunarOrbit;
    } else if (NSTS(Prestige_MannedLunarPass)) {
        return Prestige_MannedLunarPass;
    } else if (NSTS(Prestige_MannedOrbital)) {
        return Prestige_MannedOrbital;
    } else if (NSTS(Prestige_MannedSpaceMission)) {
        return Prestige_MannedSpaceMission;
    } else {
        return -1;
    }
}

char SupGoal(char *PVal)
{
    if (SSTS(Prestige_MannedLunarLanding)) {
        return Prestige_MannedLunarLanding;
    } else if (SSTS(Prestige_MannedLunarOrbit)) {
        return Prestige_MannedLunarOrbit;
    } else if (SSTS(Prestige_MannedLunarPass)) {
        return Prestige_MannedLunarPass;
    } else if (SSTS(Prestige_MannedOrbital)) {
        return Prestige_MannedOrbital;
    } else if (SSTS(Prestige_MannedSpaceMission)) {
        return Prestige_MannedSpaceMission;
    } else {
        return -1;
    }
}

int PrestNeg(char plr, int i)
{
    int negs = 0;

    negs = Data->Prestige[i].Add[3];
    Data->Prestige[i].Goal[plr]++;
    Data->Prestige[i].Points[plr] += Data->Prestige[i].Add[3];

    return negs;
}


int AllotPrest(char plr, char mis)
{
    int i, total, other, negs, mcode, mike, P_Goal, N_Goal, S_Goal, ival, cval;
    char PVal[28];

    hero = 0;
    tMo = Data->P[plr].Mission[mis].Month;
    tYr = Data->Year;
    tIDX = bIDX = 0;
    memset(PVal, 0x00, sizeof PVal);

    // SETUP INFO
    mcode = Data->P[plr].Mission[mis].MissionCode;

    GetMisType(mcode);

    other = MaxFail();

    for (i = 0; i < STEPnum; i++) {
        if (Mev[i].PComp == 5 && Mev[i].StepInfo == 0) {
            Mev[i].PComp = 0;
            Mev[i].Prest = -100;
        }

        if ((MANNED[0] + MANNED[1]) > 0) {
            if (other >= 3000) {
                Mev[i].PComp = 4;
            } else if (Mev[i].Prest >= -28 && Mev[i].StepInfo > 2999) {
                Mev[i].PComp = 4;
            }
        }
    }

    // FEMALE 'NAUTS
    PVal[Prestige_WomanInSpace] = (MA[0][0].A != NULL && MA[0][0].A->Sex)
                                  || (MA[0][1].A != NULL && MA[0][1].A->Sex)
                                  || (MA[0][2].A != NULL && MA[0][2].A->Sex)
                                  || (MA[0][3].A != NULL && MA[0][3].A->Sex)
                                  || (MA[1][0].A != NULL && MA[1][0].A->Sex)
                                  || (MA[1][1].A != NULL && MA[1][1].A->Sex)
                                  || (MA[1][2].A != NULL && MA[1][2].A->Sex)
                                  || (MA[1][3].A != NULL && MA[1][3].A->Sex);

    for (i = 0; i < STEPnum; i++) {
        ival = abs(Mev[i].Prest);
        cval = Mev[i].PComp;

        // ival of 100 seems to mean "don't record this in PVal[]"
        // Regardless of intention, it's out of bounds, so don't access or overwrite it
        if (ival != 100) {
            if (Mev[i].StepInfo == 0 && PVal[ival] == 0) {
                cval = 4;
            }

            if (PVal[ival] != 4) {
                PVal[ival] = cval;
            }
        }
    }

    // EVA FIX FOR ALTERNATE STEPS LATER IN MISSION
    if (Mis.EVA == 1 && (PVal[Prestige_Spacewalk] == 0 || PVal[Prestige_Spacewalk] == 5)) {
        PVal[Prestige_Spacewalk] = 4;
    } else if (Mis.EVA == 0 && PVal[Prestige_Spacewalk] == 5) {
        PVal[Prestige_Spacewalk] = 0;
    }

    // DOCKING FIX FOR ALTERNATE STEPS LATER IN SESSION
    if (Mis.Doc == 1 && (PVal[Prestige_MannedDocking] == 0 || PVal[Prestige_MannedDocking] == 5)) {
        PVal[Prestige_MannedSpaceMission] = 4;
    } else if (Mis.EVA == 0 && PVal[Prestige_MannedDocking] == 5) {
        PVal[Prestige_MannedDocking] = 0;
    }

    // CLEAR TOTAL VALUE
    total = 0;
    negs = 0;

    // PHOTO RECON
    if (PVal[Prestige_MannedLunarPass] > 0 && PVal[Prestige_MannedLunarPass] < 4) {
        Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety += 5;    // manned stuff gets 5
    }

    Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety = MIN(Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety, 99);

    if (death == 1) {
        for (i = 0; i < 28; i++) if (PVal[i] > 0 && PVal[i] < 4) {
                PVal[i] = 4;
            }
    }

    // GOAL FILTER: MANNED
    P_Goal = PosGoal(PVal);
    N_Goal = NegGoal(PVal);
    S_Goal = SupGoal(PVal);

    if (P_Goal == Prestige_MannedLunarLanding) { // make sure EVA was done
        if (!(PVal[Prestige_Spacewalk] >= 1 && PVal[Prestige_Spacewalk] <= 3)) {
            P_Goal = Prestige_MannedLunarOrbit;
            PVal[Prestige_MannedLunarLanding] = 0;
        }
    }

    if ((P_Goal == -1 && S_Goal == -1) && (PVal[Prestige_WomanInSpace] > 0)) {
        PVal[Prestige_WomanInSpace] = 4;
    }

    if (Check_Dock(500) == 2) {  // Success
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety += 10;
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety = MIN(Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety, Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].MaxSafety);
    } else if (Check_Dock(500) == 1) {
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety += 5;
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety = MIN(Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety, Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].MaxSafety);
    }

    if (STSp(Prestige_MannedSpaceMission) || STSn(Prestige_MannedSpaceMission)) {
        PVal[Prestige_MannedSpaceMission] = 0;    // Clear All Firsts/Negative Goals
    }

    if (STSp(Prestige_MannedOrbital) || STSn(Prestige_MannedOrbital)) {
        PVal[Prestige_MannedOrbital] = 0;
    }

    if (STSp(Prestige_MannedLunarPass) || STSn(Prestige_MannedLunarPass)) {
        PVal[Prestige_MannedLunarPass] = 0;
    }

    if (STSp(Prestige_MannedLunarOrbit) || STSn(Prestige_MannedLunarOrbit)) {
        PVal[Prestige_MannedLunarOrbit] = 0;
    }

    if (STSp(Prestige_MannedLunarLanding) || STSn(Prestige_MannedLunarLanding)) {
        PVal[Prestige_MannedLunarLanding] = 0;
    }

    // DURATION FIRSTS
    Data->P[plr].Mission[mis].Duration = MAX(Data->P[plr].Mission[mis].Duration, 1);

    if (!Mis.Dur) {
        switch (P_Goal) {
        case Prestige_MannedSpaceMission:
            mike = 7;
            Data->P[plr].Mission[mis].Duration = 1;
            break;

        case Prestige_MannedOrbital:
            mike = (Mis.Index <= 6) ? (Data->P[plr].Mission[mis].Duration = 1, 7) : (Data->P[plr].Mission[mis].Duration = 2, 12);
            break;

        case Prestige_MannedLunarPass:
            mike = 11;
            Data->P[plr].Mission[mis].Duration = 3;
            break;

        case Prestige_MannedLunarOrbit:
            mike = 10;
            Data->P[plr].Mission[mis].Duration = 4;
            break;

        case Prestige_MannedLunarLanding:
            mike = 10;
            Data->P[plr].Mission[mis].Duration = 4;
            break;

        default:
            mike = 0;
            break;
        }
    } else {
        mike = 14 - Data->P[plr].Mission[mis].Duration;
    }

    if (mike >= 8 && mike <= 12)
        if (P_Goal >= 18 || S_Goal >= 18) {
            PVal[mike] = 1;
        }

    //total+=(char) Set_Goal(plr,mike,0);

    // GOAL POSTIVE
    if (P_Goal != -1) {
        total = Set_Goal(plr, P_Goal, 0);

        if (P_Goal != 27) {
            total += Set_Goal(plr, 27, 0);
            PVal[Prestige_MannedSpaceMission] = 0;
        }

        //if (!(Data->Prestige[Prestige_MannedSpaceMission].Place==plr || Data->Prestige[Prestige_MannedSpaceMission].mPlace==plr))
        //    total+=Set_Goal(plr,27,0);
    }

    // GOAL NEGATIVE
    if (N_Goal != -1) {
        negs += PrestNeg(plr, N_Goal);
        PVal[N_Goal] = 0;
    }

    if (mcode == 32 || mcode == 36) {
        PVal[Prestige_OrbitingLab] = Check_Lab();
    }

    // TOTAL ALL MISSION FIRSTS
    for (i = 0; i < 28; i++)
        if (PVal[i] == 1 || (PVal[i] == 2 && other < 3000)) {
            total += Set_Goal(plr, i, 0);
        }

    //else if (PVal[i]==4) negs+=Set_Goal(plr,i,0);

    // CAPSULE FIRSTS   need to check for failure on capsule
    if ((P_Goal != -1 || S_Goal != -1) && other < 3000 && MANNED[0] > 0 && Data->P[plr].Mission[mis].Hard[Mission_Capsule] != -1) { // Hardware on first part
        total += Set_Goal(plr, 12 + Data->P[plr].Mission[mis].Prog, 0);
    }

    if ((P_Goal != -1 || S_Goal != -1) && other < 3000 && MANNED[1] > 0 && Data->P[plr].Mission[mis + 1].Hard[Mission_Capsule] != -1 &&
        Data->P[plr].Mission[mis + 1].part == 1) {
        total += Set_Goal(plr, 12 + Data->P[plr].Mission[mis + 1].Prog, 0);
    }

#define DNE(a,b) (Data->Prestige[b].Place==(a) || Data->Prestige[b].mPlace==(a))

    if (DNE(plr, Prestige_Duration_F)) {
        Data->P[plr].DurationLevel = 6;
    } else if (DNE(plr, Prestige_Duration_E)) {
        Data->P[plr].DurationLevel = 5;
    } else if (DNE(plr, Prestige_Duration_D)) {
        Data->P[plr].DurationLevel = 4;
    } else if (DNE(plr, Prestige_Duration_C)) {
        Data->P[plr].DurationLevel = 3;
    } else if (DNE(plr, Prestige_Duration_B)) {
        Data->P[plr].DurationLevel = 2;
    } else if (DNE(plr, Prestige_MannedSpaceMission)) {
        Data->P[plr].DurationLevel = 1;
    }

    // TOTAL ALL MISSION SUBSEQUENTS
    if (total == 0) {
        // SET SUBSEQUENT Goal
        if (S_Goal != -1 && other < 3000) {
            total = Set_Goal(plr, S_Goal, 0);
        }

        for (i = 0; i < 28; i++)
            if (PVal[i] == 1 || (PVal[i] == 2 && other < 3000)) {
                total += Set_Goal(plr, i, 0);
            } else if (PVal[i] == 3) {
                Set_Goal(plr, i, 0);
            }
    }

    // LM POINTS
    Set_LM(plr, STEPnum);

    if (mcode >= 48 && mcode <= 52 && other < 3000) {
        Set_LM(plr, STEPnum);
    }

    // ADD IN NEGATIVES AND RETURN MIN of -10
    total = ((total + negs) < -10) ? -10 : total + negs;

    if (!death && total == -10) {
        total = -7;
    }

    return total;
}


char PosGoal_Check(char *PVal)
{
    if (PSTS(Prestige_MannedLunarLanding)) {
        return 22;
    } else if (PSTS(Prestige_MannedLunarOrbit)) {
        return 20;
    } else if (PSTS(Prestige_MannedLunarPass)) {
        return 19;
    } else if (PSTS(Prestige_LunarProbeLanding)) {
        return 7;
    } else if (PSTS(Prestige_LunarFlyby)) {
        return 1;
    } else if (PSTS(Prestige_MannedOrbital)) {
        return 18;
    } else if (PSTS(Prestige_MannedSpaceMission)) {
        return 27;
    } else if (PSTS(Prestige_OrbitalSatellite)) {
        return 0;
    }

    else {
        return -1;
    }
}


int Find_MaxGoal(void)
{
    int i, ival, cval;
    char PVal[28];
    memset(PVal, 0x00, sizeof PVal);

    for (i = 0; i < STEPnum; i++) {
        ival = abs(Mev[i].Prest);
        cval = Mev[i].PComp;

        if (ival != 100) {
            PVal[ival] = cval;
        }
    }

    return PosGoal_Check(PVal);
}

int U_AllotPrest(char plr, char mis)
{
    int i = 0, total, other, negs, mcode, lun;
    char PVal[28];

    memset(PVal, 0x00, sizeof PVal);   // CLEAR TOTAL VALUE
    total = 0, negs = 0, lun = 0;
    tMo = Data->P[plr].Mission[mis].Month;
    tYr = Data->Year;
    tIDX = bIDX = 0;

    // SETUP INFO
    mcode = Data->P[plr].Mission[mis].MissionCode;
    GetMisType(mcode);

    lun = Check_Photo();

    other = MaxFail();

    if ((mcode >= 7 && mcode <= 13) || mcode == 1) { // Unmanned Probes
        switch (mcode) {
        case 1:
            i = 0;
            break; // O.S.

        case 7:
            i = 1;
            break; // L.F.B.

        case 8:
            i = 7;
            break; // L.P.L.

        case 9:
            i = 3;
            break;

        case 10:
            i = 4;
            break;

        case 11:
            i = 2;
            break;

        case 12:
            i = 5;
            break;

        case 13:
            i = 6;
            break;
        }


        if (other == 1) {
            if (mcode == 10 || mcode == 12 || mcode == 13) {
                return 0;
            }

            total = Set_Goal(plr, i, 0);
        } else {
            negs = PrestNeg(plr, i);
        }

        if (mcode == 7 || mcode == 8) {
            if (lun == 1) { // UNMANNED PHOTO RECON
                Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety += 5;
                Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety = MIN(Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety, 99);
            } // if
        } // if

        if (mcode == 8 && MaxFail() == 1) { // extra 10 for landing on Moon
            if (lun == 1) { // UNMANNED PHOTO RECON
                Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety += 10;
                Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety = MIN(Data->P[plr].Manned[MISC_HW_PHOTO_RECON].Safety, 99);
            } // if
        } // if

    } // if

    if (Check_Dock(2) == 2) {
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety += 10;
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety = MIN(Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety, Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].MaxSafety);
    } else if (Check_Dock(2) == 1) {
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety += 5;
        Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety = MIN(Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].Safety, Data->P[plr].Manned[MISC_HW_DOCKING_MODULE].MaxSafety);
    }

    return total + negs;
}

/* vim: set noet ts=4 sw=4 tw=77: */
