set(SIMDJSON_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SIMDJSON_INCLUDE
    ${SIMDJSON_INCLUDE_DIR}/simdjson/common_defs.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/isadetection.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/jsonformatutils.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/jsonioutil.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/jsonminifier.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/jsonparser.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/padded_string.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/parsedjson.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/parsedjsoniterator.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/portability.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/simdjson.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/simdjson_version.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/stage1_find_marks.h
    ${SIMDJSON_INCLUDE_DIR}/simdjson/stage2_build_tape.h
)