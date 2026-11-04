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

# STATE MACHINE
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
