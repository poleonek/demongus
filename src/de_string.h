typedef struct
{
    Uint8 *str;
    Uint64 size;
} S8;
#define PrintS8(s) (int)(s).size, (s).str

typedef enum {
    S8Match_FindLast         = (1 << 0),
    S8Match_CaseInsensitive  = (1 << 1),
    S8Match_RightSideSloppy  = (1 << 2),
    S8Match_SlashInsensitive = (1 << 3),
} S8_MatchFlags;

typedef struct {
    Uint64 index : 63;
    Uint64 found : 1;
} S8_FindResult;

static S8 S8_MakeFromCstr(const char *cstr)
{
    S8 result = {0};
    result.str = (Uint8 *)cstr;
    while (cstr[result.size]) result.size += 1;
    return result;
}

static S8 S8_Make(Uint8 *str, Uint64 size)
{
    S8 string;
    string.str = str;
    string.size = size;
    return string;
}

static S8 S8_Range(Uint8 *first, Uint8 *last)
{
    S8 string = {
        first,
        (Uint64)(last - first)
    };
    return string;
}
static S8 S8_Substring(S8 str, Uint64 min, Uint64 max)
{
    if (max > str.size) max = str.size;
    if (min > str.size) min = str.size;
    if (min > max) {
        Uint64 swap = min;
        min = max;
        max = swap;
    }
    str.size = max - min;
    str.str += min;
    return str;
}
static S8 S8_Skip(S8 str, Uint64 distance)
{
    distance = (distance > str.size ? str.size : distance);
    str.str += distance;
    str.size -= distance;
    return str;
}
static S8 S8_Chop(S8 str, Uint64 distance_from_back)
{
    distance_from_back = (distance_from_back > str.size ? str.size : distance_from_back);
    str.size -= distance_from_back;
    return str;
}
static S8 S8_Prefix(S8 str, Uint64 size)
{
    size = (size > str.size ? str.size : size);
    str.size = size;
    return str;
}
static S8 S8_Suffix(S8 str, Uint64 size)
{
    size = (size > str.size ? str.size : size);
    Uint64 distance = str.size - size;
    str.str += distance;
    str.size = size;
    return str;
}

//
// ByteIs
//
static bool ByteIsAlphaUpper(Uint8 c)
{
    return c >= 'A' && c <= 'Z';
}
static bool ByteIsAlphaLower(Uint8 c)
{
    return c >= 'a' && c <= 'z';
}
static bool ByteIsAlpha(Uint8 c)
{
    return ByteIsAlphaUpper(c) || ByteIsAlphaLower(c);
}
static bool ByteIsDigit(Uint8 c)
{
    return (c >= '0' && c <= '9');
}
static bool ByteIsAlphaNumeric(Uint8 c)
{
    return ByteIsAlphaUpper(c) || ByteIsAlphaLower(c) || ByteIsDigit(c);
}
static bool ByteIsUnreservedSymbol(Uint8 c)
{
    return (c == '~' || c == '!' || c == '$' || c == '%' || c == '^' ||
            c == '&' || c == '*' || c == '-' || c == '=' || c == '+' ||
            c == '<' || c == '.' || c == '>' || c == '/' || c == '?' ||
            c == '|');
}
static bool ByteIsReservedSymbol(Uint8 c)
{
    return (c == '{' || c == '}' || c == '(' || c == ')' || c == '\\' ||
            c == '[' || c == ']' || c == '#' || c == ',' || c == ';'  ||
            c == ':' || c == '@');
}
static bool ByteIsWhite(Uint8 c)
{
    return c == ' ' || c == '\r' || c == '\t' || c == '\f' || c == '\v' || c == '\n';
}

//
// CharTo
//
static Uint8 ByteToUpper(Uint8 c)
{
    return (c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : c;
}
static Uint8 ByteToLower(Uint8 c)
{
    return (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
}
static Uint8 ByteToForwardSlash(Uint8 c)
{
    return (c == '\\' ? '/' : c);
}

//
// S8_Match
//
static bool S8_Match(S8 a, S8 b, S8_MatchFlags flags)
{
    bool result = false;
    if (a.size == b.size || flags & S8Match_RightSideSloppy)
    {
        result = true;
        for (Uint64 i = 0; i < a.size && i < b.size; i += 1)
        {
            bool match = (a.str[i] == b.str[i]);

            if (flags & S8Match_CaseInsensitive)
                match |= (ByteToLower(a.str[i]) == ByteToLower(b.str[i]));

            if (flags & S8Match_SlashInsensitive)
                match |= (ByteToForwardSlash(a.str[i]) == ByteToForwardSlash(b.str[i]));

            if (!match)
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

static bool S8_StartsWith(S8 haystack, S8 needle, S8_MatchFlags flags)
{
    S8 h = S8_Prefix(haystack, needle.size);
    return S8_Match(h, needle, flags);
}
static bool S8_EndsWith(S8 haystack, S8 needle, S8_MatchFlags flags)
{
    S8 h = S8_Suffix(haystack, needle.size);
    return S8_Match(h, needle, flags);
}

//
// S8_Find
//
static S8_FindResult S8_Find(S8 str, S8 substring, Uint64 start_pos, S8_MatchFlags flags)
{
    S8_FindResult result = {};
    for (Uint64 i = start_pos; i < str.size; i += 1)
    {
        if (i + substring.size <= str.size)
        {
            S8 substr_from_str = S8_Substring(str, i, i+substring.size);
            if (S8_Match(substr_from_str, substring, flags))
            {
                result.index = i;
                result.found = 1;
                if (!(flags & S8Match_FindLast))
                {
                    // @speed faster implementation that searches in reverse!
                    break;
                }
            }
        }
    }
    return result;
}

static Uint64 S8_Count(S8 str, S8 substring, Uint64 start_pos, S8_MatchFlags flags)
{
    flags &= ~S8Match_FindLast; // clear FindLast flag, doesn't work here

    Uint64 counter = 0;
    for (;;)
    {
        S8_FindResult find = S8_Find(str, substring, start_pos, flags);
        if (!find.found)
        {
            break;
        }
        start_pos = find.index + substring.size;
        counter += 1;
    }
    return counter;
}

//
// S8_Consume
//
static bool S8_Consume(S8 *haystack, S8 needle, S8_MatchFlags flags)
{
    if (S8_StartsWith(*haystack, needle, flags))
    {
        *haystack = S8_Skip(*haystack, needle.size);
        return true;
    }
    return false;
}
static bool S8_ConsumeBack(S8 *haystack, S8 needle, S8_MatchFlags flags)
{
    if (S8_EndsWith(*haystack, needle, flags))
    {
        *haystack = S8_Chop(*haystack, needle.size);
        return true;
    }
    return false;
}

static S8 S8_ConsumeUntil(S8 *haystack, S8 needle, S8_MatchFlags flags)
{
    S8_FindResult find = S8_Find(*haystack, needle, 0, flags);
    if (find.found)
    {
        S8 result = S8_Prefix(*haystack, find.index);
        *haystack = S8_Skip(*haystack, find.index + needle.size);
        return result;
    }
    return (S8){0};
}
static S8 S8_ConsumeUntilBack(S8 *haystack, S8 needle, S8_MatchFlags flags)
{
    S8_FindResult find = S8_Find(*haystack, needle, 0, flags);
    if (find.found)
    {
        S8 result = S8_Skip(*haystack, find.index + needle.size);
        *haystack = S8_Prefix(*haystack, find.index);
        return result;
    }
    return (S8){0};
}

//
// Misc
//
static Uint64 S8_Hash(Uint64 seed, S8 string)
{
    return HashU64(seed, string.str, string.size);
}