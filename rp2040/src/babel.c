// SPDX-License-Identifier: MIT
#include "babel.h"

#include <string.h>

static const char alphabet[BABEL_ALPHABET_LENGTH] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', ' ', '&', '(',
  ')', '-', '_', '+'
};

static bool rebuild_file(babel_state_t *state) {
  // First evaluate the path as a conventional little-endian base-256 integer:
  // N = (((component[0] * 4900) + component[1]) * 4900 + ...).
  uint8_t integer[BABEL_MAX_FILE_SIZE + 1u] = {0};
  size_t integer_length = 0;
  for (uint32_t i = 0; i < state->depth; ++i) {
    uint32_t carry = 0;
    for (size_t j = 0; j < integer_length; ++j) {
      uint32_t value = (uint32_t)integer[j] * BABEL_DIRECTORY_COUNT + carry;
      integer[j] = (uint8_t)value;
      carry = value >> 8;
    }
    while (carry != 0) {
      if (integer_length >= sizeof(integer)) return false;
      integer[integer_length++] = (uint8_t)carry;
      carry >>= 8;
    }

    uint32_t addend = state->components[i];
    size_t j = 0;
    while (addend != 0) {
      if (j >= sizeof(integer)) return false;
      if (j >= integer_length) integer[integer_length++] = 0;
      uint32_t value = integer[j] + (addend & 0xffu);
      integer[j] = (uint8_t)value;
      addend = (addend >> 8) + (value >> 8);
      ++j;
    }
  }

  // Convert N to bijective base 256. Each output digit is 1..256, stored as
  // a byte 0..255; this is what permits leading zero bytes in represented files.
  state->length = 0;
  size_t first = 0;
  while (first < integer_length) {
    size_t j = first;
    while (j < integer_length && integer[j] == 0) {
      integer[j++] = 0xffu;
    }
    if (j == integer_length) return false;
    --integer[j];
    if (state->length >= BABEL_MAX_FILE_SIZE) return false;
    state->data[state->length++] = integer[first++];
    while (integer_length > first && integer[integer_length - 1u] == 0) --integer_length;
  }
  memset(state->data + state->length, 0, sizeof(state->data) - state->length);
  return true;
}

void babel_init(babel_state_t *state) {
  memset(state, 0, sizeof(*state));
}

void babel_reset_to_root(babel_state_t *state) {
  state->depth = 0;
  state->length = 0;
  memset(state->data, 0, sizeof(state->data));
}

bool babel_handle_valid(uint32_t object_handle) {
  if (object_handle == 0 || object_handle == UINT32_MAX) return false;
  return babel_handle_depth(object_handle) < BABEL_MAX_DEPTH;
}

uint32_t babel_handle_depth(uint32_t object_handle) {
  return (object_handle - 1u) / BABEL_OBJECTS_PER_DIRECTORY;
}

uint32_t babel_local_handle(uint32_t object_handle) {
  return (object_handle - 1u) % BABEL_OBJECTS_PER_DIRECTORY + 1u;
}

bool babel_handle_is_file(uint32_t object_handle) {
  return babel_handle_valid(object_handle) &&
         babel_local_handle(object_handle) == BABEL_FILE_LOCAL_HANDLE;
}

uint32_t babel_child_handle(uint32_t depth, uint32_t local_handle) {
  if (depth >= BABEL_MAX_DEPTH || local_handle == 0 ||
      local_handle > BABEL_OBJECTS_PER_DIRECTORY) return 0;
  return depth * BABEL_OBJECTS_PER_DIRECTORY + local_handle;
}

bool babel_select_directory(babel_state_t *state, uint32_t object_handle) {
  if (!babel_handle_valid(object_handle) || babel_handle_is_file(object_handle)) return false;

  uint32_t selected_depth = babel_handle_depth(object_handle) + 1u;
  if (selected_depth >= BABEL_MAX_DEPTH) return false;
  uint32_t previous_depth = state->depth;
  uint16_t previous_component = state->components[selected_depth - 1u];
  state->components[selected_depth - 1u] = (uint16_t)babel_local_handle(object_handle);
  state->depth = selected_depth;
  if (rebuild_file(state)) return true;
  state->components[selected_depth - 1u] = previous_component;
  state->depth = previous_depth;
  (void)rebuild_file(state);
  return false;
}

uint32_t babel_parent_handle(const babel_state_t *state, uint32_t depth) {
  if (depth == 0 || depth > state->depth) return 0;
  return babel_child_handle(depth - 1u, state->components[depth - 1u]);
}

void babel_directory_name(uint32_t local_handle, uint16_t out[BABEL_NAME_LENGTH + 1u]) {
  uint32_t index = local_handle - 1u;
  for (int i = (int)BABEL_NAME_LENGTH - 1; i >= 0; --i) {
    out[i] = (uint16_t)alphabet[index % BABEL_ALPHABET_LENGTH];
    index /= BABEL_ALPHABET_LENGTH;
  }
  out[BABEL_NAME_LENGTH] = 0;
}

const uint8_t *babel_file_data(const babel_state_t *state) { return state->data; }
size_t babel_file_size(const babel_state_t *state) { return state->length; }
