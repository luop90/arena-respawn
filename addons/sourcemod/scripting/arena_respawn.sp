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

#define PLUGIN_VERSION "1.2.1"

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

enum GameState {

  GameState_Public,
  GameState_PreTournament,
  GameState_Tournament

};

// TF2 ConVars
new Handle:cvar_arena                 = INVALID_HANDLE;
new Handle:cvar_first_blood           = INVALID_HANDLE;
new Handle:cvar_cap_enable_time       = INVALID_HANDLE;
new Handle:cvar_caplinear             = INVALID_HANDLE;

// Plugin ConVars
new Handle:cvar_plugin_loaded         = INVALID_HANDLE;
new Handle:cvar_plugin_enabled        = INVALID_HANDLE;
new Handle:cvar_invuln_time           = INVALID_HANDLE;
new Handle:cvar_cap_time              = INVALID_HANDLE;
new Handle:cvar_first_blood_threshold = INVALID_HANDLE;
new Handle:cvar_lms_critboost         = INVALID_HANDLE;
new Handle:cvar_lms_minicrits         = INVALID_HANDLE;
new Handle:cvar_logging               = INVALID_HANDLE;
new Handle:cvar_autorecord            = INVALID_HANDLE;

new cap_owner = 0;
new mid_index = 2;
new num_revived_players = 0;
new team_health_percent[2] = { 0, ... };
new Float:last_cap_time = 0.0;
new bool:client_on_point[MAXPLAYERS+1] = { false, ... };
new bool:client_is_marked[MAXPLAYERS+1] = { false, ... };
new Handle:client_mark_timer[MAXPLAYERS+1] = { INVALID_HANDLE, ... };
new Handle:team_health_hud = INVALID_HANDLE;

// Tournament variables
new GameState:state = GameState_Public;
new bool:team_ready[2] = { false, ... };
new TFClassType:team_ban[2] = { TFClass_Unknown, ... };
new Handle:hud[2] = { INVALID_HANDLE, ... };
new Handle:hud_middle = INVALID_HANDLE;
new Handle:dm_respawntimer[MAXPLAYERS+1] = { INVALID_HANDLE, ... };

// Hide these cvar changes, as they're handled by the plugin itself.
new String:quiet_cvars[][] = { "tf_arena_first_blood", "tf_arena_max_streak", "tf_caplinear" };

// Reset these conditions when regenerating players at the start of a round.
new TFCond:consumable_conds[] = { TFCond_Bonked, TFCond_CritCola, TFCond_CritHype, TFCond_RestrictToMelee };

// Preserve these variables across players' lives, but only if they're playing the same class.
new String:preserved_int_names[][] = { "m_iDecapitations", "m_iRevengeCrits" };
new preserved_ints[MAXPLAYERS+1][sizeof(preserved_int_names)];
new String:preserved_float_names[][] = { "m_flRageMeter", "m_flHypeMeter" };
new Float:preserved_floats[MAXPLAYERS+1][sizeof(preserved_float_names)];

// Sound that plays to players whose team has respawned (and neutral spectators).
new String:sound_friendly_cap[] = "mvm/mvm_revive.wav";

// Sound that plays to players when the enemy team has respawned.
new String:sound_enemy_cap[] = "misc/ks_tier_03_kill_01.wav";

// Sound that plays to both teams when the LMS receives a critboost.
new String:sound_lms_kill[] = "mvm/mvm_cpoint_klaxon.wav";

// Storage for the last played class of each player.
new TFClassType:client_last_class[MAXPLAYERS+1] = { TFClass_Unknown, ... };

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
  HookEvent("teamplay_game_over", OnGameOver);
  HookEvent("tf_game_over", OnGameOver);

  RegConsoleCmd("sm_class", Command_ChangeClass);
  RegConsoleCmd("sm_ready", Command_TeamReady);
  RegConsoleCmd("sm_unready", Command_TeamUnReady);
  RegConsoleCmd("sm_banclass", Command_TeamBan);
  RegConsoleCmd("sm_teamname", Command_TeamName);
  RegAdminCmd("ars_tournament_start", Command_StartTournament, ADMFLAG_CONFIG);
  RegAdminCmd("ars_tournament_stop", Command_StopTournament, ADMFLAG_CONFIG);

  cvar_arena = FindConVar("tf_gamemode_arena");
  cvar_first_blood = FindConVar("tf_arena_first_blood");
  cvar_cap_enable_time = FindConVar("tf_arena_override_cap_enable_time");
  cvar_caplinear = FindConVar("tf_caplinear");

  cvar_plugin_loaded = CreateConVar("ars_loaded", "1",
    "Indicates whether the plugin is loaded or not. Can be used to check if a server is A:R-capable.");

  cvar_plugin_enabled = CreateConVar("ars_enable", "1",
    "Turns Arena: Respawn on or off.");

  cvar_logging = CreateConVar("ars_log_enabled", "1",
    "1 to enable status messages in the console, 0 to disable.");

  cvar_invuln_time = CreateConVar("ars_invuln_time", "3.0",
    "How long to flash Uber on a newly respawned player.");

  cvar_cap_time = CreateConVar("ars_cap_time", "1.0",
    "Approximate amount of time (in seconds) it takes to capture the central control point.");

  cvar_first_blood_threshold = CreateConVar("ars_first_blood_threshold", "14",
    "Number of active players required to activate first blood (-1 to disable).");

  cvar_lms_critboost = CreateConVar("ars_lms_critboost", "8.0",
    "How many seconds of crit boost the Last Man Standing gets when a player on the opposing team dies or is killed.");

  cvar_lms_minicrits = CreateConVar("ars_lms_minicrits", "0",
    "Set to 1 to turn the Last Man Standing critboost into minicrits instead.");

  cvar_autorecord = CreateConVar("ars_autorecord", "0",
    "Set to 1 to automatically record tournaments (SourceTV must be enabled).");

  for (new i = 0; i < 2; i++) {
    hud[i] = CreateHudSynchronizer();
  }
  hud_middle = CreateHudSynchronizer();
  CreateTimer(2.0, Timer_DrawHUD, _, TIMER_REPEAT);
  CreateTimer(1.0, Timer_RespawnDeadPlayers, _, TIMER_REPEAT);

  CreateTimer(30.0, Timer_TournamentHintText, _, TIMER_REPEAT);

  CreateTimer(1.0, Timer_CheckTeams, _, TIMER_REPEAT);

  team_health_hud = CreateHudSynchronizer();
  CreateTimer(0.25, Timer_DrawTeamHealth, _, TIMER_REPEAT);

}

// Fired every frame.
public OnGameFrame() {

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

}

// Fired when configs execute.
public OnConfigsExecuted() {

  if (!Respawn_Enabled()) return;

  if (state == GameState_PreTournament) {
    Command_StartTournament(-1, 0);
  }

}

// Fired on map load or on plugin load/reload.
public OnMapStart() {

  mid_index = 2;

  // Load sounds
  PrecacheSound(sound_friendly_cap);
  PrecacheSound(sound_enemy_cap);
  PrecacheSound(sound_lms_kill);

}

// Handle cleanup on map end
public OnMapEnd() {

  for (new i = 1; i <= MaxClients; i++) {
    client_mark_timer[i] = INVALID_HANDLE;
  }

  if (state == GameState_Tournament) {
    state = GameState_PreTournament;
  }

}

// Fired when a client connects to the server.
public OnClientPostAdminCheck(client) {

  if (!Respawn_Enabled()) return;

  CreateTimer(1.0, Timer_Credits, client);

}

// Fired on a client leaving the server.
public OnClientDisconnect_Post(client) {

  client_last_class[client] = TFClass_Unknown;
  client_on_point[client] = false;
  client_is_marked[client] = false;

}

// Fired on teamplay_round_start.
public OnRoundStart(Handle:event, const String:name[], bool:hide_broadcast) {

  if (!Respawn_Enabled()) return;

  Respawn_LockSpawnDoors();
  Respawn_ResetRoundState();

  if (Game_CountCapPoints() == 1) {
    Respawn_SetupHealthKits();
  }

  if (Game_CountCapPoints() > 1) {
    SetConVarInt(cvar_cap_enable_time, -1);
    SetConVarInt(cvar_caplinear, 0);
  } else {
    SetConVarInt(cvar_caplinear, 1);
  }

  if (Game_CountCapPoints() == 5) {
    Log("Chosen midpoint: %d", mid_index);
    Respawn_SetupSpawns_FiveCp();
    for (new i = 1; i <= MaxClients; i++) {
      CreateTimer(0.01, Timer_RespawnPlayer, i);
    }
  }

}

// Fired after the gates open in Arena.
public OnArenaStart(Handle:event, const String:name[], bool:hide_broadcast) {

  if (!Respawn_Enabled()) return;

  Respawn_UnlockSpawnDoors();
  Respawn_SetupCapPoints();
  Game_RegeneratePlayers();
  Game_ResetConsumables();

  // Make our own round timer, if applicable.
  Respawn_CreateCapTimer();

  // Clear all MFD in case anything is lingering (if a round suddenly restarted, for example).
  for (new i = 1; i <= MaxClients; i++) {
    if (IsValidClient(i) && (GetClientTeam(i) == _:TFTeam_Red || GetClientTeam(i) == _:TFTeam_Blue)) {
      TF2_RemoveCondition(i, TFCond_MarkedForDeath);
    }
  }

  new num_players = Game_CountActivePlayers();

  Log("Round started with %d players.", num_players);

  new fb_threshold = GetConVarInt(cvar_first_blood_threshold);

  if (num_players < 6) {
    SetConVarInt(cvar_first_blood, 0);
    Client_PrintToChatAll(false, "{G}Small match rules active - capture the point for overheal!");
  } else if (num_players < fb_threshold || fb_threshold == -1 || Game_CountCapPoints() > 1) {
    SetConVarInt(cvar_first_blood, 0);
  } else {
    SetConVarInt(cvar_first_blood, 1);
    Client_PrintToChatAll(false, "{G}First blood critboost enabled!");
  }

}

// Fired by a trigger_capture_area on team capture.
public OnTeamCapture(const String:output[], caller, activator, Float:delay) {

  if (!Respawn_Enabled()) return;

  // Set the last capture time.
  last_cap_time = GetGameTime();

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

  // Reset everyone's death marks.
  for (new i = 1; i <= MaxClients; i++) {
    if (IsValidClient(i) && client_is_marked[i]) {
      TF2_RemoveCondition(i, TFCond_MarkedForDeath);
      client_is_marked[i] = false;
    }
  }

  // Set the time to capture.
  CapArea_SetCaptureTime(caller, true);

}

// Fired when win conditions have been met.
public OnWin(const String:output[], caller, activator, Float:delay) {

  if (!Respawn_Enabled()) return;
  Respawn_ResetRoundState();

}

// Fired by a trigger_capture_area when a player starts or stops coming into contact with it.
public OnPointTouch(const String:output[], caller, activator, Float:delay) {

  if (!Respawn_Enabled()) return;

  // Ignore if the activator wasn't a client.
  if (!IsValidClient(activator)) {
    return;
  }

  // Ignore if the classname of what was touched is not a trigger_capture_area.
  if (!Entity_ClassNameMatches(caller, "trigger_capture_area")) {
    return;
  }

  // Set the state of the player to be touching or not touching the point.
  if (StrEqual(output, "OnEndTouch")) {
    client_on_point[activator] = false;
  } else {
    client_on_point[activator] = true;
  }

  // Don't bother marking anyone for death if there's more than one point.
  if (Game_CountCapPoints() > 1) {
    return;
  }

  new control_point = CapArea_GetControlPoint(caller);

  if (control_point > 0) {

    // Is the player a member of the team that owns this control point?
    if (GetClientTeam(activator) == Objective_GetPointOwner(GetEntProp(control_point, Prop_Data, "m_iPointIndex"))) {

      // Note: Always reset the timer state!
      if (client_mark_timer[activator] != INVALID_HANDLE) {
        KillTimer(client_mark_timer[activator]);
        client_mark_timer[activator] = INVALID_HANDLE;
      }

      // If so, and they've started touching the point, schedule a death mark.
      if (client_on_point[activator]) {
        new Float:time = (last_cap_time + 2.0) - GetGameTime();
        if (time < 0.0) {
          time = 0.0;
        }
        client_mark_timer[activator] = CreateTimer(time, Timer_Add_Mark, activator, TIMER_FLAG_NO_MAPCHANGE);

      // Otherwise, schedule a death mark removal if they're coming off of the point.
      } else {
        client_mark_timer[activator] = CreateTimer(2.0, Timer_Remove_Mark, activator, TIMER_FLAG_NO_MAPCHANGE);
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

  if (!Respawn_Enabled()) return;

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
        if (client_mark_timer[player] != INVALID_HANDLE) {
          KillTimer(client_mark_timer[player]);
          client_mark_timer[player] = INVALID_HANDLE;
        }
        client_mark_timer[player] = CreateTimer(2.0, Timer_Add_Mark, player, TIMER_FLAG_NO_MAPCHANGE);
      }

      // If this is a small match, overheal the capturing player.
      if (Game_CountActivePlayers() < 6) {
        new Float:health = float(Entity_GetMaxHealth(player));
        Entity_SetHealth(player, RoundToFloor(health * 1.5), true);
        TF2_AddCondition(player, TFCond_Ubercharged, 0.75);
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

  if (!Respawn_Enabled()) return;

  new victim = GetClientOfUserId(GetEventInt(event, "userid"));
  new attacker = GetClientOfUserId(GetEventInt(event, "attacker"));
  new team = GetClientTeam(victim);
  new flags = GetEventInt(event, "death_flags");

  // Nobody cares about dead ringers.
  if(!IsValidClient(victim) || flags & 32) {
    return;
  }

  // If we are in pre-tournament (deathmatch) mode, instantly respawn the player,
  // resupply the attacker, but do nothing else.
  if (state == GameState_PreTournament) {
    if (dm_respawntimer[victim] == INVALID_HANDLE) {
      dm_respawntimer[victim] = CreateTimer(1.0, Timer_DMRespawnPlayer, victim);
    }
    if (IsValidClient(attacker) && attacker != victim) {
      Client_PrintToChat(victim, true, "Your opponent had {G}%d{N} health remaining.", Entity_GetHealth(attacker));
      TF2_RegeneratePlayer(attacker);
    }
    return;
  }

  // Store information about the player's state for use upon respawn.
  client_last_class[victim] = TF2_GetPlayerClass(victim);
  for (new i = 0; i < sizeof(preserved_int_names); i++) {
    preserved_ints[victim][i] = GetEntProp(victim, Prop_Send, preserved_int_names[i]);
  }
  for (new i = 0; i < sizeof(preserved_float_names); i++) {
    preserved_floats[victim][i] = GetEntPropFloat(victim, Prop_Send, preserved_float_names[i]);
  }

  // If the opposing team has only one player remaining, grant that opposing player a five-second minicrit boost.
  new enemy_team = Team_EnemyTeam(team);
  new Float:critboost_time = GetConVarFloat(cvar_lms_critboost);
  if (critboost_time > 0.0 && Team_CountAlivePlayers(enemy_team) == 1 && Team_CountAlivePlayers(team) > 2) {
    for (new i = 1; i <= MaxClients; i++) {
      if (IsValidClient(i) && IsPlayerAlive(i) && GetClientTeam(i) == enemy_team) {

        // Play a warning sound.
        EmitSoundToAll(sound_lms_kill);

        // Send a notification in chat.
        Client_PrintToChatAll(false, "The {G}Last Man Standing{N} is crit boosted!");

        // Add the critboost/minicrit effect.
        if (GetConVarInt(cvar_lms_minicrits) > 0) {
          TF2_AddCondition(i, TFCond_CritCola, critboost_time);
        } else {
          TF2_AddCondition(i, TFCond_Kritzkrieged, critboost_time);
        }
      }
    }
  }

  // If this team has only one player remaining, print a notification and play a special voice response.
  if (Team_CountAlivePlayers(team) == 2) {

    Team_PlayVO(team, "Announcer.AM_LastManAlive0%d", GetRandomInt(1,4));

    decl String:player_color[4];
    decl String:player_name[32];
    for (new i = 1; i <= MaxClients; i++) {
      if (IsValidClient(i) && IsPlayerAlive(i) && GetClientTeam(i) == team && i != victim) {

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

        GetClientName(i, player_name, sizeof(player_name));
        Client_PrintToChatAll(false, "%s%s{N} became the {G}Last Man Standing{N}!", player_color, player_name);

        break;
      }
    }

  }

}

// Fired when a game has ended because win conditions were met.
public OnGameOver(Handle:event, const String:name[], bool:hide_broadcast) {

  if (!Respawn_Enabled()) return;

  if (state == GameState_Tournament) {

    Log("Tournament over! Reverting back to deathmatch mode.");

    if (GetConVarInt(cvar_autorecord) > 0) {
      ServerCommand("tv_stoprecord");
    }

    // We are no longer in Tournament mode, revert to PreTournament.
    Command_StartTournament(-1, 0);

  }

}

public Action:OnTeamAudio(Handle:event, const String:name[], bool:hide_broadcast) {

  if (!Respawn_Enabled()) return Plugin_Continue;

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

  if (!Respawn_Enabled()) return Plugin_Continue;

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

// Player command - lets players change class in deathmatch mode.
public Action:Command_ChangeClass(client, args) {

  if (!Respawn_Enabled()) return Plugin_Handled;

  // If we're not in tournament mode, do nothing.
  if (state != GameState_PreTournament) {
    Client_PrintToChat(client, true, "{G}Not in pre-tournament mode!");
    return Plugin_Handled;
  }

  // If no class was mentioned, return.
  if (args < 1) {
    Client_PrintToChat(client, true, "{G}Please enter a class.");
    return Plugin_Handled;
  }

  new String:classname[64];
  GetCmdArgString(classname, sizeof(classname));

  new TFClassType:class = Class_GetFromName(classname);
  if (class == TFClass_Unknown) {
    Client_PrintToChat(client, true, "{G}Invalid input.");
    return Plugin_Handled;
  }

  TF2_SetPlayerClass(client, class);
  TF2_RespawnPlayer(client);

  return Plugin_Handled;

}

// Player command - sets team state to ready.
public Action:Command_TeamReady(client, args) {

  if (!Respawn_Enabled()) return Plugin_Handled;

  // If we're not in tournament mode, do nothing.
  if (state != GameState_PreTournament) {
    Client_PrintToChat(client, true, "{G}Not in pre-tournament mode!");
    return Plugin_Handled;
  }

  new team = GetClientTeam(client);
  if (IsValidClient(client) && team > _:TFTeam_Spectator) {
    team_ready[team - 2] = true;
  }

  if (Respawn_CheckTeamReadyState(GetClientTeam(client))) {
    Respawn_CheckTournamentState();
  }

  return Plugin_Handled;

}

// Player command - sets team state to unready.
public Action:Command_TeamUnReady(client, args) {

  if (!Respawn_Enabled()) return Plugin_Handled;

  // If we're not in tournament mode, do nothing.
  if (state != GameState_PreTournament) {
    Client_PrintToChat(client, true, "{G}Not in pre-tournament mode!");
    return Plugin_Handled;
  }

  new team = GetClientTeam(client);
  if (IsValidClient(client) && team > _:TFTeam_Spectator) {
    team_ready[team - 2] = false;
  }

  Respawn_CheckTournamentState();

  return Plugin_Handled;

}

// Player command - sets team class ban.
public Action:Command_TeamBan(client, args) {

  if (!Respawn_Enabled()) return Plugin_Handled;

  // If we're not in tournament mode, do nothing.
  if (state != GameState_PreTournament) {
    Client_PrintToChat(client, true, "{G}Not in pre-tournament mode!");
    return Plugin_Handled;
  }

  // If no class was mentioned, return.
  if (args < 1) {
    Client_PrintToChat(client, true, "{G}Please enter a class to ban.");
    return Plugin_Handled;
  }

  new String:classname[64];
  GetCmdArgString(classname, sizeof(classname));

  new team = GetClientTeam(client);

  new TFClassType:class = Class_GetFromName(classname);
  if (class == TFClass_Unknown) {
    Client_PrintToChat(client, true, "{G}Invalid input.");
    return Plugin_Handled;
  } else if (class == team_ban[Team_EnemyTeam(team) - 2]) {
    Client_PrintToChat(client, true, "{G}That class is already banned!");
    return Plugin_Handled;
  }
  
  if (IsValidClient(client) && team > _:TFTeam_Spectator) {
    team_ban[team - 2] = class;
  }

  Respawn_CheckTournamentState();

  return Plugin_Handled;

}

// Player command - sets team name.
public Action:Command_TeamName(client, args) {

  if (!Respawn_Enabled()) return Plugin_Handled;

  // If we're not in tournament mode, do nothing.
  if (state != GameState_PreTournament) {
    Client_PrintToChat(client, true, "{G}Not in pre-tournament mode!");
    return Plugin_Handled;
  }

  // If no name was mentioned, return.
  if (args < 1) {
    Client_PrintToChat(client, true, "{G}Please enter a team name.");
    return Plugin_Handled;
  }

  new String:team_name[16];
  GetCmdArgString(team_name, sizeof(team_name));

  new team = GetClientTeam(client);

  if (team == _:TFTeam_Red) {
    SetConVarString(FindConVar("mp_tournament_redteamname"), team_name);
  } else if (team == _:TFTeam_Blue) {
    SetConVarString(FindConVar("mp_tournament_blueteamname"), team_name);
  }

  return Plugin_Handled;

}

// Admin command - starts a tournament.
public Action:Command_StartTournament(client, args) {

  if (!Respawn_Enabled()) return Plugin_Handled;

  state = GameState_PreTournament;
  Client_PrintToChatAll(false, "{G}Entering Tournament Mode");

  for (new i = 0; i < 2; i++) {
    team_ready[i] = false;
    team_ban[i] = TFClass_Unknown;
  }

  ServerCommand("mp_tournament 0 ; exec sourcemod/respawn.pretournament.cfg");

  CapArea_Lock(CapArea_FindByIndex(0));

  return Plugin_Handled;

}

// Admin command - stops a tournament.
public Action:Command_StopTournament(client, args) {

  if (!Respawn_Enabled()) return Plugin_Handled;

  state = GameState_Public;
  Client_PrintToChatAll(false, "{G}Exiting Tournament Mode");

  if (GetConVarInt(cvar_autorecord) > 0) {
    ServerCommand("tv_stoprecord");
  }

  ServerCommand("mp_tournament 0 ; exec sourcemod/respawn.public.cfg");

  return Plugin_Handled;

}
