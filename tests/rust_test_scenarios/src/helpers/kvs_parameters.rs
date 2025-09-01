//! KVS parameters test helpers.

use rust_kvs::prelude::{FlushOnExit, InstanceId, OpenNeedDefaults, OpenNeedKvs};
use serde::{de, Deserialize, Deserializer};
use serde_json::Value;
use std::path::PathBuf;

/// KVS parameters in serde-compatible format.
#[derive(Deserialize, Debug)]
pub struct KvsParameters {
    #[serde(deserialize_with = "deserialize_instance_id")]
    pub instance_id: InstanceId,
    #[serde(default, deserialize_with = "deserialize_need_defaults")]
    pub need_defaults: Option<OpenNeedDefaults>,
    #[serde(default, deserialize_with = "deserialize_need_kvs")]
    pub need_kvs: Option<OpenNeedKvs>,
    pub dir: Option<PathBuf>,
    #[serde(default, deserialize_with = "deserialize_flush_on_exit")]
    pub flush_on_exit: Option<FlushOnExit>,
}

impl KvsParameters {
    /// Parse `KvsParameters` from JSON string.
    /// JSON is expected to contain `kvs_parameters` field.
    pub fn from_json(json_str: &str) -> Result<Self, serde_json::Error> {
        let v: Value = serde_json::from_str(json_str)?;
        serde_json::from_value(v["kvs_parameters"].clone())
    }
}

fn deserialize_instance_id<'de, D>(deserializer: D) -> Result<InstanceId, D::Error>
where
    D: Deserializer<'de>,
{
    let value = usize::deserialize(deserializer)?;
    Ok(InstanceId(value))
}

fn deserialize_need_defaults<'de, D>(deserializer: D) -> Result<Option<OpenNeedDefaults>, D::Error>
where
    D: Deserializer<'de>,
{
    let value_opt: Option<String> = Option::deserialize(deserializer)?;
    if let Some(value_str) = value_opt {
        let value = match value_str.as_str() {
            "optional" => OpenNeedDefaults::Optional,
            "required" => OpenNeedDefaults::Required,
            _ => return Err(de::Error::custom("Invalid \"need_defaults\" mode")),
        };
        return Ok(Some(value));
    }

    Ok(None)
}

fn deserialize_need_kvs<'de, D>(deserializer: D) -> Result<Option<OpenNeedKvs>, D::Error>
where
    D: Deserializer<'de>,
{
    let value_opt: Option<String> = Option::deserialize(deserializer)?;
    if let Some(value_str) = value_opt {
        let value = match value_str.as_str() {
            "optional" => OpenNeedKvs::Optional,
            "required" => OpenNeedKvs::Required,
            _ => return Err(de::Error::custom("Invalid \"need_kvs\" mode")),
        };
        return Ok(Some(value));
    }

    Ok(None)
}

fn deserialize_flush_on_exit<'de, D>(deserializer: D) -> Result<Option<FlushOnExit>, D::Error>
where
    D: Deserializer<'de>,
{
    let value_opt: Option<bool> = Option::deserialize(deserializer)?;
    if let Some(value_bool) = value_opt {
        let value = match value_bool {
            true => FlushOnExit::Yes,
            false => FlushOnExit::No,
        };
        return Ok(Some(value));
    }

    Ok(None)
}
