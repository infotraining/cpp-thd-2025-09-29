project (thread_safe_queue_tests)

find_package(Threads REQUIRED)

find_package(Catch2 3)

if(NOT Catch2_FOUND)
  Include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.0 # or a later release
  )
  FetchContent_MakeAvailable(Catch2)
endif()

enable_testing()

add_executable(thread_safe_queue_tests thread_safe_queue_tests.cpp)
target_link_libraries(thread_safe_queue_tests PRIVATE thread_safe_queue_lib Threads::Threads Catch2::Catch2WithMain)