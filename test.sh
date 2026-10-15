#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="${SHAREFRAME_BUILD_ROOT:-$ROOT}"
CPP_COMPOSE="$ROOT/tests/docker/docker-compose.yml"
E2E_COMPOSE="$ROOT/tests/e2e/docker-compose.yml"
FRONTEND="$ROOT/dashboard-frontend/frontend"

usage() {
    cat <<'EOF'
Usage: ./test.sh <command> [--docker] [--keep] [extra args]

  unit          C++ unit tests (no I/O)
  integration   C++ integration tests (SQLite, nng sockets, files)
  cpp           unit + integration
  asan          all C++ tests under AddressSanitizer + UBSan
  coverage      all C++ tests + gcovr HTML report
  frontend      dashboard frontend lint, typecheck and Vitest
  e2e           full stack in Docker + Playwright
  all           cpp + frontend + e2e

  --docker      run the C++ commands inside tests/docker instead of on the host
  --keep        e2e: leave the stack running afterwards
  --ui          e2e: open Playwright UI mode on http://localhost:8089
  extra args    forwarded to ctest (C++) or playwright (e2e),
                e.g. ./test.sh unit -R VersionCompare   ./test.sh e2e --grep skip
EOF
}

cmd="${1:-}"
[[ -n "$cmd" && "$cmd" != "-h" && "$cmd" != "--help" ]] || { usage; exit 0; }
shift

docker=0
keep=0
ui=0
args=()
for a in "$@"; do
    case "$a" in
        --docker) docker=1 ;;
        --keep) keep=1 ;;
        --ui) ui=1 ;;
        *) args+=("$a") ;;
    esac
done

run_cpp() {
    local preset=$1 label=${2:-}
    local dir="$BUILD_ROOT/build-${preset#test-}"
    [[ "$preset" == test ]] && dir="$BUILD_ROOT/build-test"

    cmake --preset "$preset" -B "$dir" >/dev/null
    cmake --build "$dir" --target shareframe-unit-tests shareframe-integration-tests -j "$(nproc)"
    ctest --test-dir "$dir" --output-on-failure -j "$(nproc)" ${label:+-L "$label"} "${args[@]}"

    if [[ "$preset" == test-coverage ]]; then
        if ! command -v gcovr >/dev/null; then
            echo "gcovr not found (pip install gcovr) - skipping report, or use --docker" >&2
            return
        fi
        mkdir -p "$dir/coverage"
        gcovr --root "$ROOT" --filter "$ROOT/src/" --filter "$ROOT/include/" \
              --object-directory "$dir" --html-details "$dir/coverage/index.html" --print-summary
        echo "Coverage report: ${dir#"$ROOT"/}/coverage/index.html"
    fi
}

run_frontend() {
    (cd "$FRONTEND" && { [[ -d node_modules ]] || npm ci; } && npm run check)
}

run_e2e() {
    export HOST_UID="$(id -u)" HOST_GID="$(id -g)"
    local compose=(docker compose -f "$E2E_COMPOSE" --profile runner)
    local status=0
    "${compose[@]}" build
    "${compose[@]}" up -d --wait board fake-server frontend
    if (( ui )); then
        echo "Playwright UI: http://localhost:8089 (dashboard: http://localhost:8088)"
        "${compose[@]}" run --rm --service-ports e2e \
            npx playwright test --ui-host=0.0.0.0 --ui-port=8089 "${args[@]}" || status=$?
    else
        "${compose[@]}" run --rm e2e npx playwright test "${args[@]}" || status=$?
    fi
    if (( keep )); then
        echo "Stack left running (dashboard: http://localhost:8088). Stop it with: docker compose -f $E2E_COMPOSE down -v"
    else
        "${compose[@]}" down -v
    fi
    echo "Playwright report: $ROOT/tests/e2e/playwright-report/index.html"
    return $status
}

case "$cmd" in
    unit|integration|cpp|asan|coverage)
        if (( docker )); then
            export HOST_UID="$(id -u)" HOST_GID="$(id -g)"
            exec docker compose -f "$CPP_COMPOSE" run --rm --build cpp "$cmd" "${args[@]}"
        fi
        case "$cmd" in
            unit) run_cpp test unit ;;
            integration) run_cpp test integration ;;
            cpp) run_cpp test ;;
            asan) run_cpp test-asan ;;
            coverage) run_cpp test-coverage ;;
        esac
        ;;
    frontend) run_frontend ;;
    e2e) run_e2e ;;
    all)
        args=()
        run_cpp test
        run_frontend
        run_e2e
        ;;
    *) usage; exit 2 ;;
esac
