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

#if defined _respawn_include_cappoint
  #endinput
#endif
#define _respawn_include_cappoint

/**
 * Gets the index (0-7) of a capture point.
 *
 * @param cap_point       Entity index of a team_control_point.
 * @return                Point index (0-7).
 */
stock CapPoint_GetIndex(cap_point) {

  return GetEntProp(cap_point, Prop_Data, "m_iPointIndex");

}

/**
 * Finds a capture point by index.
 *
 * @param cap_index       Point index (0-7).
 * @return                Entity index of a team_control_point.
 */
stock CapPoint_FindByIndex(cap_index) {

  new cap_point = -1;
  while ((cap_point = FindEntityByClassname(cap_point, "team_control_point")) != -1) {
    if (GetEntProp(cap_point, Prop_Data, "m_iPointIndex") == cap_index) {
      return cap_point;
    }
  }
  return -1;

}

/**
 * Sets the owner of a capture point.
 *
 * @param cap_point        Entity index of a team_control_point.
 * @param team             Team to set ownership to.
 * @noreturn
 */
stock CapPoint_SetOwner(cap_point, team) {

  SetVariantInt(team);
  AcceptEntityInput(cap_point, "SetOwner", -1, FindEntityByClassname(-1, "team_control_point_master"));

  new cap_base = CapPoint_GetCapBase(cap_point);
  if (cap_base > MaxClients) {
    if (team > 0) {
      SetVariantInt(team - 1);
    } else {
      SetVariantInt(0);
    }
    AcceptEntityInput(cap_base, "Skin");
  }

}

/**
 * Locks a capture point.
 *
 * @param cap_point       Entity index of a team_control_point.
 */
stock CapPoint_Lock(cap_point) {

  SetVariantInt(1);
  AcceptEntityInput(cap_point, "SetLocked", -1, FindEntityByClassname(-1, "team_control_point_master"));

}

/**
 * Unlocks a capture point.
 *
 * @param cap_point       Entity index of a team_control_point.
 */
stock CapPoint_Unlock(cap_point) {

  SetVariantInt(0);
  AcceptEntityInput(cap_point, "SetLocked", -1, FindEntityByClassname(-1, "team_control_point_master"));

}

/**
 * Attempts to find the base of a capture point.
 *
 * Unfortunately, there is no guaranteed way of doing this without reading the outputs of the
 * associated trigger_capture_area, and even then, cap points may exist without a base at all.
 *
 * Never assume that this method will return a valid entity.
 *
 * @param cap_point       Entity index of a team_control_point.
 */
stock CapPoint_GetCapBase(cap_point) {

  new prop = -1;
  new best_guess = -1;
  new Float:distance = 0.0;
  new Float:last_distance = 0.0;
  decl String:model_name[128];

  while ((prop = FindEntityByClassname(prop, "prop_dynamic")) != -1) {

    GetEntPropString(prop, Prop_Data, "m_ModelName", model_name, sizeof(model_name));
    if (StrEqual(model_name, "models/props_gameplay/cap_point_base.mdl")) {            // TODO: Include miniature (doomsday) point model as well
      distance = Entity_GetDistance(prop, cap_point);
      if (best_guess < 0 || distance < last_distance) {
        best_guess = prop;
        last_distance = distance;
      }
    }

  }

  return best_guess;

}

/*
 * Creates a team-associated dispenser beam associated with this point.
 *
 * @param cap_point       Entity index of a team_control_point.
 * @param team            Team to associate the dispenser beam with.
 * @param start_enabled   True to enable the dispenser beam immediately.
 */
stock CapPoint_CreateDispenser(cap_point, team, start_enabled = false) {

  new point_index = CapPoint_GetIndex(cap_point);

  decl String:cap_dispenser_bounds_name[64];
  Format(cap_dispenser_bounds_name, sizeof(cap_dispenser_bounds_name), "cap%d_dispenser_bounds_team%d", point_index, team);

  new cap_dispenser_bounds = Entity_FindByName(cap_dispenser_bounds_name, "dispenser_touch_trigger");
  if (cap_dispenser_bounds > 0) {
    Log("%s already exists - refusing to re-create.", cap_dispenser_bounds_name);
    return;
  }

  Log("Creating %s", cap_dispenser_bounds_name);

  cap_dispenser_bounds = CreateEntityByName("dispenser_touch_trigger");

  DispatchKeyValue(cap_dispenser_bounds, "targetname", cap_dispenser_bounds_name);

  DispatchSpawn(cap_dispenser_bounds);
  ActivateEntity(cap_dispenser_bounds);

  SetEntityModel(cap_dispenser_bounds, "models/props_gameplay/cap_point_base.mdl");
  SetEntProp(cap_dispenser_bounds, Prop_Send, "m_nSolidType", 2);
  SetEntProp(cap_dispenser_bounds, Prop_Send, "m_fEffects", GetEntProp(cap_dispenser_bounds, Prop_Send, "m_fEffects") | 32);

  decl Float:position[3];
  Entity_GetAbsOrigin(cap_point, position);
  Entity_SetAbsOrigin(cap_dispenser_bounds, position);

  decl String:cap_point_name[64];
  Entity_GetName(cap_point, cap_point_name, sizeof(cap_point_name));
  SetVariantString(cap_point_name);
  AcceptEntityInput(cap_dispenser_bounds, "SetParent");

  new Float:min_bounds[3] = { -300.0, -300.0, -300.0 };
  new Float:max_bounds[3] = { 300.0, 300.0, 300.0 };
  Entity_SetMinSize(cap_dispenser_bounds, min_bounds);
  Entity_SetMaxSize(cap_dispenser_bounds, max_bounds);

  decl String:cap_dispenser_name[64];
  Format(cap_dispenser_name, sizeof(cap_dispenser_name), "cap%d_dispenser_team%d", point_index, team);

  new cap_dispenser = Entity_FindByName(cap_dispenser_name, "dispenser_touch_trigger");
  if (cap_dispenser > 0) {
    Log("%s already exists - refusing to re-create.", cap_dispenser_name);
    return;
  }

  Log("Creating %s", cap_dispenser_name);

  cap_dispenser = CreateEntityByName("mapobj_cart_dispenser");

  DispatchKeyValue(cap_dispenser, "targetname", cap_dispenser_name);
  DispatchKeyValue(cap_dispenser, "touch_trigger", cap_dispenser_bounds_name);
  DispatchKeyValue(cap_dispenser, "spawnflags", "12");
  if (team == _:TFTeam_Red) {
    DispatchKeyValue(cap_dispenser, "TeamNum", "2");
  } else if (team == _:TFTeam_Blue) {
    DispatchKeyValue(cap_dispenser, "TeamNum", "3");
  } else {
    Log("Failed to create mapobj_cart_dispenser for team #%d", team);
    return;
  }

  DispatchSpawn(cap_dispenser);
  ActivateEntity(cap_dispenser);

  if (start_enabled) {
    AcceptEntityInput(cap_dispenser, "Enable");
  } else {
    AcceptEntityInput(cap_dispenser, "Disable");
  }

  Entity_SetAbsOrigin(cap_dispenser, position);
  SetVariantString(cap_point_name);
  AcceptEntityInput(cap_dispenser, "SetParent");

  // Get the trigger_capture_area to add outputs to.
  new cap_area = CapArea_FindByIndex(point_index);

  // Enable healbeam on capture.
  Entity_AddFormattedOutput(cap_area, "OnCapTeam%d %s:Enable::0:-1", team - 1, cap_dispenser_name);

  // Disable enemy healbeam on capture.
  Entity_AddFormattedOutput(cap_area, "OnCapTeam%d cap%d_dispenser_team%d:Disable::0:-1", team - 1, point_index, Team_EnemyTeam(team));

}
