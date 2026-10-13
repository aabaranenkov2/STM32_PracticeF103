#!/usr/bin/env bash

set -euo pipefail

readonly PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly OPENOCD="${HOME}/.platformio/packages/tool-openocd/bin/openocd"
readonly OPENOCD_SCRIPTS="${HOME}/.platformio/packages/tool-openocd/scripts"
readonly ARM_NM="${HOME}/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-nm"
readonly RESULT_SIZE=40
readonly COMPLETE_MASK="00003ffe"
readonly TEST_COUNT=13

usage() {
    echo "Использование:"
    echo "  ./board_test.sh         определить плату и показать команду"
    echo "  ./board_test.sh --run   определить, прошить и расшифровать тест"
}

if [[ ${1:-} == "--help" || ${1:-} == "-h" ]]; then
    usage
    exit 0
fi
if [[ $# -gt 1 || (${1:-} != "" && ${1:-} != "--run") ]]; then
    usage >&2
    exit 2
fi

for command_name in st-info st-flash pio od awk; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "Ошибка: не найдена команда ${command_name}" >&2
        exit 1
    fi
done

probe_output="$(st-info --probe)"
flash_bytes="$(awk '/^[[:space:]]*flash:/ {print $2; exit}' <<<"${probe_output}")"
sram_bytes="$(awk '/^[[:space:]]*sram:/ {print $2; exit}' <<<"${probe_output}")"
chip_id="$(awk '/^[[:space:]]*chipid:/ {print $2; exit}' <<<"${probe_output}")"

if [[ -z ${flash_bytes} || -z ${sram_bytes} || -z ${chip_id} ]]; then
    echo "Ошибка: ST-Link не смог определить параметры платы." >&2
    exit 1
fi

tap_output=""
tap_status=0
if [[ -x ${OPENOCD} ]]; then
    set +e
    tap_output="$("${OPENOCD}" -s "${OPENOCD_SCRIPTS}" \
        -f interface/stlink.cfg -c "transport select hla_swd" \
        -f target/stm32f1x.cfg -c init -c shutdown 2>&1)"
    tap_status=$?
    set -e
else
    echo "Ошибка: не найден OpenOCD: ${OPENOCD}" >&2
    exit 1
fi

tap_kind="standard"
if [[ ${tap_status} -ne 0 ]]; then
    if [[ ${tap_output} == *"0x2ba01477"* ]]; then
        tap_kind="clone"
    else
        echo "Ошибка определения SWD ID:" >&2
        echo "${tap_output}" >&2
        exit 1
    fi
fi

case "${flash_bytes}" in
    32768)
        crystal="STM32F103C6T6/C6T6A"
        profile="selftest_f103c6"
        if [[ ${tap_kind} == "clone" ]]; then
            profile="selftest_f103c6_clone"
        fi
        ;;
    65536|131072)
        if [[ ${flash_bytes} -eq 131072 ]]; then
            crystal="STM32F103C8/CB-совместимый клон"
        else
            crystal="STM32F103C8T6"
        fi
        profile="selftest_f103c8"
        if [[ ${tap_kind} == "clone" ]]; then
            profile="selftest_f103c8_clone"
        fi
        ;;
    *)
        echo "Неподдерживаемый размер Flash: ${flash_bytes} байт" >&2
        exit 1
        ;;
esac

echo "Обнаружен кристалл: ${crystal}"
echo "Chip ID ST-Link: ${chip_id}"
echo "Flash: $((flash_bytes / 1024)) КБ"
echo "RAM: $((sram_bytes / 1024)) КБ"
echo "SWD: ${tap_kind}"
echo "Рекомендуемый профиль: ${profile}"
echo
echo "Команда теста:"
echo "pio run -e ${profile} -t upload"

if [[ ${1:-} != "--run" ]]; then
    exit 0
fi

echo
echo "Сборка и загрузка теста..."
pio run --project-dir "${PROJECT_DIR}" -e "${profile}" -t upload

elf="${PROJECT_DIR}/.pio/build/${profile}/firmware.elf"
if [[ ! -x ${ARM_NM} || ! -f ${elf} ]]; then
    echo "Ошибка: не найден ELF или arm-none-eabi-nm." >&2
    exit 1
fi

result_address="$("${ARM_NM}" -n "${elf}" |
    awk '$3 == "g_self_test_result" {print $1; exit}')"
if [[ -z ${result_address} ]]; then
    echo "Ошибка: структура результата отсутствует в прошивке." >&2
    exit 1
fi

result_file="$(mktemp)"
trap 'rm -f "${result_file}"' EXIT

signature=""
completed_mask=""
error_mask="00000000"
warning_mask="00000000"
watchdog="00000000"

echo "Ожидание завершения теста..."
for _attempt in {1..30}; do
    sleep 1
    if st-flash read "${result_file}" "0x${result_address}" \
        "${RESULT_SIZE}" >/dev/null 2>&1; then
        words=($(od -An -v -t x4 "${result_file}"))
        if [[ ${#words[@]} -eq 10 ]]; then
            signature="${words[0]}"
            completed_mask="${words[1]}"
            error_mask="${words[2]}"
            warning_mask="${words[3]}"
            watchdog="${words[9]}"
            if [[ ${completed_mask} == "${COMPLETE_MASK}" ]]; then
                break
            fi
        fi
    fi
done

if [[ ${signature} != "54455354" ]]; then
    echo "Ошибка: результат теста не удалось прочитать." >&2
    exit 1
fi

test_description() {
    case "$1" in
        1)  echo "определение кристалла и размера Flash" ;;
        2)  echo "HSE и PLL 72 МГц" ;;
        3)  echo "оперативная память RAM" ;;
        4)  echo "аппаратный CRC" ;;
        5)  echo "DMA memory-to-memory" ;;
        6)  echo "стирание и запись Flash" ;;
        7)  echo "ADC, VREFINT и датчик температуры" ;;
        8)  echo "таймеры TIM1/TIM2/TIM3/TIM4" ;;
        9)  echo "CAN internal loopback" ;;
        10) echo "внешний кварц LSE 32,768 кГц отсутствует" ;;
        11) echo "RTC от доступного тактового источника" ;;
        12) echo "backup-регистры" ;;
        13) echo "независимый watchdog IWDG" ;;
        *)  echo "неизвестная проверка" ;;
    esac
}

completed_value=$((16#${completed_mask}))
error_value=$((16#${error_mask}))
warning_value=$((16#${warning_mask}))
completed_count=0
error_count=0
warning_count=0

for ((test_number = 1; test_number <= TEST_COUNT; ++test_number)); do
    if (( (completed_value & (1 << test_number)) != 0 )); then
        ((completed_count += 1))
    fi
    if (( (error_value & (1 << test_number)) != 0 )); then
        ((error_count += 1))
    fi
    if (( (warning_value & (1 << test_number)) != 0 )); then
        ((warning_count += 1))
    fi
done

passed_count=$((completed_count - error_count - warning_count))

echo
echo "Результат автономного теста"
echo "Кристалл: ${crystal}"
echo "Выполнено проверок: ${completed_count} из ${TEST_COUNT}"
echo "Успешно: ${passed_count}"
echo "Количество обнаруженных ошибок: ${error_count}"
echo "Количество предупреждений: ${warning_count}"

for ((test_number = 1; test_number <= TEST_COUNT; ++test_number)); do
    if (( (error_value & (1 << test_number)) != 0 )); then
        echo "Ошибка ${test_number}: $(test_description "${test_number}")"
    fi
done
for ((test_number = 1; test_number <= TEST_COUNT; ++test_number)); do
    if (( (warning_value & (1 << test_number)) != 0 )); then
        echo "Предупреждение ${test_number}: $(test_description "${test_number}")"
    fi
done

if (( error_count != 0 )); then
    echo "Итог: FAIL"
    exit 1
elif (( warning_count != 0 )); then
    echo "Итог: PASS с предупреждениями"
else
    echo "Итог: PASS"
fi
