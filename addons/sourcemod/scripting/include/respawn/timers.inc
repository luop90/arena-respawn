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