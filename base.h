#ifndef BASE_H
#define BASE_H

// Base Layer
// ====================================================================================================
#define read_only __attribute__((section(".rodata")))

// C11
// typedef _Bool bool;
// #define true 1
// #define false 0
#define alignof(x) _Alignof(x)

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
#ifdef _WIN32
typedef unsigned long long u64;
typedef signed long int s64;
#else
typedef unsigned long int u64;
typedef signed long long s64;
#endif
typedef typeof(sizeof(0u)) uz;
typedef typeof(sizeof(0)) sz;

#define U8_MAX 0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFFull

#define S8_MAX (s8)0x7F
#define S16_MAX (s16)0x7FFF
#define S32_MAX (s32)0x7FFFFFFF
#define S64_MAX (s64)0x7FFFFFFFFFFFFFFFll

#define S8_MIN (s8)0x80
#define S16_MIN (s16)0x8000
#define S32_MIN (s32)0x80000000
#define S64_MIN (s64)0x8000000000000000ll

#define MIN(A,B) (((A)<(B))?(A):(B))
#define MAX(A,B) (((A)>(B))?(A):(B))
#define CLAMPTOP(A,X) MIN(A,X)
#define CLAMPBOT(X,B) MAX(X,B)

#define ASSERT_ALWAYS(x) do{if(!(x)) {__builtin_trap();}}while(0)
#ifdef DEBUG
#define ASSERT(x) ASSERT_ALWAYS(x)
#else
#define ASSERT(x) (void)(x)
#endif

#define KiB(x) ((x) << 10)
#define MiB(x) ((x) << 20)

static const u8 integer_symbol_reverse[128] =
{
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

// Arena
// ----------------------------------------------------------------------------------------------------

 typedef struct Arena Arena;
 struct Arena {
    u8 *beg;
    u8 *end;
    uz cap;
 };

typedef struct ArenaUsage ArenaUsage;
struct ArenaUsage
{
    float perc;
    u16 mib;
    u16 kib;
    u16 bytes;
};

static Arena arena_init(void *mem, uz cap);
static u8 *arena_alloc(Arena *arena, uz size, uz align_size, uz count);
#define arena_push(a, t, n) (t*)arena_alloc(a, sizeof(t), alignof(t), n)
static ArenaUsage arena_report_usage(Arena *arena);

// Char functions
// ----------------------------------------------------------------------------------------------------

static inline bool char_is_space(u8 c);
static inline bool char_is_upper(u8 c);
static inline bool char_is_lower(u8 c);
static inline bool char_is_alpha(u8 c);
static inline bool char_is_digit(u8 c, u32 radix);
static inline bool char_is_alnum(u8 c);


// String
// ----------------------------------------------------------------------------------------------------

typedef struct String String;
struct String {
    u8 *str;
    uz len;
};

#define str_lit(s) ((String){(u8*)s, sizeof(s)-1})

static uz cstr_length(char *s);
static bool str_match(String a, String b);
static bool str_starts_with(String s, String prefix);
static String str_push_copy(Arena *arena, String s);
static inline String str_from_range(u8 *first, u8 *one_past_last);
static String str_from_file(Arena *arena, String path);

// nullprogram hashing function
static uz str_hash(String s);

// inline manipulation
static inline bool str_in_bounds(String s, uz at);
static inline String str_skip(String s, uz count);
static inline String str_skip_space(String s);
static inline String str_skip_line(String s);
static inline String str_trim_left(String source);
static inline String str_trim_right(String source);
static inline String str_trim(String source);

static uz str_find_first_non_space(String source, uz offset);
static uz str_find_char(String source, u8 c, uz offset);
static uz str_find_newline(String source, uz offset);

static void str_to_upper(String s);
static void str_to_lower(String s);

static String str_postfix(String s, uz len);
static String str_prefix(String s, uz len);

// String splits & lists
// ----------------------------------------------------------------------------------------------------
typedef struct Split Split;
struct Split {
    String head;
    String tail;
    bool ok;
};

Split str_split_once(String input, s8 split_char);


typedef struct StringNode StringNode;
struct StringNode {
    String string;
    String *next;
};

typedef struct StringList StringList;
struct StringList {
    StringNode *first;
    StringNode *last;
    uz node_count;
    uz total_size;
};

// Conversion
// ----------------------------------------------------------------------------------------------------
static u32 safe_cast_u32(u64 x);
static u64 u64_from_str(String s, u32 radix);
static u32 str_to_u32(String s);
static bool str_to_bool(String s);


// OS I/O
// ----------------------------------------------------------------------------------------------------
static uz os_read_file(char *path, u8 *buffer, uz cap);

// IMPLEMENTATION
// ====================================================================================================
#ifdef BASE_IMPLEMENTATION

static Arena arena_init(void *mem, uz cap)
{
    Arena arena = {};
    arena.beg = (u8*)mem;
    if (arena.beg) {
        arena.end = arena.beg + cap;
        arena.cap = cap;
    }

    return arena;
}


static u8 *arena_alloc(Arena *arena, uz size, uz align_size, uz count)
{
    uz alloc_size = size * count;
    uz avail_size = arena->end - arena->beg;
    uz align = -(uz)arena->beg & (align_size - 1);
    if (avail_size < alloc_size + align) {
        ASSERT_ALWAYS(false);
    }

    u8 *ptr = arena->beg + align;
    arena->beg += align + alloc_size;
    __builtin_memset(ptr, 0, count);

    return ptr;
}


static ArenaUsage arena_report_usage(Arena *arena)
{
    uz free = (uz)(arena->end - arena->beg);
    uz usage = arena->cap - free;
    float perc = (float)usage/(float)arena->cap * 100;
    u16 b = (u16)(usage & ((1 << 10) - 1));
    u16 kib = (u16)((usage >> 10) & ((1 << 10) - 1));
    u16 mib = (u16)((usage >> 20) & ((1 << 10) - 1));

    ArenaUsage result = {
        .perc = perc,
        .mib = mib,
        .kib = kib,
        .bytes = b,
    };

    return result;
}

// String
// ----------------------------------------------------------------------------------------------------

static inline bool char_is_space(u8 c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v');
}

static inline bool char_is_upper(u8 c)
{
    return ('A' <= c && c <= 'Z');
}

static inline bool char_is_lower(u8 c)
{
    return ('a' <= c && c <= 'z');
}

static inline bool char_is_alpha(u8 c)
{
    return (char_is_lower(c) || char_is_upper(c));
}

static inline bool char_is_alnum(u8 c)
{
    return (char_is_alpha(c) || char_is_digit(c, 10));
}

static inline bool char_is_digit(u8 c, u32 radix)
{
    bool result = false;
    if (1 <= radix && radix <= 16) {
        u8 val = integer_symbol_reverse[c];
        if (val < radix) {
            result = true;
        }
    }
    return result;
}


static uz cstr_length(char *s)
{
    uz len = 0;
    if (s) {
        char *ptr = s;
        for (; *ptr != 0; ptr++);
        len = (uz)(ptr - s);
    }
    return len;
}

static bool str_match(String a, String b)
{
    bool result = a.len == b.len;
    result &= (!a.len || !__builtin_memcmp(a.str, b.str, a.len));

    return result;
}

static bool str_starts_with(String s, String prefix)
{
    uz len = CLAMPTOP(s.len, prefix.len);
    s.len = len;
    prefix.len = len;
    return str_match(s, prefix);
}


static String str_push_copy(Arena *arena, String s)
{
    String result = {};
    result.len = s.len;
    result.str = arena_push(arena, u8, s.len);
    __builtin_memcpy(result.str, s.str, s.len);

    return result;
}


static inline String str_from_range(u8 *first, u8 *one_past_last)
{
    String result = { first, (uz)(one_past_last - first) };
    return result;
}

static String str_from_file(Arena *arena, String path)
{
    char raw_path[256];
    for (uz i = 0; i < path.len; i++) {
        raw_path[i] = path.str[i];
    }
    raw_path[path.len] = '\0';

    uz cap = arena->end - arena->beg;

    String result = {};
    result.str = arena->beg;
    result.len = os_read_file(raw_path, arena->beg, cap );

    arena->beg += result.len;

    return result;
}

// nullprogram hashing function
static uz str_hash(String s)
{
    uz hash = 0x100;
    for (uz i = 0; i < s.len; i++) {
        hash ^= s.str[i] & 255;
        hash *= 1111111111111111111;
    }
    return hash;
}


static inline bool str_in_bounds(String s, uz at)
{
    bool result = (at < s.len);
    return result;
}

static inline String str_skip(String s, uz count)
{
    count = CLAMPTOP(s.len, count);
    s.str += count;
    s.len -= count;
    return s;
}

static inline String str_skip_space(String s)
{
    while ( s.len && char_is_space(s.str[0])) {
        s.str += 1;
        s.len -= 1;
    }
    return s;
}

static String str_skip_line(String s)
{
    while (s.len && s.str[0] != '\n') { s.str++; s.len--; }
    s.str += 1;
    s.len -= 1;
    return s;
}


static inline String str_trim_right(String s)
{
    while ( s.len && char_is_space(s.str[s.len])) {
        s.len -= 1;
    }
    return s;
}

static inline String str_trim_left(String s)
{
    while ( s.len && char_is_space(s.str[0])) {
        s.str += 1;
        s.len -= 1;
    }
    return s;
}

static inline String str_trim(String s)
{
    s = str_trim_left(s);
    s = str_trim_right(s);
    return s;
}

static uz str_find_char(String source, u8 c, uz offset)
{
    uz at = offset;
    while (str_in_bounds(source, at) && source.str[at] != c) { at++; }
    return at;
}

static uz str_find_first_non_space(String source, uz offset)
{
    uz at = offset;
    while (str_in_bounds(source, at) && char_is_space(source.str[at])) { at++; }
    return at;
}

static uz str_find_newline(String source, uz offset)
{
    uz at = offset;
    while (str_in_bounds(source, at) && source.str[at] != '\n') { at++; }
    at++;
    return at;
}

static void str_to_upper(String s)
{
    for (uz i = 0; i < s.len; i++) {
        s.str[i] &= (0x20 ^ 0xFF);
    }
}

static void str_to_lower(String s)
{
    for (uz i = 0; i < s.len; i++) {
        s.str[i] |= 0x20;
    }
}

static String str_postfix(String s, uz len)
{
    ASSERT_ALWAYS(len <= s.len);
    s.str += len;
    s.len -= len;
    return s;
}

static String str_prefix(String s, uz len)
{
    s.len = CLAMPTOP(s.len, len);
    return s;
}

Split str_split_once(String input, s8 split_char) {
    Split split = {};
    uz i = 0;
    for (; i < input.len && input.str[i] != split_char; i++) {}
    split.ok = (i < input.len);
    split.head = str_prefix(input, i);
    split.tail = str_postfix(input, i + split.ok);

    return split;
}

static u32 safe_cast_u32(u64 x) {
    ASSERT_ALWAYS( x <= U32_MAX);
    u32 result = (u32)x;
    return result;
}


static u64 u64_from_str(String s, u32 radix)
{
    u64 result = 0;
    if ( 1 < radix && radix <= 16) {
        for (uz i = 0; i < s.len; i++) {
            result *= radix;
            result += integer_symbol_reverse[(s.str[i] & 0x7f)];
        }
    }

    return result;
}


static u32 str_to_u32(String s)
{
    String prefix = str_prefix(s, 2);
    uz prefix_size = 0;
    uz radix = 0;
    if (str_match(prefix, str_lit("0x"))) {
        radix = 16;
        prefix_size = 2;
    } else if (str_match(prefix, str_lit("0b"))) {
        radix = 2;
        prefix_size = 2;
    } else {
        radix = 10;
    }

    String integer = str_postfix(s, prefix_size);
    u32 result = safe_cast_u32(u64_from_str(integer, radix));

    return result;
}

static bool str_to_bool(String s)
{
    bool result = 0;
    if (str_match(s, str_lit("true"))) {
        result = 1;
    }

    return result;
}


// OS I/O
// ----------------------------------------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
static uz os_read_file(char *path, u8 *buffer, uz cap)
{
    uz has_read_total = 0;
    WIN32_FILE_ATTRIBUTE_DATA data = {0};
    GetFileAttributesExA(path, GetFileExInfoStandard, &data);
    u64 size = (((u64)data.nFileSizeHigh) << 32 | (u64)data.nFileSizeLow);

    HANDLE handle = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle) {

        for (u64 offset = 0; offset < size;) {
            u64 to_read = size - offset;
            DWORD has_read = 0;
            ReadFile(handle, buffer + offset, to_read, &has_read, 0);
            offset += has_read;
            has_read_total += has_read;
        }
    }
    return has_read_total;
}

#else

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
static uz os_read_file(char *path, u8 *buffer, uz cap)
{
    struct stat st;
    stat(path, &st);
    uz size = st.st_size;
    int fd = open(path, O_RDONLY);

    uz has_read_total = 0;
    while (has_read_total < size) {
        sz has_read = read(fd, buffer, size);
        has_read_total += (uz)has_read;
    }

    close(fd);

    return has_read_total;
}

#endif


#endif  // BASE_IMPLEMENTATION


#endif

