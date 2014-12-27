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
      ban[0] &= ~(1 << 5);
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

// (For tournament mode) Force both teams to be ready
public Action:Timer_ForceTeamsReady(Handle:timer) {

  for (new i = 1; i < 32; i++) {
    GameRules_SetProp("m_bTeamReady", 1, .element=i);
  }

}

// In pre-tournament mode, reminds players of the tournament commands.
public Action:Timer_TournamentHintText(Handle:timer) {

  if (!Respawn_Enabled()) return;

  // If we're not in pre-tournament mode, don't do anything.
  if (state != GameState_PreTournament) { return; }

  PrintHintTextToAll("%s\n%s", "Commands: !ready, !unready, !teamname, !banclass.",
    "Use !class to switch classes.");

}
