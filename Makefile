ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: all
all: build

.PHONY: build
build:
	gcc sgdbop.c curl-helper.c string-helpers.c -lcurl -o linux-release/SGDBop

.PHONY: install
install: build
	install -d linux-release/SGDBop $(DESTDIR)$(PREFIX)/bin
