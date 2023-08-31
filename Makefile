.PHONY: lint iwyu cppcheck install

headers := $(shell find include -type f -name '*.hpp')
srcs := $(shell find src -type f -name '*.cpp')

cppflags := -Werror -Wall -Wextra -Wnon-virtual-dtor -Wno-psabi -Wold-style-cast -Wcast-align -Wunused -Wno-gnu-anonymous-struct -Wno-nested-anon-types

bin:
	mkdir -p bin

bin/alpaca-daemon: bin src/alpaca-daemon.cpp $(headers) include/nlohmann/json.hpp
	g++ -O3 -std=c++20 $(cppflags) src/alpaca-daemon.cpp -o bin/alpaca-daemon $(shell pkg-config --cflags libhttpserver) $(shell pkg-config --libs libhttpserver) -I./include -fno-exceptions -fno-rtti

include/nlohmann/json.hpp:
	mkdir -p include/nlohmann
	wget https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp -O include/nlohmann/json.hpp
	wget https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json_fwd.hpp -O include/nlohmann/json_fwd.hpp

bin/test-usb: bin src/test-usb.cpp $(headers)
	g++ -std=c++20 -Wno-psabi -Wall -Werror src/test-usb.cpp -o bin/test-usb $(shell pkg-config --cflags libhttpserver) $(shell pkg-config --libs libhttpserver) -Iinclude

iwyu: $(headers) $(srcs)
	for f in $^; do \
	  include-what-you-use -std=c++20 -Wno-psabi -Wall -Werror $$f $(shell pkg-config --cflags libhttpserver) -Iinclude ; \
	done

cppcheck: $(headers) $(srcs)
	for f in $^; do \
	  cppcheck --std=c++20 -Iinclude --suppress=missingIncludeSystem \
	    --enable=warning --enable=style --enable=performance --enable=portability \
	    --enable=information --enable=missingInclude $$f ; \
	done

lint:
	cpplint --exclude include/rva/ --recursive .

install:
	systemctl stop alpaca-daemon.service || echo "alpaca deamon not installed"
	cp alpaca-daemon.service /etc/systemd/system/alpaca-daemon.service
	cp bin/alpaca-daemon /usr/local/bin/alpaca-daemon
	systemctl daemon-reload
	systemctl enable alpaca-daemon.service
	systemctl start alpaca-daemon.service
	systemctl status alpaca-daemon.service
