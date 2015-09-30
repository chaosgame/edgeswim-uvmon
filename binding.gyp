{
  "targets": [
    {
      "include_dirs" : [
          "<!(node -e \"require('nan')\")"
      ],
      "cflags": [
        "-Wall",
        "-Werror"
      ],
      "target_name": "uvmon",
      "sources": ["uvmon.cc", "histogram.cc", "profiler_wrapper.cc"]
    }
  ]
}
