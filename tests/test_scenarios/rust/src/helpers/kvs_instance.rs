//! KVS instance test helpers.

use crate::helpers::kvs_parameters::KvsParameters;
use rust_kvs::prelude::{ErrorCode, JsonBackendBuilder, Kvs, KvsBuilder};

/// Create KVS instance based on provided parameters.
pub fn kvs_instance(kvs_parameters: KvsParameters) -> Result<Kvs, ErrorCode> {
    let mut builder = KvsBuilder::new(kvs_parameters.instance_id);

    // Set `defaults` mode.
    if let Some(flag) = kvs_parameters.defaults {
        builder = builder.defaults(flag);
    }

    // Set `kvs_load` mode.
    if let Some(flag) = kvs_parameters.kvs_load {
        builder = builder.kvs_load(flag);
    }

    // Set `backend`.
    let mut backend_builder = JsonBackendBuilder::new();
    let mut set_backend = false;

    // Set working directory.
    if let Some(working_dir) = kvs_parameters.dir {
        backend_builder = backend_builder.working_dir(working_dir);
        set_backend = true;
    }

    // Set max number of snapshots.
    if let Some(snapshot_max_count) = kvs_parameters.snapshot_max_count {
        backend_builder = backend_builder.snapshot_max_count(snapshot_max_count);
        set_backend = true;
    }

    // Set backend, if backend parameters were provided.
    if set_backend {
        builder = builder.backend(Box::new(backend_builder.build()));
    }

    let kvs: Kvs = builder.build()?;

    Ok(kvs)
}
