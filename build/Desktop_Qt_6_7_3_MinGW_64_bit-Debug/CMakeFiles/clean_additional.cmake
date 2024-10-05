# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\HeavyInsight_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\HeavyInsight_autogen.dir\\ParseCache.txt"
  "HeavyInsight_autogen"
  )
endif()
