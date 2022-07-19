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
