#pragma once

typedef enum
{
    VALUE_TYPE_STRING,
    VALUE_TYPE_INT32,
} persistence_value_type_t;

void persistence_init(const char *namespace_name);
void persistence_save(persistence_value_type_t value_type, const char *key, const void *value);
void *persistence_load(persistence_value_type_t value_type, const char *key, void *out);
void persistence_deinit();
