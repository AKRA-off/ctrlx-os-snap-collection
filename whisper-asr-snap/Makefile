SHELL := /bin/bash

# Always run `hf` via pipx to avoid relying on local `hf` installations.
hf := pipx run --spec "huggingface_hub[cli]" hf

SNAP_NAME ?= whisper-asr
ENGINE ?= cpu

# Some backends are only compatible with some CPU architectures.
# By default, models that are not compatible with the current CPU architecture are not downloaded.
# Set DOWNLOAD_ALL_MODELS=1 to force downloading all models regardless of CPU compatibility.
DOWNLOAD_ALL_MODELS ?= 0

.PHONY: all help init init-submodules install-deps download-models \
	download-model-base download-model-small download-model-base-ov download-model-small-ov \
	build install upload smoke-test

all: help

#
# Main targets
#

help: ## Show this help message
	@echo "Usage: make <target>"
	@echo
	@echo "Targets:"
	@# List all targets with descriptions (lines starting with '##'):
	@grep -E '^[a-zA-Z0-9_-]+:.*## .*$$' $(MAKEFILE_LIST) | \
		sort | \
		awk 'BEGIN {FS = ":.*## "}; {printf "  %-11s %s\n", $$1, $$2}'

init: init-submodules install-deps download-models ## Initialize the build environment (dependencies, model weights, submodules, etc.)

build: ## Build the snap
	./dev/build.sh

install: ## Install the snap
	./dev/install.sh

upload: ## Upload the snap
	./dev/upload.sh

smoke-test: ## Run smoke tests (override with SNAP_NAME=... ENGINE=...)
	sudo ./dev/smoke-test.sh $(SNAP_NAME) $(ENGINE)

#
# Supporting targets
#

install-deps:
	@echo "Installing dependencies..."
	@# Ensure pipx is available for running the hf CLI.
	@command -v pipx >/dev/null 2>&1 || { \
		sudo apt-get update; \
		sudo apt-get install -y pipx; \
	}

init-submodules:
	@echo "Initializing submodules..."
	@if git submodule status | grep -q '^-'; then \
		git submodule update --init; \
	fi

download-models: download-model-base download-model-small download-model-base-ov download-model-small-ov

download-model-base:
	@echo "Downloading Faster Whisper Base model weights..."
	$(hf) download Systran/faster-whisper-base \
		--local-dir components/model-faster-whisper-base/

download-model-small:
	@echo "Downloading Faster Whisper Small model weights..."
	$(hf) download Systran/faster-whisper-small \
		--local-dir components/model-faster-whisper-small/

download-model-base-ov:
	@if [[ "$(DOWNLOAD_ALL_MODELS)" = "1" || "$$(uname -m)" =~ ^(x86_64|amd64)$$ ]]; then \
		echo "Downloading Whisper Base OpenVINO model weights..."; \
		$(hf) download OpenVINO/whisper-base-fp16-ov \
			--local-dir components/model-whisper-base-ov/; \
	else \
		echo "Skipping Whisper Base OpenVINO model weights download: not on amd64 (set DOWNLOAD_ALL_MODELS=1 to override)"; \
	fi

download-model-small-ov:
	@if [[ "$(DOWNLOAD_ALL_MODELS)" = "1" || "$$(uname -m)" =~ ^(x86_64|amd64)$$ ]]; then \
		echo "Downloading Whisper Small OpenVINO model weights..."; \
		$(hf) download OpenVINO/whisper-small-fp16-ov \
			--local-dir components/model-whisper-small-ov/; \
	else \
		echo "Skipping Whisper Small OpenVINO model weights download: not on amd64 (set DOWNLOAD_ALL_MODELS=1 to override)"; \
	fi
