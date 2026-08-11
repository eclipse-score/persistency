load("@score_docs_as_code//:docs.bzl", "docs_bundle")

docs_bundle(
    name = "kvs_docs",
    code_targets = [
        "//src/cpp/src:kvs_cpp",
        "//src/rust/rust_kvs:rust_kvs",
    ],
    source_dir = "kvs/docs",
    visibility = ["//visibility:public"],
)

docs_bundle(
    name = "json_docs",
    source_dir = "json/docs",
    visibility = ["//visibility:public"],
)
