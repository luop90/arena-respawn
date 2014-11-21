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
