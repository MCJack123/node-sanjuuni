{
    "targets": [{
        "target_name": "sanjuuni",
        "cflags": [ '-fexceptions' ],
        "cflags_cc": [ '-fexceptions' ],
        "sources": [
            "sanjuuni/src/cc-pixel.cpp",
            "sanjuuni/src/cc-pixel-cl.cpp",
            "sanjuuni/src/generator.cpp",
            "sanjuuni/src/octree.cpp",
            "sanjuuni/src/quantize.cpp",
            "module.cpp"
        ],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")",
            "sanjuuni/src"
        ],
        'libraries': [],
        'dependencies': [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        'defines': [ 'NO_POCO' ]
    }]
}