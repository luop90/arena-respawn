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

#if defined _respawn_include_timers
  #endinput
#endif
#define _respawn_include_timers

// Displays a welcome message.
public Action:Timer_Credits(Handle:timer, any:client) {

  if (!IsValidClient(client)) {
    return;
  }

  Client_PrintToChat(client, true, "This server is running {G}Arena: Respawn v%s{N} by {G}awk{N}.", PLUGIN_VERSION);
  Client_PrintToChat(client, true, "For news and updates, join the official steam group at {G}http://steam.respawn.tf/{N}.");

}

// Checks to see if a player should be marked for death, then does so.
public Action:Timer_Add_Mark(Handle:timer, any:client) {

  client_mark_timer[client] = INVALID_HANDLE;

  // If the player is no longer on the control point, disregard.
  if (!client_on_point[client] || client_is_marked[client] || !IsValidClient(client)) {
    return;
  }

  TF2_AddCondition(client, TFCond_MarkedForDeath);
  client_is_marked[client] = true;

}

// Removes a player's death mark.
public Action:Timer_Remove_Mark(Handle:timer, any:client) {

  client_mark_timer[client] = INVALID_HANDLE;

  // If the player is still on the control point, do not remove the mark.
  if (!client_is_marked[client] || client_on_point[client] || !IsValidClient(client)) {
    return;
  }

  TF2_RemoveCondition(client, TFCond_MarkedForDeath);
  client_is_marked[client] = false;

}

// Forces a player to respawn, even if they are already alive.
public Action:Timer_RespawnPlayer(Handle:timer, any:client) {

  if (IsValidClient(client)) {

    TF2_RespawnPlayer(client);
  }

}

// Regenerates a player.
public Action:Timer_RegeneratePlayer(Handle:timer, any:client) {

  if (!IsValidClient(client)) { return; }

  TF2_RegeneratePlayer(client);

}

// Forces a player to respawn, but only if they are dead.
public Action:Timer_DMRespawnPlayer(Handle:timer, any:client) {

  if (IsValidClient(client) && !IsPlayerAlive(client)) {
    TF2_RespawnPlayer(client);
  }
  dm_respawntimer[client] = INVALID_HANDLE;

}

// In pre-tournament mode, keep players alive.
public Action:Timer_RespawnDeadPlayers(Handle:timer) {

  if (!Respawn_Enabled()) return;

  if (state != GameState_PreTournament) {
    return;
  }

  for (new i = 1; i <= MaxClients; i++) {

    if (IsValidClient(i) && GetClientTeam(i) >= _:TFTeam_Red
      && !IsPlayerAlive(i) && dm_respawntimer[i] == INVALID_HANDLE) {
      dm_respawntimer[i] = CreateTimer(1.0, Timer_DMRespawnPlayer, i);
    }

  }

}

// Draws the A:R tournament state HUD.
public Action:Timer_DrawHUD(Handle:timer) {

  if (!Respawn_Enabled()) return;

  // If we're not in pre-tournament mode, don't draw a HUD.
  if (state != GameState_PreTournament) {
    return;
  }

  new red;
  new green;
  new blue;
  new Float:x;
  new Float:y;

  decl String:ready[16];
  decl String:ban[16];
  decl String:team_name[32];

  for (new team = _:TFTeam_Red; team <= _:TFTeam_Blue; team++) {

    team_name = "";
    if (team == _:TFTeam_Red) {
      x = 0.35;
      red = 255;
      green = 0;
      blue = 0;
      GetConVarString(FindConVar("mp_tournament_redteamname"), team_name, sizeof(team_name));
    } else if (team == _:TFTeam_Blue) {
      x = 0.55;
      red = 100;
      green = 100;
      blue = 255;
      GetConVarString(FindConVar("mp_tournament_blueteamname"), team_name, sizeof(team_name));
    }
    y = 0.1;

    SetHudTextParams(x, y, 2.01, red, green, blue, 255, 0, 0.0, 0.0, 0.0);

    // Prepare ready/not ready text
    if (team_ready[team - 2]) {
      ready = "Ready";
    } else {
      ready = "Not Ready";
    }

    // Prepare class ban text
    if (team_ban[team - 2] == TFClass_Unknown) {
      ban = "NONE";
    } else {
      Class_GetName(team_ban[team - 2], ban, sizeof(ban));
      ban[0] &= ~(1 << 5); // Capitalize the name of the banned class
    }

    // Display message to all clients.
    for (new i = 1; i <= MaxClients; i++) {
      if (!IsValidClient(i)) {
        continue;
      }
      ShowSyncHudText(i, hud[team - 2], "%s\n%s\nBan: %s", team_name, ready, ban);
    }

  }

  // Add "vs." in the center
  for (new i = 1; i <= MaxClients; i++) {
    if (!IsValidClient(i)) {
      continue;
    }
    SetHudTextParams(-1.0, y, 2.1, 255, 255, 255, 255);
    ShowSyncHudText(i, hud_middle, "vs.");
  }

}

// Draws team health on the HUD.
public Action:Timer_DrawTeamHealth(Handle:timer) {

  if (!Respawn_Enabled()) return;

  // If we're not in public mode, don't draw a HUD.
  if (state != GameState_Public) {
    return;
  }

  // If this is CTF mode, draw the intel carrier's health instead.
  new flag = FindEntityByClassname(-1, "item_teamflag");
  if (flag > -1) {
    new owner = GetEntPropEnt(flag, Prop_Data, "m_hOwnerEntity");

    if (IsValidClient(owner)) {
      new health = Entity_GetHealth(owner);
      new max_health = Entity_GetMaxHealth(owner);

      new Float:health_per = float(health) / float(max_health);

      new red = RoundToCeil((255.0 * 4.0) * (0.75 - health_per));
      if (red > 255) red = 255;
      if (red < 0)   red = 0;

      new green = RoundToCeil((255.0 * 4.0) * (health_per - 0.25));
      if (green > 255) green = 255;
      if (green < 0)   green = 0;

      for (new i = 1; i <= MaxClients; i++) {

        if (!IsValidClient(i)) continue;

        SetHudTextParams(0.14, 0.90, 2.0, red, green, 0, 255);
        ShowSyncHudText(i, team_health_hud, "Flag carrier: %d/%d",
          health, max_health);

      }
    }

    return;
  }

  // Otherwise, draw both teams' health.
  new team_health[2];
  new team_health_max[2];
  new team;

  for (new i = 1; i <= MaxClients; i++) {

    if (IsValidClient(i)) {
      team = GetClientTeam(i);
      if (team > 1) {
        team_health[team - 2] += Entity_GetHealth(i);
        team_health_max[team - 2] += Entity_GetMaxHealth(i);
      }
    }

  }

  for (new i = 0; i < 2; i++) {
    if (team_health_max[i] <= 0) {
      team_health_percent[i] = 0;
    } else {
      team_health_percent[i] = RoundToFloor((float(team_health[i]) / float(team_health_max[i])) * 100.0);
    }
  }

  for (new i = 1; i <= MaxClients; i++) {

    if (IsValidClient(i)) {

      SetHudTextParams(0.14, 0.90, 2.0, 255, 255, 255, 255);

      if (GetClientTeam(i) == _:TFTeam_Blue) {
        ShowSyncHudText(i, team_health_hud, "BLU health: %d%%\nRED health: %d%",
          team_health_percent[1], team_health_percent[0]);
      } else {
        ShowSyncHudText(i, team_health_hud, "RED health: %d%%\nBLU health: %d%",
          team_health_percent[0], team_health_percent[1]);
      }

    }

  }

}

// (For tournament mode) Force both teams to be ready
public Action:Timer_ForceTeamsReady(Handle:timer) {

  for (new i = 1; i < 32; i++) {
    GameRules_SetProp("m_bTeamReady", 1, .element=i);
  }

}

// (For tournament mode) Announces that a match is live after exec console spam.
public Action:Timer_AnnounceLiveMatch(Handle:timer) {

  Client_PrintToChatAll(false, "{G}Match is live!");

  // Announce the class bans to the chat.
  decl String:banned[2][16];
  for (new i = 0; i < 2; i++) {
    Class_GetName(team_ban[i], banned[i], 16);
    banned[i][0] &= ~(1 << 5);
  }
  Client_PrintToChatAll(true, "{G}%s{N} and {G}%s{N} are banned.", banned[0], banned[1]);

}

// In pre-tournament mode, reminds players of the tournament commands.
public Action:Timer_TournamentHintText(Handle:timer) {

  if (!Respawn_Enabled()) return;

  // If we're not in pre-tournament mode, don't do anything.
  if (state != GameState_PreTournament) { return; }

  PrintHintTextToAll("%s\n%s", "Commands: !ready, !unready, !teamname, !banclass.",
    "Use !class to switch classes.");

}

// Checks the team balance and reacts accordingly.
public Action:Timer_CheckTeams(Handle:timer) {

  if (!Respawn_Enabled()) return;

  // If we're not using the Arena queue, do nothing.
  if (GetConVarInt(FindConVar("tf_arena_use_queue")) != 1) return;

  // If we're not actually in a round, don't do anything.
  if (GameRules_GetRoundState() < RoundState_Preround
      || state != GameState_Public) {
    return;
  }

  // Finally, if we're less than 5 seconds into a new round, do nothing.
  // This is to avoid conflicting with the Arena balance logic.
  if (GetGameTime() < (last_round_ended +
      GetConVarInt(FindConVar("mp_bonusroundtime")) + 5.0)) {
    return;
  }

  // Otherwise, check the number of players on either team.
  new players_to_add = 0;
  new team = _:TFTeam_Unassigned;
  new red_players = Team_CountPlayers(_:TFTeam_Red);
  new blu_players = Team_CountPlayers(_:TFTeam_Blue);

  if (red_players > blu_players) {
    players_to_add = red_players - blu_players;
    team = _:TFTeam_Blue;
  } else if (blu_players > red_players) {
    players_to_add = blu_players - red_players;
    team = _:TFTeam_Red;
  }

  // If teams are uneven, add players from the Arena queue.
  if (team != _:TFTeam_Unassigned) {
    for (new i = 0; i < players_to_add; i++) {

      // If we're out of players in the queue, get outta here.
      if (Game_PlayersInQueue() <= 0) {
        break;
      }

      // Otherwise, add someone else from the queue to the associated team.
      for (new j = 1; j <= MaxClients; j++) {
        if (IsValidClient(j) && Player_InArenaQueue(j)) {
          ChangeClientTeam(j, team);
          break;
        }
      }

    }
  }

  // If we still have players in the queue by this point, add them in twos.
  team = _:TFTeam_Red;
  for (new i = 0; i <= MaxClients; i++) {

    if (IsValidClient(i) && Player_InArenaQueue(i)
        && (Team_CountPlayers(team) + 1 <= Team_CountPlayers(Team_EnemyTeam(team))
            || Game_PlayersInQueue() >= 2)) {
      ChangeClientTeam(i, team);
      team = Team_EnemyTeam(team);
    }

  }

}
