# I have Wno-deprecated-declarations because as of version 3.0 OpenSSL low level ecdsa is deprecated for external use.
# It is stated to continue to stay for internal use (and is used internally a fair bit), so I am using it.

TESTS_DIR := ./tests
OBJ_DIR := ./objs
SRC_FILES := $(wildcard $(TESTS_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(TESTS_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
LDFLAGS := -lssl -lcrypto -lboost_random -lboost_program_options -lboost_unit_test_framework -lboost_system -fsanitize=address
CPPFLAGS := -Wall -Wno-deprecated-declarations -pedantic -std=gnu++23 -ggdb -fdiagnostics-color -fsanitize=address

# linux-test: objs/contextInit.o objs/erc20.o
linux-test: $(OBJ_FILES)
	g++ -o $@ $^ $(LDFLAGS)	

$(OBJ_DIR)/%.o: $(TESTS_DIR)/%.cpp
	g++ $(CPPFLAGS) -c -o $@ $<

clean:
	rm $(OBJ_DIR)/*.o
#	rm linux-test

windows-test: tests/encoder.cpp tests/abi.cpp tests/contextInit.cpp tests/erc20.cpp tests/fixed.cpp
	g++ $^ -o test -lssl -lcrypto -Wall -pedantic -std=gnu++23 -lboost_random-mt -lboost_program_options-mt -lboost_json-mt -ggdb -fdiagnostics-color -lws2_32 -lwsock32 -lcrypt32 -lboost_unit_test_framework-mt -lboost_system-mt -fuse-ld=lld