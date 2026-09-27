// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "babel.h"

static uint32_t local_for_name(char first, char second) {
  const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789! &()-_+";
  const char *a = strchr(alphabet, first);
  const char *b = strchr(alphabet, second);
  assert(a && b);
  return (uint32_t)((a - alphabet) * BABEL_ALPHABET_LENGTH + (b - alphabet) + 1);
}

static void select_name(babel_state_t *state, uint32_t depth, char first, char second) {
  uint32_t handle = babel_child_handle(depth, local_for_name(first, second));
  assert(babel_select_directory(state, handle));
}

static void expect_bytes(const babel_state_t *state, const uint8_t *expected, size_t length) {
  assert(babel_file_size(state) == length);
  assert(memcmp(babel_file_data(state), expected, length) == 0);
}

static void expect_integer(const babel_state_t *state, uint64_t value) {
  uint8_t expected[16];
  size_t length = 0;
  while (value != 0) {
    --value;
    expected[length++] = (uint8_t)value;
    value >>= 8;
  }
  expect_bytes(state, expected, length);
}

int main(void) {
  babel_state_t state;
  babel_init(&state);
  expect_bytes(&state, NULL, 0);

  select_name(&state, 0, 'A', 'A');
  const uint8_t zero[] = {0x00};
  expect_bytes(&state, zero, sizeof(zero));

  babel_reset_to_root(&state);
  select_name(&state, 0, 'A', 'B');
  const uint8_t one[] = {0x01};
  expect_bytes(&state, one, sizeof(one));

  babel_reset_to_root(&state);
  select_name(&state, 0, 'D', 't');
  const uint8_t ff[] = {0xff};
  expect_bytes(&state, ff, sizeof(ff));

  babel_reset_to_root(&state);
  select_name(&state, 0, 'D', 'u');
  const uint8_t two_zeroes[] = {0x00, 0x00};
  expect_bytes(&state, two_zeroes, sizeof(two_zeroes));

  // Going back to an ancestor and selecting a sibling must rebuild, not append.
  babel_reset_to_root(&state);
  select_name(&state, 0, 'A', 'A');
  select_name(&state, 1, 'A', 'A');
  select_name(&state, 0, 'A', 'B');
  expect_bytes(&state, one, sizeof(one));

  uint16_t name[3];
  babel_directory_name(1, name);
  assert(name[0] == 'A' && name[1] == 'A' && name[2] == 0);
  babel_directory_name(BABEL_DIRECTORY_COUNT, name);
  assert(name[0] == '+' && name[1] == '+' && name[2] == 0);

  assert(babel_handle_is_file(BABEL_FILE_LOCAL_HANDLE));
  assert(!babel_handle_is_file(1));
  assert(babel_parent_handle(&state, 0) == 0);

  // Independent 64-bit model checks a spread of multi-level paths.
  for (uint32_t seed = 1; seed < 200; ++seed) {
    babel_reset_to_root(&state);
    uint64_t value = 0;
    for (uint32_t depth = 0; depth < 4; ++depth) {
      uint32_t local = (seed * 977u + depth * 1231u) % BABEL_DIRECTORY_COUNT + 1u;
      assert(babel_select_directory(&state, babel_child_handle(depth, local)));
      value = value * BABEL_DIRECTORY_COUNT + local;
      expect_integer(&state, value);
    }
  }
  puts("babel tests passed");
  return 0;
}
