#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
APP_VERSION="$(tr -d '[:space:]' <"${PROJECT_ROOT}/VERSION")"
BUILD_ROOT="${PROJECT_ROOT}/.windows-build"
DOWNLOADS="${BUILD_ROOT}/downloads"
export WINEPREFIX="${BUILD_ROOT}/wineprefix"
export WINEARCH=win64
export WINEDEBUG="${WINEDEBUG:--all}"

PYTHON_VERSION="3.12.7"
PYTHON_INSTALLER="${DOWNLOADS}/python-${PYTHON_VERSION}-amd64.exe"
PYTHON_URL="https://www.python.org/ftp/python/${PYTHON_VERSION}/python-${PYTHON_VERSION}-amd64.exe"
INNO_VERSION="6.7.3"
INNO_INSTALLER="${DOWNLOADS}/innosetup-${INNO_VERSION}.exe"
INNO_URL="https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-${INNO_VERSION}.exe"
WINDOWS_PYTHON="${WINEPREFIX}/drive_c/Python312/python.exe"
INNO_COMPILER="${WINEPREFIX}/drive_c/InnoSetup6/ISCC.exe"
SETUP_NAME="LotSensorSetup-v${APP_VERSION}.exe"

if [[ ! "${APP_VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Некорректная версия в VERSION: ${APP_VERSION}" >&2
  exit 1
fi

download() {
  local url="$1"
  local destination="$2"
  if [[ -s "${destination}" ]]; then
    return
  fi
  curl --fail --location --retry 3 --output "${destination}" "${url}"
}

command -v wine >/dev/null
command -v wineboot >/dev/null
command -v winepath >/dev/null
command -v curl >/dev/null

mkdir -p "${DOWNLOADS}" "${PROJECT_ROOT}/dist" "${PROJECT_ROOT}/build"
download "${PYTHON_URL}" "${PYTHON_INSTALLER}"
download "${INNO_URL}" "${INNO_INSTALLER}"

wineboot --init

if [[ ! -x "${WINDOWS_PYTHON}" ]]; then
  wine "${PYTHON_INSTALLER}" /quiet InstallAllUsers=0 \
    "TargetDir=C:\\Python312" Include_launcher=0 Include_test=0 \
    Include_doc=0 Include_debug=0 Include_pip=1 PrependPath=0
fi

if [[ ! -f "${INNO_COMPILER}" ]]; then
  wine "${INNO_INSTALLER}" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP- \
    "/DIR=C:\\InnoSetup6"
fi

wine "${WINDOWS_PYTHON}" -m pip install --disable-pip-version-check \
  "pyinstaller==6.11.1" "pyserial==3.5"

PROJECT_WIN="$(winepath -w "${PROJECT_ROOT}")"
rm -f "${PROJECT_ROOT}/dist/LotSensor.exe" \
      "${PROJECT_ROOT}/dist/LotSensorSetup-"*.exe

wine "${WINDOWS_PYTHON}" -m PyInstaller --noconfirm --clean \
  --distpath "${PROJECT_WIN}\\dist" \
  --workpath "${PROJECT_WIN}\\build\\pyinstaller-windows" \
  "${PROJECT_WIN}\\packaging\\windows\\lot_sensor.spec"

wine "${INNO_COMPILER}" "/DAppVersion=${APP_VERSION}" \
  "${PROJECT_WIN}\\packaging\\windows\\LotSensor.iss"

test -s "${PROJECT_ROOT}/dist/LotSensor.exe"
test -s "${PROJECT_ROOT}/dist/${SETUP_NAME}"
printf 'Готово: %s\n' "${PROJECT_ROOT}/dist/${SETUP_NAME}"
