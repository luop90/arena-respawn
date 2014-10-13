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

#if defined _respawn_include_client
  #endinput
#endif
#define _respawn_include_client

/* Returns True if the client is an ingame player, False otherwise.
 * Checks are performed in the order least likely to spew an error.
 *
 * @return                  If the client is a valid client.
 */
stock bool:IsValidClient(client) {

  return (client > 0 && client <= MaxClients &&
    IsClientConnected(client) && IsClientInGame(client) &&
    !IsClientReplay(client) && !IsClientSourceTV(client));

}
