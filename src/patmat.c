/* Quoted path so a syntax check without -I still finds the public header. */
#include "../include/patmat.h"

#include <stddef.h>

#define PATMAT_MAX_DEPTH 64

enum match_result {
    MATCH = 0,
    NOMATCH = 1,
    BAD = 2
};

static int fold_byte(unsigned char byte)
{
    if (byte >= 'A' && byte <= 'Z') {
        return byte - 'A' + 'a';
    }
    return byte;
}

static int is_word_space(unsigned char byte)
{
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\v' ||
           byte == '\f' || byte == '\r';
}

static int at_word_start(const char *text, const char *start)
{
    if (text == start) {
        return 1;
    }
    return is_word_space((unsigned char)text[-1]);
}

static const char *find_unescaped(const char *text, char target)
{
    while (*text != '\0') {
        if (*text == '\\') {
            if (text[1] == '\0') {
                return NULL;
            }
            text += 2;
            continue;
        }
        if (*text == target) {
            return text;
        }
        text++;
    }
    return NULL;
}

static int read_atom(const char **pat, unsigned char *atom)
{
    const char *cursor = *pat;

    if (*cursor == '\0' || *cursor == ']') {
        return 0;
    }
    if (*cursor == '\\') {
        if (cursor[1] == '\0') {
            return 0;
        }
        *atom = (unsigned char)cursor[1];
        *pat = cursor + 2;
        return 1;
    }
    *atom = (unsigned char)*cursor;
    *pat = cursor + 1;
    return 1;
}

/* pat points at '['. On success *after is the byte after the closing ']'. */
static int walk_class(const char *pat, int have_byte, unsigned char byte,
                      int *matched, const char **after)
{
    int negate = 0;
    int any = 0;

    pat++;
    if (*pat == '^') {
        negate = 1;
        pat++;
    }
    if (*pat == ']') {
        if (matched != NULL) {
            *matched = negate ? 1 : 0;
        }
        *after = pat + 1;
        return 0;
    }

    while (*pat != '\0' && *pat != ']') {
        unsigned char lo = 0;
        unsigned char hi = 0;
        int is_range = 0;

        if (!read_atom(&pat, &lo)) {
            return -1;
        }
        if (*pat == '-') {
            if (pat[1] == '\0') {
                return -1;
            }
            if (pat[1] == ']') {
                if (find_unescaped(pat + 2, ']') == NULL) {
                    return -1;
                }
                hi = (unsigned char)']';
                pat += 2;
                is_range = 1;
            } else {
                pat++;
                if (!read_atom(&pat, &hi)) {
                    return -1;
                }
                is_range = 1;
            }
        }
        if (have_byte) {
            int folded = fold_byte(byte);

            if (is_range) {
                int low = fold_byte(lo);
                int high = fold_byte(hi);

                if (low <= high && folded >= low && folded <= high) {
                    any = 1;
                }
            } else if (fold_byte(lo) == folded) {
                any = 1;
            }
        }
    }
    if (*pat != ']') {
        return -1;
    }
    if (matched != NULL) {
        *matched = negate ? !any : any;
    }
    *after = pat + 1;
    return 0;
}

static int check_pattern(const char *pat)
{
    int in_brace = 0;

    while (*pat != '\0') {
        if (*pat == '\\') {
            if (pat[1] == '\0') {
                return -1;
            }
            pat += 2;
            continue;
        }
        if (*pat == '[') {
            const char *after = NULL;

            if (walk_class(pat, 0, 0, NULL, &after) != 0) {
                return -1;
            }
            pat = after;
            continue;
        }
        if (*pat == '{') {
            if (in_brace) {
                return -1;
            }
            in_brace = 1;
            pat++;
            continue;
        }
        if (*pat == '}') {
            in_brace = 0;
            pat++;
            continue;
        }
        pat++;
    }
    if (in_brace) {
        return -1;
    }
    return 0;
}

static int match_here(const char *pat, const char *text, int inword, int depth);

static int match_star(const char *pat, const char *text, const char *start,
                      int inword, int depth)
{
    while (*pat == '*' || *pat == '?') {
        if (*pat == '?') {
            if (*text == '\0' || (inword && is_word_space((unsigned char)*text))) {
                return NOMATCH;
            }
            text++;
        }
        pat++;
    }

    if (inword) {
        while (*text != '\0' && !is_word_space((unsigned char)*text)) {
            int result = match_here(pat, text, inword, depth + 1);

            if (result != NOMATCH) {
                return result;
            }
            text++;
        }
        return match_here(pat, text, inword, depth + 1);
    }
    if (*pat == '\0') {
        return MATCH;
    }
    if (*pat == '{') {
        if (at_word_start(text, start)) {
            int result = match_here(pat, text, inword, depth + 1);

            if (result != NOMATCH) {
                return result;
            }
        }
        if (*text == '\0') {
            return NOMATCH;
        }
        for (text++; *text != '\0'; text++) {
            if (is_word_space((unsigned char)text[-1])) {
                int result = match_here(pat, text, inword, depth + 1);

                if (result != NOMATCH) {
                    return result;
                }
            }
        }
        return NOMATCH;
    }
    if (*pat == '[') {
        while (*text != '\0') {
            int result = match_here(pat, text, inword, depth + 1);

            if (result != NOMATCH) {
                return result;
            }
            text++;
        }
        return NOMATCH;
    }

    {
        unsigned char want;

        if (pat[0] == '\\' && pat[1] != '\0') {
            want = (unsigned char)pat[1];
        } else {
            want = (unsigned char)pat[0];
        }
        want = (unsigned char)fold_byte(want);
        while (*text != '\0') {
            if ((unsigned char)fold_byte((unsigned char)*text) == want) {
                int result = match_here(pat, text, inword, depth + 1);

                if (result != NOMATCH) {
                    return result;
                }
            }
            text++;
        }
    }
    return NOMATCH;
}

static int match_brace(const char *pat, const char *text, const char *start,
                       int depth)
{
    const char *end;
    const char *alt;
    int result = NOMATCH;

    if (!at_word_start(text, start)) {
        return NOMATCH;
    }
    end = find_unescaped(pat, '}');
    if (end == NULL) {
        return BAD;
    }
    alt = pat + 1;
    while (alt <= end) {
        result = match_here(alt, text, 1, depth + 1);
        if (result != NOMATCH) {
            break;
        }
        alt = find_unescaped(alt, '|');
        if (alt == NULL || alt > end) {
            break;
        }
        alt++;
    }
    if (result != MATCH) {
        return result;
    }
    pat = end + 1;
    while (*text != '\0' && !is_word_space((unsigned char)*text)) {
        text++;
    }
    return match_here(pat, text, 0, depth);
}

static int match_here(const char *pat, const char *text, int inword, int depth)
{
    const char *start = text;

    if (depth > PATMAT_MAX_DEPTH) {
        return BAD;
    }
    while (*pat != '\0') {
        if (*pat == '\\') {
            if (pat[1] == '\0' || *text == '\0' ||
                fold_byte((unsigned char)pat[1]) != fold_byte((unsigned char)*text)) {
                return NOMATCH;
            }
            pat += 2;
            text++;
            continue;
        }
        if (*pat == '?') {
            if (*text == '\0' || (inword && is_word_space((unsigned char)*text))) {
                return NOMATCH;
            }
            pat++;
            text++;
            continue;
        }
        if (*pat == '*') {
            return match_star(pat, text, start, inword, depth);
        }
        if (*pat == '[') {
            const char *after = NULL;
            int matched = 0;

            if (*text == '\0' || (inword && is_word_space((unsigned char)*text))) {
                return NOMATCH;
            }
            if (walk_class(pat, 1, (unsigned char)*text, &matched, &after) != 0) {
                return BAD;
            }
            if (!matched) {
                return NOMATCH;
            }
            pat = after;
            text++;
            continue;
        }
        if (*pat == '{') {
            const char *end = find_unescaped(pat, '}');
            int result;

            if (end == NULL) {
                return BAD;
            }
            result = match_brace(pat, text, start, depth);
            if (result != MATCH) {
                return result;
            }
            /* match_brace already consumed the word and the rest of the
               pattern when it returned MATCH. */
            return MATCH;
        }
        if ((*pat == '}' || *pat == '|') && inword) {
            if (*text != '\0' && !is_word_space((unsigned char)*text)) {
                return NOMATCH;
            }
            return MATCH;
        }
        if (*text == '\0' ||
            fold_byte((unsigned char)*pat) != fold_byte((unsigned char)*text)) {
            return NOMATCH;
        }
        pat++;
        text++;
    }
    if (*text == '\0') {
        return MATCH;
    }
    return NOMATCH;
}

int patmat_match(const char *text, const char *pattern, int *matched)
{
    int result;

    if (text == NULL || pattern == NULL || matched == NULL) {
        return PATMAT_INVAL;
    }
    if (check_pattern(pattern) != 0) {
        return PATMAT_BADPAT;
    }
    result = match_here(pattern, text, 0, 0);
    if (result == BAD) {
        return PATMAT_BADPAT;
    }
    *matched = result == MATCH ? 1 : 0;
    return PATMAT_OK;
}
