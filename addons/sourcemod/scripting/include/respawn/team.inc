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

#if defined _respawn_include_team
  #endinput
#endif
#define _respawn_include_team

/**
 * Returns the opposite team in TF2 (i.e. RED if the team is BLU and vice-versa).
 *
 * @param team             Team to find the enemy team of.
 * @return                 The enemy team.
 * @error                  If the team is not RED or BLU.
 */
stock Team_EnemyTeam(team) {

  if (team < 2 || team > 3) {
    ThrowError("Attempted to find the enemy team of team #%d, which is not a team in play.", team);
  }
  return 2 + (team + 1) % 2;

}

/**
 * Emits a sound to an entire team.
 * Takes the same parameters as EmitSoundToClient().
 *
 * @param team              Team index.
 * @param sample		        Sound file name relative to the "sounds" folder.
 * @param entity		        Entity to emit from.
 * @param channel		        Channel to emit with.
 * @param level			        Sound level.
 * @param flags	         	  Sound flags.
 * @param volume		        Sound volume.
 * @param pitch			        Sound pitch.
 * @param speakerentity   	Unknown.
 * @param origin		        Sound origin.
 * @param dir			          Sound direction.
 * @param updatePos	        Unknown.
 * @param soundtime      	  Alternate time to play sound for.
 * @noreturn
 */
stock Team_EmitSound(team,
                     const String:sample[],
                     entity = SOUND_FROM_PLAYER,
                     channel = SNDCHAN_AUTO,
                     level = SNDLEVEL_NORMAL,
                     flags = SND_NOFLAGS,
                     Float:volume = SNDVOL_NORMAL,
                     pitch = SNDPITCH_NORMAL,
                     speakerentity = -1,
                     const Float:origin[3] = NULL_VECTOR,
                     const Float:dir[3] = NULL_VECTOR,
                     bool:updatePos = true,
                     Float:soundtime = 0.0) {

  for (new i = 1; i <= MaxClients; i++) {

    if (IsValidClient(i) && GetClientTeam(i) == team) {
      EmitSoundToClient(i, sample, entity, channel, level, flags, volume, pitch,
        speakerentity, origin, dir, updatePos, soundtime);
    }

  }

}

/*
 * Plays a voiceover to the given team via tf_gamerules.
 * 
 * @param team             Index of the team to play the voicover to.
 * @param vo_raw           Name of the voiceover rule.
 * @noreturn
 */
stock Team_PlayVO(team, const String:vo_raw[], any:...) {

  // Format the VO name (useful when picking a random VO)
  decl String:vo[255];
  VFormat(vo, sizeof(vo), vo_raw, 3);

  new gamerules = FindEntityByClassname(-1, "tf_gamerules");
  if (gamerules > 0) {

    decl String:input[32] = "PlayVO";
    if (team == _:TFTeam_Red) {
      input = "PlayVORed";
    } else if (team == _:TFTeam_Blue) {
      input = "PlayVOBlue";
    }

    Log("%s %s", input, vo);

    SetVariantString(vo);
    AcceptEntityInput(gamerules, input);

  } else {
    ThrowError("Could not find a tf_gamerules entity!");
  }

}

/**
 * Counts the players in a team, alive or dead.
 *
 * @param team             Team index.
 * @return                 Number of players.
 */
stock Team_CountPlayers(team) {

  new count = 0;
  for (new i = 1; i <= MaxClients; i++) {
    if (IsValidClient(i) && GetClientTeam(i) == team) {
      count++;
    }
  }
  return count;

}

/**
 * Counts the number of living players in a team.
 *
 * @param team             Team index.
 * @return                 Number of living players.
 */
stock Team_CountAlivePlayers(team) {

   new count = 0;
   for (new i = 1; i <= MaxClients; i++) {
     if (IsValidClient(i) && GetClientTeam(i) == team && IsPlayerAlive(i)) {
       count++;
     }
   }
   return count;

 }

/**
 * Respawns the dead members of a team.
 *
 * @param team
 * @param silent
 * @return
 */
stock Team_RespawnWithEffects(team, silent = false) {

  num_revived_players = 0;
  for (new i = 1; i <= MaxClients ; i++) {
    if (IsValidClient(i)) {
      if (IsClientObserver(i) && GetClientTeam(i) == team) {
        Player_RespawnWithEffects(i);
        num_revived_players++;
      }
    }
  }

  if (num_revived_players > 0 && !silent) {
    new enemy_team = Team_EnemyTeam(team);
    Team_EmitSound(_:TFTeam_Unassigned, sound_friendly_cap);
    Team_EmitSound(_:TFTeam_Spectator,  sound_friendly_cap);
    Team_EmitSound(team,                sound_friendly_cap);
    Team_EmitSound(enemy_team,          sound_enemy_cap);

    roundstat_players_respawned[team - 2] += num_revived_players;
  }

  return num_revived_players;

}
