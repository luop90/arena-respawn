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

#if defined _respawn_include_respawn
  #endinput
#endif
#define _respawn_include_respawn

/**
 * Prints a formatted log message to the console.
 *
 * @noreturn
 */
public Log(const String:to_log[], any:...) {

  if (GetConVarInt(cvar_logging) != 1) {
    return;
  }

  new String:prefixed_string[512];
  new String:log_string[512];
  prefixed_string = "[Arena: Respawn] ";
  StrCat(prefixed_string, sizeof(prefixed_string), to_log);
  VFormat(log_string, sizeof(log_string), prefixed_string, 2);

  PrintToServer(log_string);

}

/**
 * Checks if Arena: Respawn is currently enabled.
 *
 * @noreturn
 */
stock Respawn_Enabled() {

  // If we are not in Arena mode, we should not be enabled.
  if (!GetConVarInt(cvar_arena)) {
    return false;

  // Otherwise, check ars_enable to find out.
  } else if (GetConVarInt(cvar_plugin_enabled) <= 0) {
    return false;

  } else {
    return true;

  }

}

/**
 * Resets per-round tracking variables.
 *
 * @noreturn
 */
stock Respawn_ResetRoundState() {

  cap_owner = 0;
  for (new i = 0; i <= MaxClients; i++) {
    client_last_class[i] = TFClass_Unknown;
    client_is_marked[i] = false;
    client_on_point[i] = false;
    last_cap_time = GetGameTime();
  }

}

/**
 * Modifies Control Point behavior for Arena: Respawn.
 * Changes the capture rate to the time indicated by the ars_cap_time convar, adds particle effects, and prevents round
 * win upon a single point capture.
 *
 * @noreturn
 */
stock Respawn_SetupCapPoints() {

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
    Respawn_SetupCapPoints_FiveCp();
  }

}

/**
 * Sets up a team_round_timer tied to the map's team_control_point, which will reset after 50 seconds.
 * This allows double capping to force a win in case of a stalemate or hiding players.
 *
 * @noreturn
 */
stock Respawn_CreateCapTimer() {

  // Games without a defined midpoint do not need a timer.
  if (Game_CountCapPoints() != 1) {
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

  if (cap_point < 0) return;

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
stock Respawn_LockSpawnDoors() {

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
stock Respawn_UnlockSpawnDoors() {

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
 * Decreases health pickups on non-Arena maps.
 *
 * @noreturn
 */
stock Respawn_SetupHealthKits() {

  decl String:map_name[64];
  GetCurrentMap(map_name, sizeof(map_name));

  new Float:red_spawn[3];
  new Float:blu_spawn[3];
  new Float:red_distance;
  new Float:blu_distance;
  new Float:point_distance;

  Game_AveragePlayerSpawn(_:TFTeam_Red, red_spawn);
  Game_AveragePlayerSpawn(_:TFTeam_Blue, blu_spawn);

  // If this is an Arena map, assume the mapper knew what they were doing and
  // leave it alone.
  if (strncmp(map_name, "arena_", 6) != 0) {

    // If a large healthkit is present, downgrade all healthkits by a factor of one.
    if (Game_CountEntities("item_healthkit_full") > 0) {

      Game_ReplaceEntities("item_healthkit_medium", "item_healthkit_small");
      Game_ReplaceEntities("item_healthkit_full", "item_healthkit_medium");

    // If there are too many medium healthkits, downgrade all healthkits in non-contested areas.
    } else if (Game_CountEntities("item_healthkit_medium") > 2) {

      new pack = -1;

      while ((pack = FindEntityByClassname(pack, "item_healthkit_medium")) != -1) {

        red_distance = Entity_GetDistanceOrigin(pack, red_spawn);
        blu_distance = Entity_GetDistanceOrigin(pack, blu_spawn);

        if (FloatAbs(red_distance - blu_distance) >= 512.0) {
          Entity_ReplaceWith(pack, "item_healthkit_small");
        }

      }

    }

  }

  // If we reach this stage and there are no medium healthkits, even if this is an Arena map,
  // search for small healthkits in contested areas and upgrade the one furthest from the point.
  if (Game_CountEntities("item_healthkit_medium", false) < 1) {

    // If there is exactly one healthkit, we can skip all the calculations and upgrade that.
    // If that healthkit is not in a balanced area, that's the map's fault!
    if (Game_CountEntities("item_healthkit_small", false) == 1) {

      Entity_ReplaceWith(FindEntityByClassname(-1, "item_healthkit_small"), "item_healthkit_medium");

    // Otherwise, find an appropriate pill to upgrade.
    } else {

      new pill = -1;
      new otherpill = -1;
      new pill_to_upgrade = -1;
      new control_point = CapPoint_FindByIndex(0);
      new bool:bad_pill = false;

      while ((pill = FindEntityByClassname(pill, "item_healthkit_small")) != -1) {

        red_distance = Entity_GetDistanceOrigin(pill, red_spawn);
        blu_distance = Entity_GetDistanceOrigin(pill, blu_spawn);

        // If this pill is equidistant from both spawns, it's a good candidate!
        if (FloatAbs(red_distance - blu_distance) < 512.0) {

          // However, if another pill exists that is the same distance away from the control
          // point as this one is, and which is ALSO equidistant from spawn, upgrading this pill
          // would make the map lopsided - so mark it as a bad location for an upgrade.
          otherpill = -1;
          bad_pill = false;
          point_distance = Entity_GetDistance(pill, control_point);
          while ((otherpill = FindEntityByClassname(otherpill, "item_healthkit_small")) != -1) {
            if (otherpill == pill
                || FloatAbs(Entity_GetDistanceOrigin(otherpill, red_spawn) -
                   Entity_GetDistanceOrigin(otherpill, blu_spawn)) >= 512.0) {
              continue;
            }
            if (FloatAbs(Entity_GetDistance(otherpill, control_point) - point_distance) < 256.0) {
              bad_pill = true;
              break;
            }
          }

          // If this is not a bad pill location, and it is further from the point than the
          // previous candidate, name this pill as the most likely candidate.
          if (!bad_pill && (pill_to_upgrade == -1
            || Entity_GetDistance(pill, control_point) > Entity_GetDistance(pill_to_upgrade, control_point))) {
            pill_to_upgrade = pill;
          }
        }

      }

      if (pill_to_upgrade > 0) {
        Entity_ReplaceWith(pill_to_upgrade, "item_healthkit_medium");
      }

    }

  }

}

/**
 * Checks if a tournament is ready to start, and starts the tournament if it is.
 *
 * @noreturn
 */
stock Respawn_CheckTournamentState() {

  // If we aren't in pre-tournament mode, we can never be ready to start.
  if (state != GameState_PreTournament) {
    return;
  }

  new bool:cancel = false;

  // Otherwise, check if both teams are ready and have banned a class.
  for (new i = 0; i < 2; i++) {
    if (!team_ready[i]) { cancel = true; }
    if (team_ban[i] == TFClass_Unknown) { cancel = true; }
  }

  if (!cancel) {
    cancel = !(Respawn_CheckTeamReadyState(_:TFTeam_Red) && Respawn_CheckTeamReadyState(_:TFTeam_Blue));
  }

  // Cancel the countdown if necessary.
  if (cancel) {
    // do stuff
    return;
  }

  // Otherwise, start tournament mode and auto-ready the teams.
  Respawn_StartTournamentRound();

}

/**
 * Begin an Arena: Respawn tournament.
 * Forces the game into tournament mode with both teams ready.
 *
 * @noreturn
 */
stock Respawn_StartTournamentRound() {

  // Put ourselves in tournament mode.
  state = GameState_Tournament;
  ServerCommand("exec sourcemod/respawn.tournament.cfg");

  // Ban the classes that are supposed to be banned.
  decl String:classname[16];
  for (new i = 0; i < 2; i++) {
    Class_GetName(team_ban[i], classname, sizeof(classname));
    ServerCommand("tf_tournament_classlimit_%s 0", classname);
  }

  ServerCommand("mp_tournament 1 ; mp_tournament_restart");

  CreateTimer(1.0, Timer_AnnounceLiveMatch);
  CreateTimer(1.0, Timer_ForceTeamsReady);

  // Begin recording (if configured to do so)
  if (GetConVarInt(cvar_autorecord) > 0) {
    decl String:timestamp[32];
    FormatTime(timestamp, sizeof(timestamp), "%F_%H-%M-%S");

    decl String:map_name[32];
    GetCurrentMap(map_name, sizeof(map_name));

    ServerCommand("tv_stoprecord ; tv_record ars_%s_%dv%d_%s",
      timestamp,
      Team_CountPlayers(_:TFTeam_Red),
      Team_CountPlayers(_:TFTeam_Blue),
      map_name);
  }

}

/**
 * Checks if a team is ready for a tournament.
 * Returns false if the team is in an invalid tournament state.
 *
 * @return                 True if the team is valid and ready, false if not.
 */
stock bool:Respawn_CheckTeamReadyState(team) {

  decl String:team_name[64];
  decl String:player_name[64];
  decl String:class_name[64];
  new String:team_color[] = "N";
  new TFClassType:class = TFClass_Unknown;
  new class_players[10] = { -1, ... };

  if (team == _:TFTeam_Red) {
    team_color = "R";
    GetConVarString(FindConVar("mp_tournament_redteamname"), team_name, sizeof(team_name));
  } else if (team == _:TFTeam_Blue) {
    team_color = "B";
    GetConVarString(FindConVar("mp_tournament_blueteamname"), team_name, sizeof(team_name));
  }

  // Ensure that:
  // This team has locked in their class ban;
  // No one is playing a banned class;
  // No one is playing multiple of the same class.
  for (new i = 1; i <= MaxClients; i++) {
    
    if (!IsValidClient(i) || GetClientTeam(i) != team) {
      continue;
    }

    class = TF2_GetPlayerClass(i);

    Class_GetName(class, class_name, sizeof(class_name));
    GetClientName(i, player_name, sizeof(player_name));

    if (team_ban[team - 2] == TFClass_Unknown) {
      
      Client_PrintToChatAll(true, "Team {%s}%s{N} is not ready - you must ban a class. Use {G}!banclass{N} to set your team's ban.",
        team_color, team_name);
      team_ready[team - 2] = false;
      return false;

    } else if (class == team_ban[team - 2] || class == team_ban[Team_EnemyTeam(team) - 2]) {

      Client_PrintToChatAll(true, "Team {%s}%s{N} is not ready - {G}%s{N} is playing a banned class (%s). Use {G}!class{N} to change.",
        team_color, team_name, player_name, class_name);
      team_ready[team - 2] = false;
      return false;

    } else if (class_players[class] != -1) {

      Client_PrintToChatAll(true, "Team {%s}%s{N} is not ready - multiple players are playing the same class (%s). Use {G}!class{N} to change.",
        team_color, team_name, class_name);
      team_ready[team - 2] = false;
      return false;

    }

    class_players[class] = i;

  }

  return true;

}

/**
 * (Experimental) Set up spawns for 5cp play.
 *
 * @noreturn
 */
stock Respawn_SetupSpawns_FiveCp() {

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
stock Respawn_SetupCapPoints_FiveCp() {

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

/**
 * Creates a team_round_timer responsible for keeping track of how long the flag has been held.
 * When the timer ends, force the flag to drop regardless of the player currently carrying it.
 *
 * @noreturn
 */
stock Respawn_SetupFlagTimer() {


  new timer = Entity_FindByName("flagtimer", "team_round_timer");
  if (timer > 0) {
    Log("flagtimer already exists; refusing to re-create.");
    return;
  }

  timer = CreateEntityByName("team_round_timer");
  if (timer < 0) {
    Log("Failed to create team_round_timer!");
    return;
  }

  DispatchKeyValue(timer, "targetname", "flagtimer");
  DispatchKeyValue(timer, "StartDisabled", "1");
  DispatchKeyValue(timer, "show_in_hud", "1");
  DispatchKeyValue(timer, "start_paused", "0");
  DispatchKeyValue(timer, "auto_countdown", "0");
  DispatchKeyValue(timer, "timer_length", "10");

  DispatchSpawn(timer);

  Entity_AddOutput(timer, "OnFinished respawnflag:ForceDrop::0:-1");

}

/**
 * Sets up an item team_flag at the coordinates given by an info_target with the targetname "flagpos".
 * If there is no info_target specified, attempt (poorly, and with great inaccuracy) to place a flag
 * at the midpoint of the map.
 *
 * info_targets should be preconfigured in Stripper: Source for this to function properly.
 *
 * @noreturn
 */
stock Respawn_SetupFlags() {

  if (!Respawn_Enabled()) return;

  if (FindEntityByClassname(-1, "item_teamflag") > 0) {
    Log("Flag found; refusing to spawn another one.");
    return;
  }

  // If this map has an info_target called 'flagpos', spawn a flag there.
  new bool:flag_spawned = false;
  new flag = -1;
  new flag_index = -1;
  new target = -1;
  decl Float:pos[3];
  decl Float:flag_pos[2][3];
  decl String:name[64];

  while ((target = FindEntityByClassname(target, "info_target")) != -1) {
    Entity_GetName(target, name, sizeof(name));
    if (StrEqual(name, "flagpos")) {

      Entity_GetTrueAbsOrigin(target, pos);

      Respawn_SpawnFlag(pos);

      flag_spawned = true;

    } else if (StrEqual(name, "team_flagpos")) {

      flag_index++;
      if (flag_index > 1) { Log("Found a flag out of expected bounds: %d", flag_index); continue; }
      Entity_GetTrueAbsOrigin(target, flag_pos[flag_index]);

      new healthkit = CreateEntityByName("item_healthkit_full");
      DispatchKeyValueVector(healthkit, "origin", flag_pos[flag_index]);
      DispatchSpawn(healthkit);

    }
  }

  // If no flag has spawned, attempt to spawn one in the middle of the map.

  if (flag_index < 1 || flag_spawned) return;

  new Float:mid_x = (flag_pos[0][0] + flag_pos[1][0]) / 2.0;
  new Float:mid_y = (flag_pos[0][1] + flag_pos[1][1]) / 2.0;

  Log("Flag midpoint: %f, %f", mid_x, mid_y);

  pos[0] = mid_x;
  pos[1] = mid_y;
  pos[2] = 16384.0;
  new Float:vec[3];
  vec[0] = mid_x;
  vec[1] = mid_y;
  vec[2] = -pos[2];

  TR_TraceRay(pos, vec, MASK_ALL, RayType_EndPoint);

  // FIRST HIT: The top of the skybox (or other surrounding brush).
  if (TR_DidHit()) {

    decl Float:hitpos[3];
    TR_GetEndPosition(hitpos);
    hitpos[2] = hitpos[2] - 1.0;

    TR_TraceRay(hitpos, vec, MASK_ALL, RayType_EndPoint);

    // SECOND HIT: The world under the skybox.
    if (TR_DidHit()) {

      TR_GetEndPosition(hitpos);

      flag = CreateEntityByName("item_teamflag");
      DispatchKeyValueVector(flag, "origin", hitpos);

      DispatchSpawn(flag);
      Log("Spawned item_teamflag at %f %f %f", hitpos[0], hitpos[1], hitpos[2]);

      return;

    }

  }

  Log("Did not find a place to spawn an item_teamflag.");

}

/**
 * Creates an item_teamflag with the following properties:
 * - Neutral skin upon spawn and drop
 * - Starts the flagtimer team_round_timer when picked up
 * - Drop/Return outputs hook the special OnFlagDrop and OnFlagReturn methods
 *
 * @noreturn
 */
stock Respawn_SpawnFlag(Float:pos[3]) {

  new logic = FindEntityByClassname(-1, "tf_logic_arena");

  new flag = CreateEntityByName("item_teamflag");
  DispatchKeyValueVector(flag, "origin", pos);

  DispatchKeyValue(flag, "targetname", "respawnflag");
  DispatchKeyValue(flag, "skin", "2");
  DispatchKeyValue(flag, "ReturnTime", "50");
  DispatchKeyValue(flag, "VisibleWhenDisabled", "1");

  Entity_AddOutput(flag, "OnPickupTeam1 !self:SetTeam:2:0:-1");
  Entity_AddOutput(flag, "OnPickupTeam1 !self:skin:0:0:-1");
  Entity_AddOutput(flag, "OnPickupTeam2 !self:SetTeam:3:0:-1");
  Entity_AddOutput(flag, "OnPickupTeam2 !self:skin:1:0:-1");

  Entity_AddOutput(flag, "OnPickup flagtimer:Enable::0:-1");
  Entity_AddOutput(flag, "OnPickup flagtimer:Restart::0:-1");
  Entity_AddOutput(flag, "OnPickup flagtimer:ShowInHUD:1:0:-1");

  Entity_AddOutput(flag, "OnDrop !self:skin:2:0:-1");
  Entity_AddOutput(flag, "OnDrop flagtimer:Disable::0:-1");
  Entity_AddOutput(flag, "OnDrop flagtimer:Restart::0:-1");
  Entity_AddOutput(flag, "OnDrop flagtimer:ShowInHUD:0:0:-1");

  Entity_AddOutput(flag, "OnReturn !self:SetTeam:0:0:-1");
  Entity_AddOutput(flag, "OnReturn !self:skin:2:0:-1");

  if (logic > -1) {
    DispatchKeyValue(flag, "StartDisabled", "1");
    Entity_AddOutput(logic, "OnCapEnabled respawnflag:Enable::0:-1");
  }

  HookSingleEntityOutput(flag, "OnDrop", OnFlagDrop);
  HookSingleEntityOutput(flag, "OnReturn", OnFlagReturn);

  DispatchSpawn(flag);

}
