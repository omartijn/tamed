set(test-sources
    main.cpp
)

add_executable(tamed-test ${test-sources})
target_link_libraries(tamed-test tamed::tamed)
