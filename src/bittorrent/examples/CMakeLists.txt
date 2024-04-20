build_lib_example(
  NAME vodsim
  SOURCE_FILES vodsim.cc
  LIBRARIES_TO_LINK
    ${libbittorrent}
)
build_lib_example(
  NAME vodsim-no-real-time
  SOURCE_FILES vodsim-no-realtime.cc
  LIBRARIES_TO_LINK
    ${libbittorrent}
)
