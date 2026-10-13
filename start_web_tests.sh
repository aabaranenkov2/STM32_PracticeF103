#!/usr/bin/env bash
# Запуск web/server.py и открытие блока «Тест энкодеров».
# На одном /dev/ttyUSB0 допустим только ОДИН server.py — иначе кнопки на странице мигают.
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${PORT:-8765}"
SERIAL="${SERIAL:-/dev/ttyUSB0}"
SIMULATE="${SIMULATE:-0}"
STOP_DUPLICATES="${STOP_DUPLICATES:-1}"
PID_FILE="${PROJECT_DIR}/.web-server.pid"
LOG_FILE="${PROJECT_DIR}/.web-server.log"
LOCK_FILE="${XDG_DATA_HOME:-${HOME}/.local/share}/LotSensor/gateway.lock"

server_pids() {
  pgrep -f "${PROJECT_DIR}/web/server\.py|web/server\.py" 2>/dev/null || true
}

count_servers() {
  local pids
  pids="$(server_pids)"
  [[ -z "${pids}" ]] && echo 0 && return
  echo "${pids}" | wc -w
}

port_responds() {
  local p="$1"
  curl -sf "http://127.0.0.1:${p}/api/status" >/dev/null 2>&1
}

find_active_port() {
  local p
  for p in "${PORT}" 8765 8766 8767 8768; do
    if port_responds "${p}"; then
      echo "${p}"
      return 0
    fi
  done
  return 1
}

serial_holders() {
  if [[ ! -e "${SERIAL}" ]]; then
    return 0
  fi
  lsof -t "${SERIAL}" 2>/dev/null || true
}

stop_all_servers() {
  local pid
  echo "Останавливаю все экземпляры web/server.py..."
  while read -r pid; do
    [[ -z "${pid}" ]] && continue
    kill "${pid}" 2>/dev/null || true
  done < <(server_pids)
  sleep 1
  while read -r pid; do
    [[ -z "${pid}" ]] && continue
    kill -9 "${pid}" 2>/dev/null || true
  done < <(server_pids)
  rm -f "${PID_FILE}"
}

check_duplicates() {
  local n holders holder pid
  n="$(count_servers)"
  if (( n <= 1 )); then
    return 0
  fi
  echo "Ошибка: найдено ${n} процессов web/server.py на одном USB — связь мигает." >&2
  pgrep -af "web/server\.py" >&2 || true
  if [[ "${STOP_DUPLICATES}" == "1" ]]; then
    stop_all_servers
    echo "Лишние процессы остановлены. Запускаю один сервер..."
    return 0
  fi
  echo "Остановите вручную: pkill -f 'web/server.py'" >&2
  echo "Или: STOP_DUPLICATES=1 ./start_web_tests.sh" >&2
  exit 1
}

check_serial_conflict() {
  local holders holder pid
  [[ "${SIMULATE}" == "1" ]] && return 0
  [[ ! -e "${SERIAL}" ]] && return 0
  holders="$(serial_holders)"
  [[ -z "${holders}" ]] && return 0
  n="$(count_servers)"
  if (( n == 0 )); then
    echo "Предупреждение: ${SERIAL} занят другим процессом (не server.py):" >&2
    while read -r holder; do
      [[ -z "${holder}" ]] && continue
      ps -p "${holder}" -o pid=,comm= 2>/dev/null >&2 || true
    done <<< "${holders}"
    exit 1
  fi
}

start_server() {
  local -a args=(python3 "${PROJECT_DIR}/web/server.py" --no-browser --port "${PORT}")
  if [[ "${SIMULATE}" == "1" ]]; then
    args+=(--simulate)
  else
    args+=(--serial "${SERIAL}")
  fi
  : >"${LOG_FILE}"
  nohup "${args[@]}" >>"${LOG_FILE}" 2>&1 &
  echo $! >"${PID_FILE}"
}

wait_for_port() {
  local p="$1"
  local i
  for i in $(seq 1 40); do
    if port_responds "${p}"; then
      return 0
    fi
    sleep 0.25
  done
  return 1
}

check_duplicates

ACTIVE_PORT=""
if ACTIVE_PORT="$(find_active_port)"; then
  echo "Сервер уже работает на порту ${ACTIVE_PORT}"
  if [[ -f "${LOCK_FILE}" ]]; then
    echo "Блокировка: ${LOCK_FILE} (PID $(cat "${LOCK_FILE}" 2>/dev/null || echo '?'))"
  fi
else
  check_serial_conflict
  if [[ -f "${PID_FILE}" ]] && ! kill -0 "$(cat "${PID_FILE}")" 2>/dev/null; then
    rm -f "${PID_FILE}"
  fi
  if [[ -f "${LOCK_FILE}" ]] && [[ ! -s "${LOCK_FILE}" || ! -d "/proc/$(cat "${LOCK_FILE}" 2>/dev/null)" ]]; then
    echo "Удаляю устаревший lock (процесс не найден)..."
    rm -f "${LOCK_FILE}"
  fi
  echo "Запуск сервера на порту ${PORT} (${SERIAL})..."
  start_server
  if ! wait_for_port "${PORT}"; then
    echo "Ошибка: сервер не ответил на порту ${PORT}." >&2
    if [[ -f "${LOG_FILE}" ]]; then
      echo "--- последние строки ${LOG_FILE} ---" >&2
      tail -20 "${LOG_FILE}" >&2 || true
    fi
    if grep -q "уже запущен" "${LOG_FILE}" 2>/dev/null; then
      echo "Другой server.py уже держит gateway.lock. Выполните:" >&2
      echo "  pgrep -af 'web/server.py'" >&2
      echo "  STOP_DUPLICATES=1 ./start_web_tests.sh" >&2
    fi
    exit 1
  fi
  ACTIVE_PORT="${PORT}"
  echo "Сервер готов."
fi

URL="http://127.0.0.1:${ACTIVE_PORT}/#devTest"
CONNECTED="$(curl -sf "http://127.0.0.1:${ACTIVE_PORT}/api/status" \
  | python3 -c "import sys,json; print(json.load(sys.stdin).get('connected', False))" 2>/dev/null \
  || echo False)"
if [[ "${CONNECTED}" != "True" ]]; then
  echo "Предупреждение: контроллер не подключён (connected=false)."
  echo "Проверьте USB, кабель и что нет второго server.py: pgrep -af 'web/server.py'"
else
  echo "Связь с контроллером: OK"
fi

echo "Открываю: ${URL}"
xdg-open "${URL}" 2>/dev/null \
  || sensible-browser "${URL}" 2>/dev/null \
  || echo "Откройте вручную: ${URL}"
