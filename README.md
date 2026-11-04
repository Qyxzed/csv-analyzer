# Инструкция по компиляции(gcc)
Для сборки проекта используется следующая команда:

```bash
gcc -Wall -Wextra -std=c99 -I. src/types.c src/csv_parser.c src/binary_io.c src/analyzer.c src/main.c -o csvstat.exe
```
# Инструкция по компиляции(makefile)
​Use the Makefile in the root directory to compile the project:  
**​make** — Standard build.  
**​make sanitize** — Build with memory sanitizers.  
**​make release** — Optimized release build.  
**​make clean** — Remove compiled files.

# SYMBOL STATE MACHINE
```mermaid
stateDiagram-v2
    direction LR
    [*] --> S_NORMAL

    state "Обычное поле" as S_NORMAL
    state "Поле внутри кавычек" as S_QUOTED
    state "После кавычки" as S_QUOTE_END
    state "Ошибка" as S_ERROR

    S_NORMAL --> S_QUOTED : Символ " (если буфер пуст или из пробелов)
    S_NORMAL --> S_ERROR : Символ " (если в буфере уже есть символы)
    S_NORMAL --> S_NORMAL : Разделитель (,) [Сохранить поле]
    S_NORMAL --> S_NORMAL : \\n или \\0 [Сохранить поле + Завершить строку]
    S_NORMAL --> S_NORMAL : Любой другой символ [Добавить в буфер]

    S_QUOTED --> S_QUOTED : Любой символ (включая , и \\n при --multiline) [Добавить в буфер]
    S_QUOTED --> S_QUOTED : Двойная кавычка ("") [Добавить одну " в буфер]
    S_QUOTED --> S_QUOTE_END : Одиночная кавычка (") [Закрыть кавычки]
    S_QUOTED --> S_ERROR : \\0 (Конец файла) [Незакрытая кавычка]
    S_QUOTED --> S_ERROR : \\n (если НЕ --multiline)

    S_QUOTE_END --> S_NORMAL : Разделитель (,) [Сохранить поле]
    S_QUOTE_END --> S_NORMAL : \\n или \\0 [Сохранить поле + Завершить строку]
    S_QUOTE_END --> S_QUOTE_END : Пробелы / Табы [Игнорировать]
    S_QUOTE_END --> S_ERROR : Любой другой символ [Данные после кавычки]
```
# CELL TYPE STATE MACHINE
```mermaid
stateDiagram-v2
    direction LR
    [*] --> S_START

    state "Начало" as S_START
    state "Знак" as S_SIGN
    state "Целая часть" as S_INT
    state "Точка" as S_DOT
    state "Дробная часть" as S_FRAC
    state "Экспонента" as S_EXP
    state "Знак эксп." as S_EXP_SIGN
    state "Цифры эксп." as S_EXP_DIGITS
    state "Строка" as S_STRING
    state "NULL" as S_NULL

    S_START --> S_SIGN : '+' или '-'
    S_START --> S_INT : '0'-'9'
    S_START --> S_STRING : любой другой символ
    S_START --> S_NULL : конец строки (пустая, без кавычек)

    S_SIGN --> S_INT : '0'-'9'
    S_SIGN --> S_STRING : любой другой символ

    S_INT --> S_INT : '0'-'9'
    S_INT --> S_DOT : '.'
    S_INT --> S_EXP : 'e' или 'E' (только с --scientific)
    S_INT --> [*] : конец строки (Результат: INT)
    S_INT --> S_STRING : любой другой символ

    S_DOT --> S_FRAC : '0'-'9'
    S_DOT --> S_STRING : любой другой символ или конец строки

    S_FRAC --> S_FRAC : '0'-'9'
    S_FRAC --> S_EXP : 'e' или 'E' (только с --scientific)
    S_FRAC --> [*] : конец строки (Результат: DOUBLE)
    S_FRAC --> S_STRING : любой другой символ

    S_EXP --> S_EXP_SIGN : '+' или '-'
    S_EXP --> S_EXP_DIGITS : '0'-'9'
    S_EXP --> S_STRING : любой другой символ

    S_EXP_SIGN --> S_EXP_DIGITS : '0'-'9'
    S_EXP_SIGN --> S_STRING : любой другой символ

    S_EXP_DIGITS --> S_EXP_DIGITS : '0'-'9'
    S_EXP_DIGITS --> [*] : конец строки (Результат: DOUBLE)
    S_EXP_DIGITS --> S_STRING : любой другой символ

    S_STRING --> [*] : Результат: STRING
    S_NULL --> [*] : Результат: NULL
```
