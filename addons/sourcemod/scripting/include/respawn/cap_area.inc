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

#if defined _respawn_include_cap_area
  #endinput
#endif
#define _respawn_include_cap_area

/**
 * Returns the index of a trigger_capture_area's associated team_control_point.
 *
 * @param cap_area         Index of the trigger_capture_area.
 * @return                 Index of the trigger_capture_area's team_control_point.
 */
stock CapArea_GetControlPoint(cap_area) {

  decl String:control_point_name[64];
  GetEntPropString(cap_area, Prop_Data, "m_iszCapPointName", control_point_name, sizeof(control_point_name));
  return Entity_FindByName(control_point_name, "team_control_point");

}

/*
 * Set the capture time of a trigger_capture_area and reflect changes on the HUD.
 * A netural control point caps twice as slowly as an owned control point.
 *
 * @param cap_area        Index of the trigger_capture_area.
 * @param force_shorter   Always set the shorter (non-netural) capture time.
 * @noreturn
 */
stock CapArea_SetCaptureTime(cap_area, force_shorter = false) {

  // Retrieve the game objective resource to reflect HUD changes.
  new cap_objective = Game_GetObjective();

  // Find the team_control_point associated with this trigger_capture_area.
  new cap_point = CapArea_GetControlPoint(cap_area);

  // Which control point is this? (can be 0-7)
  new point_index = GetEntProp(cap_point, Prop_Data, "m_iPointIndex");

  // Calculate point capture time based on the number of cap points.
  new Float:cap_time = GetConVarFloat(cvar_cap_time) * Game_CountCapPoints() * Game_CountCapPoints();

  // If the point is currently neutral, double the capture time.
  if (!force_shorter && Objective_GetPointOwner(point_index) == 0) {
    cap_time = cap_time * 2;
  }

  // Set point capture time.
  Entity_SetKeyValueFloat(cap_area, "area_time_to_cap", cap_time);

  // Disable team capture multipliers.
  Entity_SetKeyValue(cap_area, "team_numcap_2", 1);
  Entity_SetKeyValue(cap_area, "team_numcap_3", 1);

  // Reflect the new capture time on clients' HUDs.
  SetEntPropFloat(cap_objective, Prop_Data, "m_flTeamCapTime", cap_time * 2, _:TFTeam_Red  * 8 + point_index);
  SetEntPropFloat(cap_objective, Prop_Data, "m_flTeamCapTime", cap_time * 2, _:TFTeam_Blue * 8 + point_index);

}
