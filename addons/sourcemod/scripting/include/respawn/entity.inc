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

#if defined _respawn_include_entity
  #endinput
#endif
#define _respawn_include_entity

/**
 * Sets an entity's internal keyvalue.
 *
 * @param entity           Entity index.
 * @param key
 * @param value
 * @return                 True if successful, otherwise false.
 */
stock Entity_SetKeyValue(entity, const String:key[], value) {

  new String:output[64];
  Format(output, sizeof(output), "%s %d", key, value);
  return Entity_AddOutput(entity, output);

}

/**
 * Sets an entity's internal keyvalue.
 *
 * @param entity           Entity index.
 * @param key
 * @param value
 * @return                 True if successful, otherwise false.
 */
stock Entity_SetKeyValueFloat(entity, const String:key[], Float:value) {

  new String:output[64];
  Format(output, sizeof(output), "%s %f", key, value);
  return Entity_AddOutput(entity, output);

}

/**
 * Sets an entity's internal keyvalue.
 *
 * @param entity           Entity index.
 * @param key
 * @param value
 * @return                 True if successful, otherwise false.
 */
stock Entity_SetKeyValueString(entity, const String:key[], const String:value[]) {

  new String:output[64];
  Format(output, sizeof(output), "%s %s", key, value);
  return Entity_AddOutput(entity, output);

}

/**
 * Finds the origin of an entity relative to the world.
 * Contrast with smlib's Entity_GetAbsOrigin, which only finds the origin relative to the entity's parent.
 *
 * @param entity           Entity index.
 * @param vector           3 dimensional vector array.
 * @noreturn
 */
stock Entity_GetTrueAbsOrigin(entity, Float:vector[3]) {

  Entity_GetAbsOrigin(entity, vector);

  new parent = Entity_GetParent(entity);

  if (parent > 0) {
    decl Float:parent_vector[3];
    Entity_GetTrueAbsOrigin(parent, parent_vector);
    AddVectors(vector, parent_vector, vector);
  }

}

/**
 * Returns the entity's minimum coordinates relative to the world.
 *
 * @param entity           Entity index.
 * @param vector           3 dimensional vector array.
 * @noreturn
 */
stock Entity_GetAbsMinSize(entity, Float:vector[3]) {

  decl Float:origin[3];
  Entity_GetTrueAbsOrigin(entity, origin);

  decl Float:relvec[3];
  Entity_GetMinSize(entity, relvec);

  AddVectors(origin, relvec, vector);

}

/**
 * Returns the entity's maximum coordinates relative to the world.
 *
 * @param entity           Entity index.
 * @param vector           3 dimensional vector array.
 * @noreturn
 */
stock Entity_GetAbsMaxSize(entity, Float:vector[3]) {

  decl Float:origin[3];
  Entity_GetTrueAbsOrigin(entity, origin);

  decl Float:relvec[3];
  Entity_GetMaxSize(entity, relvec);

  AddVectors(origin, relvec, vector);

}


/**
 * Adds an entity output using a formatted string.
 *
 * @param entity           Entity index.
 * @param output           Unformatted output string.
 * @noreturn
 */
stock Entity_AddFormattedOutput(ent, const String:output[], any:...) {

  new String:output_string[512];
  VFormat(output_string, sizeof(output_string), output, 3);

  SetVariantString(output_string);
  AcceptEntityInput(ent, "AddOutput");

}

/**
 * Sends an input to all entities of the specified class.
 *
 * @param classname        Classname to search for.
 * @param input            Input to send.
 * @noreturn
 */
stock Entity_AcceptInputAll(const String:classname[], const String:input[]) {

  new ent = -1;
  while ((ent = FindEntityByClassname(ent, classname)) != -1) {
    AcceptEntityInput(ent, input);
    Log("Sending input %s to entity %d", input, ent);
  }

}

/**
 * Checks if the bounding boxes of two entities intersect.
 *
 * @return                 True if the entities intersect.
 */
stock Entity_Intersect(ent1, ent2) {

  new Float:abs_min1[3], Float:abs_min2[3], Float:abs_max1[3], Float:abs_max2[3];

  Entity_GetAbsMinSize(ent1, abs_min1);
  Entity_GetAbsMaxSize(ent1, abs_max1);
  Entity_GetAbsMinSize(ent2, abs_min2);
  Entity_GetAbsMaxSize(ent2, abs_max2);

  if( abs_min1[0] > abs_max2[0] ||
      abs_min1[1] > abs_max2[1] ||
      abs_min1[2] > abs_max2[2] ||
      abs_max1[0] < abs_min2[0] ||
      abs_max1[1] < abs_min2[1] ||
      abs_max1[2] < abs_min2[2] )
      return false;

  return true;

}

/**
 * Removes an old healthkit and spawns a new one in with the same position and
 * rotation as the old one.
 *
 * @param entity           Entity index.
 * @param classname        Classname of the replacement entity.
 * @noreturn
 */
stock Entity_ReplaceWith(entity, const String:classname[]) {

  decl Float:pos[3];
  Entity_GetTrueAbsOrigin(entity, pos);

  decl Float:vec[3];
  Entity_GetAbsAngles(entity, vec);

  Entity_Kill(entity);

  new new_ent = CreateEntityByName(classname);
  DispatchKeyValueVector(new_ent, "origin", pos);
  DispatchKeyValueVector(new_ent, "angles", vec);
  DispatchSpawn(new_ent);

}
