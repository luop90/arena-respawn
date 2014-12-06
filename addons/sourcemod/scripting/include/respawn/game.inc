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
