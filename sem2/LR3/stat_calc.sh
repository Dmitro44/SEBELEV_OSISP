#!/bin/bash

if [ -z "$1" ]; then
    echo "Ошибка: Не указан входной файл." >&2
    echo "Использование: $0 <путь_к_файлу.csv>" >&2
    exit 1
fi

INPUT_FILE="$1"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Ошибка: Файл '$INPUT_FILE' не найден." >&2
    exit 1
fi

awk '
BEGIN {
    # Установка разделителя полей (CSV)
    FS = ","
}

{
    # Игнорирование строки-заголовка
    if (NR == 1 && $1 ~ /^[Нн]аименование/) next;

    # Валидация формата (ровно 3 колонки)
    if (NF != 3 || $1 == "" || $2 == "" || $3 == "") {
        print "Warning: Line " NR " has invalid format (skipped): " $0 > "/dev/stderr"
        next
    }

    name = $1
    qty = $2
    price = $3

    # Валидация типов данных (цифры, положительные значения)
    if (qty !~ /^[0-9]+$/ || price !~ /^[0-9]+(\.[0-9]+)?$/) {
        print "Warning: Line " NR " contains invalid numbers (skipped): " $0 > "/dev/stderr"
        next
    }

    # Накопление суммы и стоимости
    total_qty[name] += qty
    total_cost[name] += (qty * price)
}

END {
    print ""
    # Печать красивой шапки таблицы
    printf "%-20s | %-12s | %-12s | %-15s\n", "Наименование", "Общ. кол-во", "Ср. цена", "Общая стоимость"
    print "-----------------------------------------------------------------------"

    # Расчет средних значений и форматированный вывод
    for (name in total_qty) {
        if (total_qty[name] > 0) {
            avg_price = total_cost[name] / total_qty[name]
        } else {
            avg_price = 0
        }
        
        printf "%-20s | %-12d | %-12.2f | %-15.2f\n", name, total_qty[name], avg_price, total_cost[name]
    }
}' "$INPUT_FILE"
