add_definitions (
# log_lib
#    -DLOG_LOCKFREE_QUEUE
)
aux_source_directory (. DIR_SRC)
add_library (log_lib STATIC ${DIR_SRC})
