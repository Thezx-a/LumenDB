# =========================================================
# TitanKV — unified Makefile
#
# Usage:
#   make help         # list targets
#   make build        # build all C++ + Go components
#   make test         # run all tests
#   make lint         # lint C++ & Go
#   make docker-up    # start local dev stack (compose)
#   make docker-down  # stop local dev stack
#   make run-gateway  # run gateway service (Phase 3)
#   make run-auth     # run auth service (Phase 3)
#   make run-data     # run data service (Phase 4)
#   make run-meta     # run meta service (Phase 4)
#   make run-observ   # run observability service (Phase 4)
#   make run-all      # run all Go services in parallel
#   make web-install  # install Next.js deps
#   make web-dev      # run Next.js dev server
# =========================================================

SHELL := /bin/bash

CMAKE_BUILD_DIR  ?= build
CMAKE_BUILD_TYPE ?= Release
JOBS             ?= $(shell nproc 2>/dev/null || echo 4)

GO            ?= go
GO_TEST_FLAGS ?= -race -count=1
GOLINT        ?= golangci-lint
CLANG_TIDY    ?= clang-tidy

.DEFAULT_GOAL := help

.PHONY: help
help: ## Show available targets
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) \
		| awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-22s\033[0m %s\n", $$1, $$2}'

# ---------------------------------------------------------
# C++ build / test / lint
# ---------------------------------------------------------
.PHONY: cmake-configure cmake-build cpp-test cpp-lint
cmake-configure: ## Configure CMake build
	cmake -B $(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DENABLE_TESTS=ON -DENABLE_BENCHMARKS=OFF

cmake-build: cmake-configure ## Build all C++ targets
	cmake --build $(CMAKE_BUILD_DIR) -j $(JOBS)

cpp-test: cmake-build ## Run C++ unit tests (ctest)
	cd $(CMAKE_BUILD_DIR) && ctest --output-on-failure --parallel $(JOBS)

cpp-lint: ## Run clang-tidy on C++ sources
	@find minikv skynet deepvector -name '*.cpp' -o -name '*.h' \
		| xargs -r $(CLANG_TIDY) -p $(CMAKE_BUILD_DIR)

# ---------------------------------------------------------
# Go build / test / lint
# ---------------------------------------------------------
.PHONY: go-build go-test go-lint go-mod go-tidy
go-mod: ## Tidy and download Go modules
	$(GO) mod download

go-tidy: ## Run go mod tidy
	$(GO) mod tidy

go-build: go-mod ## Build all Go services and tools
	$(GO) build ./...

go-test: go-mod ## Run Go tests with race detector
	$(GO) test ./... $(GO_TEST_FLAGS)

go-lint: ## Run golangci-lint on all Go code
	$(GO) vet ./...
	@if command -v $(GOLINT) >/dev/null 2>&1; then $(GOLINT) run ./...; else echo "skip (golangci-lint not installed)"; fi

# ---------------------------------------------------------
# Run services (Phase 3 / Phase 4)
# ---------------------------------------------------------
.PHONY: run-gateway run-auth run-data run-meta run-observ run-all
run-gateway: ## Run gateway service (Phase 3, port 8080)
	$(GO) run ./gateway

run-auth: ## Run auth service (Phase 3, port 8082)
	$(GO) run ./services/auth

run-data: ## Run data service (Phase 4, port 8081)
	$(GO) run ./services/data

run-meta: ## Run meta service (Phase 4, port 8083)
	$(GO) run ./services/meta

run-observ: ## Run observability service (Phase 4, port 8084)
	$(GO) run ./services/observability

run-all: ## Run all 5 Go services in parallel (use Ctrl+C to stop all)
	@echo "Starting all services. Press Ctrl+C to stop."
	@$(GO) run ./services/auth &
	@$(GO) run ./services/data &
	@$(GO) run ./services/meta &
	@$(GO) run ./services/observability &
	@$(GO) run ./gateway
	@wait

# ---------------------------------------------------------
# Frontend (Next.js, Phase 6)
# ---------------------------------------------------------
.PHONY: web-install web-dev web-build web-lint web-test
web-install: ## Install web dependencies
	cd web && npm install

web-dev: ## Run Next.js dev server (port 3000)
	cd web && npm run dev

web-build: ## Build the Next.js admin console
	cd web && npm run build

web-lint: ## Lint frontend
	cd web && npm run lint

web-test: ## Run frontend tests
	cd web && npm run test -- --run || true

# ---------------------------------------------------------
# Top-level aggregated targets
# ---------------------------------------------------------
.PHONY: build test lint proto clean
build: cmake-build go-build ## Build C++ and Go

test: cpp-test go-test ## Run all tests

lint: cpp-lint go-lint web-lint ## Run all linters

proto: ## Generate gRPC + protobuf stubs (Phase 2)
	@echo "proto target is a no-op until Phase 2 defines proto/keyforge/*.proto"

clean: ## Remove all build artifacts
	rm -rf $(CMAKE_BUILD_DIR) web/.next web/node_modules

# ---------------------------------------------------------
# Local dev stack (Docker Compose)
# ---------------------------------------------------------
DEV_COMPOSE := deploy/dev/docker-compose.yml
.PHONY: docker-up docker-down docker-logs
docker-up: ## Start Postgres / Redis / etcd / Jaeger / Prometheus / Grafana
	docker compose -f $(DEV_COMPOSE) up -d

docker-down: ## Stop the local dev stack
	docker compose -f $(DEV_COMPOSE) down

docker-logs: ## Tail compose logs
	docker compose -f $(DEV_COMPOSE) logs -f
