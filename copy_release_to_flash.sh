#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="$(tr -d '[:space:]' <"${PROJECT_ROOT}/VERSION")"
FLASH="${1:-}"
INSTALLER="${PROJECT_ROOT}/dist/LotSensorSetup-v${VERSION}.exe"
RELEASE_DIR="${PROJECT_ROOT}/releases/v${VERSION}"

if [[ -z "${FLASH}" ]]; then
  echo "Использование: $0 /media/${USER}/ИМЯ_ФЛЕШКИ" >&2
  exit 2
fi
if [[ ! -d "${FLASH}" ]] || [[ ! -w "${FLASH}" ]]; then
  echo "Флешка недоступна для записи: ${FLASH}" >&2
  exit 3
fi
if [[ ! -s "${INSTALLER}" ]] || [[ ! -d "${RELEASE_DIR}" ]]; then
  echo "Сначала соберите выпуск версии ${VERSION}" >&2
  exit 4
fi

cp -v "${INSTALLER}" "${FLASH}/"
cp -v "${RELEASE_DIR}/"* "${FLASH}/"
sync
echo "Версия ${VERSION} полностью скопирована в ${FLASH}"
