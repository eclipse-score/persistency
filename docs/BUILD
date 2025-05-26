load("@score_docs_as_code//:docs.bzl", "docs")

# Creates all documentation targets:
# - `docs:incremental` for building docs incrementally at runtime
# - `docs:live_preview` for live preview in the browser without an IDE
# - `docs:ide_support` for creating python virtualenv for IDE support
# - `docs:docs` for building documentation at build-time

docs(
    #   conf_dir = "docs",
    docs_targets = [
        {
            "suffix": "",
            "external_needs_info": [
                {
                    "base_url": "https://eclipse-score.github.io/score/main",
                    "json_url": "https://eclipse-score.github.io/score/main/needs.json",
                    "id_prefix": "score_",
                },
            ],
        },
    ],
    #    source_dir = "docs",
    source_files_to_scan_for_needs_links = [],
)
