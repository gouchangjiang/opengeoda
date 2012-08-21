ifeq ($(shell uname),Darwin)
MAC = 1
endif

ifeq ($(shell uname),MinGW)
WINDOWS = 1
endif

ifeq ($(shell uname),Linux)
LINUX = 1
endif
