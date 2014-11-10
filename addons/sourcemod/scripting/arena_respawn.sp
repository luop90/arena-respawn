/**
 * Copyright © 2014 awk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma semicolon 1

#define PLUGIN_VERSION "1.0.0"

#include <sourcemod>
#include <sdktools>
#include <tf2>
#include <tf2_stocks>
#include <smlib>

public Plugin:myinfo = {

  name        = "Arena: Respawn",
  author      = "awk",
  description = "A gamemode that gives Arena second chances.",
  version     = PLUGIN_VERSION,
  url         = "http://steam.respawn.tf/"

}

new Handle:cvar_first_blood = INVALID_HANDLE;
new Handle:cvar_invuln_time = INVALID_HANDLE;
new Handle:cvar_cap_time = INVALID_HANDLE;
new Handle:cvar_first_blood_threshold = INVALID_HANDLE;
new Handle:cvar_lms_minicrits = INVALID_HANDLE;
new Handle:cvar_logging = INVALID_HANDLE;
new Handle:cvar_force_arena = INVALID_HANDLE;

new cap_owner = 0;
new num_revived_players = 0;
new point_last_touched[MAXPLAYERS+1] = { -1, ... };

// Hide these cvar changes, as they're handled by the plugin itself.
new String:quiet_cvars[][] = { "tf_arena_first_blood" };

// Preserve these variables across players' lives, but only if they're playing the same class.
new String:preserved_int_names[][] = { "m_iKillStreak", "m_iDecapitations", "m_iRevengeCrits" };
new preserved_ints[MAXPLAYERS+1][sizeof(preserved_int_names)];
new String:preserved_float_names[][] = { "m_flRageMeter", "m_flHypeMeter" };
new Float:preserved_floats[MAXPLAYERS+1][sizeof(preserved_float_names)];

// Sound that plays to players whose team has respawned (and neutral spectators).
new String:sound_friendly_cap[] = "mvm/mvm_revive.wav";

// Sound that plays to players when the enemy team has respawned.
new String:sound_enemy_cap[] = "misc/ks_tier_03_kill_01.wav";

// Storage for the last played class of each player.
new TFClassType:last_class[MAXPLAYERS+1] = { TFClass_Unknown, ... };

// Load Respawn-specific functions
#include <respawn>

public OnPluginStart() {

  HookEntityOutput("tf_gamerules", "OnWonByTeam1", OnWin);
  HookEntityOutput("tf_gamerules", "OnWonByTeam2", OnWin);
  HookEntityOutput("trigger_capture_area", "OnCapTeam1", OnTeamCapture);
  HookEntityOutput("trigger_capture_area", "OnCapTeam2", OnTeamCapture);
  HookEntityOutput("trigger_capture_area", "OnStartTouch", OnPointTouch);
  HookEntityOutput("trigger_capture_area", "OnEndTouch", OnPointTouch);

  HookEvent("server_cvar", SilenceConVarChange, EventHookMode_Pre);
  HookEvent("teamplay_round_start", OnRoundStart);
  HookEvent("teamplay_broadcast_audio", OnTeamAudio, EventHookMode_Pre);
  HookEvent("teamplay_point_captured", OnPointCaptured);
  HookEvent("arena_round_start", OnArenaStart);
  HookEvent("player_death", OnPlayerDeath);

  cvar_first_blood = FindConVar("tf_arena_first_blood");

  cvar_logging = CreateConVar("ars_log_enabled", "1",
    "1 to enable status messages in the console, 0 to disable.");

  cvar_force_arena = CreateConVar("ars_force_arena", "0",
    "(Experimental) Set to 1 to attempt to force any map into Arena mode.");

  cvar_invuln_time = CreateConVar("ars_invuln_time", "3.0",
    "How long to flash Uber on a newly respawned player.");

  cvar_cap_time = CreateConVar("ars_cap_time", "1.0",
    "Approximate amount of time (in seconds) it takes to capture the central control point.");

  cvar_first_blood_threshold = CreateConVar("ars_first_blood_threshold", "14",
    "Number of active players required to activate first blood (-1 to disable).");

  cvar_lms_minicrits = CreateConVar("ars_lms_minicrits", "8.0",
    "How many seconds of minicrits the Last Man Standing gets when a player on the opposing team dies or is killed.");

}

// Fired on map load or on plugin load/reload.
public OnMapStart() {

  // Load sounds
  PrecacheSound(sound_friendly_cap);
  PrecacheSound(sound_enemy_cap);

}

// Fired when a client connects to the server.
public OnClientPostAdminCheck(client) {

  CreateTimer(1.0, Timer_Credits, client);

}

// Fired on a client leaving the server.
public OnClientDisconnect_Post(client) {

  last_class[client] = TFClass_Unknown;

}

// Fired on teamplay_round_start.
public OnRoundStart(Handle:event, const String:name[], bool:hide_broadcast) {

  if (GetConVarInt(cvar_force_arena) > 0) {
    Game_ForceArenaMode();
  }

}

// Fired after the gates open in Arena.
public OnArenaStart(Handle:event, const String:name[], bool:hide_broadcast) {

  Game_RegeneratePlayers();
  Game_SetupCapPoints();
  Game_ResetRoundState();

  if (Game_CountCapPoints() == 1) {
    Game_CreateCapTimer();
  }

  // Clear all MFD in case anything is lingering (if a round suddenly restarted, for example).
  for (new i = 1; i <= MaxClients; i++) {
    if (IsValidClient(i) && (GetClientTeam(i) == _:TFTeam_Red || GetClientTeam(i) == _:TFTeam_Blue)) {
      TF2_RemoveCondition(i, TFCond_MarkedForDeath);
    }
  }

  new num_players = Game_CountActivePlayers();

  Log("Round started with %d players.", num_players);

  new fb_threshold = GetConVarInt(cvar_first_blood_threshold);

  if (num_players < 4) {
    SetConVarInt(cvar_first_blood, 0);
    Client_PrintToChatAll(false, "{G}1v1 rules active - capture the point for overheal!");
  } else if (num_players < fb_threshold || fb_threshold == -1) {
    SetConVarInt(cvar_first_blood, 0);
  } else {
    SetConVarInt(cvar_first_blood, 1);
    Client_PrintToChatAll(false, "{G}First blood critboost enabled!");
  }

}

// Fired by a trigger_capture_area on team capture.
public OnTeamCapture(const String:output[], caller, activator, Float:delay) {

  // Determine the team that triggered this capture.
  new team = 0;
  if (StrEqual(output, "OnCapTeam1")) {
    team = _:TFTeam_Red;
  } else if (StrEqual(output, "OnCapTeam2")) {
    team = _:TFTeam_Blue;
  }

  if (Game_CountCapPoints() > 1 || cap_owner != team) {
    Log("Point captured by team #%d", team);
    cap_owner = team;

    num_revived_players = 0;
    for (new i = 1; i <= MaxClients ; i++) {
      if (IsValidClient(i)) {
        if (IsClientObserver(i) && GetClientTeam(i) == team) {
          Player_RespawnWithEffects(i);
          num_revived_players++;
        }
      }
    }
  } else {
    Log("Point re-captured by team #%d", team);

    new cap_master = FindEntityByClassname(-1, "team_control_point_master");
    if (IsValidEntity(cap_master)) {
      SetVariantInt(team);
      AcceptEntityInput(cap_master, "SetWinner");
    } else {
      Log("cap_master is not a valid entity (%d).", cap_master);
    }
  }

}

public OnWin(const String:output[], caller, activator, Float:delay) {

  Game_ResetRoundState();

}

// Fired by a trigger_capture_area when a player starts or stops coming into contact with it.
public OnPointTouch(const String:output[], caller, activator, Float:delay) {

  // Ignore if the activator wasn't a client.
  if (!IsValidClient(activator)) {
    return;
  }

  // Ignore if the classname of what was touched is not a trigger_capture_area.
  if (!Entity_ClassNameMatches(caller, "trigger_capture_area")) {
    return;
  }

  // Don't bother marking anyone for death if there's more than one point.
  if (Game_CountCapPoints() > 1) {
    return;
  }

  // Always remove the MFD condition before potentially re-adding a new one.
  TF2_RemoveCondition(activator, TFCond_MarkedForDeath);

  new control_point = CapArea_GetControlPoint(caller);

  if (control_point > 0) {

    // Is the player a member of the team that owns this control point?
    if (GetClientTeam(activator) == Objective_GetPointOwner(GetEntProp(control_point, Prop_Data, "m_iPointIndex"))) {

      // If so, and they're coming off of the point, give them a lingering two-second MFD effect.
      if (StrEqual(output, "OnEndTouch")) {
        TF2_AddCondition(activator, TFCond_MarkedForDeath, 2.0);

      // Otherwise, mark them for death indefinitely. When they get off the point, this output will fire again.
      } else {
        TF2_AddCondition(activator, TFCond_MarkedForDeath);
      }

    }

  } else {

    // If this message appears, the map Respawn is running on could probably do with some special Stripper rules.
    Log("Client %d touched trigger_capture_area not associated with a control point.");

  }

}

// Fired as a netevent when a point has been successfully captured.
// Unlike the team_control_point output, this allows us access to the players who captured the point.
public OnPointCaptured(Handle:event, const String:name[], bool:hide_broadcast) {

  new String:cap_message[256];
  new String:player_name[64];
  new String:player_color[3];

  // Get the players who captured the point.
  new String:cappers[64];
  GetEventString(event, "cappers", cappers, sizeof(cappers));

  // Determine which color to use for highlighting player names in chat.
  new team = GetEventInt(event, "team");

  switch(team) {

    case (_:TFTeam_Red): {
      player_color = "{R}";
    }
    case (_:TFTeam_Blue): {
      player_color = "{B}";
    }
    default: {
      player_color = "{N}";
    }

  }

  for (new i = 0; i < strlen(cappers); i++) {

    new player = cappers[i];
    if (IsValidClient(player)) {

      if (Game_CountCapPoints() == 1) {
        // If this is the only point, mark the capturing players for death.
        TF2_RemoveCondition(player, TFCond_MarkedForDeath);
        TF2_AddCondition(player, TFCond_MarkedForDeath);
      }

      // If this is 1v1, overheal the capturing player.
      if (Game_CountActivePlayers() < 4) {
        new Float:health = float(Entity_GetMaxHealth(player));
        Entity_SetHealth(player, RoundToFloor(health * 1.5), true);
      }

      // Add the player name (team-colored) to the capture message.
      GetClientName(player, player_name, sizeof(player_name));
      StrCat(cap_message, sizeof(cap_message), player_color);
      StrCat(cap_message, sizeof(cap_message), player_name);
      if (i != strlen(cappers) - 1) {
        StrCat(cap_message, sizeof(cap_message), ", ");
      }

    }

  }

  // Announce the number of players respawned.
  new String:players_str[16] = "players!";
  if (num_revived_players == 1) {
    players_str = "player!";
  }

  new String:revived_message[64];
  Format(revived_message, sizeof(revived_message), "{N} revived %d %s", num_revived_players, players_str);

  StrCat(cap_message, sizeof(cap_message), revived_message);

  // Only announce and play sounds if at least one player was respawned.
  if (num_revived_players > 0) {
    Client_PrintToChatAll(false, cap_message);

    new enemy_team = Team_EnemyTeam(team);
    Team_EmitSound(_:TFTeam_Unassigned, sound_friendly_cap);
    Team_EmitSound(_:TFTeam_Spectator,  sound_friendly_cap);
    Team_EmitSound(team,                sound_friendly_cap);
    Team_EmitSound(enemy_team,          sound_enemy_cap);
  }

}

// Fired when a player dies or activates their Dead Ringer.
public OnPlayerDeath(Handle:event, const String:name[], bool:hide_broadcast) {

  new victim = GetClientOfUserId(GetEventInt(event, "userid"));
  new team = GetClientTeam(victim);
  new flags = GetEventInt(event, "death_flags");

  // Nobody cares about dead ringers.
  if(!IsValidClient(victim) || flags & 32) {
    return;
  }

  // Store information about the player's state for use upon respawn.
  last_class[victim] = TF2_GetPlayerClass(victim);
  for (new i = 0; i < sizeof(preserved_int_names); i++) {
    preserved_ints[victim][i] = GetEntProp(victim, Prop_Send, preserved_int_names[i]);
  }
  for (new i = 0; i < sizeof(preserved_float_names); i++) {
    preserved_floats[victim][i] = GetEntPropFloat(victim, Prop_Send, preserved_float_names[i]);
  }

  // If the opposing team has only one player remaining, grant that opposing player a five-second minicrit boost.
  new enemy_team = Team_EnemyTeam(team);
  new Float:minicrit_time = GetConVarFloat(cvar_lms_minicrits);
  if (minicrit_time > 0.0 && Team_CountAlivePlayers(enemy_team) == 1) {
    for (new i = 1; i <= MaxClients; i++) {
      if (IsValidClient(i) && IsPlayerAlive(i) && GetClientTeam(i) == enemy_team) {
        TF2_AddCondition(i, TFCond_CritCola, minicrit_time);
      }
    }
  }

  // If this team has only one player remaining, play a special voice response.
  if (Team_CountAlivePlayers(team) == 2) {

    Team_PlayVO(team, "Announcer.AM_LastManAlive0%d", GetRandomInt(1,4));

  }

}

public Action:OnTeamAudio(Handle:event, const String:name[], bool:hide_broadcast) {

  new String:vo[64];
  GetEventString(event, "sound", vo, sizeof(vo));

  new team = GetEventInt(event, "team");

  // Special (disappointed) response on a team that lost on a single-cap map with players left alive.
  if (StrEqual(vo, "Game.YourTeamLost") && Team_CountAlivePlayers(team) > 0 && Game_CountCapPoints() == 1) {

    Team_PlayVO(team, "Announcer.AM_LastManForfeit0%d", GetRandomInt(1,4));
    return Plugin_Handled;

  }

  return Plugin_Continue;

}

// Intercepts configuration variable changes and silences some of them.
public Action:SilenceConVarChange(Handle:event, String:name[], bool:hide_broadcast) {

  new String:cvar_name[64];
  GetEventString(event, "cvarname", cvar_name, sizeof(cvar_name));

  for (new i = 0; i < sizeof(quiet_cvars); i++) {
    if (StrEqual(quiet_cvars[i], cvar_name)) {
      SetEventBroadcast(event, true);
      return Plugin_Changed;
    }
  }

  return Plugin_Continue;

}

// Prints a prefixed log to the console.
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
