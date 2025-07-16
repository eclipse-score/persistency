// Abstraction for JSON handling in KVS
// This module provides a trait for JSON operations and an implementation using tinyjson

use std::collections::HashMap;

pub type JsonValue = tinyjson::JsonValue;

// Implementation for tinyjson
use tinyjson::{JsonGenerateError, JsonParseError};

#[derive(Default)]
pub struct TinyJson;

// Conversion between KvsValue and JsonValue
use crate::kvs_value::KvsValue;

pub trait KvsJson {
    type Value;
    fn parse(s: &str) -> Result<Self::Value, crate::error_code::ErrorCode>;
    fn stringify(val: &Self::Value) -> Result<String, crate::error_code::ErrorCode>;
    fn get_object(val: &Self::Value) -> Option<&HashMap<String, Self::Value>>;
    fn get_array(val: &Self::Value) -> Option<&Vec<Self::Value>>;
    fn get_f64(val: &Self::Value) -> Option<f64>;
    fn get_bool(val: &Self::Value) -> Option<bool>;
    fn get_string(val: &Self::Value) -> Option<&str>;
    fn is_null(val: &Self::Value) -> bool;
    fn to_kvs_value(val: Self::Value) -> KvsValue;
    /// Convert a &KvsValue into the backend's JSON value type
    fn from_kvs_value(val: &KvsValue) -> Self::Value;
}

impl KvsJson for TinyJson {
    type Value = JsonValue;
    fn parse(s: &str) -> Result<Self::Value, crate::error_code::ErrorCode> {
        s.parse()
            .map_err(|_e: JsonParseError| crate::error_code::ErrorCode::JsonParserError)
    }
    fn stringify(val: &Self::Value) -> Result<String, crate::error_code::ErrorCode> {
        val.stringify()
            .map_err(|_e: JsonGenerateError| crate::error_code::ErrorCode::JsonParserError)
    }
    fn get_object(val: &Self::Value) -> Option<&HashMap<String, Self::Value>> {
        val.get::<HashMap<String, JsonValue>>()
    }
    fn get_array(val: &Self::Value) -> Option<&Vec<Self::Value>> {
        val.get::<Vec<JsonValue>>()
    }
    fn get_f64(val: &Self::Value) -> Option<f64> {
        val.get::<f64>().copied()
    }
    fn get_bool(val: &Self::Value) -> Option<bool> {
        val.get::<bool>().copied()
    }
    fn get_string(val: &Self::Value) -> Option<&str> {
        val.get::<String>().map(|s| s.as_str())
    }
    fn is_null(val: &Self::Value) -> bool {
        matches!(val, JsonValue::Null)
    }
    fn to_kvs_value(val: Self::Value) -> KvsValue {
        KvsValue::from(&val)
    }
    fn from_kvs_value(val: &KvsValue) -> Self::Value {
        JsonValue::from(val)
    }
}
