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
} DeviceDef;

typedef struct {
    json_t *dev;   /* deep-copied device JSON */
    int position;  /* numeric slot */
    int length;    /* device length in bytes */
} DeviceInstance;

/* ---------- Helpers ---------- */

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

/* Total device byte length (max of inp/out/mem end offsets) */
static int compute_device_length(json_t *device) {
    int inp_len = compute_section_length(json_object_get(device, "inp"));
    int out_len = compute_section_length(json_object_get(device, "out"));
    int mem_len = compute_section_length(json_object_get(device, "mem"));

    int max = inp_len;
    if (out_len > max) max = out_len;
    if (mem_len > max) max = mem_len;

    return max;
}

/* Compute Summary.inpTotal / outTotal in bytes */
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
                json_t *bit_len_json = json_array_get(value, 2);
                if (!json_is_string(bit_len_json))
                    continue;
                int bit_len = atoi(json_string_value(bit_len_json));
                *inp_total += ceil_div(bit_len, 8);
            }
        }

        /* Outputs */
        if (out && json_is_object(out)) {
            json_object_foreach(out, key, value) {
                if (!json_is_array(value) || json_array_size(value) < 3)
                    continue;
                json_t *bit_len_json = json_array_get(value, 2);
                if (!json_is_string(bit_len_json))
                    continue;
                int bit_len = atoi(json_string_value(bit_len_json));
                *out_total += ceil_div(bit_len, 8);
            }
        }
    }
}

/* Load device template JSON: TEMPLATE_DIR/<device_key>.json */
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

/* Slot ordering: left modules (31,30,...) before base(0), then right modules (1,2,...) */
static int device_position_cmp(const void *a, const void *b) {
    const DeviceInstance *da = (const DeviceInstance *)a;
    const DeviceInstance *db = (const DeviceInstance *)b;
    int pa = da->position;
    int pb = db->position;

    /* Decide side: left (-1), base (0), right (+1) */
    int side_a = 0;
    int side_b = 0;

    if (pa == 0) side_a = 0;
    else if (pa >= 16) side_a = -1;   /* left of base (e.g. 31,30,29...) */
    else side_a = +1;                 /* right of base (1,2,3...) */

    if (pb == 0) side_b = 0;
    else if (pb >= 16) side_b = -1;
    else side_b = +1;

    if (side_a != side_b)
        return side_a - side_b;       /* left (-1) < base (0) < right (1) */

    /* Same side: determine ordering within side */
    if (side_a == -1) {
        /* Left side: 31,30,29... (descending) */
        return pb - pa;
    } else if (side_a == +1) {
        /* Right side: 1,2,3... (ascending) */
        return pa - pb;
    } else {
        /* Both base (should be only one) */
        return 0;
    }
}

/* ---------- Catalog parsing ---------- */

static int load_device_group(json_t *catalog, const char *group_name,
                             DeviceDef *list, int max_count) {
    int count = 0;

    for (size_t i = 0; i < json_array_size(catalog); i++) {
        json_t *group = json_array_get(catalog, i);
        if (!json_is_object(group))
            continue;

        const char *title = json_string_value(json_object_get(group, "title"));
        if (!title || strcmp(title, group_name) != 0)
            continue;

        json_t *children = json_object_get(group, "children");
        if (!children || !json_is_array(children))
            continue;

        for (size_t j = 0; j < json_array_size(children) && count < max_count; j++) {
            json_t *dev = json_array_get(children, j);
            if (!json_is_object(dev))
                continue;

            const char *dev_title = json_string_value(json_object_get(dev, "title"));
            const char *dev_key   = json_string_value(json_object_get(dev, "key"));

            if (!dev_title || !dev_key)
                continue;

            strncpy(list[count].title, dev_title, sizeof(list[count].title) - 1);
            list[count].title[sizeof(list[count].title) - 1] = '\0';

            strncpy(list[count].key, dev_key, sizeof(list[count].key) - 1);
            list[count].key[sizeof(list[count].key) - 1] = '\0';

            count++;
        }
        break; /* only one group with this name */
    }

    return count;
}

/* ---------- main ---------- */

int main(void) {
    json_error_t error;
    json_t *catalog = json_load_file(CATALOG_PATH, 0, &error);

    if (!catalog) {
        fprintf(stderr, "Error loading catalog.json: %s\n", error.text);
        return 1;
    }

    /* 1) Select Base Device (Connect 4/5/Core/etc.) */
    DeviceDef base_list[16];
    int base_count = load_device_group(catalog, "Base Devices", base_list, 16);

    if (base_count == 0) {
        printf("No Base Devices found in catalog.\n");
        json_decref(catalog);
        return 1;
    }

    printf("Select Base Device:\n");
    for (int i = 0; i < base_count; i++) {
        printf("%2d. %s\n", i + 1, base_list[i].title);
    }

    int base_choice;
    printf("\nEnter base device number: ");
    if (scanf("%d", &base_choice) != 1 || base_choice < 1 || base_choice > base_count) {
        printf("Invalid choice.\n");
        json_decref(catalog);
        return 1;
    }

    DeviceDef base_selected = base_list[base_choice - 1];

    /* 2) Select I/O Devices (MIO, DIO, DI, DO, AIO, RO, etc.) */

    DeviceDef io_list[32];
    int io_count = load_device_group(catalog, "I/O Devices", io_list, 32);

    if (io_count == 0) {
        printf("No I/O Devices found in catalog.\n");
        json_decref(catalog);
        return 1;
    }

    /* We'll collect instances (base + multiple IO) */
    DeviceInstance instances[32];
    int inst_count = 0;

    /* 2a) Add Base Device at slot 0 automatically (you can change this if needed) */
    json_t *base_tmpl = load_device_template(base_selected.key);
    if (!base_tmpl) {
        json_decref(catalog);
        return 1;
    }

    json_t *base_dev = json_deep_copy(base_tmpl);
    int base_len = compute_device_length(base_dev);

    /* Set base position "0" as string */
    json_object_set_new(base_dev, "position", json_string("0"));
    json_object_set_new(base_dev, "name", json_string(base_selected.title));
    json_object_set_new(base_dev, "bmk",  json_string(base_selected.title));
    /* offset will be computed later after sorting */

    instances[inst_count].dev      = base_dev;
    instances[inst_count].position = 0;
    instances[inst_count].length   = base_len;
    inst_count++;

    json_decref(base_tmpl);

    /* 2b) Loop: add I/O devices */
    int add_more = 1;
    while (add_more && inst_count < (int)(sizeof(instances)/sizeof(instances[0]))) {
        printf("\nSelect I/O Device to add (0 = finish):\n");
        for (int i = 0; i < io_count; i++) {
            printf("%2d. %s\n", i + 1, io_list[i].title);
        }
        printf(" 0. Finish\n");

        int io_choice;
        printf("Enter choice: ");
        if (scanf("%d", &io_choice) != 1) {
            printf("Invalid input.\n");
            json_decref(catalog);
            return 1;
        }

        if (io_choice == 0) {
            add_more = 0;
            break;
        }

        if (io_choice < 1 || io_choice > io_count) {
            printf("Invalid device choice.\n");
            continue;
        }

        DeviceDef io_selected = io_list[io_choice - 1];

        int slot;
        printf("\nWhich slot the device is located?\n");
        printf("... | 29 | 30 | 31 | 0 Base Device | 1 | 2 | ...\n");
        printf("Enter slot number for %s: ", io_selected.title);
        if (scanf("%d", &slot) != 1) {
            printf("Invalid slot.\n");
            json_decref(catalog);
            return 1;
        }

        if (slot == 0) {
            printf("Slot 0 is reserved for Base Device. Skipping.\n");
            continue;
        }

        json_t *tmpl = load_device_template(io_selected.key);
        if (!tmpl) {
            json_decref(catalog);
            return 1;
        }

        json_t *dev = json_deep_copy(tmpl);
        int dev_len = compute_device_length(dev);

        char pos_str[32];
        snprintf(pos_str, sizeof(pos_str), "%d", slot);
        json_object_set_new(dev, "position", json_string(pos_str));
        json_object_set_new(dev, "name", json_string(io_selected.title));
        json_object_set_new(dev, "bmk",  json_string(io_selected.title));
        /* offset will be computed later */

        instances[inst_count].dev      = dev;
        instances[inst_count].position = slot;
        instances[inst_count].length   = dev_len;
        inst_count++;

        json_decref(tmpl);
    }

    json_decref(catalog);

    if (inst_count == 0) {
        printf("No devices selected.\n");
        return 1;
    }

    /* 3) Sort devices by slot (left, base, right) and compute offsets */
    qsort(instances, inst_count, sizeof(DeviceInstance), device_position_cmp);

    int offset = 0;
    for (int i = 0; i < inst_count; i++) {
        json_object_set_new(instances[i].dev, "offset", json_integer(offset));
        offset += instances[i].length;
    }

    /* 4) Build Devices array in sorted order */
    json_t *devices_array = json_array();
    for (int i = 0; i < inst_count; i++) {
        json_array_append_new(devices_array, instances[i].dev);
        /* do NOT decref here, root will own them */
    }

    /* 5) Build App */
    json_t *app = json_object();
    json_object_set_new(app, "name",     json_string("PiCtory"));
    json_object_set_new(app, "version",  json_string("2.14.0"));
    json_object_set_new(app, "saveTS",   json_string("20250101000000"));
    json_object_set_new(app, "language", json_string("en"));
    json_t *layout = json_object();
    json_object_set_new(app, "layout", layout);

    /* 6) Build Summary */
    int inp_total = 0, out_total = 0;
    compute_summary_totals(devices_array, &inp_total, &out_total);

    json_t *summary = json_object();
    json_object_set_new(summary, "inpTotal", json_integer(inp_total));
    json_object_set_new(summary, "outTotal", json_integer(out_total));

    /* 7) Connections (empty) */
    json_t *connections = json_array();

    /* 8) Root config */
    json_t *root = json_object();
    json_object_set_new(root, "App",         app);
    json_object_set_new(root, "Summary",     summary);
    json_object_set_new(root, "Devices",     devices_array);
    json_object_set_new(root, "Connections", connections);

    /* 9) Write config.rsc */
    if (json_dump_file(root, OUTPUT_CONFIG, JSON_INDENT(0)) != 0) {
        fprintf(stderr, "Failed to write %s\n", OUTPUT_CONFIG);
        json_decref(root);
        return 1;
    }

    printf("\nconfig.rsc generated in current folder.\n");
    printf("Base device: %s (slot 0)\n", base_selected.title);
    printf("Total devices: %d\n", inst_count);
    printf("Total input bytes:  %d\n", inp_total);
    printf("Total output bytes: %d\n", out_total);

    json_decref(root);
    return 0;
}
