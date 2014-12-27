/**
 * Copyright © 2014 awk
 *
 * This file is part of ARENA: RESPAWN.
 *
 * ARENA: RESPAWN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ARENA: RESPAWN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with ARENA: RESPAWN.  If not, see <http://www.gnu.org/licenses/>.
 */

#if defined _respawn_include_class
  #endinput
#endif
#define _respawn_include_class

/**
 * Gets the numerical TF2 player class from a string.
 * Someday this should be replaced with something more translation-friendly, but
 * use this as a fallback if the proper translation file is not installed.
 *
 * @return class           TFClassType of the named class.
 */
stock TFClassType:Class_GetFromName(const String:classname[]) {

  if (StrContains(classname, "scout", false) > -1
      || StrContains(classname, "scoot", false) > -1
      || StrContains(classname, "boston", false) > -1) {
    return TFClass_Scout;

  } else if (StrContains(classname, "sniper", false) > -1
            || StrContains(classname, "longshoot", false) > -1
            || StrContains(classname, "australian", false) > -1
            || StrContains(classname, "new zealand", false) > -1) {
    return TFClass_Sniper;

  } else if (StrContains(classname, "soldier", false) > -1
             || StrContains(classname, "solly", false) > -1
             || StrContains(classname, "american", false) > -1
             || (StrContains(classname, "rocket", false) > -1 && StrContains(classname, "w/o rocket", false) < 0)) {
    return TFClass_Soldier;

  } else if (StrContains(classname, "demo", false) > -1
             || StrContains(classname, "scottish", false) > -1
             || StrContains(classname, "scotsm", false) > -1
             || StrContains(classname, "explo", false) > -1) {
    return TFClass_DemoMan;

  } else if (StrContains(classname, "medic", false) > -1
             || StrContains(classname, "german", false) > -1
             || StrContains(classname, "doctor", false) > -1
             || StrContains(classname, "heal", false) > -1) {
    return TFClass_Medic;

  } else if (StrContains(classname, "heavy", false) > -1
             || StrContains(classname, "hoovy", false) > -1
             || StrContains(classname, "russia", false) > -1) {
    return TFClass_Heavy;

  } else if (StrContains(classname, "pyro", false) > -1
             || StrContains(classname, "hudda", false) > -1
             || StrContains(classname, "fire", false) > -1) {
    return TFClass_Pyro;

  } else if (StrContains(classname, "spy", false) > -1
             || StrContains(classname, "french", false) > -1
             || StrContains(classname, "sphee", false) > -1
             || StrContains(classname, "spee", false) > -1
             || StrContains(classname, "crab", false) > -1) {
    return TFClass_Spy;

  } else if (StrContains(classname, "engi", false) > -1
             || StrContains(classname, "neer", false) > -1
             || StrContains(classname, "builder", false) > -1
             || StrContains(classname, "texas", false) > -1
             || StrContains(classname, "sentry", false) > -1) {
    return TFClass_Engineer;

  }

  return TFClass_Unknown;

}

/**
 * Returns the internal player class name used by TF2
 * (i.e. when using tf_tournament_classlimit).
 *
 * @noreturn
 */
stock Class_GetName(TFClassType:class, String:name[], len) {

  switch (class) {
    case TFClass_Scout: {
      strcopy(name, len, "scout");
    }
    case TFClass_Sniper: {
      strcopy(name, len, "sniper");
    }
    case TFClass_Soldier: {
      strcopy(name, len, "soldier");
    }
    case TFClass_DemoMan: {
      strcopy(name, len, "demoman");
    }
    case TFClass_Medic: {
      strcopy(name, len, "medic");
    }
    case TFClass_Heavy: {
      strcopy(name, len, "heavy");
    }
    case TFClass_Pyro: {
      strcopy(name, len, "pyro");
    }
    case TFClass_Spy: {
      strcopy(name, len, "spy");
    }
    case TFClass_Engineer: {
      strcopy(name, len, "engineer");
    }
    default: {
      strcopy(name, len, "");
    }
  }

}
