project(
    'tasks',
    'cpp',

    version: '0.1',

    default_options: [
        'cpp_std=c++23',
        'buildtype=debug'
    ]
)

add_project_link_arguments(
    '-lstdc++exp', # enable std::print
    language: 'cpp'
)

subproject('app')
subproject('sqlitecpp')

deps = [
    dependency('libapp'),
    dependency('sqlitecpp')
]

subdir('src')

executable(
    'tasks',
    tasks_src,

    dependencies: deps
)