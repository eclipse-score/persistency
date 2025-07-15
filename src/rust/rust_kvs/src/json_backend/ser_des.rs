//! Serialization and deserialization logic for KvsValue <-> JsonValue

use super::json_value::JsonValue;
use crate::kvs_value::KvsValue;
use std::collections::HashMap;

impl From<&JsonValue> for KvsValue {
    fn from(val: &JsonValue) -> KvsValue {
        match val {
            JsonValue::Number(n) => KvsValue::Number(*n),
            JsonValue::Boolean(b) => KvsValue::Boolean(*b),
            JsonValue::String(s) => KvsValue::String(s.clone()),
            JsonValue::Null => KvsValue::Null,
            JsonValue::Array(arr) => KvsValue::Array(arr.iter().map(KvsValue::from).collect()),
            JsonValue::Object(obj) => KvsValue::Object(
                obj.iter()
                    .map(|(k, v)| (k.clone(), KvsValue::from(v)))
                    .collect(),
            ),
        }
    }
}

impl From<&KvsValue> for JsonValue {
    fn from(val: &KvsValue) -> JsonValue {
        match val {
            KvsValue::Number(n) => JsonValue::Number(*n),
            KvsValue::Boolean(b) => JsonValue::Boolean(*b),
            KvsValue::String(s) => JsonValue::String(s.clone()),
            KvsValue::Null => JsonValue::Null,
            KvsValue::Array(arr) => JsonValue::Array(arr.iter().map(JsonValue::from).collect()),
            KvsValue::Object(map) => JsonValue::Object(
                map.iter()
                    .map(|(k, v)| (k.clone(), JsonValue::from(v)))
                    .collect(),
            ),
        }
    }
}
