// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************
use crate::error_code::ErrorCode;
use crate::kvs_value::{KvsMap, KvsValue};

/// `KvsValue` serialization trait.
/// Allows object to be serialized into `KvsValue`.
pub trait KvsSerialize {
    type Error;

    /// Serialize object to `KvsValue`.
    fn to_kvs(&self) -> Result<KvsValue, Self::Error>;
}

macro_rules! impl_kvs_serialize_for_t_checked_cast {
    ($t:ty, $internal_t:ty, $variant:ident) => {
        impl KvsSerialize for $t {
            type Error = ErrorCode;

            fn to_kvs(&self) -> Result<KvsValue, Self::Error> {
                if let Ok(casted) = <$internal_t>::try_from(self.clone()) {
                    Ok(KvsValue::$variant(casted))
                } else {
                    Err(ErrorCode::SerializationFailed(
                        "Value to KvsValue cast failed".to_string(),
                    ))
                }
            }
        }
    };
}

macro_rules! impl_kvs_serialize_for_t {
    ($t:ty, $variant:ident) => {
        impl KvsSerialize for $t {
            type Error = ErrorCode;

            fn to_kvs(&self) -> Result<KvsValue, Self::Error> {
                Ok(KvsValue::$variant(self.clone()))
            }
        }
    };
}

impl_kvs_serialize_for_t!(u8, U8);
impl_kvs_serialize_for_t!(u16, U16);
impl_kvs_serialize_for_t!(u32, U32);
impl_kvs_serialize_for_t!(u64, U64);
impl_kvs_serialize_for_t_checked_cast!(usize, u64, U64);
impl_kvs_serialize_for_t!(i8, I8);
impl_kvs_serialize_for_t!(i16, I16);
impl_kvs_serialize_for_t!(i32, I32);
impl_kvs_serialize_for_t!(i64, I64);
impl_kvs_serialize_for_t_checked_cast!(isize, i64, I64);
impl_kvs_serialize_for_t!(f32, F32);
impl_kvs_serialize_for_t!(f64, F64);
impl_kvs_serialize_for_t!(bool, Boolean);
impl_kvs_serialize_for_t!(String, String);
impl_kvs_serialize_for_t!(Vec<KvsValue>, Array);
impl_kvs_serialize_for_t!(KvsMap, Object);

impl KvsSerialize for &str {
    type Error = ErrorCode;

    fn to_kvs(&self) -> Result<KvsValue, Self::Error> {
        Ok(KvsValue::String(self.to_string()))
    }
}

impl KvsSerialize for () {
    type Error = ErrorCode;

    fn to_kvs(&self) -> Result<KvsValue, Self::Error> {
        Ok(KvsValue::Null)
    }
}

/// `KvsValue` deserialization trait.
/// Allows object to be deserialized from `KvsValue`.
pub trait KvsDeserialize: Sized {
    type Error;

    /// Deserialize object from `KvsValue`.
    fn from_kvs(kvs_value: &KvsValue) -> Result<Self, Self::Error>;
}

macro_rules! impl_kvs_deserialize_for_t_checked_cast {
    ($t:ty, $variant:ident) => {
        impl KvsDeserialize for $t {
            type Error = ErrorCode;

            fn from_kvs(kvs_value: &KvsValue) -> Result<Self, Self::Error> {
                match kvs_value {
                    KvsValue::$variant(value) => <$t>::try_from(value.clone())
                        .map_err(|_| ErrorCode::DeserializationFailed("KvsValue to value cast failed".to_string())),
                    _ => Err(ErrorCode::DeserializationFailed(
                        "Invalid KvsValue variant provided".to_string(),
                    )),
                }
            }
        }
    };
}

macro_rules! impl_kvs_deserialize_for_t {
    ($t:ty, $variant:ident) => {
        impl KvsDeserialize for $t {
            type Error = ErrorCode;

            fn from_kvs(kvs_value: &KvsValue) -> Result<Self, Self::Error> {
                if let KvsValue::$variant(value) = kvs_value {
                    Ok(value.clone())
                } else {
                    Err(ErrorCode::DeserializationFailed(
                        "Invalid KvsValue variant provided".to_string(),
                    ))
                }
            }
        }
    };
}

impl_kvs_deserialize_for_t!(u8, U8);
impl_kvs_deserialize_for_t!(u16, U16);
impl_kvs_deserialize_for_t!(u32, U32);
impl_kvs_deserialize_for_t!(u64, U64);
impl_kvs_deserialize_for_t_checked_cast!(usize, U64);
impl_kvs_deserialize_for_t!(i8, I8);
impl_kvs_deserialize_for_t!(i16, I16);
impl_kvs_deserialize_for_t!(i32, I32);
impl_kvs_deserialize_for_t!(i64, I64);
impl_kvs_deserialize_for_t_checked_cast!(isize, I64);
impl_kvs_deserialize_for_t!(f32, F32);
impl_kvs_deserialize_for_t!(f64, F64);
impl_kvs_deserialize_for_t!(bool, Boolean);
impl_kvs_deserialize_for_t!(String, String);
impl_kvs_deserialize_for_t!(Vec<KvsValue>, Array);
impl_kvs_deserialize_for_t!(KvsMap, Object);

impl KvsDeserialize for () {
    type Error = ErrorCode;

    fn from_kvs(kvs_value: &KvsValue) -> Result<Self, Self::Error> {
        if let KvsValue::Null = kvs_value {
            Ok(())
        } else {
            Err(ErrorCode::DeserializationFailed(
                "Invalid KvsValue variant provided".to_string(),
            ))
        }
    }
}

#[cfg(test)]
mod serialize_tests {
    use crate::kvs_serialize::{KvsDeserialize, KvsSerialize};
    use crate::kvs_value::{KvsMap, KvsValue};

    macro_rules! serialize_numeric_test {
        ($name:ident, $t:ty, $variant:ident) => {
            #[test]
            fn $name() {
                for value in [
                    <$t>::MIN,
                    <$t>::MIN / (3 as $t),
                    (1 as $t),
                    <$t>::MAX / (3 as $t),
                    <$t>::MAX,
                ] {
                    let kvs_value = value.to_kvs().unwrap();
                    assert_eq!(kvs_value, KvsValue::$variant(value));
                    assert_eq!(kvs_value.get::<$t>(), Some(&value));
                    assert_eq!(<$t>::try_from(&kvs_value).unwrap(), value);
                    assert_eq!(<$t>::from_kvs(&kvs_value).unwrap(), value);
                }
            }
        };
    }

    serialize_numeric_test!(test_i8_ok, i8, I8);
    serialize_numeric_test!(test_i16_ok, i16, I16);
    serialize_numeric_test!(test_i32_ok, i32, I32);
    serialize_numeric_test!(test_i64_ok, i64, I64);
    serialize_numeric_test!(test_u8_ok, u8, U8);
    serialize_numeric_test!(test_u16_ok, u16, U16);
    serialize_numeric_test!(test_u32_ok, u32, U32);
    serialize_numeric_test!(test_u64_ok, u64, U64);
    serialize_numeric_test!(test_f32_ok, f32, F32);
    serialize_numeric_test!(test_f64_ok, f64, F64);

    #[test]
    fn test_isize_ok() {
        for value in [isize::MIN, -123, 0, 123, isize::MAX] {
            let kvs_value = value.to_kvs().unwrap();
            assert_eq!(kvs_value, KvsValue::I64(value as i64));
        }
    }

    #[test]
    fn test_usize_ok() {
        for value in [usize::MIN, 123, usize::MAX] {
            let kvs_value = value.to_kvs().unwrap();
            assert_eq!(kvs_value, KvsValue::U64(value as u64));
        }
    }

    #[test]
    fn test_bool_ok() {
        let value = true;
        let kvs_value = value.to_kvs().unwrap();
        assert_eq!(kvs_value, KvsValue::Boolean(value));
    }

    #[test]
    fn test_string_ok() {
        let value = "test".to_string();
        let kvs_value = value.to_kvs().unwrap();
        assert_eq!(kvs_value, KvsValue::String(value));
    }

    #[test]
    fn test_str_ok() {
        let value = "test";
        let kvs_value = value.to_kvs().unwrap();
        assert_eq!(kvs_value, KvsValue::String(value.to_string()));
    }

    #[test]
    fn test_array_ok() {
        let value = vec![
            KvsValue::String("one".to_string()),
            KvsValue::String("two".to_string()),
            KvsValue::String("three".to_string()),
        ];
        let kvs_value = value.to_kvs().unwrap();
        assert_eq!(kvs_value, KvsValue::Array(value));
    }

    #[test]
    fn test_object_ok() {
        let value = KvsMap::from([
            ("first".to_string(), KvsValue::from(-321i32)),
            ("second".to_string(), KvsValue::from(1234u32)),
            ("third".to_string(), KvsValue::from(true)),
        ]);
        let kvs_value = value.to_kvs().unwrap();
        assert_eq!(kvs_value, KvsValue::Object(value));
    }

    #[test]
    fn test_unit_ok() {
        let value = ();
        let kvs_value = value.to_kvs().unwrap();
        assert_eq!(kvs_value, KvsValue::Null);
    }
}

#[cfg(test)]
mod deserialize_tests {
    use crate::error_code::ErrorCode;
    use crate::kvs_serialize::KvsDeserialize;
    use crate::kvs_value::{KvsMap, KvsValue};

    #[test]
    fn test_i8_ok() {
        for value in [i8::MIN, i8::MAX / 3, i8::MAX] {
            let kvs_value = KvsValue::I8(value);
            assert_eq!(i8::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_i8_invalid_variant() {
        let kvs_value = KvsValue::I32(i8::MIN as i32);
        let result = i8::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_i16_ok() {
        for value in [i16::MIN, i16::MAX / 3, i16::MAX] {
            let kvs_value = KvsValue::I16(value);
            assert_eq!(i16::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_i16_invalid_variant() {
        let kvs_value = KvsValue::I32(i16::MIN as i32);
        let result = i16::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_i32_ok() {
        for value in [i32::MIN, i32::MAX / 3, i32::MAX] {
            let kvs_value = KvsValue::I32(value);
            assert_eq!(i32::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_i32_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = i32::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_i64_ok() {
        for value in [i64::MIN, i64::MAX / 3, i64::MAX] {
            let kvs_value = KvsValue::I64(value);
            assert_eq!(i64::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_i64_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = i64::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_isize_ok() {
        for value in [isize::MIN, isize::MAX / 3, isize::MAX] {
            let kvs_value = KvsValue::I64(value as i64);
            assert_eq!(isize::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_isize_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = isize::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_u8_ok() {
        for value in [u8::MIN, u8::MAX / 3, u8::MAX] {
            let kvs_value = KvsValue::U8(value);
            assert_eq!(u8::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_u8_invalid_variant() {
        let kvs_value = KvsValue::U32(u8::MIN as u32);
        let result = u8::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_u16_ok() {
        for value in [u16::MIN, u16::MAX / 3, u16::MAX] {
            let kvs_value = KvsValue::U16(value);
            assert_eq!(u16::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_u16_invalid_variant() {
        let kvs_value = KvsValue::U32(u16::MIN as u32);
        let result = u16::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_u32_ok() {
        for value in [u32::MIN, u32::MAX / 3, u32::MAX] {
            let kvs_value = KvsValue::U32(value);
            assert_eq!(u32::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_u32_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = u32::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_u64_ok() {
        for value in [u64::MIN, u64::MAX / 3, u64::MAX] {
            let kvs_value = KvsValue::U64(value);
            assert_eq!(u64::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_u64_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = u64::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_usize_ok() {
        for value in [usize::MIN, usize::MAX / 3, usize::MAX] {
            let kvs_value = KvsValue::U64(value as u64);
            assert_eq!(usize::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_usize_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = usize::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_f32_ok() {
        for value in [f32::MIN, -12.5, 0.0, f32::MIN_POSITIVE, 0.1, f32::MAX] {
            let kvs_value = KvsValue::F32(value);
            assert_eq!(f32::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_f32_invalid_variant() {
        let kvs_value = KvsValue::F64(12.5);
        let result = f32::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_f64_ok() {
        for value in [f64::MIN, -5432.1, 0.0, f64::MIN_POSITIVE, 0.1, f64::MAX] {
            let kvs_value = KvsValue::F64(value);
            assert_eq!(f64::from_kvs(&kvs_value).unwrap(), value);
        }
    }

    #[test]
    fn test_f64_invalid_variant() {
        let kvs_value = KvsValue::F32(12.5);
        let result = f64::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_bool_ok() {
        let kvs_value = KvsValue::Boolean(true);
        let value = bool::from_kvs(&kvs_value).unwrap();
        assert_eq!(value, *kvs_value.get::<bool>().unwrap());
    }

    #[test]
    fn test_bool_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = bool::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_string_ok() {
        let kvs_value = KvsValue::String("test".to_string());
        let value = String::from_kvs(&kvs_value).unwrap();
        assert_eq!(value, *kvs_value.get::<String>().unwrap());
    }

    #[test]
    fn test_string_invalid_variant() {
        let kvs_value = KvsValue::Boolean(true);
        let result = String::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_array_ok() {
        let kvs_value = KvsValue::Array(vec![
            KvsValue::String("one".to_string()),
            KvsValue::String("two".to_string()),
            KvsValue::String("three".to_string()),
        ]);
        let value = Vec::from_kvs(&kvs_value).unwrap();
        assert_eq!(value, *kvs_value.get::<Vec<KvsValue>>().unwrap());
    }

    #[test]
    fn test_array_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = Vec::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_object_ok() {
        let kvs_value = KvsValue::Object(KvsMap::from([
            ("first".to_string(), KvsValue::from(-321i32)),
            ("second".to_string(), KvsValue::from(1234u32)),
            ("third".to_string(), KvsValue::from(true)),
        ]));
        let value = KvsMap::from_kvs(&kvs_value).unwrap();
        assert_eq!(value, *kvs_value.get::<KvsMap>().unwrap());
    }

    #[test]
    fn test_object_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = KvsMap::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }

    #[test]
    fn test_unit_ok() {
        let kvs_value = KvsValue::Null;
        <()>::from_kvs(&kvs_value).unwrap();
        // No need for comparing unit values.
    }

    #[test]
    fn test_unit_invalid_variant() {
        let kvs_value = KvsValue::String("invalid string".to_string());
        let result = <()>::from_kvs(&kvs_value);
        assert!(result
            .is_err_and(|e| e == ErrorCode::DeserializationFailed("Invalid KvsValue variant provided".to_string())));
    }
}
