BUILD_DIR ?= build
SOURCE_DIR ?= .
CPP_COMPILER ?= g++
C_COMPILER ?= gcc
BUILD_TYPE ?= Release
CLEAN ?= 0
PRESERVE ?= 0
LINUX_IMAGE ?= multi-threading-practice-linux
LINUX_PLATFORM ?= linux/amd64
# Compiler pairs (C:C++) run by test-linux, the same ones the ubuntu CI legs use
LINUX_COMPILERS ?= gcc:g++ clang:clang++

compile:
	if test "$(CLEAN)" -eq "1" && test -d "$(BUILD_DIR)"; then rm -rf "$(BUILD_DIR)"; fi
	cmake -B "$(BUILD_DIR)" -S "$(SOURCE_DIR)" \
	      -DCMAKE_CXX_COMPILER="$(CPP_COMPILER)" \
	      -DCMAKE_C_COMPILER="$(C_COMPILER)" \
	      -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
	      $(if $(filter Debug,$(BUILD_TYPE)),-DBUILD_TESTING=ON,-DBUILD_TESTING=OFF) \
	      $(EXTRA_CMAKE_FLAGS)

	cmake --build "$(BUILD_DIR)" --config "$(BUILD_TYPE)"

rebuild:
	$(MAKE) compile CLEAN=1
rebuild-debug:
	$(MAKE) rebuild BUILD_TYPE=Debug
rebuild-release:
	$(MAKE) rebuild BUILD_TYPE=Release
rebuild-asan:
	$(MAKE) rebuild BUILD_TYPE=Debug EXTRA_CMAKE_FLAGS="-DENABLE_ASAN=ON"

test:
	if test "$(PRESERVE)" -eq "0"; then $(MAKE) rebuild-debug; fi
	GTEST_COLOR=1 ctest --test-dir "$(BUILD_DIR)" --build-config "Debug" --output-on-failure
test-asan:
	if test "$(PRESERVE)" -eq "0"; then $(MAKE) rebuild-asan; fi
	GTEST_COLOR=1 ctest --test-dir "$(BUILD_DIR)" --build-config "Debug" --output-on-failure

linux-image:
	docker build --platform "$(LINUX_PLATFORM)" -t "$(LINUX_IMAGE)" -f docker/linux.Dockerfile docker
# Runs what each ubuntu CI leg runs: a clean Release build, then the Debug test suite
test-linux: linux-image
	for pair in $(LINUX_COMPILERS); do \
	    c_compiler=$${pair%%:*}; cpp_compiler=$${pair##*:}; \
	    echo "==> Linux $$cpp_compiler"; \
	    docker run --rm --platform "$(LINUX_PLATFORM)" -v "$(CURDIR):/src:ro" -w /src "$(LINUX_IMAGE)" \
	        sh -c "export CMAKE_BUILD_PARALLEL_LEVEL=\$$(nproc) \
	               && make rebuild BUILD_DIR=/tmp/build BUILD_TYPE=Release \
	                       CPP_COMPILER=$$cpp_compiler C_COMPILER=$$c_compiler \
	               && make test BUILD_DIR=/tmp/build \
	                       CPP_COMPILER=$$cpp_compiler C_COMPILER=$$c_compiler" \
	        || exit 1; \
	done

.PHONY: compile rebuild rebuild-debug rebuild-release rebuild-asan test test-asan linux-image test-linux
