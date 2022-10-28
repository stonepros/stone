add_stone_test(test_stone_daemon.py
  ${Python3_EXECUTABLE} -m nose ${CMAKE_CURRENT_SOURCE_DIR}/test_stone_daemon.py)
add_stone_test(test_stone_argparse.py
  ${Python3_EXECUTABLE} -m nose ${CMAKE_CURRENT_SOURCE_DIR}/test_stone_argparse.py)
