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
 * @return                 Number of players on RED and BLU.
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
 * Returns the number of control points in the map.
 *
 * @return                 The number of team_control_point entities present.
 */
stock Game_CountCapPoints() {

  return Game_CountEntities("team_control_point");

}

/**
 * Returns the number of entities of the given class present in the game.
 *
 * @param classname        The classname to search for.
 * @param include_marked   Whether to include entities marked for death.
 * @return                 The number of the given entity type present.
 */
stock Game_CountEntities(const String:classname[], bool:include_marked = true) {

  new count = 0;
  new ent = -1;
  while ((ent = FindEntityByClassname(ent, classname)) != -1) {
    if (include_marked || !(Entity_GetEFlags(ent) & EFL_KILLME)) {
      count++;
    }
  }
  return count;

}

/**
 * Returns the amount of time (in seconds) since a control point was last captured.
 *
 * @return                 Time since a control point was last captured.
 */
stock Game_TimeSinceLastCap() {

  return GetGameTime() - last_cap_time;

}

/**
 * Replaces entities of a given classname with new entities in the same
 * position and rotation with the replacement classname.
 *
 * @param old_classname    Classname to search for.
 * @param new_classname    Classname to replace with.
 * @noreturn
 */
stock Game_ReplaceEntities(const String:old_classname[], const String:new_classname[]) {

  new ent = -1;
  while ((ent = FindEntityByClassname(ent, old_classname)) != -1) {
    Entity_ReplaceWith(ent, new_classname);
  }

}

/**
 * Finds the position of the average spawn location of the given team.
 *
 * @param team             Team index.
 * @param pos              Vector of the average spawn location.
 * @noreturn
 */
stock Game_AveragePlayerSpawn(team, Float:pos[3]) {

  new spawn = -1;
  new count = 0;
  new Float:avg_pos[3] = { 0.0, ... };
  decl Float:spawn_pos[3];
  while ((spawn = FindEntityByClassname(spawn, "info_player_teamspawn")) != -1) {
    if (GetEntProp(spawn, Prop_Data, "m_iInitialTeamNum") != team) {
      continue;
    }
    Entity_GetTrueAbsOrigin(spawn, spawn_pos);
    avg_pos[0] += spawn_pos[0];
    avg_pos[1] += spawn_pos[1];
    avg_pos[2] += spawn_pos[2];
    count++;
  }

  if (count > 0) {
    pos[0] = avg_pos[0] / count;
    pos[1] = avg_pos[1] / count;
    pos[2] = avg_pos[2] / count;
  } else {
    pos[0] = 0.0;
    pos[1] = 0.0;
    pos[2] = 0.0;
  }

}

/**
 * Returns the number of players in the Arena queue, if any exist.
 *
 * @return                 Number of players in the queue.
 */
stock Game_PlayersInQueue() {

  new count;
  for (new i = 1; i <= MaxClients; i++) {
    if (IsValidClient(i) && Player_InArenaQueue(i)) {
      count++;
    }
  }
  return count;

}

/**
 * Resets all consumable items in play.
 *
 * @noreturn
 */
stock Game_ResetConsumables() {

  // Remove Sandviches from the world. (Heavy is sad.)
  new healthpack = -1;
  decl String:model_name[32];
  while ((healthpack = FindEntityByClassname(healthpack, "item_healthkit_medium")) != -1) {

    if (IsValidEntity(healthpack)) {
      GetEntPropString(healthpack, Prop_Data, "m_iszModel", model_name, sizeof(model_name));
      if (StrContains(model_name, "plate")) {
        AcceptEntityInput(healthpack, "Kill");
      }
    }

  }

  // Undo any consumable effects (Bonk, Crit-a-Cola, etc.)
  for (new i = 1; i <= MaxClients; i++) {
    if (!IsValidClient(i)) continue;
    for (new j = 0; j < sizeof(consumable_conds); j++) {
      if (TF2_IsPlayerInCondition(i, consumable_conds[j])) {
        TF2_RemoveCondition(i, consumable_conds[j]);
      }
    }
  }

}
