/**
 * @file json_utils.h
 * @brief Minimal JSON parsing helpers for flat objects.
 */

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Span of a JSON object within a buffer.
 */
typedef struct
{
    const char *start;
    const char *end;
} JsonSpan_t;

/**
 * @brief Read a JSON file into a null-terminated buffer.
 *
 * @param path File path to read.
 * @param out_buf Output pointer for allocated buffer.
 * @param out_len Output pointer for buffer length, may be NULL.
 * @param max_len Maximum allowed file size.
 * @return 0 on success, non-zero on failure.
 */
int JsonUtils_ReadFileToBuffer(const char *path, char **out_buf, size_t *out_len, size_t max_len);

/**
 * @brief Find the span of an object value for a top-level key.
 *
 * @param json Null-terminated JSON string.
 * @param key Object key to locate.
 * @param span Output span covering the object text.
 * @return 1 if found, 0 if not found.
 */
int JsonUtils_FindObjectSpan(const char *json, const char *key, JsonSpan_t *span);

/**
 * @brief Parse a number value for a key within a span.
 *
 * @param span Object span to search.
 * @param key Key to parse.
 * @param out Output double value.
 * @return 1 if parsed, 0 if not found or invalid.
 */
int JsonUtils_ParseNumberInSpan(JsonSpan_t span, const char *key, double *out);

/**
 * @brief Parse a boolean value for a key within a span.
 *
 * @param span Object span to search.
 * @param key Key to parse.
 * @param out Output boolean as int (1 true, 0 false).
 * @return 1 if parsed, 0 if not found or invalid.
 */
int JsonUtils_ParseBoolInSpan(JsonSpan_t span, const char *key, int *out);

/**
 * @brief Parse a string value for a key within a span.
 *
 * @param span Object span to search.
 * @param key Key to parse.
 * @param out Output buffer for the string.
 * @param out_len Length of output buffer.
 * @return 1 if parsed, 0 if not found or invalid.
 */
int JsonUtils_ParseStringInSpan(JsonSpan_t span, const char *key, char *out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* JSON_UTILS_H */
