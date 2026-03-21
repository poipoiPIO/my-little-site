#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ==================== Markup ==================== */
#define _CT_TAG(tag, ...) "<" tag ">" __VA_ARGS__ "</" tag ">"
#define _CT_TAG_ATTR(tag, attrs, ...)                                          \
  "<" tag " " attrs ">" __VA_ARGS__ "</" tag ">"
#define _CT_TAG_SELF(tag, attrs) "<" tag " " attrs " />"
#define _CT_TAG_CLASS(tag, class, ...)                                         \
  "<" tag " class=\"" class "\">" __VA_ARGS__ "</" tag ">"

#define _CT_ATTR(key, val) key "=\"" val "\""
#define _CT_ATTR2(key1, val1, key2, val2)                                      \
  key1 "=\"" val1 "\" " key2 "=\"" val2 "\""
#define _CT_ATTR3(key1, val1, key2, val2, key3, val3)                          \
  key1 "=\"" val1 "\" " key2 "=\"" val2 "\" " key3 "=\"" val3 "\""

#define _CT_CSS_RULE(selector, ...) selector " { " __VA_ARGS__ " }\n"
#define _CT_CSS_PROP(prop, value) prop ": " value "; "
#define _CT_CSS_AT(at_rule, ...) at_rule " { " __VA_ARGS__ " }\n"

#define CT_TAG(tag, ...) _CT_TAG(tag, __VA_ARGS__)
#define CT_TAG_ATTR(tag, attrs, ...) _CT_TAG_ATTR(tag, attrs, __VA_ARGS__)
#define CT_TAG_SELF(tag, attrs) _CT_TAG_SELF(tag, attrs)
#define CT_TAG_CLASS(tag, class, ...) _CT_TAG_CLASS(tag, class, __VA_ARGS__)

#define CT_BR() CT_TAG_SELF("br", "")
#define CT_CSS_RULE(selector, ...) _CT_CSS_RULE(selector, __VA_ARGS__)
#define CT_CSS_PROP(prop, value) _CT_CSS_PROP(prop, value)
#define CT_CSS_AT(at_rule, ...) _CT_CSS_AT(at_rule, __VA_ARGS__)

/* ==================== Config ==================== */
#define REQUEST_BUF_SIZE 8192
#define IO_BUF_SIZE 4096
#define PAGE_BUF_SIZE 32768
#define CONTACTS_BUF_SIZE 512
#define CONTENT_BUF_SIZE 1024
#define COUNTER_STR_BUF_SIZE 32
#define SUBMIT_BUTTON_BUF_SIZE 96
#define PATH_BUF_SIZE 2048
#define METHOD_BUF_SIZE 16
#define DEFAULT_PORT 8000
#define DEFAULT_COUNTER_DIR "./data"
#define DEFAULT_COUNTER_FILE "./data/visitor-count.txt"
#define DEFAULT_COMMENTS_WAL "./data/comments.wal"
#define COUNTER_FLUSH_EVERY 100
#define MAX_IP_LEN 64
#define MAX_NICKNAME_LEN 32
#define MAX_MESSAGE_LEN 98
#define MAX_WAL_FIELD_LEN 512
#define MAX_MARQUEE_TEXT_LEN 8192
#define MARQUEE_HTML_BUF_SIZE 9216
#define ARENA_DEFAULT_BLOCK_SIZE 16384
#define DAY_SECONDS 86400
#define WEEK_SECONDS 604800

/* ==================== Arena ==================== */
typedef struct ArenaBlock {
  struct ArenaBlock *next;
  size_t used;
  size_t capacity;
  unsigned char data[];
} ArenaBlock;

typedef struct {
  ArenaBlock *first;
  ArenaBlock *last;
  size_t default_block_size;
} Arena;

static void arena_init(Arena *arena, size_t default_block_size) {
  arena->first = NULL;
  arena->last = NULL;
  arena->default_block_size = default_block_size;
}

static void arena_release(Arena *arena) {
  ArenaBlock *block = arena->first;
  while (block != NULL) {
    ArenaBlock *next = block->next;
    free(block);
    block = next;
  }
  arena->first = NULL;
  arena->last = NULL;
}

static int arena_add_block(Arena *arena, size_t min_capacity) {
  size_t capacity = arena->default_block_size;
  ArenaBlock *block;

  if (capacity < min_capacity) {
    capacity = min_capacity;
  }
  if (capacity > SIZE_MAX - sizeof(ArenaBlock)) {
    return -1;
  }

  block = (ArenaBlock *)malloc(sizeof(ArenaBlock) + capacity);
  if (block == NULL) {
    return -1;
  }

  block->next = NULL;
  block->used = 0;
  block->capacity = capacity;

  if (arena->last != NULL) {
    arena->last->next = block;
  } else {
    arena->first = block;
  }
  arena->last = block;
  return 0;
}

static size_t align_up_size(size_t value, size_t alignment) {
  size_t remainder;
  if (alignment == 0) {
    return value;
  }
  remainder = value % alignment;
  if (remainder == 0) {
    return value;
  }
  if (value > SIZE_MAX - (alignment - remainder)) {
    return SIZE_MAX;
  }
  return value + (alignment - remainder);
}

static void *arena_alloc(Arena *arena, size_t size) {
  const size_t alignment = _Alignof(max_align_t);
  ArenaBlock *block;
  size_t offset;
  size_t min_capacity;

  if (size == 0) {
    size = 1;
  }

  block = arena->last;
  if (block != NULL) {
    offset = align_up_size(block->used, alignment);
    if (offset <= block->capacity && size <= block->capacity - offset) {
      void *ptr = block->data + offset;
      block->used = offset + size;
      return ptr;
    }
  }

  min_capacity = size;
  if (min_capacity <= SIZE_MAX - alignment) {
    min_capacity += alignment;
  } else {
    return NULL;
  }

  if (arena_add_block(arena, min_capacity) != 0) {
    return NULL;
  }

  block = arena->last;
  offset = align_up_size(block->used, alignment);
  if (offset > block->capacity || size > block->capacity - offset) {
    return NULL;
  }

  block->used = offset + size;
  return block->data + offset;
}

static void *arena_calloc(Arena *arena, size_t count, size_t size) {
  size_t total;
  void *ptr;

  if (count == 0 || size == 0) {
    return arena_alloc(arena, 1);
  }
  if (count > SIZE_MAX / size) {
    return NULL;
  }
  total = count * size;
  ptr = arena_alloc(arena, total);
  if (ptr != NULL) {
    memset(ptr, 0, total);
  }
  return ptr;
}

/* ==================== App State ==================== */
static const char *QUOTES[] = {
#include "quotes.inc"
};

static unsigned long COUNTER = 0;
static unsigned long DIRTY_VISITS = 0;

static const char *get_random_quote(void) {
  size_t count = sizeof(QUOTES) / sizeof(QUOTES[0]);
  return QUOTES[rand() % count];
}

static const char *content_type_from_path(const char *path) {
  const char *ext = strrchr(path, '.');
  if (ext == NULL) {
    return "application/octet-stream";
  }

  if (strcmp(ext, ".css") == 0)
    return "text/css; charset=utf-8";
  if (strcmp(ext, ".png") == 0)
    return "image/png";
  if (strcmp(ext, ".webp") == 0)
    return "image/webp";
  if (strcmp(ext, ".ttf") == 0)
    return "font/ttf";
  if (strcmp(ext, ".html") == 0)
    return "text/html; charset=utf-8";

  return "application/octet-stream";
}

/* ==================== Counter ==================== */
static const char *counter_file_path(void) {
  const char *path_from_env = getenv("MEOW_COUNTER_FILE");
  if (path_from_env != NULL && path_from_env[0] != '\0') {
    return path_from_env;
  }
  return DEFAULT_COUNTER_FILE;
}

static const char *comments_wal_path(void) {
  const char *path_from_env = getenv("MEOW_COMMENTS_WAL");
  if (path_from_env != NULL && path_from_env[0] != '\0') {
    return path_from_env;
  }
  return DEFAULT_COMMENTS_WAL;
}

static int ensure_default_counter_dir(void) {
  struct stat st;

  if (stat(DEFAULT_COUNTER_DIR, &st) == 0) {
    return S_ISDIR(st.st_mode) ? 0 : -1;
  }

  if (errno != ENOENT) {
    return -1;
  }

  if (mkdir(DEFAULT_COUNTER_DIR, 0755) == 0 || errno == EEXIST) {
    return 0;
  }

  return -1;
}

static int prepare_data_storage_for_path(const char *path) {
  if ((strcmp(path, DEFAULT_COUNTER_FILE) == 0 ||
       strcmp(path, DEFAULT_COMMENTS_WAL) == 0) &&
      ensure_default_counter_dir() != 0) {
    return -1;
  }

  return 0;
}

static int load_counter(void) {
  const char *path = counter_file_path();
  FILE *fp;
  unsigned long value = 0;

  if (prepare_data_storage_for_path(path) != 0) {
    return -1;
  }

  fp = fopen(path, "r");
  if (fp == NULL) {
    if (errno == ENOENT) {
      COUNTER = 0;
      return 0;
    }
    return -1;
  }

  if (fscanf(fp, "%lu", &value) == 1) {
    COUNTER = value;
  } else {
    COUNTER = 0;
  }

  fclose(fp);
  return 0;
}

static int save_counter(void) {
  const char *path = counter_file_path();
  char tmp_path[PATH_BUF_SIZE];
  FILE *fp;
  int n;

  if (prepare_data_storage_for_path(path) != 0) {
    return -1;
  }

  n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
  if (n < 0 || (size_t)n >= sizeof(tmp_path)) {
    return -1;
  }

  fp = fopen(tmp_path, "w");
  if (fp == NULL) {
    return -1;
  }

  if (fprintf(fp, "%lu\n", COUNTER) < 0) {
    fclose(fp);
    remove(tmp_path);
    return -1;
  }

  if (fflush(fp) != 0) {
    fclose(fp);
    remove(tmp_path);
    return -1;
  }

  if (fclose(fp) != 0) {
    remove(tmp_path);
    return -1;
  }

  if (rename(tmp_path, path) != 0) {
    remove(tmp_path);
    return -1;
  }

  return 0;
}

static int flush_counter_if_needed(int force) {
  if (!force && DIRTY_VISITS < COUNTER_FLUSH_EVERY) {
    return 0;
  }

  if (DIRTY_VISITS == 0) {
    return 0;
  }

  if (save_counter() != 0) {
    return -1;
  }

  DIRTY_VISITS = 0;
  return 0;
}

/* ==================== Comments ==================== */
typedef struct {
  char ip[MAX_IP_LEN];
  time_t timestamp;
  char nickname[MAX_NICKNAME_LEN + 1];
  char message[MAX_MESSAGE_LEN + 1];
} MarqueeEntry;

static int parse_hex_byte(const char *src, unsigned char *out) {
  char hex_pair[3];
  char *endptr = NULL;
  long value;

  hex_pair[0] = src[0];
  hex_pair[1] = src[1];
  hex_pair[2] = '\0';

  errno = 0;
  value = strtol(hex_pair, &endptr, 16);
  if (errno != 0 || endptr != hex_pair + 2 || value < 0 || value > 255) {
    return -1;
  }

  *out = (unsigned char)value;
  return 0;
}

static int url_decode_to_buffer(const char *src, size_t src_len, char *dst,
                                size_t dst_size, int plus_as_space) {
  size_t si;
  size_t di = 0;

  if (dst_size == 0) {
    return -1;
  }

  for (si = 0; si < src_len; si++) {
    unsigned char ch = (unsigned char)src[si];
    if (ch == '%' && si + 2 < src_len) {
      if (parse_hex_byte(src + si + 1, &ch) != 0) {
        return -1;
      }
      si += 2;
    } else if (plus_as_space && ch == '+') {
      ch = ' ';
    }

    if (di + 1 >= dst_size) {
      return -1;
    }
    dst[di++] = (char)ch;
  }

  dst[di] = '\0';
  return 0;
}

static int url_encode_to_buffer(const char *src, char *dst, size_t dst_size) {
  static const char HEX[] = "0123456789ABCDEF";
  size_t si;
  size_t di = 0;

  if (dst_size == 0) {
    return -1;
  }

  for (si = 0; src[si] != '\0'; si++) {
    unsigned char ch = (unsigned char)src[si];
    if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
      if (di + 1 >= dst_size) {
        return -1;
      }
      dst[di++] = (char)ch;
      continue;
    }

    if (di + 3 >= dst_size) {
      return -1;
    }
    dst[di++] = '%';
    dst[di++] = HEX[(ch >> 4) & 0xF];
    dst[di++] = HEX[ch & 0xF];
  }

  dst[di] = '\0';
  return 0;
}

static void trim_in_place(char *text) {
  static const char *WS = " \t\n\r\f\v";
  size_t start = strspn(text, WS);
  size_t end;
  size_t len;

  if (start > 0) {
    memmove(text, text + start, strlen(text + start) + 1);
  }

  len = strlen(text);
  end = len;
  while (end > 0 && isspace((unsigned char)text[end - 1])) {
    end--;
  }
  text[end] = '\0';
}

static int html_escape_to_buffer(const char *src, char *dst, size_t dst_size) {
  size_t di = 0;
  size_t si;

  if (dst_size == 0) {
    return -1;
  }

  for (si = 0; src[si] != '\0'; si++) {
    const char *entity = NULL;
    size_t entity_len = 0;
    char ch = src[si];

    if (ch == '&') {
      entity = "&amp;";
    } else if (ch == '<') {
      entity = "&lt;";
    } else if (ch == '>') {
      entity = "&gt;";
    } else if (ch == '"') {
      entity = "&quot;";
    } else if (ch == '\'') {
      entity = "&#39;";
    }

    if (entity != NULL) {
      entity_len = strlen(entity);
      if (di + entity_len >= dst_size) {
        return -1;
      }
      memcpy(dst + di, entity, entity_len);
      di += entity_len;
    } else {
      if (di + 1 >= dst_size) {
        return -1;
      }
      dst[di++] = ch;
    }
  }

  dst[di] = '\0';
  return 0;
}

static int parse_form_fields(const char *body, size_t body_len, char *nickname,
                             size_t nickname_size, char *message,
                             size_t message_size) {
  size_t pos = 0;
  char key[64];
  char value[MAX_WAL_FIELD_LEN];
  int has_nickname = 0;
  int has_message = 0;

  nickname[0] = '\0';
  message[0] = '\0';

  while (pos < body_len) {
    size_t pair_end = pos;
    size_t eq_pos;

    while (pair_end < body_len && body[pair_end] != '&') {
      pair_end++;
    }

    eq_pos = pos;
    while (eq_pos < pair_end && body[eq_pos] != '=') {
      eq_pos++;
    }

    if (eq_pos < pair_end) {
      if (url_decode_to_buffer(body + pos, eq_pos - pos, key, sizeof(key), 1) !=
          0) {
        return -1;
      }
      if (url_decode_to_buffer(body + eq_pos + 1, pair_end - eq_pos - 1, value,
                               sizeof(value), 1) != 0) {
        return -1;
      }

      trim_in_place(value);

      if (strcmp(key, "nickname") == 0) {
        if (strlen(value) > MAX_NICKNAME_LEN ||
            strlen(value) + 1 > nickname_size) {
          return -1;
        }
        strcpy(nickname, value);
        has_nickname = 1;
      } else if (strcmp(key, "message") == 0) {
        if (strlen(value) > MAX_MESSAGE_LEN ||
            strlen(value) + 1 > message_size) {
          return -1;
        }
        strcpy(message, value);
        has_message = 1;
      }
    }

    pos = pair_end + (pair_end < body_len ? 1 : 0);
  }

  if (!has_nickname || !has_message || nickname[0] == '\0' ||
      message[0] == '\0') {
    return -1;
  }

  return 0;
}

static int append_comment_to_wal(const char *ip, const char *nickname,
                                 const char *message, time_t now) {
  const char *path = comments_wal_path();
  FILE *fp;
  char encoded_nickname[MAX_WAL_FIELD_LEN];
  char encoded_message[MAX_WAL_FIELD_LEN];

  if (prepare_data_storage_for_path(path) != 0) {
    return -1;
  }

  if (url_encode_to_buffer(nickname, encoded_nickname,
                           sizeof(encoded_nickname)) != 0) {
    return -1;
  }
  if (url_encode_to_buffer(message, encoded_message, sizeof(encoded_message)) !=
      0) {
    return -1;
  }

  fp = fopen(path, "a");
  if (fp == NULL) {
    return -1;
  }

  if (fprintf(fp, "%lld\t%s\t%s\t%s\n", (long long)now, ip, encoded_nickname,
              encoded_message) < 0) {
    fclose(fp);
    return -1;
  }

  if (fclose(fp) != 0) {
    return -1;
  }

  return 0;
}

static int read_last_submission_time_for_ip(const char *ip, time_t *out_last) {
  const char *path = comments_wal_path();
  FILE *fp;
  char line[4096];
  time_t last = 0;

  *out_last = 0;
  fp = fopen(path, "r");
  if (fp == NULL) {
    if (errno == ENOENT) {
      return 0;
    }
    return -1;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    char *save_ptr = NULL;
    char *timestamp_s = strtok_r(line, "\t", &save_ptr);
    char *ip_s = strtok_r(NULL, "\t", &save_ptr);
    if (timestamp_s == NULL || ip_s == NULL) {
      continue;
    }

    if (strcmp(ip_s, ip) == 0) {
      long long ts_ll = strtoll(timestamp_s, NULL, 10);
      if (ts_ll > 0 && (time_t)ts_ll > last) {
        last = (time_t)ts_ll;
      }
    }
  }

  fclose(fp);
  *out_last = last;
  return 0;
}

static int can_submit_comment_from_ip(const char *ip, time_t now) {
  time_t last = 0;
  if (read_last_submission_time_for_ip(ip, &last) != 0) {
    return 1;
  }
  if (last == 0) {
    return 1;
  }
  if (now < last) {
    return 0;
  }
  return (now - last) >= DAY_SECONDS;
}

static int find_marquee_entry_index(MarqueeEntry *entries, size_t count,
                                    const char *ip) {
  size_t i;
  for (i = 0; i < count; i++) {
    if (strcmp(entries[i].ip, ip) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int compare_marquee_entries_desc(const void *a, const void *b) {
  const MarqueeEntry *ea = (const MarqueeEntry *)a;
  const MarqueeEntry *eb = (const MarqueeEntry *)b;

  if (ea->timestamp < eb->timestamp) {
    return 1;
  }
  if (ea->timestamp > eb->timestamp) {
    return -1;
  }
  return 0;
}

static int parse_wal_record_line(char *line, time_t week_ago,
                                 MarqueeEntry *out_entry) {
  char *save_ptr = NULL;
  char *timestamp_s = strtok_r(line, "\t", &save_ptr);
  char *ip_s = strtok_r(NULL, "\t", &save_ptr);
  char *nickname_s = strtok_r(NULL, "\t", &save_ptr);
  char *message_s = strtok_r(NULL, "\t", &save_ptr);
  long long ts_ll;
  size_t message_len;

  if (timestamp_s == NULL || ip_s == NULL || nickname_s == NULL ||
      message_s == NULL) {
    return -1;
  }

  ts_ll = strtoll(timestamp_s, NULL, 10);
  if (ts_ll <= 0 || (time_t)ts_ll < week_ago) {
    return -1;
  }

  message_len = strcspn(message_s, "\r\n");
  message_s[message_len] = '\0';

  if (url_decode_to_buffer(nickname_s, strlen(nickname_s), out_entry->nickname,
                           sizeof(out_entry->nickname), 0) != 0 ||
      url_decode_to_buffer(message_s, strlen(message_s), out_entry->message,
                           sizeof(out_entry->message), 0) != 0) {
    return -1;
  }

  strncpy(out_entry->ip, ip_s, sizeof(out_entry->ip) - 1);
  out_entry->ip[sizeof(out_entry->ip) - 1] = '\0';
  out_entry->timestamp = (time_t)ts_ll;
  return 0;
}

static int upsert_latest_entry(Arena *arena, MarqueeEntry **entries,
                               size_t *count, size_t *capacity,
                               const MarqueeEntry *candidate) {
  int idx = find_marquee_entry_index(*entries, *count, candidate->ip);

  if (idx >= 0) {
    if (candidate->timestamp >= (*entries)[idx].timestamp) {
      (*entries)[idx] = *candidate;
    }
    return 0;
  }

  if (*count == *capacity) {
    size_t new_capacity = *capacity == 0 ? 8 : (*capacity * 2);
    MarqueeEntry *new_entries =
        arena_calloc(arena, new_capacity, sizeof(MarqueeEntry));
    if (new_entries == NULL) {
      return -1;
    }
    if (*entries != NULL && *count > 0) {
      memcpy(new_entries, *entries, *count * sizeof(MarqueeEntry));
    }
    *entries = new_entries;
    *capacity = new_capacity;
  }

  (*entries)[*count] = *candidate;
  (*count)++;
  return 0;
}

static int collect_weekly_latest_entries(Arena *arena, time_t now,
                                         MarqueeEntry **out_entries,
                                         size_t *out_count) {
  const char *path = comments_wal_path();
  FILE *fp;
  char line[4096];
  MarqueeEntry *entries = NULL;
  size_t count = 0;
  size_t capacity = 0;
  time_t week_ago = now - WEEK_SECONDS;

  *out_entries = NULL;
  *out_count = 0;

  fp = fopen(path, "r");
  if (fp == NULL) {
    if (errno == ENOENT) {
      return 0;
    }
    return -1;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    MarqueeEntry candidate;
    if (parse_wal_record_line(line, week_ago, &candidate) != 0) {
      continue;
    }
    if (upsert_latest_entry(arena, &entries, &count, &capacity, &candidate) !=
        0) {
      fclose(fp);
      return -1;
    }
  }

  fclose(fp);
  if (count > 1) {
    qsort(entries, count, sizeof(MarqueeEntry), compare_marquee_entries_desc);
  }
  *out_entries = entries;
  *out_count = count;
  return 0;
}

static int render_marquee_entries(Arena *arena, const MarqueeEntry *entries,
                                  size_t count, char *out, size_t out_size) {
  size_t i;
  size_t used = 0;
  char *escaped_nickname;
  char *escaped_message;

  if (out_size == 0) {
    return -1;
  }
  out[0] = '\0';

  if (count == 0) {
    return 0;
  }

  escaped_nickname = arena_alloc(arena, (MAX_NICKNAME_LEN + 1) * 6);
  escaped_message = arena_alloc(arena, (MAX_MESSAGE_LEN + 1) * 6);
  if (escaped_nickname == NULL || escaped_message == NULL) {
    return -1;
  }

  for (i = 0; i < count; i++) {
    int written;

    if (html_escape_to_buffer(entries[i].nickname, escaped_nickname,
                              (MAX_NICKNAME_LEN + 1) * 6) != 0 ||
        html_escape_to_buffer(entries[i].message, escaped_message,
                              (MAX_MESSAGE_LEN + 1) * 6) != 0) {
      continue;
    }

    written = snprintf(
        out + used, out_size - used, "%s<span class=\"sharp\">%s</span>: %s",
        used == 0 ? "" : " | ", escaped_nickname, escaped_message);
    if (written < 0 || (size_t)written >= out_size - used) {
      break;
    }
    used += (size_t)written;
  }

  if (used == 0) {
    out[0] = '\0';
  }
  return 0;
}

static int build_marquee_text(Arena *arena, char *out, size_t out_size,
                              time_t now, int *out_has_messages) {
  MarqueeEntry *entries = NULL;
  size_t count = 0;
  int rc;

  if (out_size == 0 || out_has_messages == NULL) {
    return -1;
  }
  *out_has_messages = 0;
  out[0] = '\0';

  if (collect_weekly_latest_entries(arena, now, &entries, &count) != 0) {
    return -1;
  }

  rc = render_marquee_entries(arena, entries, count, out, out_size);
  if (rc != 0 || out[0] == '\0') {
    return rc;
  }
  *out_has_messages = 1;
  return 0;
}

static void get_client_ip(const struct sockaddr *addr, char *out,
                          size_t out_size) {
  const void *src = NULL;
  if (out_size == 0) {
    return;
  }

  if (addr->sa_family == AF_INET) {
    src = &((const struct sockaddr_in *)addr)->sin_addr;
  } else if (addr->sa_family == AF_INET6) {
    src = &((const struct sockaddr_in6 *)addr)->sin6_addr;
  }

  if (src == NULL || inet_ntop(addr->sa_family, src, out, out_size) == NULL) {
    strncpy(out, "unknown", out_size - 1);
    out[out_size - 1] = '\0';
  }
}

static int is_valid_ip_literal(const char *ip) {
  struct in_addr addr4;
  struct in6_addr addr6;
  return inet_pton(AF_INET, ip, &addr4) == 1 ||
         inet_pton(AF_INET6, ip, &addr6) == 1;
}

static int copy_trimmed_token(const char *src, size_t src_len, char *dst,
                              size_t dst_size) {
  size_t start = 0;
  size_t end = src_len;
  size_t out_len;

  if (dst_size == 0) {
    return -1;
  }

  while (start < src_len && isspace((unsigned char)src[start])) {
    start++;
  }
  while (end > start && isspace((unsigned char)src[end - 1])) {
    end--;
  }

  out_len = end - start;
  if (out_len == 0 || out_len + 1 > dst_size) {
    return -1;
  }

  memcpy(dst, src + start, out_len);
  dst[out_len] = '\0';
  return 0;
}

static int find_header_value(const char *request, size_t headers_end,
                             const char *header_name,
                             const char **out_value_start,
                             size_t *out_value_len) {
  const char *p = request;
  const char *end = request + headers_end;
  size_t header_name_len = strlen(header_name);

  while (p < end && *p != '\n') {
    p++;
  }
  if (p < end) {
    p++;
  }

  while (p < end) {
    const char *line_end = p;
    size_t line_len;
    const char *value_start;

    while (line_end < end && *line_end != '\n') {
      line_end++;
    }

    line_len = (size_t)(line_end - p);
    if (line_len > 0 && p[line_len - 1] == '\r') {
      line_len--;
    }
    if (line_len == 0) {
      break;
    }

    if (line_len > header_name_len &&
        strncasecmp(p, header_name, header_name_len) == 0 &&
        p[header_name_len] == ':') {
      value_start = p + header_name_len + 1;
      while (value_start < p + line_len &&
             isspace((unsigned char)*value_start)) {
        value_start++;
      }

      *out_value_start = value_start;
      *out_value_len = (size_t)((p + line_len) - value_start);
      return 0;
    }

    p = line_end < end ? line_end + 1 : end;
  }

  return -1;
}

static void resolve_client_ip(const char *request, size_t headers_end,
                              const char *fallback_ip, char *out,
                              size_t out_size) {
  const char *value_start = NULL;
  size_t value_len = 0;
  char candidate[MAX_IP_LEN];

  if (out_size == 0) {
    return;
  }

  if (find_header_value(request, headers_end, "X-Forwarded-For", &value_start,
                        &value_len) == 0) {
    const char *comma = memchr(value_start, ',', value_len);
    size_t first_len =
        comma != NULL ? (size_t)(comma - value_start) : value_len;

    if (copy_trimmed_token(value_start, first_len, candidate,
                           sizeof(candidate)) == 0 &&
        is_valid_ip_literal(candidate)) {
      strncpy(out, candidate, out_size - 1);
      out[out_size - 1] = '\0';
      return;
    }
  }

  if (find_header_value(request, headers_end, "X-Real-IP", &value_start,
                        &value_len) == 0) {
    if (copy_trimmed_token(value_start, value_len, candidate,
                           sizeof(candidate)) == 0 &&
        is_valid_ip_literal(candidate)) {
      strncpy(out, candidate, out_size - 1);
      out[out_size - 1] = '\0';
      return;
    }
  }

  strncpy(out, fallback_ip, out_size - 1);
  out[out_size - 1] = '\0';
}

/* ==================== Markup Fragments ==================== */
#define BUILD_CONTACTS()                                                       \
  CT_TAG_ATTR("a", _CT_ATTR("href", "https://www.github.com/poipoiPIO"),       \
              "Github | ")                                                     \
  CT_TAG_ATTR("a", _CT_ATTR("href", "https://t.me/lilyape"), "Telegram | ")    \
  CT_TAG_ATTR(                                                                 \
      "a",                                                                     \
      _CT_ATTR("href",                                                         \
               "https://www.linkedin.com/in/kirill-gnapovskii-b96b1a292"),     \
      "LinkedIn | ")                                                           \
  CT_TAG_ATTR("a", _CT_ATTR("href", "mailto:lappee@yahoo.com"), "Email!")

#define BUILD_CONTENT()                                                        \
  CT_TAG_CLASS("div", "paragraph",                                             \
               CT_TAG("h4", CT_TAG_CLASS("span", "sharp", "#") "About me:")    \
                   CT_TAG("p", "A bookworm software engineer curious about "   \
                               "in-depth details of the surrounding world."))  \
  CT_TAG_CLASS(                                                                \
      "div", "paragraph",                                                      \
      CT_TAG("h4", CT_TAG_CLASS("span", "sharp", "#") "Things I like:")        \
          CT_TAG("ul", CT_TAG("li", "System Programming")                      \
                           CT_TAG("li", "Software engineering")                \
                               CT_TAG("li", "Fishing")))

#define BUILD_PAGE_HEAD()                                                      \
  CT_TAG("head",                                                               \
         CT_TAG_SELF("meta", _CT_ATTR("charset", "utf-8"))                    \
             CT_TAG_SELF("meta", _CT_ATTR2("name", "viewport", "content",     \
                                           "width=device-width, initial-scale=1")) \
                 CT_TAG_SELF("link",                                           \
                             _CT_ATTR3("rel", "stylesheet", "type", "text/css", \
                                       "href", "./static/main.css"))           \
                     CT_TAG_SELF("link",                                       \
                                 _CT_ATTR2("rel", "prefetch", "href",          \
                                           "https://avatars.githubusercontent.com/u/82707867")) \
                         CT_TAG_SELF("link",                                   \
                                     _CT_ATTR3("rel", "icon", "type",          \
                                               "image/x-icon", "href",         \
                                               "./static/images/favicon-32x32.png")) \
                             CT_TAG("title", "lappee-site &lt;3"))

#define BUILD_MARQUEE_BAR()                                                    \
  CT_TAG_CLASS("div", "latest-comments",                                       \
               CT_TAG_ATTR("marquee",                                          \
                           _CT_ATTR3("behavior", "scroll", "direction", "left", \
                                     "scrollamount", "4"),                     \
                           "%s"))

#define BUILD_COMMENT_FLOATER()                                                \
  CT_TAG_CLASS(                                                                \
      "div", "comment-floater",                                                \
      CT_TAG_CLASS("div", "comment-warning",                                   \
                   CT_TAG("strong", "Don&#39;t be rude!")                      \
                       CT_TAG_CLASS("div", "comment-warning-subtext",          \
                                    "You only have one attempt in 24h"))       \
          CT_TAG_ATTR("form",                                                  \
                      "method=\"post\" action=\"/comment\" class=\"comment-form\"", \
                      CT_TAG_SELF("input",                                     \
                                  "type=\"text\" name=\"nickname\" placeholder=\"nickname\" maxlength=\"32\" required") \
                          CT_TAG_ATTR("textarea",                              \
                                      "name=\"message\" placeholder=\"your message of the day...!\" maxlength=\"98\" required", \
                                      "")                                       \
                          "%s"))

#define BUILD_CHEER_BLOCK()                                                    \
  CT_TAG_CLASS("div", "cheer-me-up",                                           \
               CT_TAG_ATTR("a", _CT_ATTR("href", ""),                          \
                           CT_TAG_SELF("img",                                  \
                                       _CT_ATTR2("src", "./static/images/trout.png", \
                                                 "alt", "CheerMeUp")))         \
                   CT_TAG("div", "%s"))

#define BUILD_MAIN_PANEL()                                                     \
  CT_TAG_CLASS(                                                                \
      "div", "body",                                                           \
      CT_TAG_CLASS(                                                            \
          "div", "wrapper",                                                    \
          CT_TAG("header",                                                     \
                 CT_TAG_CLASS("div", "info",                                   \
                              CT_TAG("h1", "Lappely")                          \
                                  CT_TAG("b",                                  \
                                         CT_TAG("em", "19 y.o | He/Him | Rus") \
                                             CT_BR()                            \
                                             "&#9731; Somehow a software engineer!") \
                                      CT_BR() CT_BR()                          \
                                          "%s"                                 \
                                          CT_TAG_CLASS("div", "cattoes", ""))  \
                     CT_TAG_CLASS("div", "avatar",                             \
                                  CT_TAG_SELF("img",                           \
                                              _CT_ATTR3("src",                 \
                                                        "https://avatars.githubusercontent.com/u/82707867", \
                                                        "alt", "Profile image", \
                                                        "class", "avatar-image")))) \
              CT_TAG("section", "%s")                                          \
                  CT_TAG("footer",                                             \
                         CT_TAG("sub", "You are our very %sth visitor!")       \
                             CT_TAG_CLASS(                                     \
                                 "sub", "right-footer",                        \
                                 "This site is made with a forgotten granny technology " \
                                     CT_TAG_ATTR("a",                          \
                                                 _CT_ATTR("href",               \
                                                          "https://www.github.com/poipoiPIO/my-little-site"), \
                                                 "Link!")))))

#define BUILD_PAGE()                                                           \
  "<!DOCTYPE html>"                                                            \
  CT_TAG("html",                                                               \
         BUILD_PAGE_HEAD()                                                     \
             CT_TAG("body", "%s" BUILD_COMMENT_FLOATER()                      \
                                 BUILD_CHEER_BLOCK() BUILD_MAIN_PANEL()))

/* ==================== Markup Styles ==================== */
#define BUILD_CSS()                                                            \
  CT_CSS_RULE("@font-face", CT_CSS_PROP("font-family", "gdk9")                 \
                                CT_CSS_PROP("src", "url(./fonts/gdk9.ttf)"))   \
  CT_CSS_RULE("@font-face", CT_CSS_PROP("font-family", "pf7")                  \
                                CT_CSS_PROP("src", "url(./fonts/pf7.ttf)"))    \
  CT_CSS_RULE("@font-face", CT_CSS_PROP("font-family", "mplus")                \
                                CT_CSS_PROP("src", "url(./fonts/mplus.ttf)"))  \
  CT_CSS_RULE("body",                                                          \
              CT_CSS_PROP("background-image", "url('./images/bg.png')")        \
                  CT_CSS_PROP("padding", "min(5vw, 5vh)")                      \
                      CT_CSS_PROP("background-repeat", "repeat"))              \
  CT_CSS_RULE(".cheer-me-up", CT_CSS_PROP("position", "fixed")                 \
                                  CT_CSS_PROP("font-family", "gdk9")           \
                                      CT_CSS_PROP("bottom", "2rem")            \
                                          CT_CSS_PROP("display", "none")       \
                                              CT_CSS_PROP("height", "10vw"))   \
  CT_CSS_RULE(".cheer-me-up>div",                                              \
              CT_CSS_PROP("width", "220px") CT_CSS_PROP("text-align", "left")) \
  CT_CSS_RULE(".cheer-me-up>a>img", CT_CSS_PROP("height", "100%"))             \
  CT_CSS_RULE(".latest-comments", CT_CSS_PROP("display", "none"))              \
  CT_CSS_RULE(".latest-comments marquee",                                      \
              CT_CSS_PROP("font-family", "gdk9")                               \
                  CT_CSS_PROP("font-size", "17px")                             \
                      CT_CSS_PROP("padding", "0.45rem 0.8rem"))                \
  CT_CSS_RULE(".comment-floater", CT_CSS_PROP("display", "none")               \
                                      CT_CSS_PROP("font-family", "gdk9")       \
                                          CT_CSS_PROP("padding", "0.75rem")    \
                                              CT_CSS_PROP("width", "17rem")    \
                                                  CT_CSS_PROP("z-index", "4")) \
  CT_CSS_RULE(".comment-warning", CT_CSS_PROP("font-size", "14px")             \
                                      CT_CSS_PROP("margin-bottom", "0.5rem")  \
                                          CT_CSS_PROP("text-align", "center")) \
  CT_CSS_RULE(".comment-warning strong", CT_CSS_PROP("display", "block"))      \
  CT_CSS_RULE(".comment-warning-subtext",                                       \
              CT_CSS_PROP("font-size", "12px")                                 \
                  CT_CSS_PROP("margin-top", "0.1rem"))                         \
  CT_CSS_RULE(".comment-form", CT_CSS_PROP("display", "flex")                  \
                                   CT_CSS_PROP("flex-direction", "column")     \
                                       CT_CSS_PROP("gap", "0.35rem"))          \
  CT_CSS_RULE(                                                                 \
      ".comment-form input,.comment-form textarea,.comment-form button",       \
      CT_CSS_PROP("font-family", "gdk9") CT_CSS_PROP("font-size", "14px")      \
          CT_CSS_PROP("padding", "0.35rem 0.5rem")                             \
              CT_CSS_PROP("border", "0.25rem ridge rgba(211, 220, 50, .6)")    \
                  CT_CSS_PROP("background-color", "#ffffe0"))                  \
  CT_CSS_RULE(".comment-form textarea",                                        \
              CT_CSS_PROP("resize", "vertical")                                \
                  CT_CSS_PROP("min-height", "3.5rem")                          \
                      CT_CSS_PROP("max-height", "8rem"))                       \
  CT_CSS_RULE(".comment-form button[disabled]",                                \
              CT_CSS_PROP("opacity", "0.5")                                    \
                  CT_CSS_PROP("cursor", "not-allowed"))                        \
  CT_CSS_RULE(                                                                 \
      ".wrapper",                                                              \
      CT_CSS_PROP("font-family", "mplus") CT_CSS_PROP("font-size", "14px")     \
          CT_CSS_PROP("padding", "min(1vw, 1vh)") CT_CSS_PROP(                 \
              "background-color", "#ffffe0") CT_CSS_PROP("opacity", "0.9")     \
              CT_CSS_PROP("width", "70vw")                                     \
                  CT_CSS_PROP("border", "0.75rem ridge rgba(211, 220, 50, "    \
                                        ".6)"))                                \
  CT_CSS_RULE(                                                                 \
      "header",                                                                \
      CT_CSS_PROP("display", "flex") CT_CSS_PROP("flex-direction", "row")      \
          CT_CSS_PROP("justify-content", "space-between")                      \
              CT_CSS_PROP("gap", "1rem") CT_CSS_PROP("padding", "0.5rem 1rem") \
                  CT_CSS_PROP("font-family", "pf7"))                           \
  CT_CSS_RULE("header>.avatar>img",                                            \
              CT_CSS_PROP("width", "90%")                                      \
                  CT_CSS_PROP("border", "1rem ridge rgba(211, 220, 50, .6)"))  \
  CT_CSS_RULE("footer", CT_CSS_PROP("display", "flex")                         \
                            CT_CSS_PROP("justify-content", "space-between"))   \
  CT_CSS_RULE(".right-footer", CT_CSS_PROP("text-align", "end"))               \
  CT_CSS_RULE("a", CT_CSS_PROP("text-decoration", "none"))                     \
  CT_CSS_RULE("h1", CT_CSS_PROP("margin-bottom", "8px"))                       \
  CT_CSS_RULE("h4", CT_CSS_PROP("margin-bottom", "0.3rem"))                    \
  CT_CSS_RULE(".body", CT_CSS_PROP("display", "flex")                          \
                           CT_CSS_PROP("align-items", "center")                \
                               CT_CSS_PROP("justify-content", "center"))       \
  CT_CSS_RULE(".width-100", CT_CSS_PROP("width", "100%"))                      \
  CT_CSS_RULE(".width-75", CT_CSS_PROP("width", "75%"))                        \
  CT_CSS_RULE(".logo", CT_CSS_PROP("text-align", "left"))                      \
  CT_CSS_RULE(".sharp", CT_CSS_PROP("color", "blue"))                          \
  CT_CSS_RULE(".paragraph", CT_CSS_PROP("margin", "5px 0px 10px 13px")         \
                                CT_CSS_PROP("font-family", "gdk9"))            \
  CT_CSS_RULE(".paragraph>p", CT_CSS_PROP("margin-left", "2rem")               \
                                  CT_CSS_PROP("text-indent", "2em")            \
                                      CT_CSS_PROP("margin-right", "2rem"))     \
  CT_CSS_RULE(                                                                 \
      ".cattoes",                                                              \
      CT_CSS_PROP("height", "4.2rem") CT_CSS_PROP("margin-top", "0.5rem")      \
          CT_CSS_PROP("width", "14rem") CT_CSS_PROP(                           \
              "background", "url('./images/catto-bg.webp') 0 0.1rem "          \
                            "repeat") CT_CSS_PROP("background-size", "17rem")) \
  CT_CSS_RULE("::marker", CT_CSS_PROP("color", "grey"))                        \
  CT_CSS_AT(                                                                   \
      "@keyframes float",                                                      \
      CT_CSS_RULE("0%", CT_CSS_PROP("transform", "translatey(0px)"))           \
          CT_CSS_RULE("50%", CT_CSS_PROP("transform", "translatey(-20px)"))    \
              CT_CSS_RULE("100%",                                              \
                          CT_CSS_PROP("transform", "translatey(0px)")))        \
  CT_CSS_AT(                                                                   \
      "@media only screen and (min-width: 1320px)",                            \
      CT_CSS_RULE(".cheer-me-up", CT_CSS_PROP("display", "flex")) CT_CSS_RULE( \
          ".cheer-me-up",                                                      \
          CT_CSS_PROP("animation", "float 5s ease-in-out infinite"))           \
          CT_CSS_RULE(                                                         \
              ".latest-comments",                                              \
              CT_CSS_PROP("display", "block") CT_CSS_PROP("position", "fixed") \
                  CT_CSS_PROP("top", "0.75rem") CT_CSS_PROP("left", "50%")     \
                      CT_CSS_PROP("transform", "translateX(-50%)")             \
                          CT_CSS_PROP("width", "68vw")                         \
                              CT_CSS_PROP("max-width", "940px")                \
                                  CT_CSS_PROP("background-color", "#ffffe0")   \
                                      CT_CSS_PROP("border", "0.75rem ridge "   \
                                                            "rgba(211, 220, "  \
                                                            "50, .6)")         \
                                          CT_CSS_PROP("opacity", "0.92")       \
                                              CT_CSS_PROP("z-index", "5"))     \
              CT_CSS_RULE(".body", CT_CSS_PROP("margin-top", "4.8rem"))        \
                  CT_CSS_RULE(                                                 \
                      ".comment-floater",                                      \
                      CT_CSS_PROP("display", "block") CT_CSS_PROP(             \
                          "position", "fixed") CT_CSS_PROP("bottom", "1.3rem") \
                          CT_CSS_PROP("right", "1.3rem") CT_CSS_PROP(          \
                              "background-color", "#ffffe0")                   \
                              CT_CSS_PROP("border", "0.75rem ridge rgba(211, " \
                                                    "220, 50, .6)")            \
                                  CT_CSS_PROP("opacity", "0.92")               \
                                      CT_CSS_PROP("animation", "float 5s "     \
                                                               "ease-in-out "  \
                                                               "infinite")))   \
  CT_CSS_AT("@media only screen and (min-width: 810px)",                       \
            CT_CSS_RULE(".wrapper", CT_CSS_PROP("width", "62vw")               \
                                        CT_CSS_PROP("max-width", "540px"))     \
                CT_CSS_RULE("header>.avatar>img",                              \
                            CT_CSS_PROP("width", "12rem") CT_CSS_PROP(         \
                                "border", "1rem ridge rgba(211, 220, 50, "     \
                                          ".6)")))                             \
  CT_CSS_AT(                                                                   \
      "@media only screen and (max-width: 590px)",                             \
      CT_CSS_RULE(".cattoes", CT_CSS_PROP("height", "2rem")                    \
                                  CT_CSS_PROP("width", "16.7rem")              \
                                      CT_CSS_PROP("margin", "0.5rem auto"))    \
          CT_CSS_RULE(".wrapper", CT_CSS_PROP("padding-top", "0rem")           \
                                      CT_CSS_PROP("width", "85vw"))            \
              CT_CSS_RULE("header", CT_CSS_PROP("text-align", "center"))       \
                  CT_CSS_RULE(".paragraph>p",                                  \
                              CT_CSS_PROP("margin-left", "0rem")               \
                                  CT_CSS_PROP("text-indent", "2em")            \
                                      CT_CSS_PROP("margin-right", "1rem"))     \
                      CT_CSS_RULE("header>.avatar",                            \
                                  CT_CSS_PROP("width", "100%")                 \
                                      CT_CSS_PROP("display", "flex")           \
                                          CT_CSS_PROP("flex-direction", "row") \
                                              CT_CSS_PROP("justify-content",   \
                                                          "center"))           \
                          CT_CSS_RULE(                                         \
                              "header>.avatar>img",                            \
                              CT_CSS_PROP("width", "70vw")                     \
                                  CT_CSS_PROP("align-self", "center")          \
                                      CT_CSS_PROP("border",                    \
                                                  "1rem ridge rgba(211, 220, " \
                                                  "50, .6)"))                  \
                              CT_CSS_RULE(                                     \
                                  "header",                                    \
                                  CT_CSS_PROP("display", "flex") CT_CSS_PROP(  \
                                      "flex-direction", "column")              \
                                      CT_CSS_PROP("padding", "0.5rem")         \
                                          CT_CSS_PROP("font-family", "pf7"))   \
                                  CT_CSS_RULE(".spacer",                       \
                                              CT_CSS_PROP("height", "1rem")))

static const char *generate_css(void) { return BUILD_CSS(); }

/* ==================== HTTP ==================== */
static int send_all(int fd, const void *data, size_t len) {
  size_t offset = 0;
  const char *buf = (const char *)data;
  while (offset < len) {
    ssize_t sent = write(fd, buf + offset, len - offset);
    if (sent <= 0) {
      return -1;
    }
    offset += (size_t)sent;
  }
  return 0;
}

static int send_response(int fd, const char *status, const char *content_type,
                         const void *body, size_t body_len) {
  char header[512];
  int header_len = snprintf(header, sizeof(header),
                            "HTTP/1.1 %s\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %zu\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            status, content_type, body_len);

  if (header_len < 0 || (size_t)header_len >= sizeof(header)) {
    return -1;
  }

  if (send_all(fd, header, (size_t)header_len) != 0) {
    return -1;
  }
  if (body_len > 0 && send_all(fd, body, body_len) != 0) {
    return -1;
  }

  return 0;
}

static int send_text_response(int fd, const char *status, const char *text) {
  return send_response(fd, status, "text/plain; charset=utf-8", text,
                       strlen(text));
}

static int send_404(int fd) {
  return send_text_response(fd, "404 Not Found", "404 Not Found\n");
}

static int send_405(int fd) {
  return send_text_response(fd, "405 Method Not Allowed",
                            "405 Method Not Allowed\n");
}

static int send_400(int fd) {
  return send_text_response(fd, "400 Bad Request", "400 Bad Request\n");
}

static int send_500(int fd) {
  return send_text_response(fd, "500 Internal Server Error",
                            "500 Internal Server Error\n");
}

static int send_redirect(int fd, const char *location) {
  char header[512];
  int header_len = snprintf(header, sizeof(header),
                            "HTTP/1.1 303 See Other\r\n"
                            "Location: %s\r\n"
                            "Content-Length: 0\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            location);
  if (header_len < 0 || (size_t)header_len >= sizeof(header)) {
    return -1;
  }
  return send_all(fd, header, (size_t)header_len);
}

static size_t find_headers_end(const char *request) {
  const char *p = strstr(request, "\r\n\r\n");
  if (p != NULL) {
    return (size_t)(p - request) + 4;
  }

  p = strstr(request, "\n\n");
  if (p != NULL) {
    return (size_t)(p - request) + 2;
  }

  return 0;
}

static long parse_content_length(const char *request, size_t headers_end) {
  const char *p = request;
  const char *end = request + headers_end;

  while (p < end && *p != '\n') {
    p++;
  }
  if (p < end) {
    p++;
  }

  while (p < end) {
    const char *line_end = p;
    size_t line_len;
    while (line_end < end && *line_end != '\n') {
      line_end++;
    }
    line_len = (size_t)(line_end - p);
    if (line_len > 0 && p[line_len - 1] == '\r') {
      line_len--;
    }

    if (line_len >= 15 && strncasecmp(p, "Content-Length:", 15) == 0) {
      char number_buf[32];
      size_t n = 0;
      const char *v = p + 15;
      char *endptr = NULL;
      long value;

      while (v < p + line_len && isspace((unsigned char)*v)) {
        v++;
      }

      while (v < p + line_len && n + 1 < sizeof(number_buf)) {
        number_buf[n++] = *v++;
      }
      number_buf[n] = '\0';

      value = strtol(number_buf, &endptr, 10);
      if (endptr == number_buf || *endptr != '\0' || value < 0) {
        return -1;
      }
      return value;
    }

    p = line_end < end ? line_end + 1 : end;
  }

  return 0;
}

static int read_http_request(int fd, char *request, size_t request_size,
                             size_t *out_len, size_t *out_headers_end) {
  size_t total = 0;
  size_t headers_end = 0;
  size_t expected_total = 0;

  while (total + 1 < request_size) {
    ssize_t n = read(fd, request + total, request_size - total - 1);
    if (n <= 0) {
      break;
    }
    total += (size_t)n;
    request[total] = '\0';

    if (headers_end == 0) {
      long content_length;
      headers_end = find_headers_end(request);
      if (headers_end == 0) {
        continue;
      }

      content_length = parse_content_length(request, headers_end);
      if (content_length < 0) {
        return -1;
      }
      expected_total = headers_end + (size_t)content_length;
    }

    if (headers_end != 0 && total >= expected_total) {
      *out_len = total;
      *out_headers_end = headers_end;
      return 0;
    }
  }

  if (headers_end == 0) {
    return -1;
  }
  if (expected_total > total) {
    return -1;
  }

  *out_len = total;
  *out_headers_end = headers_end;
  return 0;
}

static int has_parent_ref(const char *path) {
  return strstr(path, "..") != NULL;
}

static int serve_static_file(int fd, const char *url_path) {
  char fs_path[PATH_BUF_SIZE];
  FILE *fp;
  char io_buf[IO_BUF_SIZE];

  if (strncmp(url_path, "/static/", 8) != 0 || has_parent_ref(url_path)) {
    return send_404(fd);
  }

  if (snprintf(fs_path, sizeof(fs_path), ".%s", url_path) >=
      (int)sizeof(fs_path)) {
    return send_404(fd);
  }

  fp = fopen(fs_path, "rb");
  if (fp == NULL) {
    if (errno == ENOENT) {
      return send_404(fd);
    }
    return send_500(fd);
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return send_500(fd);
  }

  long file_size = ftell(fp);
  if (file_size < 0) {
    fclose(fp);
    return send_500(fd);
  }
  rewind(fp);

  const char *content_type = content_type_from_path(fs_path);
  char header[512];
  int header_len = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %ld\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            content_type, file_size);
  if (header_len < 0 || (size_t)header_len >= sizeof(header)) {
    fclose(fp);
    return send_500(fd);
  }

  if (send_all(fd, header, (size_t)header_len) != 0) {
    fclose(fp);
    return -1;
  }

  while (!feof(fp)) {
    size_t bytes_read = fread(io_buf, 1, sizeof(io_buf), fp);
    if (bytes_read > 0 && send_all(fd, io_buf, bytes_read) != 0) {
      fclose(fp);
      return -1;
    }
    if (ferror(fp)) {
      fclose(fp);
      return -1;
    }
  }

  fclose(fp);
  return 0;
}

static int parse_request(const char *request, char *method, char *path) {
  char line[PATH_BUF_SIZE + METHOD_BUF_SIZE + 16];
  const char *line_end = strstr(request, "\r\n");

  if (!line_end) {
    line_end = strchr(request, '\n');
  }
  if (!line_end) {
    return -1;
  }

  size_t line_len = (size_t)(line_end - request);
  if (line_len == 0 || line_len >= sizeof(line)) {
    return -1;
  }

  memcpy(line, request, line_len);
  line[line_len] = '\0';

  if (sscanf(line, "%15s %2047s", method, path) != 2) {
    return -1;
  }

  if (method[0] == '\0' || path[0] == '\0') {
    return -1;
  }

  return 0;
}

typedef struct {
  char *page;
  char *marquee_html;
  char *contacts;
  char *content;
  char *counter_str;
  char *submit_button;
} PageRenderBuffers;

static int allocate_page_render_buffers(Arena *arena,
                                        PageRenderBuffers *buffers) {
  buffers->page = arena_alloc(arena, PAGE_BUF_SIZE);
  buffers->marquee_html = arena_alloc(arena, MARQUEE_HTML_BUF_SIZE);
  buffers->contacts = arena_alloc(arena, CONTACTS_BUF_SIZE);
  buffers->content = arena_alloc(arena, CONTENT_BUF_SIZE);
  buffers->counter_str = arena_alloc(arena, COUNTER_STR_BUF_SIZE);
  buffers->submit_button = arena_alloc(arena, SUBMIT_BUTTON_BUF_SIZE);

  if (buffers->page == NULL || buffers->marquee_html == NULL ||
      buffers->contacts == NULL || buffers->content == NULL ||
      buffers->counter_str == NULL ||
      buffers->submit_button == NULL) {
    return -1;
  }

  return 0;
}

static int compose_page_markup(const PageRenderBuffers *buffers,
                               const char *marquee_text, int show_marquee,
                               int can_submit,
                               const char **out_page) {
  const char *quote = get_random_quote();
  int written;

  if (show_marquee && marquee_text[0] != '\0') {
    written = snprintf(buffers->marquee_html, MARQUEE_HTML_BUF_SIZE,
                       BUILD_MARQUEE_BAR(), marquee_text);
    if (written < 0 || (size_t)written >= MARQUEE_HTML_BUF_SIZE) {
      return -1;
    }
  } else {
    buffers->marquee_html[0] = '\0';
  }

  written = snprintf(buffers->contacts, CONTACTS_BUF_SIZE, BUILD_CONTACTS());
  if (written < 0 || (size_t)written >= CONTACTS_BUF_SIZE) {
    return -1;
  }
  written = snprintf(buffers->content, CONTENT_BUF_SIZE, BUILD_CONTENT());
  if (written < 0 || (size_t)written >= CONTENT_BUF_SIZE) {
    return -1;
  }
  written = snprintf(buffers->counter_str, COUNTER_STR_BUF_SIZE, "%lu", COUNTER);
  if (written < 0 || (size_t)written >= COUNTER_STR_BUF_SIZE) {
    return -1;
  }

  written = snprintf(buffers->submit_button, SUBMIT_BUTTON_BUF_SIZE,
                     "<button type=\"submit\" %s>Submit</button>",
                     can_submit ? "" : "disabled");
  if (written < 0 || (size_t)written >= SUBMIT_BUTTON_BUF_SIZE) {
    return -1;
  }

  written = snprintf(buffers->page, PAGE_BUF_SIZE, BUILD_PAGE(),
                     buffers->marquee_html, buffers->submit_button, quote,
                     buffers->contacts,
                     buffers->content, buffers->counter_str);
  if (written < 0 || (size_t)written >= PAGE_BUF_SIZE) {
    const char *fallback =
        "<!DOCTYPE html><html><body>Rendering error</body></html>";
    strncpy(buffers->page, fallback, PAGE_BUF_SIZE - 1);
    buffers->page[PAGE_BUF_SIZE - 1] = '\0';
  }

  *out_page = buffers->page;
  return 0;
}

/* ==================== Server ==================== */
typedef enum {
  ROUTE_HOME,
  ROUTE_COMMENT_POST,
  ROUTE_CSS,
  ROUTE_STATIC_ASSET,
  ROUTE_METHOD_NOT_ALLOWED,
  ROUTE_NOT_FOUND
} RouteKind;

typedef struct {
  const char *method;
  const char *path;
  const char *body;
  size_t body_len;
  const char *client_ip;
  time_t now;
  Arena *arena;
} RequestContext;

static RouteKind resolve_route(const char *method, const char *path) {
  int is_get = strcmp(method, "GET") == 0;
  int is_post = strcmp(method, "POST") == 0;

  if (is_get && strcmp(path, "/") == 0) {
    return ROUTE_HOME;
  }
  if (is_post && strcmp(path, "/comment") == 0) {
    return ROUTE_COMMENT_POST;
  }
  if (is_get && strcmp(path, "/static/main.css") == 0) {
    return ROUTE_CSS;
  }
  if (is_get && strncmp(path, "/static/", 8) == 0) {
    return ROUTE_STATIC_ASSET;
  }
  if (!is_get && !is_post) {
    return ROUTE_METHOD_NOT_ALLOWED;
  }
  return ROUTE_NOT_FOUND;
}

static int handle_home_route(int fd, const RequestContext *ctx) {
  char *marquee_text = arena_alloc(ctx->arena, MAX_MARQUEE_TEXT_LEN);
  PageRenderBuffers page_buffers;
  const char *page;
  int can_submit;
  int has_marquee_messages = 0;

  if (marquee_text == NULL ||
      allocate_page_render_buffers(ctx->arena, &page_buffers) != 0) {
    return send_500(fd);
  }

  can_submit = can_submit_comment_from_ip(ctx->client_ip, ctx->now);

  COUNTER++;
  DIRTY_VISITS++;
  if (flush_counter_if_needed(0) != 0) {
    fprintf(stderr, "Warning: failed to persist visitor counter batch\n");
  }

  build_marquee_text(ctx->arena, marquee_text, MAX_MARQUEE_TEXT_LEN, ctx->now,
                     &has_marquee_messages);

  if (compose_page_markup(&page_buffers, marquee_text, has_marquee_messages,
                          can_submit, &page) != 0) {
    return send_500(fd);
  }

  return send_response(fd, "200 OK", "text/html; charset=utf-8", page,
                       strlen(page));
}

static int handle_comment_route(int fd, const RequestContext *ctx) {
  char *nickname = arena_alloc(ctx->arena, MAX_NICKNAME_LEN + 1);
  char *message = arena_alloc(ctx->arena, MAX_MESSAGE_LEN + 1);

  if (nickname == NULL || message == NULL) {
    return send_500(fd);
  }

  if (parse_form_fields(ctx->body, ctx->body_len, nickname, MAX_NICKNAME_LEN + 1,
                        message, MAX_MESSAGE_LEN + 1) != 0) {
    return send_redirect(fd, "/");
  }
  if (!can_submit_comment_from_ip(ctx->client_ip, ctx->now)) {
    return send_redirect(fd, "/");
  }
  if (append_comment_to_wal(ctx->client_ip, nickname, message, ctx->now) != 0) {
    return send_500(fd);
  }

  return send_redirect(fd, "/");
}

static int handle_css_route(int fd) {
  const char *css = generate_css();
  if (css[0] == '\0') {
    return send_500(fd);
  }
  return send_response(fd, "200 OK", "text/css; charset=utf-8", css,
                       strlen(css));
}

static int dispatch_route(int fd, const RequestContext *ctx, RouteKind route) {
  switch (route) {
  case ROUTE_HOME:
    return handle_home_route(fd, ctx);
  case ROUTE_COMMENT_POST:
    return handle_comment_route(fd, ctx);
  case ROUTE_CSS:
    return handle_css_route(fd);
  case ROUTE_STATIC_ASSET:
    return serve_static_file(fd, ctx->path);
  case ROUTE_METHOD_NOT_ALLOWED:
    return send_405(fd);
  case ROUTE_NOT_FOUND:
  default:
    return send_404(fd);
  }
}

static int process_client_request(int client_fd, const char *client_ip,
                                  const char *request, size_t request_len,
                                  size_t headers_end) {
  char method[METHOD_BUF_SIZE] = {0};
  char path[PATH_BUF_SIZE] = {0};
  const char *body;
  size_t body_len;
  char *query_start;
  RequestContext ctx;
  RouteKind route;
  Arena arena;

  if (parse_request(request, method, path) != 0) {
    return send_400(client_fd);
  }

  query_start = strchr(path, '?');
  if (query_start != NULL) {
    *query_start = '\0';
  }

  body = request + headers_end;
  body_len = request_len >= headers_end ? request_len - headers_end : 0;

  arena_init(&arena, ARENA_DEFAULT_BLOCK_SIZE);

  ctx.method = method;
  ctx.path = path;
  ctx.body = body;
  ctx.body_len = body_len;
  ctx.client_ip = client_ip;
  ctx.now = time(NULL);
  ctx.arena = &arena;

  route = resolve_route(ctx.method, ctx.path);
  {
    int rc = dispatch_route(client_fd, &ctx, route);
    arena_release(&arena);
    return rc;
  }
}

static int start_server(int port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return -1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_port = htons(port),
                             .sin_addr.s_addr = INADDR_ANY};

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, 5) < 0) {
    perror("listen");
    close(server_fd);
    return -1;
  }

  printf("Server on port %d\n", port);

  while (1) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0)
      continue;

    char peer_ip[MAX_IP_LEN];
    char client_ip[MAX_IP_LEN];
    get_client_ip((struct sockaddr *)&client_addr, peer_ip, sizeof(peer_ip));

    char request[REQUEST_BUF_SIZE];
    size_t request_len = 0;
    size_t headers_end = 0;
    if (read_http_request(client_fd, request, sizeof(request), &request_len,
                          &headers_end) != 0) {
      send_400(client_fd);
      close(client_fd);
      continue;
    }

    resolve_client_ip(request, headers_end, peer_ip, client_ip,
                      sizeof(client_ip));

    process_client_request(client_fd, client_ip, request, request_len,
                           headers_end);

    close(client_fd);
  }
}

/* ==================== Main ==================== */
int main(int argc, char **argv) {
  int port = DEFAULT_PORT;

  if (argc > 2) {
    fprintf(stderr, "Usage: %s [port]\n", argv[0]);
    return 1;
  }

  if (argc == 2) {
    char *endptr = NULL;
    long parsed = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0' || parsed < 1 || parsed > 65535) {
      fprintf(stderr, "Invalid port: %s\n", argv[1]);
      return 1;
    }
    port = (int)parsed;
  }

  srand((unsigned int)time(NULL));
  if (load_counter() != 0) {
    fprintf(stderr,
            "Warning: failed to load visitor counter; starting from zero\n");
    COUNTER = 0;
  }
  DIRTY_VISITS = 0;

  return start_server(port);
}
