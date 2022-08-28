# As of version 3.0 OpenSSL low level ecdsa is deprecated for external use, but is stated to continue to stay stable for internal use, therefore I am using it
# linux-test: tests/encoder.cpp tests/abi.cpp tests/contextInit.cpp tests/erc20.cpp tests/fixed.cpp

TESTS_DIR := ./tests
OBJ_DIR := ./objs
SRC_FILES := $(wildcard $(TESTS_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(TESTS_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
LDFLAGS := -lssl -lcrypto -lboost_random -lboost_program_options -lboost_unit_test_framework -lboost_system
CPPFLAGS := -Wall -Wno-deprecated-declarations -pedantic -std=gnu++23 -ggdb -fdiagnostics-color

linux-test: $(OBJ_FILES)
	g++ -o $@ $^ $(LDFLAGS)	

$(OBJ_DIR)/%.o: $(TESTS_DIR)/%.cpp
	g++ $(CPPFLAGS) -c -o $@ $<



windows: main.cpp
	g++ main.cpp -o result -lssl -lcrypto -Wall -pedantic -std=gnu++23 -lboost_random-mt -lboost_program_options-mt -lboost_json-mt -ggdb -fdiagnostics-color -lws2_32 -lwsock32 -lcrypt32 -fuse-ld=lld

windows-test: tests/encoder.cpp tests/abi.cpp tests/contextInit.cpp tests/erc20.cpp tests/fixed.cpp
	g++ $^ -o test -lssl -lcrypto -Wall -pedantic -std=gnu++23 -lboost_random-mt -lboost_program_options-mt -lboost_json-mt -ggdb -fdiagnostics-color -lws2_32 -lwsock32 -lcrypt32 -lboost_unit_test_framework-mt -lboost_system-mt -fuse-ld=lld

linux-static: main.cpp
	g++ main.cpp -o result -lssl -lcrypto -Wall -pedantic -std=gnu++20 -lboost_random -lboost_program_options -static -static-libgcc -static-libstdc++

linux: main.cpp
	g++ main.cpp -o result -lssl -lcrypto -Wall -pedantic -std=gnu++20 -lboost_random -lboost_program_options

windows-static: main.cpp
	g++ main.cpp -o result -lssl -lcrypto -Wall -pedantic -std=gnu++20 -lboost_random-mt -lboost_program_options-mt -lboost_json-mt -ggdb -fdiagnostics-color -lws2_32 -lwsock32 -static -static-libgcc -static-libstdc++
