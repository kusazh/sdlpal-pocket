.DEFAULT_GOAL := help
.PHONY: help build release renderer test
JOBS ?= 6

help:
	@echo 'make build     Build the Pocket ELF (Docker/OrbStack; no game data)'
	@echo 'make release   Build resource-free ZIP with source and licenses'
	@echo 'make renderer  Build the host FM converter (C++11)'
	@echo 'make test GAME_DIR=/path/to/PAL  Desktop check with private DOS data'

build:
	bash tools/sdk-container.sh -C src/pal make -j$(JOBS) all

release: build
	python3 src/pal/release.py

renderer:
	$(MAKE) -C src/pal tools/render-rix

test:
	@test -n "$(GAME_DIR)" || { echo 'Set GAME_DIR to your own DOS PAL directory'; exit 1; }
	$(MAKE) -C src/pal resources GAME_DIR="$(GAME_DIR)"
	GAME_DIR="$(GAME_DIR)" bash src/pal/smoke.sh
