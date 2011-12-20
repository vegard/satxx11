set -e
set -u


# Set this to 0 to disable assertions in the hotpath. This will make the
# solver slower at the expense of better self-checking.
config_assert_hotpath=1

# Set this to 1 to display verbose debug messages.
config_debug=0


git_revision="`git rev-parse --verify HEAD`"

(echo -n 'static const char git_diff[] = "';
	git diff | sed -r 's/([\"\\\n])/\\\1/g' | sed -r 's/$/\\n\\/g';
	echo '";') > .git_diff.hh

(echo -n 'static const char git_diff_cached[] = "';
	git diff --cached | sed -r 's/([\"\\\n])/\\\1/g' | sed -r 's/$/\\n\\/g';
	echo '";') > .git_diff_cached.hh

defines="-DCONFIG_ASSERT_HOTPATH=${config_assert_hotpath} -DCONFIG_DEBUG=${config_debug} -DGIT_REVISION=\"${git_revision}\""

g++ -std=gnu++0x -O3 -Wall -g -Iinclude ${defines} -o solver main.cc -lboost_program_options -lpthread
