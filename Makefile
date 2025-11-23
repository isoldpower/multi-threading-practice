BUILD_DIR ?= build
SOURCE_DIR ?= .
CPP_COMPILER ?= g++
C_COMPILER ?= gcc
BUILD_TYPE ?= Release
CLEAN ?= 0

compile:
	if test "$(CLEAN)" -eq "1" && test -d "$(BUILD_DIR)"; then rm -rf "$(BUILD_DIR)"; fi
	cmake -B "$(BUILD_DIR)" -S "$(SOURCE_DIR)" \
			-DCMAKE_CXX_COMPILER="$(CPP_COMPILER)" \
			-DCMAKE_C_COMPILER="$(C_COMPILER)" \
			-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)"
	cmake --build "$(BUILD_DIR)" --config "$(BUILD_TYPE)"
rebuild:
	$(MAKE) compile CLEAN=1
rebuild-debug:
	$(MAKE) rebuild BUILD_TYPE=Debug
rebuild-release:
	$(MAKE) rebuild BUILD_TYPE=Release

test:
	if ! test -d "$(BUILD_DIR)"; then "$(MAKE)" rebuild-debug; fi
	GTEST_COLOR=1 ctest --test-dir "$(BUILD_DIR)" --build-config "$(BUILD_TYPE)" --output-on-failure
