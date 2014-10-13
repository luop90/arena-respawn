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

#if defined _respawn_include_player
  #endinput
#endif
#define _respawn_include_player

/**
 * Respawns a player with an Uber flash and teleport trail effects.
 *
 * @param player           Index of the player to respawn.
 * @noreturn
 */
stock Player_RespawnWithEffects(player) {

  // Respawn the player into the game.
  TF2_RespawnPlayer(player);

  // Invuln for 3 seconds to prevent spawn camping (Arena maps have no spawn barrier).
  TF2_AddCondition(player, TFCond_Ubercharged, GetConVarFloat(cvar_invuln_time));

  // Teleporter trail effect to identify recently respawned players.
  TF2_AddCondition(player, TFCond_TeleportedGlow, 10.0);

  // Schedule a voice response.
  CreateTimer(GetRandomFloat(0.0, 3.0), Player_RespawnResponse, player);

  // Retrieve stored properties if the player is playing the same class they died with.
  new bool:restore = false;
  if (TF2_GetPlayerClass(player) == last_class[player]) {
    restore = true;
  }
  for (new i = 0; i < sizeof(preserved_int_names); i++) {
    if (restore) {
      SetEntProp(player, Prop_Send, preserved_int_names[i], preserved_ints[player][i]);
    } else {
      preserved_ints[player][i] = 0;
    }
  }
  for (new i = 0; i < sizeof(preserved_float_names); i++) {
    if (restore) {
      SetEntPropFloat(player, Prop_Send, preserved_float_names[i], preserved_floats[player][i]);
    } else {
      preserved_floats[player][i] = 0.0;
    }
  }

}

/**
 * Plays a respawn-related voice response.
 *
 * @param timer           Handle of the timer used to schedule this event.
 * @param player          Index of the player to prompt a response from.
 * @noreturn
 */
public Action:Player_RespawnResponse(Handle:timer, any:player) {

  new response_type = GetRandomInt(1,2);

  switch(response_type) {

    // MvM-style resurrection line
    case 1: {
      SetVariantString("TLK_RESURRECTED");
      AcceptEntityInput(player, "SpeakResponseConcept");
    }

    // Battle cry
    case 2: {
      FakeClientCommand(player, "voicemenu 2 1");
    }

  }

}
