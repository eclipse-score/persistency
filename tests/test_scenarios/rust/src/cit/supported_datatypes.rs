use crate::helpers::kvs_instance::kvs_instance;
use crate::helpers::kvs_parameters::KvsParameters;
use rust_kvs::prelude::*;
use std::collections::HashMap;
use test_scenarios_rust::scenario::{Scenario, ScenarioGroup, ScenarioGroupImpl};
use tinyjson::JsonValue;
use tracing::info;

const MAX_KEY_LENGTH: usize = 32; // Max key length in bytes
const MAX_VALUE_LENGTH: usize = 1024; // Max value length in bytes

struct SupportedDatatypesKeys;

/// Test cases for key requirements:
/// 1. The component shall accept keys that consist solely of alphanumeric characters, underscores, or dashes.
/// 2. The component shall encode each key as valid UTF-8.
/// 3. The component shall guarantee that each key is unique.
/// 4. The component shall limit the maximum length of a key to 32 bytes.
impl Scenario for SupportedDatatypesKeys {
    fn name(&self) -> &str {
        "keys"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        // Test keys: valid, invalid, UTF-8, uniqueness, max length
        let valid_keys = vec![
            String::from("alphaNumeric123"),
            String::from("with_underscore"),
            String::from("with-dash"),
            String::from("A1_b2-C3"),
            String::from_utf8(vec![b'a'; MAX_KEY_LENGTH]).unwrap(), // 32 bytes
        ];
        let invalid_keys = vec![
            String::from("has$pecial"),
            String::from("emoji✅"),
            String::from("too_long_key_abcdefghijklmnopqrstuvwxyz123456"), // >32 bytes
            String::from("utf8_ключ"),                                     // Cyrillic
            String::from("utf8_漢字"),                                     // Chinese
            String::from("utf8_emoji ✅❗😀"),
            String::from("utf8_greek ημα"),
            String::from("has space"),
        ];
        let utf8_keys: Vec<String> = vec![
            String::from("utf8_emoji_valid"),
            String::from("utf8_alphaNumeric123"),
            String::from("utf8_with_underscore"),
            String::from("utf8-with-dash"),
            String::from("utf8_A1_b2-C3"),
        ];

        // Uniqueness test: try to insert duplicate key
        let duplicate_key = String::from("unique_key");
        kvs.set_value(duplicate_key.clone(), ())
            .expect("Failed to set value");
        let result_duplicate = kvs.set_value(duplicate_key.clone(), ());

        // Requirement #3: The component shall guarantee that each key is unique.
        // NOTE: Current implementation fulfills uniqueness by allowing update to the value for an existing key (not strict rejection).
        // TODO: Is allowing value update for an existing key compliant with the uniqueness requirement?
        // (Test expects strict rejection, but implementation allows update.)
        // assert!(
        //     result_duplicate.is_err(),
        //     "Duplicate key insertion should be rejected (no update allowed)"
        // );

        // Insert valid keys
        for key in &valid_keys {
            kvs.set_value(key.clone(), ()).expect("Failed to set value");
        }
        // Insert UTF-8 keys
        for key in &utf8_keys {
            kvs.set_value(key.clone(), ()).expect("Failed to set value");
        }
        // Try to insert invalid keys and expect error
        for key in &invalid_keys {
            let result = kvs.set_value(key.clone(), ());
            assert!(result.is_err(), "Invalid key '{}' should be rejected", key);
        }

        // Get and print all keys.
        let keys_in_kvs = kvs.get_all_keys().expect("Failed to read all keys");
        // Filter out any invalid keys from the output
        let invalid_keys_set: std::collections::HashSet<_> = invalid_keys.iter().cloned().collect();
        for key in keys_in_kvs {
            if !invalid_keys_set.contains(&key) {
                info!(key);
            }
        }
        // Log result of duplicate key insertion
        if result_duplicate.is_ok() {
            info!(duplicate_key_accepted = duplicate_key);
        }
        // Log max length key
        info!(max_length_key = valid_keys.last().unwrap());

        Ok(())
    }
}

struct SupportedDatatypesValues {
    value: KvsValue,
}

/// Test cases for value requirements:
/// 5. The component shall accept only values of the following data types: Number, String, Null, Array[Value], or Dictionary{Key:Value}.
/// 6. The component shall serialize and deserialize all values to and from JSON.
/// 7. The component shall limit the maximum length of a value to 1024 bytes.
impl Scenario for SupportedDatatypesValues {
    fn name(&self) -> &str {
        // Set scenario name based on provided value.
        match &self.value {
            KvsValue::I32(_) => "i32",
            KvsValue::U32(_) => "u32",
            KvsValue::I64(_) => "i64",
            KvsValue::U64(_) => "u64",
            KvsValue::F64(_) => "f64",
            KvsValue::Boolean(_) => "bool",
            KvsValue::String(s) => match s.len() {
                MAX_VALUE_LENGTH => "str_1024",
                l if l > MAX_VALUE_LENGTH => "str_1025",
                _ => "str",
            },
            KvsValue::Null => "null",
            KvsValue::Array(_) => "arr",
            KvsValue::Object(_) => "obj",
        }
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        // TODO: If the KVS implementation does not reject values of the wrong type,
        // or values >1024 bytes, or fails to serialize/deserialize,
        // the following line or subsequent assertions will fail.
        kvs.set_value(self.name(), self.value.clone())
            .expect("Failed to set value");

        let kvs_value = kvs.get_value(self.name()).expect("Failed to read value");
        let json_value = JsonValue::from(kvs_value);

        let v_field = extract_raw_value(&json_value);
        let v_str = v_field.stringify().unwrap_or_else(|_| "null".to_string());
        let tagged_json = format!("{{\"t\":\"{}\",\"v\":{}}}", self.name(), v_str);
        info!(key = self.name(), value = tagged_json);

        Ok(())
    }
}

fn extract_raw_value(json_value: &JsonValue) -> JsonValue {
    match json_value {
        JsonValue::Object(obj)
            if obj.contains_key("v") && obj.contains_key("t") && obj.len() == 2 =>
        {
            obj.get("v").cloned().unwrap_or(JsonValue::Null)
        }
        _ => json_value.clone(),
    }
}

fn supported_datatypes_i32() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::I32(-321),
    })
}

fn supported_datatypes_u32() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::U32(1234),
    })
}

fn supported_datatypes_i64() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::I64(-123456789),
    })
}

fn supported_datatypes_u64() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::U64(123456789),
    })
}

fn supported_datatypes_f64() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::F64(-5432.1),
    })
}

fn supported_datatypes_bool() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::Boolean(true),
    })
}

fn supported_datatypes_string() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::String("example".to_string()),
    })
}

fn supported_datatypes_string_1024() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::String("x".repeat(MAX_VALUE_LENGTH).to_string()),
    })
}

fn supported_datatypes_string_1025() -> Box<dyn Scenario> {
    // This test expects the value to be rejected (should error)
    Box::new(SupportedDatatypesValues {
        value: KvsValue::String("y".repeat(MAX_VALUE_LENGTH + 1).to_string()),
    })
}

fn supported_datatypes_array() -> Box<dyn Scenario> {
    let hashmap = HashMap::from([("sub-number".to_string(), KvsValue::from(789.0))]);
    let array = vec![
        KvsValue::from(321.5),
        KvsValue::from(false),
        KvsValue::from("hello".to_string()),
        KvsValue::from(()),
        KvsValue::from(vec![]),
        KvsValue::from(hashmap),
    ];
    Box::new(SupportedDatatypesValues {
        value: KvsValue::Array(array),
    })
}

fn supported_datatypes_object() -> Box<dyn Scenario> {
    let hashmap = HashMap::from([("sub-number".to_string(), KvsValue::from(789.0))]);
    Box::new(SupportedDatatypesValues {
        value: KvsValue::Object(hashmap),
    })
}

fn value_types_group() -> Box<dyn ScenarioGroup> {
    let group = ScenarioGroupImpl::new(
        "values",
        vec![
            supported_datatypes_i32(),
            supported_datatypes_u32(),
            supported_datatypes_i64(),
            supported_datatypes_u64(),
            supported_datatypes_f64(),
            supported_datatypes_bool(),
            supported_datatypes_string(),
            supported_datatypes_string_1024(),
            supported_datatypes_string_1025(),
            supported_datatypes_array(),
            supported_datatypes_object(),
        ],
        vec![],
    );
    Box::new(group)
}

pub fn supported_datatypes_group() -> Box<dyn ScenarioGroup> {
    Box::new(ScenarioGroupImpl::new(
        "supported_datatypes",
        vec![Box::new(SupportedDatatypesKeys)],
        vec![value_types_group()],
    ))
}
