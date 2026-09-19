include(FetchContent)
# Override with -DFETCHCONTENT_SOURCE_DIR_QUANTUM_UI=/path/to/quantum-ui.
# Once published, replace SOURCE_DIR with GIT_REPOSITORY and a pinned GIT_TAG.
FetchContent_Declare(quantum_ui SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../packages/quantum-ui")
FetchContent_MakeAvailable(quantum_ui)
