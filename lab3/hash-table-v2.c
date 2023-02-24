#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "hash-table-base.h"

struct list_entry {
  const char *key;
  uint32_t value;
  SLIST_ENTRY(list_entry) pointers;
};

SLIST_HEAD(list_head, list_entry);

struct hash_table_entry {
  struct list_head list_head;
  pthread_mutex_t mutex;
};

struct hash_table_v2 {
  struct hash_table_entry entries[HASH_TABLE_CAPACITY];
};

struct hash_table_v2 *hash_table_v2_create() {
  struct hash_table_v2 *hash_table = calloc(1, sizeof(struct hash_table_v2));
  assert(hash_table != NULL);
  for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
    struct hash_table_entry *entry = &hash_table->entries[i];
    SLIST_INIT(&entry->list_head);

    int error = pthread_mutex_init(&entry->mutex, NULL);
    if (error != 0) {
      for (size_t j = 0; j < i; j++) {
        pthread_mutex_destroy(&hash_table->entries[j].mutex);
      }
      exit(error);
    }
  }

  return hash_table;
}

static struct hash_table_entry *get_hash_table_entry(
    struct hash_table_v2 *hash_table, const char *key) {
  assert(key != NULL);
  uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
  struct hash_table_entry *entry = &hash_table->entries[index];
  return entry;
}

// NOLINTNEXTLINE(misc-unused-parameters)
static struct list_entry *get_list_entry(struct hash_table_v2 *hash_table,
                                         const char *key,
                                         struct list_head *list_head) {
  assert(key != NULL);

  struct list_entry *entry = NULL;

  SLIST_FOREACH(entry, list_head, pointers) {
    if (strcmp(entry->key, key) == 0) {
      return entry;
    }
  }
  return NULL;
}

bool hash_table_v2_contains(struct hash_table_v2 *hash_table, const char *key) {
  struct hash_table_entry *hash_table_entry =
      get_hash_table_entry(hash_table, key);
  struct list_head *list_head = &hash_table_entry->list_head;
  struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
  return list_entry != NULL;
}

void hash_table_v2_add_entry(struct hash_table_v2 *hash_table, const char *key,
                             uint32_t value) {
  struct hash_table_entry *hash_table_entry =
      get_hash_table_entry(hash_table, key);

  int error = pthread_mutex_lock(&hash_table_entry->mutex);
  if (error != 0) {
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; i++) {
      pthread_mutex_destroy(&hash_table->entries[i].mutex);
    }
    exit(error);
  }

  struct list_head *list_head = &hash_table_entry->list_head;
  struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);

  /* Update the value if it already exists */
  if (list_entry != NULL) {
    list_entry->value = value;
    return;
  }

  list_entry = calloc(1, sizeof(struct list_entry));
  list_entry->key = key;
  list_entry->value = value;
  SLIST_INSERT_HEAD(list_head, list_entry, pointers);

  error = pthread_mutex_unlock(&hash_table_entry->mutex);
  if (error != 0) {
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; i++) {
      pthread_mutex_destroy(&hash_table->entries[i].mutex);
    }
    exit(error);
  }
}

uint32_t hash_table_v2_get_value(struct hash_table_v2 *hash_table,
                                 const char *key) {
  struct hash_table_entry *hash_table_entry =
      get_hash_table_entry(hash_table, key);
  struct list_head *list_head = &hash_table_entry->list_head;
  struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
  assert(list_entry != NULL);
  return list_entry->value;
}

void hash_table_v2_destroy(struct hash_table_v2 *hash_table) {
  for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
    struct hash_table_entry *entry = &hash_table->entries[i];
    struct list_head *list_head = &entry->list_head;
    struct list_entry *list_entry = NULL;
    while (!SLIST_EMPTY(list_head)) {
      list_entry = SLIST_FIRST(list_head);
      SLIST_REMOVE_HEAD(list_head, pointers);
      free(list_entry);
    }

    int error = pthread_mutex_destroy(&entry->mutex);
    if (error != 0) {
      exit(error);
    }
  }
  free(hash_table);
}
