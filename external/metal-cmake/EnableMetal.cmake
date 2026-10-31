include(CheckLanguage)
include(MetalShaderSupport)

if(APPLE)
    check_language(Metal)

    if(CMAKE_Metal_COMPILER)
        enable_language(Metal)
        message(STATUS "Metal detected as supported language. Included into build.")
    else()
        execute_process(COMMAND xcode-select -p
                OUTPUT_VARIABLE _developer_dir OUTPUT_STRIP_TRAILING_WHITESPACE)

        if(_developer_dir MATCHES "CommandLineTools")
            message(WARNING "Metal compiler not found: active developer directory is Command Line Tools.\n"
                    "Install Xcode, then run:\n"
                    "  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer\n"
                    "  xcodebuild -downloadComponent MetalToolchain")
        else()
            message(WARNING "Metal toolchain is not installed. Run:\n"
                    "  xcodebuild -downloadComponent MetalToolchain")
        endif()

        unset(CMAKE_Metal_COMPILER CACHE)
    endif()
else()
    message(STATUS "Metal is not available on this platform. Skipping.")
endif()