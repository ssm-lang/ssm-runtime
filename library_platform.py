Import("env")
from os.path import join, realpath

platform = env['PIOFRAMEWORK'][0]
board = env['PIOENV']

platform_dir = realpath(join("platform", platform))
boards_dir = realpath(join(platform_dir, "boards"))
board_dir = realpath(join(boards_dir, board))

# Specify platform/board includes
env.Append(
    CPPPATH=[
        realpath(join(platform_dir, "include")),
        realpath(join(board_dir, "include")),
    ]
)
# Specify common and platform source directories
env.Replace(
    SRC_FILTER=[
        "-<*>",
        "+<src>",
        "+<%s>" % platform_dir,
        "-<%s>" % boards_dir,
        "+<%s>" % board_dir,
    ]
)
# Platform defines
env.Append(
    CPPDEFINES=[
        # This will be needed if we ever have a 64-bit timer platform,
        # to set PLATFORM_TIMER64
    ]
)
