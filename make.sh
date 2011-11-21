set -e
set -u


# Set this to 0 to disable assertions in the hotpath. This will make the
# solver slower at the expense of better self-checking.
config_assert_hotpath=1

# Set this to 1 to display verbose debug messages.
config_debug=0


defines="-DCONFIG_ASSERT_HOTPATH=${config_assert_hotpath} -DCONFIG_DEBUG=${config_debug}"

g++ -std=gnu++0x -O3 -Wall -g -D_LGPL_SOURCE ${defines} -o solver main.cc -lboost_program_options -lpthread
