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
