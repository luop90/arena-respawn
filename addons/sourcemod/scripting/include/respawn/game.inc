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
    client_last_class[i] = TFClass_Unknown;
    client_is_marked[i] = false;
    client_on_point[i] = false;
    last_cap_time = GetGameTime();
  }

}

/**
 * Returns the number of control points in the map.
 *
 * @return                  The number of team_control_point entities present.
 */
stock Game_CountCapPoints() {

  return Game_CountEntities("team_control_point");

}

/**
 * Returns the number of entities of the given class present in the game.
 *
 * @param                   The classname to search for.
 * @return                  The number of the given entity type present.
 */
stock Game_CountEntities(const String:classname[]) {

  new count = 0;
  new ent = -1;
  while ((ent = FindEntityByClassname(ent, classname)) != -1) {
    count++;
  }
  return count;

}

/**
 * Returns the amount of time (in seconds) since a control point was last captured.
 *
 * @return                  Time since a control point was last captured.
 */
stock Game_TimeSinceLastCap() {

  return GetGameTime() - last_cap_time;

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

  new cap_area = -1;
  new cap_point;
  new point_index;

  while ((cap_area = FindEntityByClassname(cap_area, "trigger_capture_area")) != -1) {

    // Set the capture time for this cap_area.
    CapArea_SetCaptureTime(cap_area);

    // Find the exact position of the cap point.
    cap_point = CapArea_GetControlPoint(cap_area);
    decl Float:position[3];
    Entity_GetTrueAbsOrigin(cap_point, position);

    // Find or assign this point a name.
    new String:cap_point_name[64];
    Entity_GetName(cap_point, cap_point_name, sizeof(cap_point_name));

    point_index = CapPoint_GetIndex(cap_point);
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

      Entity_AddFormattedOutput(cap_point, "OnCapTeam%d %s:Stop::0:-1", team - 1, cap_effect_name);
      Entity_AddFormattedOutput(cap_point, "OnCapTeam%d %s:Start::0.1:-1", team - 1, cap_effect_name);

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

  // Special 5cp logic.
  if (Game_CountCapPoints() == 5) {
    Game_SetupCapPoints_FiveCp();
  }

}

/**
 * Sets up a team_round_timer tied to the map's team_control_point, which will reset after 50 seconds.
 * This allows double capping to force a win in case of a stalemate or hiding players.
 *
 * @noreturn
 */
stock Game_CreateCapTimer() {

  // Games without a defined midpoint do not need a timer.
  if (Game_CountCapPoints() > 1 && Game_CountCapPoints() < 5) {
    return;
  }

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
  DispatchKeyValue(timer, "start_paused", "0");
  DispatchKeyValue(timer, "auto_countdown", "0");

  if (Game_CountCapPoints() == 5) {
    DispatchKeyValue(timer, "timer_length", "31");
  } else {
    DispatchKeyValue(timer, "timer_length", "51");
  }

  DispatchSpawn(timer);

  new cap_point = -1;
  if (Game_CountCapPoints() == 5) {
    cap_point = CapPoint_FindByIndex(mid_index);
  } else {
    cap_point = FindEntityByClassname(cap_point, "team_control_point");
  }
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

    // In a 3cp game, the gamestate should reset alongside the midpoint.
    if (Game_CountCapPoints() == 5) {

      new blu_last = CapArea_FindByIndex(mid_index - 1);
      new red_last = CapArea_FindByIndex(mid_index + 1);

      decl String:blu_last_name[64];
      decl String:red_last_name[64];

      Entity_GetName(blu_last, blu_last_name, sizeof(blu_last_name));
      Entity_GetName(red_last, red_last_name, sizeof(red_last_name));

      Entity_AddFormattedOutput(timer, "OnFinished %s:Disable::0:-1", blu_last_name);
      Entity_AddFormattedOutput(timer, "OnFinished %s:Disable::0:-1", red_last_name);

      Entity_AddFormattedOutput(timer, "OnFinished %s:SetTeamCanCap:2 0:0:-1", blu_last_name);
      Entity_AddFormattedOutput(timer, "OnFinished %s:SetTeamCanCap:3 0:0:-1", red_last_name);

    }

  } else {
    Log("Could not find a tf_gamerules entity!");
  }

}

/*
 * Makes all spawn doors on the map untriggerable.
 *
 * @noreturn
 */
stock Game_LockSpawnDoors() {

  new filter = -1;
  while ((filter = FindEntityByClassname(filter, "filter_activator_tfteam")) != -1) {
    Entity_SetKeyValue(filter, "TeamNum", 0);
    Entity_SetKeyValue(filter, "negated", 0);
  }

}

/*
 * Opens all standard spawn doors.
 * Makes all non-standard spawn doors on the map triggerable by both teams.
 *
 * @noreturn
 */
stock Game_UnlockSpawnDoors() {

  new filter = -1;
  while ((filter = FindEntityByClassname(filter, "filter_activator_tfteam")) != -1) {
    Entity_SetKeyValue(filter, "TeamNum", 0);
    Entity_SetKeyValue(filter, "negated", 1);
  }

  // Find trigger_multiples that are likely to be spawn door triggers, and func_doors that
  // are likely to be spawn doors.

  new trigger = -1;
  new door = -1;
  new prop = -1;
  new String:filter_name[64];

  while ((trigger = FindEntityByClassname(trigger, "trigger_multiple")) != -1) {

    GetEntPropString(trigger, Prop_Data, "m_iFilterName", filter_name, sizeof(filter_name));

    // If this trigger_multiple has a team filter, it is likely to be a spawn door trigger.
    if (!StrEqual(filter_name, "")) {

      // To verify, loop through the func_doors and see if any are contained within the trigger.
      door = -1;
      while ((door = FindEntityByClassname(door, "func_door")) != -1) {

        // If there is a func_door in the trigger, this is definitely a spawn door.
        if (Entity_Intersect(door, trigger)) {

          // Open the spawn door.
          AcceptEntityInput(door, "Open");

          // Disable the trigger_multiple to prevent it from closing the door.
          AcceptEntityInput(trigger, "Disable");

          // Search for any prop_dynamics also within the trigger bounds, and try to set an open animation.
          // If none exists, this is harmless.
          prop = -1;
          while ((prop = FindEntityByClassname(prop, "prop_dynamic")) != -1) {

            if (Entity_Intersect(prop, trigger)) {
              SetVariantString("open");
              AcceptEntityInput(prop, "SetAnimation");
            }

          }

        }

      }

    }

  }

}

/**
 * (Experimental) Set up spawns for 5cp play.
 *
 * @noreturn
 */
stock Game_SetupSpawns_FiveCp() {

  // Create an array to store spawn entities.
  new spawns[128] = { -1, ... };

  // Current entity in search.
  new spawn = -1;

  // Current spawn number.
  new spawn_counter = 0;

  // Store all spawn entity indexes in an array.
  while ((spawn = FindEntityByClassname(spawn, "info_player_teamspawn")) != -1) {
    spawns[spawn_counter] = spawn;
    spawn_counter++;
  }

  // Loop through the stored indexes.
  for (new i = 0; i < sizeof(spawns); i++) {

    spawn = spawns[i];

    // If spawn is -1, we've hit the end of the loop.
    if (spawn < 0) {
      break;
    }

    // Which team is this spawnpoint associated with?
    new team = GetEntProp(spawn, Prop_Data, "m_iInitialTeamNum");

    // Determine if this spawn is valid for this round of 5cp.
    decl String:associated_point[64];
    GetEntPropString(spawn, Prop_Data, "m_iszControlPointName", associated_point, sizeof(associated_point));
    decl String:point_name[8][64];
    for (new p = 0; p < 5; p++) {
      Entity_GetName(CapPoint_FindByIndex(p), point_name[p], 64);
    }
    if (mid_index == 2 && StrEqual(associated_point, point_name[2])
      || mid_index == 1 && (
        (team == _:TFTeam_Blue && StrEqual(associated_point, point_name[0]))
        || (team == _:TFTeam_Red && StrEqual(associated_point, point_name[1])))
      || mid_index == 3 && (
        (team == _:TFTeam_Red && StrEqual(associated_point, point_name[4]))
        || (team == _:TFTeam_Blue && StrEqual(associated_point, point_name[3])))) {

      // Create a new spawnpoint to replace the old spawnpoint.
      new new_spawn = CreateEntityByName("info_player_teamspawn");

      // Set the new spawnpoint to the position of the old spawnpoint.
      decl Float:pos[3];
      Entity_GetAbsOrigin(spawn, pos);
      Entity_SetAbsOrigin(new_spawn, pos);

      // Rotate the new spawnpoint to match the old spawnpoint.
      decl Float:angles[3];
      Entity_GetAbsAngles(spawn, angles);
      Entity_SetAbsAngles(new_spawn, angles);

      // Associate the new spawnpoint with the same team as the old spawnpoint.
      decl String:str[16];
      Format(str, sizeof(str), "%d", team);
      DispatchKeyValue(new_spawn, "TeamNum", str);

      // Spawn the new info_player_teamspawn.
      DispatchSpawn(new_spawn);
      ActivateEntity(new_spawn);

    }

    // Kill the original spawn point.
    AcceptEntityInput(spawn, "Kill");

  }

}

/**
 * (Experimental) Set up capture points for 5cp play.
 *
 * @noreturn
 */
stock Game_SetupCapPoints_FiveCp() {

  new cap_area = -1;
  new cap_point;
  new point_index;

  while ((cap_area = FindEntityByClassname(cap_area, "trigger_capture_area")) != -1) {
    cap_point = CapArea_GetControlPoint(cap_area);
    point_index = CapPoint_GetIndex(cap_point);

    if (point_index == mid_index) {
      CapArea_Unlock(cap_area);
    } else if (point_index == mid_index + 1) {
      CapPoint_SetOwner(cap_point, _:TFTeam_Red);
      //CapPoint_CreateDispenser(cap_point, _:TFTeam_Red, true);
      CapArea_Lock(cap_area);
    } else if (point_index == mid_index - 1) {
      CapPoint_SetOwner(cap_point, _:TFTeam_Blue);
      //CapPoint_CreateDispenser(cap_point, _:TFTeam_Blue, true);
      CapArea_Lock(cap_area);
    } else if (point_index > mid_index + 1) {
      //CapPoint_SetOwner(cap_point, _:TFTeam_Red);
      CapArea_Lock(cap_area);
      AcceptEntityInput(cap_point, "HideModel");
    } else if (point_index < mid_index - 1) {
      //CapPoint_SetOwner(cap_point, _:TFTeam_Blue);
      CapArea_Lock(cap_area);
      AcceptEntityInput(cap_point, "HideModel");
    } else {
      CapArea_Lock(cap_area);
    }

    // 3cp point logic
    for (new team = _:TFTeam_Red; team <= _:TFTeam_Blue; team++) {

      // If this is mid, a successful capture by this team should open up the opposing team's last,
      // and lock the last of the capturing team.
      if (point_index == mid_index) {

        new dir = 0;
        if (team == _:TFTeam_Blue) {
          dir = 1;
        } else {
          dir = -1;
        }

        new enemy_last_index = mid_index + dir;
        new allied_last_index = mid_index - dir;

        decl String:enemy_last_cap_area_name[64];
        CapArea_GetName(CapArea_FindByIndex(enemy_last_index), enemy_last_cap_area_name, sizeof(enemy_last_cap_area_name));

        decl String:allied_last_cap_area_name[64];
        CapArea_GetName(CapArea_FindByIndex(allied_last_index), allied_last_cap_area_name, sizeof(allied_last_cap_area_name));

        // Lock our last point.
        Entity_AddFormattedOutput(cap_area, "OnCapTeam%d %s:SetTeamCanCap:%d 0:0:-1", team - 1, allied_last_cap_area_name, Team_EnemyTeam(team));
        Entity_AddFormattedOutput(cap_area, "OnCapTeam%d %s:Disable::0:-1", team - 1, allied_last_cap_area_name);

        // Unlock enemy's last point.
        Entity_AddFormattedOutput(cap_area, "OnCapTeam%d %s:SetTeamCanCap:%d 1:0:-1", team - 1, enemy_last_cap_area_name, team);
        Entity_AddFormattedOutput(cap_area, "OnCapTeam%d %s:Enable::0:-1", team - 1, enemy_last_cap_area_name);

      }

      // Get the control point master.
      decl String:cp_master_name[64];
      Entity_GetName(FindEntityByClassname(-1, "team_control_point_master"), cp_master_name, sizeof(cp_master_name));
      if (StrEqual(cp_master_name, "")) {
        cp_master_name = "team_control_point_master";
      }

      // If this is BLU last, a capture by RED should win the game.
      if (point_index == mid_index - 1 && team == _:TFTeam_Red) {
        Entity_AddFormattedOutput(cap_area, "OnCapTeam%d %s:SetWinner:%d:0:-1", team - 1, cp_master_name, team);
      }

      // If this is RED last, a capture by BLU should win the game.
      if (point_index == mid_index + 1 && team == _:TFTeam_Blue) {
        Entity_AddFormattedOutput(cap_area, "OnCapTeam%d %s:SetWinner:%d:0:-1", team - 1, cp_master_name, team);
      }

    }

  }

}
