/**
 * @file test_json.c
 * @brief Simple test to read calibration.json and print selected fields.
 */

#include <stdio.h>
#include <stdlib.h>
#include "json_utils.h"

#define CALIBRATION_JSON_PATH "../data/machine/calibration.json"
#define CAL_JSON_MAX_SIZE (64 * 1024)
#define CAL_DATE_MAX_LEN 64


/**
 * @brief Print calibration section fields.
 *
 * @param label Section label for output.
 * @param span Object span of the section.
 * @return 0 on success, non-zero on failure.
 */
static int PrintCalibrationSection(const char *label, JsonSpan_t span)
{
    int is_calibrated = 0;
    char date_buf[CAL_DATE_MAX_LEN];

    if (!JsonUtils_ParseBoolInSpan(span, "is_calibrated", &is_calibrated)) {
        return -1;
    }

    if (!JsonUtils_ParseStringInSpan(span, "calibration_date", date_buf, sizeof(date_buf))) {
        return -1;
    }

    printf("%s: is_calibrated=%s, calibration_date=%s\n",
           label,
           is_calibrated ? "true" : "false",
           date_buf);

    return 0;
}

/**
 * @brief Entry point for JSON calibration test.
 *
 * @return 0 on success, non-zero on failure.
 */
int main(void)
{
    char *json = NULL;

    if (JsonUtils_ReadFileToBuffer(CALIBRATION_JSON_PATH, &json, NULL, CAL_JSON_MAX_SIZE) != 0) {
        printf("Failed to read calibration.json\n");
        return 1;
    }

    JsonSpan_t tilt_span;
    JsonSpan_t rotate_span;

    if (!JsonUtils_FindObjectSpan(json, "tilt", &tilt_span)) {
        free(json);
        printf("Missing tilt section\n");
        return 1;
    }

    if (!JsonUtils_FindObjectSpan(json, "rotate", &rotate_span)) {
        free(json);
        printf("Missing rotate section\n");
        return 1;
    }

    if (PrintCalibrationSection("tilt", tilt_span) != 0) {
        free(json);
        printf("Failed to parse tilt section\n");
        return 1;
    }

    if (PrintCalibrationSection("rotate", rotate_span) != 0) {
        free(json);
        printf("Failed to parse rotate section\n");
        return 1;
    }

    free(json);
    return 0;
}
