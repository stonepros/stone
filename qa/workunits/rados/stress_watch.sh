#!/bin/sh -e

stone_test_stress_watch
stone_multi_stress_watch rep reppool repobj
stone_multi_stress_watch ec ecpool ecobj

exit 0
