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

#if defined _respawn_include_objective
  #endinput
#endif
#define _respawn_include_objective

/**
 * Returns index of the team owning a Control Point.
 *
 * @param point            Point index.
 * @return                 The team that owns the control point.
 */
stock Objective_GetPointOwner(point) {

  new objective = Game_GetObjective();
  return GetEntProp(objective, Prop_Data, "m_iOwner", 4, point);

}
