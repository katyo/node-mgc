ifeq ($(mode),debug)
flags += -d
endif

all:
	@echo Use with target: config build test

config:
	@node-gyp $(flags) configure

build:
	@node-gyp $(flags) build

test:
	@cd test && node simple
	@cd test && node brutal

.PHONY: config build test
