Import("env")
from os.path import join, realpath

platform = env['PIOFRAMEWORK'][0]
board = env['PIOENV']

# Specify platform/board includes
env.Append(
    CPPPATH=[
        realpath(join("platform", platform, "include")),
        realpath(join("platform", platform, "boards", board, "include")),
    ]
)
# Specify common and platform source directories
env.Replace(
    SRC_FILTER=[
        "-<*>",
        "+<src/>",
        "+<%s/>" % realpath(join("platform", platform)),
    ]
)
# Platform defines
env.Append(
    CPPDEFINES=[
        "PLATFORM_BOARD_%s" % board,
    ]
)
