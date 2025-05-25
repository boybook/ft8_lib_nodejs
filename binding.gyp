{
  "targets": [
    {
      "target_name": "ft8_lib",
      "sources": [
        "src/ft8_lib_node.cpp",
        "src/message_wrapper.cpp",
        "src/encoder_wrapper.cpp",
        "src/decoder_wrapper.cpp",
        "src/audio_utils.cpp",
        "ft8_lib/ft8/constants.c",
        "ft8_lib/ft8/crc.c",
        "ft8_lib/ft8/decode.c",
        "ft8_lib/ft8/encode.c",
        "ft8_lib/ft8/ldpc.c",
        "ft8_lib/ft8/message.c",
        "ft8_lib/ft8/text.c",
        "ft8_lib/common/audio.c",
        "ft8_lib/common/monitor.c",
        "ft8_lib/common/wave.c",
        "ft8_lib/fft/kiss_fft.c",
        "ft8_lib/fft/kiss_fftr.c"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "ft8_lib",
        "ft8_lib/ft8",
        "ft8_lib/common",
        "ft8_lib/fft"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "libraries": [],
      "conditions": [
        ["OS=='win'", {
          "defines": [
            "_WIN32_WINNT=0x0600"
          ],
          "conditions": [
            ["target_arch=='x64' and CC=='gcc'", {
              # MinGW配置
              "cflags_cc": [
                "-std=c++20",
                "-fexceptions"
              ],
              "cflags_c": [
                "-std=c99"
              ]
            }, {
              # MSVC配置
              "msvs_settings": {
                "VCCLCompilerTool": {
                  "ExceptionHandling": 1,
                  "AdditionalOptions": ["/std:c++20"]
                }
              }
            }]
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.12"
          }
        }],
        ["OS=='linux'", {
          "cflags_cc": [
            "-std=c++17",
            "-fexceptions"
          ]
        }]
      ]
    },
    {
      "target_name": "ft8_lib_static",
      "type": "static_library",
      "sources": [
        "ft8_lib/ft8/constants.c",
        "ft8_lib/ft8/crc.c",
        "ft8_lib/ft8/decode.c",
        "ft8_lib/ft8/encode.c",
        "ft8_lib/ft8/ldpc.c",
        "ft8_lib/ft8/message.c",
        "ft8_lib/ft8/text.c",
        "ft8_lib/common/monitor.c",
        "ft8_lib/common/wave.c",
        "ft8_lib/fft/kiss_fft.c",
        "ft8_lib/fft/kiss_fftr.c"
      ],
      "include_dirs": [
        "ft8_lib",
        "ft8_lib/ft8",
        "ft8_lib/common",
        "ft8_lib/fft"
      ],
      "defines": [
        "LOG_LEVEL=0"
      ],
      "conditions": [
        ["OS=='win'", {
          "defines": [
            "_CRT_SECURE_NO_WARNINGS"
          ]
        }]
      ]
    }
  ]
}
