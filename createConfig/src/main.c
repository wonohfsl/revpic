#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

#define CATALOG_PATH "/var/www/revpi/pictory/resources/data/catalog.json"
#define TEMPLATE_DIR "./device_templates"
#define OUTPUT_CONFIG "config.rsc"

typedef struct {
    char title[128];
    char key[128];
} IODevice;

/* Helper: ceiling division */
static int ceil_div(int a, int b) {
    return (a + b - 1) / b;
}

/* Compute byte length of a section (inp/out/mem) */
static int compute_section_length(json_t *section) {
    if (!section || !json_is_object(section))
        return 0;

    const char *key;
    json_t *value;
    int max_end = 0;

    json_object_foreach(section, key, value) {
        if (!json_is_array(value) || json_array_size(value) < 4)
            continue;

        json_t *bit_len_json  = json_array_get(value, 2);
        json_t *byte_off_json = json_array_get(value, 3);

        if (!json_is_string(bit_len_json) || !json_is_string(byte_off_json))
            continue;

        int bit_len  = atoi(json_string_value(bit_len_json));
        int byte_off = atoi(json_string_value(byte_off_json));

        int bytes = ceil_div(bit_len, 8);
        int end   = byte_off + bytes;

        if (end > max_end)
            max_end = end;
    }

    return max_end;
}

/* Compute total device byte length */
static int compute_device_length(json_t *device) {
    int inp_len = compute_section_length(json_object_get(device, "inp"));
    int out_len = compute_section_length(json_object_get(device, "out"));
    int mem_len = compute_section_length(json_object_get(device, "mem"));

    int max = inp_len;
    if (out_len > max) max = out_len;
    if (mem_len > max) max = mem_len;

    return max;
}

/* Compute Summary totals */
static void compute_summary_totals(json_t *devices_array, int *inp_total, int *out_total) {
    *inp_total = 0;
    *out_total = 0;

    for (size_t i = 0; i < json_array_size(devices_array); i++) {
        json_t *dev = json_array_get(devices_array, i);

        json_t *inp = json_object_get(dev, "inp");
        json_t *out = json_object_get(dev, "out");

        const char *key;
        json_t *value;

        /* Inputs */
        if (inp && json_is_object(inp)) {
            json_object_foreach(inp, key, value) {
                if (!json_is_array(value) || json_array_size(value) < 3)
                    continue;

                int bit_len = atoi(json_string_value(json_array_get(value, 2)));
                *inp_total += ceil_div(bit_len, 8);
            }
        }

        /* Outputs */
        if (out && json_is_object(out)) {
            json_object_foreach(out, key, value) {
                if (!json_is_array(value) || json_array_size(value) < 3)
                    continue;

                int bit_len = atoi(json_string_value(json_array_get(value, 2)));
                *out_total += ceil_div(bit_len, 8);
            }
        }
    }
}

/* Load device template JSON */
static json_t *load_device_template(const char *device_key) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.json", TEMPLATE_DIR, device_key);

    json_error_t error;
    json_t *device = json_load_file(path, 0, &error);

    if (!device) {
        fprintf(stderr, "Error loading template '%s': %s\n", path, error.text);
        return NULL;
    }

    return device;
}

int main(void) {
    json_error_t error;
    json_t *catalog = json_load_file(CATALOG_PATH, 0, &error);

    if (!catalog) {
        fprintf(stderr, "Error loading catalog.json: %s\n", error.text);
        return 1;
    }

    IODevice devices[32];
    int device_count = 0;

    printf("Please select I/O device to add:\n");

    /* Find "I/O Devices" group */
    for (size_t i = 0; i < json_array_size(catalog); i++) {
        json_t *group = json_array_get(catalog, i);
        const char *group_title = json_string_value(json_object_get(group, "title"));

        if (group_title && strcmp(group_title, "I/O Devices") == 0) {
            json_t *children = json_object_get(group, "children");

            for (size_t j = 0; j < json_array_size(children); j++) {
                json_t *dev = json_array_get(children, j);

                const char *dev_title = json_string_value(json_object_get(dev, "title"));
                const char *dev_key   = json_string_value(json_object_get(dev, "key"));

                if (dev_title && dev_key) {
                    strncpy(devices[device_count].title, dev_title, 127);
                    strncpy(devices[device_count].key, dev_key, 127);

                    printf("%2d. %s\n", device_count + 1, dev_title);
                    device_count++;
                }
            }
            break;
        }
    }

    if (device_count == 0) {
        printf("No I/O devices found.\n");
        return 1;
    }

    /* User selects device */
    int choice;
    printf("\nEnter device number: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > device_count) {
        printf("Invalid choice.\n");
        return 1;
    }

    IODevice selected = devices[choice - 1];

    /* Ask for slot */
    int slot;
    printf("\nWhich slot the device is located?\n");
    printf("... | 29 | 30 | 31 | 0 Base Device | 1 | 2 | ...\n");
    printf("Enter slot number: ");
    scanf("%d", &slot);

    json_decref(catalog);

    /* Load template */
    json_t *device_template = load_device_template(selected.key);
    if (!device_template)
        return 1;

    /* Build Devices array */
    json_t *devices_array = json_array();

    json_t *dev = json_deep_copy(device_template);

    /* Compute offset */
    int dev_len = compute_device_length(dev);
    json_object_set_new(dev, "offset", json_integer(0));

    /* Set position */
    char pos_str[32];
    snprintf(pos_str, sizeof(pos_str), "%d", slot);
    json_object_set_new(dev, "position", json_string(pos_str));

    /* Update name/bmk */
    json_object_set_new(dev, "name", json_string(selected.title));
    json_object_set_new(dev, "bmk", json_string(selected.title));

    json_array_append_new(devices_array, dev);
    json_decref(device_template);

    /* Build App */
    json_t *app = json_object();
    json_object_set_new(app, "name", json_string("PiCtory"));
    json_object_set_new(app, "version", json_string("2.14.0"));
    json_object_set_new(app, "saveTS", json_string("20250101000000"));
    json_object_set_new(app, "language", json_string("en"));

    json_t *layout = json_object();
    json_object_set_new(app, "layout", layout);

    /* Build Summary */
    int inp_total, out_total;
    compute_summary_totals(devices_array, &inp_total, &out_total);

    json_t *summary = json_object();
    json_object_set_new(summary, "inpTotal", json_integer(inp_total));
    json_object_set_new(summary, "outTotal", json_integer(out_total));

    /* Empty Connections */
    json_t *connections = json_array();

    /* Build root */
    json_t *root = json_object();
    json_object_set_new(root, "App", app);
    json_object_set_new(root, "Summary", summary);
    json_object_set_new(root, "Devices", devices_array);
    json_object_set_new(root, "Connections", connections);

    /* Write config.rsc */
    if (json_dump_file(root, OUTPUT_CONFIG, JSON_INDENT(2)) != 0) {
        fprintf(stderr, "Failed to write %s\n", OUTPUT_CONFIG);
        return 1;
    }

    printf("\nconfig.rsc generated.\n");
    printf("Device: %s\n", selected.title);
    printf("Slot:   %d\n", slot);
    printf("Device length: %d bytes\n", dev_len);

    json_decref(root);
    return 0;
}
