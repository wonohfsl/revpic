/**
 * @file json_utils.c
 * @brief Minimal JSON parsing helpers for flat objects.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_utils.h"

/**
 * @brief Skip whitespace within a bounded buffer.
 *
 * @param p Current cursor position.
 * @param end End pointer (one past last).
 * @return Pointer to first non-whitespace character or end.
 */
static const char *JsonUtils_SkipWs(const char *p, const char *end)
{
    while (p < end && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

/**
 * @brief Find a quoted key within a buffer range.
 *
 * @param start Start pointer.
 * @param end End pointer (one past last).
 * @param key Key to match.
 * @return Pointer to key occurrence or NULL.
 */
static const char *JsonUtils_FindKeyInRange(const char *start, const char *end, const char *key)
{
    char pattern[64];
    size_t key_len = strlen(key);

    if (key_len + 2 >= sizeof(pattern)) return NULL;

    pattern[0] = '"';
    memcpy(&pattern[1], key, key_len);
    pattern[key_len + 1] = '"';
    pattern[key_len + 2] = '\0';

    const char *p = start;
    while ((p = strstr(p, pattern)) != NULL) {
        if (p >= end) return NULL;
        if (p + (key_len + 2) <= end) return p;
        p += (key_len + 2);
    }

    return NULL;
}

/**
 * @brief Read a JSON file into a null-terminated buffer.
 *
 * @param path File path to read.
 * @param out_buf Output pointer for allocated buffer.
 * @param out_len Output pointer for buffer length, may be NULL.
 * @param max_len Maximum allowed file size.
 * @return 0 on success, non-zero on failure.
 */
int JsonUtils_ReadFileToBuffer(const char *path, char **out_buf, size_t *out_len, size_t max_len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    long len = ftell(fp);
    if (len <= 0 || (max_len > 0 && (size_t)len > max_len)) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    size_t read_len = fread(buf, 1, (size_t)len, fp);
    fclose(fp);

    if (read_len != (size_t)len) {
        free(buf);
        return -1;
    }

    buf[len] = '\0';
    *out_buf = buf;
    if (out_len) {
        *out_len = (size_t)len;
    }
    return 0;
}

/**
 * @brief Find the span of an object value for a top-level key.
 *
 * @param json Null-terminated JSON string.
 * @param key Object key to locate.
 * @param span Output span covering the object text.
 * @return 1 if found, 0 if not found.
 */
int JsonUtils_FindObjectSpan(const char *json, const char *key, JsonSpan_t *span)
{
    const char *end = json + strlen(json);
    const char *p = JsonUtils_FindKeyInRange(json, end, key);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    p = JsonUtils_SkipWs(p, end);
    if (*p != '{') return 0;

    const char *obj_start = p;
    int depth = 0;
    int in_string = 0;
    int escape = 0;

    for (; p < end; p++) {
        char c = *p;

        if (in_string) {
            if (escape) {
                escape = 0;
            } else if (c == '\\') {
                escape = 1;
            } else if (c == '"') {
                in_string = 0;
            }
            continue;
        }

        if (c == '"') {
            in_string = 1;
            continue;
        }

        if (c == '{') depth++;
        if (c == '}') {
            depth--;
            if (depth == 0) {
                span->start = obj_start;
                span->end = p + 1;
                return 1;
            }
        }
    }

    return 0;
}

/**
 * @brief Parse a number value for a key within a span.
 *
 * @param span Object span to search.
 * @param key Key to parse.
 * @param out Output double value.
 * @return 1 if parsed, 0 if not found or invalid.
 */
int JsonUtils_ParseNumberInSpan(JsonSpan_t span, const char *key, double *out)
{
    const char *p = JsonUtils_FindKeyInRange(span.start, span.end, key);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p || p >= span.end) return 0;
    p++;
    p = JsonUtils_SkipWs(p, span.end);

    char *endptr = NULL;
    double val = strtod(p, &endptr);
    if (p == endptr) return 0;
    if (endptr > span.end) return 0;

    *out = val;
    return 1;
}

/**
 * @brief Parse a boolean value for a key within a span.
 *
 * @param span Object span to search.
 * @param key Key to parse.
 * @param out Output boolean as int (1 true, 0 false).
 * @return 1 if parsed, 0 if not found or invalid.
 */
int JsonUtils_ParseBoolInSpan(JsonSpan_t span, const char *key, int *out)
{
    const char *p = JsonUtils_FindKeyInRange(span.start, span.end, key);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p || p >= span.end) return 0;
    p++;
    p = JsonUtils_SkipWs(p, span.end);

    if ((span.end - p) >= 4 && strncmp(p, "true", 4) == 0) {
        *out = 1;
        return 1;
    }
    if ((span.end - p) >= 5 && strncmp(p, "false", 5) == 0) {
        *out = 0;
        return 1;
    }

    return 0;
}

/**
 * @brief Parse a string value for a key within a span.
 *
 * @param span Object span to search.
 * @param key Key to parse.
 * @param out Output buffer for the string.
 * @param out_len Length of output buffer.
 * @return 1 if parsed, 0 if not found or invalid.
 */
int JsonUtils_ParseStringInSpan(JsonSpan_t span, const char *key, char *out, size_t out_len)
{
    if (!out || out_len == 0) return 0;

    const char *p = JsonUtils_FindKeyInRange(span.start, span.end, key);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p || p >= span.end) return 0;
    p++;
    p = JsonUtils_SkipWs(p, span.end);

    if (p >= span.end || *p != '"') return 0;
    p++;

    size_t i = 0;
    while (p < span.end && *p != '"') {
        if (i + 1 >= out_len) return 0;

        if (*p == '\\') {
            p++;
            if (p >= span.end) return 0;
            switch (*p) {
                case '"': out[i++] = '"'; break;
                case '\\': out[i++] = '\\'; break;
                case '/': out[i++] = '/'; break;
                case 'b': out[i++] = '\b'; break;
                case 'f': out[i++] = '\f'; break;
                case 'n': out[i++] = '\n'; break;
                case 'r': out[i++] = '\r'; break;
                case 't': out[i++] = '\t'; break;
                default: return 0;
            }
            p++;
            continue;
        }

        out[i++] = *p++;
    }

    if (p >= span.end || *p != '"') return 0;

    out[i] = '\0';
    return 1;
}
