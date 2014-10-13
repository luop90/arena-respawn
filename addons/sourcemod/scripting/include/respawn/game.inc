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

#if defined _respawn_include_game
  #endinput
#endif
#define _respawn_include_game

/**
 * Returns the number of in-game players.
 *
 * @return                  Number of players on RED and BLU.
 */
stock Game_CountActivePlayers() {
  new n = 0;
  for (new i = 1; i <= MaxClients; i++) {
    if (IsValidClient(i) && (GetClientTeam(i) == _:TFTeam_Red || GetClientTeam(i) == _:TFTeam_Blue)) {
      n++;
    }
  }
  return n;
}

/**
 * Returns the tf_objective_resource controlling HUD elements in the game.
 *
 * @return                 Index of the tf_objective_resource.
 * @error                  No resource exists.
 */
stock Game_GetObjective() {

  new cap_objective = FindEntityByClassname(-1, "tf_objective_resource");
  if (cap_objective < 0) {
    ThrowError("Could not find a tf_objective_resource!");
  }
  return cap_objective;

}

/**
 * Restocks the health and ammo of all players.
 *
 * @noreturn
 */
stock Game_RegeneratePlayers() {

  for (new i = 1; i <= MaxClients; i++) {

    if (IsValidClient(i) && (GetClientTeam(i) == _:TFTeam_Red || GetClientTeam(i) == _:TFTeam_Blue)) {
      TF2_RegeneratePlayer(i);
    }

  }

}

/**
 * Resets per-round tracking variables.
 *
 * @noreturn
 */
stock Game_ResetRoundState() {

  cap_owner = 0;
  for (new i = 0; i <= MaxClients; i++) {
    last_class[i] = TFClass_Unknown;
    point_last_touched[i] = -1;
  }

}

/**
 * Returns the number of control points in the map.
 *
 * @return                  The number of team_control_point entities present.
 */
stock Game_CountCapPoints() {

  new count = 0;
  new point = -1;
  while ((point = FindEntityByClassname(point, "team_control_point")) != -1) {
    count++;
  }
  return count;

}


/**
 * Modifies Control Point behavior for Arena: Respawn.
 * Changes the capture rate to the time indicated by the ars_cap_time convar, adds particle effects, and prevents round
 * win upon a single point capture.
 *
 * @noreturn
 */
stock Game_SetupCapPoints() {

  new cap_master = FindEntityByClassname(-1, "team_control_point_master");
  if (cap_master < 0) {
    Log("Could not find a valid team_control_point_master.");
    return;
  }

  // If there is only one control point, prevent either team from triggering a round win.
  if (Game_CountCapPoints() == 1) {
    SetEntProp(cap_master, Prop_Data, "m_iInvalidCapWinner", 1);
  } else {
    SetEntProp(cap_master, Prop_Data, "m_iInvalidCapWinner", 0);
  }

  // Retrieve the game objective resource to reflect HUD changes.
  new cap_objective = Game_GetObjective();

  new cap_area = -1;
  new cap_point;
  new point_index;

  while ((cap_area = FindEntityByClassname(cap_area, "trigger_capture_area")) != -1) {

    // Set point capture time based on the number of cap points.
    new Float:cap_time = GetConVarFloat(cvar_cap_time) * Game_CountCapPoints() * Game_CountCapPoints();
    Entity_SetKeyValueFloat(cap_area, "area_time_to_cap", cap_time);

    // Disable team capture multipliers.
    Entity_SetKeyValue(cap_area, "team_numcap_2", 1);
    Entity_SetKeyValue(cap_area, "team_numcap_3", 1);

    // Find the team_control_point associated with this trigger_capture_area.
    cap_point = CapArea_GetControlPoint(cap_area);

    // Which control point is this? (can be 0-7)
    point_index = GetEntProp(cap_point, Prop_Data, "m_iPointIndex");

    // Reflect the new capture time on clients' HUDs.
    SetEntPropFloat(cap_objective, Prop_Data, "m_flTeamCapTime", cap_time * 2, _:TFTeam_Red  * 8 + point_index);
    SetEntPropFloat(cap_objective, Prop_Data, "m_flTeamCapTime", cap_time * 2, _:TFTeam_Blue * 8 + point_index);

    // Find the exact position of the cap point.
    decl Float:position[3];
    Entity_GetTrueAbsOrigin(cap_point, position);

    // Find or assign this point a name.
    new String:cap_point_name[64];
    Entity_GetName(cap_point, cap_point_name, sizeof(cap_point_name));

    Log("Cap point targetname: %s", cap_point_name);

    if (StrEqual("", cap_point_name)) {
      Format(cap_point_name, sizeof(cap_point_name), "cap_point_%d", point_index);
      Entity_SetName(cap_point, cap_point_name);
    }

    // Add particle effects upon team capture.
    for (new team = _:TFTeam_Red; team <= _:TFTeam_Blue; team++) {

      decl String:cap_effect_name[64];
      Format(cap_effect_name, sizeof(cap_effect_name), "cap%d_effect_team%d", point_index, team);

      new cap_effect = Entity_FindByName(cap_effect_name, "info_particle_system");
      if (cap_effect > 0) {
        Log("%s already exists - refusing to re-create.", cap_effect_name);
        continue;
      }

      Log("Creating %s", cap_effect_name);

      cap_effect = CreateEntityByName("info_particle_system");

      DispatchKeyValue(cap_effect, "targetname", cap_effect_name);
      if (team == _:TFTeam_Red) {
        DispatchKeyValue(cap_effect, "effect_name", "powercore_embers_red");
      } else if (team == _:TFTeam_Blue) {
        DispatchKeyValue(cap_effect, "effect_name", "powercore_embers_blue");
      } else {
        Log("Failed to create capture point effect for team #%d", team);
        continue;
      }

      DispatchSpawn(cap_effect);
      ActivateEntity(cap_effect);

      // Attach to the team_control_point.
      Entity_SetAbsOrigin(cap_effect, position);
      SetVariantString(cap_point_name);
      AcceptEntityInput(cap_effect, "SetParent");

      // Fire the particle effect.
      AcceptEntityInput(cap_effect, "Start");

      // Set entity outputs to fire the effect on the associated team capture.
      decl String:input[64];

      Format(input, sizeof(input), "OnCapTeam%d %s:Stop::0:-1", team - 1, cap_effect_name);
      SetVariantString(input);
      AcceptEntityInput(cap_point, "AddOutput");

      Format(input, sizeof(input), "OnCapTeam%d %s:Start::0.1:-1", team - 1, cap_effect_name);
      SetVariantString(input);
      AcceptEntityInput(cap_point, "AddOutput");

      // If the game has two capture points, capturing one should reset the other.
      if (Game_CountCapPoints() == 2) {

        new other_point = -1;
        while ((other_point = FindEntityByClassname(other_point, "team_control_point")) != -1) {

          if (other_point == cap_point) {
            continue;
          }

          decl String:other_point_name[32];
          Entity_GetName(other_point, other_point_name, sizeof(other_point_name));

          Format(input, sizeof(input), "OnCapTeam%d %s:SetOwner:0:2:-1", team - 1, other_point_name);
          SetVariantString(input);
          AcceptEntityInput(cap_point, "AddOutput");

        }

      }

    }

  }

}

/**
 * Sets up a team_round_timer tied to the map's team_control_point, which will reset after 50 seconds.
 * This allows double capping to force a win in case of a stalemate or hiding players.
 *
 * @noreturn
 */
stock Game_CreateCapTimer() {

  new timer = Entity_FindByName("point_reset_timer", "team_round_timer");
  if (timer > 0) {
    Log("point_reset_timer already exists; refusing to re-create.");
    return;
  }

  timer = CreateEntityByName("team_round_timer");
  if (timer < 0) {
    Log("Failed to create team_round_timer!");
    return;
  }

  DispatchKeyValue(timer, "targetname", "point_reset_timer");
  DispatchKeyValue(timer, "StartDisabled", "1");
  DispatchKeyValue(timer, "show_in_hud", "1");
  DispatchKeyValue(timer, "timer_length", "51");
  DispatchKeyValue(timer, "start_paused", "0");
  DispatchKeyValue(timer, "auto_countdown", "0");

  DispatchSpawn(timer);

  new cap_point = FindEntityByClassname(-1, "team_control_point");
  new String:cap_point_name[64];
  Entity_GetName(cap_point, cap_point_name, sizeof(cap_point_name));

  // If this is the first capture, the timer needs to be enabled.
  SetVariantString("OnCapTeam1 point_reset_timer:Enable::0:-1");
  AcceptEntityInput(cap_point, "AddOutput");
  SetVariantString("OnCapTeam2 point_reset_timer:Enable::0:-1");
  AcceptEntityInput(cap_point, "AddOutput");

  // If this isn't the first capture, the timer needs to be restarted.
  SetVariantString("OnCapTeam1 point_reset_timer:Restart::0:-1");
  AcceptEntityInput(cap_point, "AddOutput");
  SetVariantString("OnCapTeam2 point_reset_timer:Restart::0:-1");
  AcceptEntityInput(cap_point, "AddOutput");

  // Force this timer to appear in the HUD.
  SetVariantString("OnCapTeam1 point_reset_timer:ShowInHUD:1:0:-1");
  AcceptEntityInput(cap_point, "AddOutput");
  SetVariantString("OnCapTeam2 point_reset_timer:ShowInHUD:1:0:-1");
  AcceptEntityInput(cap_point, "AddOutput");

  new String:output_string[64];
  Format(output_string, sizeof(output_string), "OnFinished %s:SetOwner:0:0:-1", cap_point_name);
  SetVariantString(output_string);
  AcceptEntityInput(timer, "AddOutput");

  new gamerules = FindEntityByClassname(-1, "tf_gamerules");
  if (gamerules > 0) {
    new String:gamerules_name[64];
    Entity_GetName(gamerules, gamerules_name, sizeof(gamerules_name));

    Format(output_string, sizeof(output_string), "OnFinished %s:PlayVO:Announcer.AM_CapEnabledRandom:0:-1", gamerules_name);
    SetVariantString(output_string);
    AcceptEntityInput(timer, "AddOutput");
  } else {
    Log("Could not find a tf_gamerules entity!");
  }

}

/**
 * (Experimental) Attempt to force a map into being an Arena: Respawn map.
 * 
 * @noreturn
 */
stock Game_ForceArenaMode() {

  new String:remove_entities[][] = {
    "team_round_timer",
    "func_regenerate",
    "tf_logic_koth",
    "func_respawnroomvisualizer"
  };

  for (new i = 0; i < sizeof(remove_entities); i++) {
    new entity = -1;
    while ((entity = FindEntityByClassname(entity, remove_entities[i])) != -1) {
      AcceptEntityInput(entity, "Disable");
      AcceptEntityInput(entity, "Kill");
    }
  }

  new door = -1;
  while ((door = FindEntityByClassname(door, "func_door")) != -1) {
    AcceptEntityInput(door, "Open");
  }

  new arena_logic = FindEntityByClassname(-1, "tf_logic_arena");
  if (arena_logic < 0) {
    arena_logic = CreateEntityByName("tf_logic_arena");
    ActivateEntity(arena_logic);
    DispatchSpawn(arena_logic);
    AcceptEntityInput(arena_logic, "Enable");
  }

  new cap_point = -1;
  while ((cap_point = FindEntityByClassname(cap_point, "team_control_point")) != -1) {
    SetVariantInt(0);
    AcceptEntityInput(cap_point, "SetOwner");
    SetVariantInt(0);
    AcceptEntityInput(cap_point, "SetLocked");
  }

}
