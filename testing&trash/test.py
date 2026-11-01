import os
import subprocess
import sys

# Если вы видите это сообщение, то знайте, что тестирующая программа 
# была написана грязными китайскими AI-холопами на которых не la parfum, конкретно - собственные мозги, 
# а francuzskoe der`mo, конкретно - gemini ai

EXECUTABLE = "./csvstat.exe" if os.name == "nt" else "./csvstat"
TESTS_DIR = "tests"
RESULTS_DIR = "test_results" 

def run_command(cmd_args):
    """Запуск процесса, возвращает код возврата, stdout и stderr."""
    try:
        result = subprocess.run(
            cmd_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace"
        )
        return result.returncode, result.stdout, result.stderr
    except Exception as e:
        return -1, "", str(e)

def main():
    if not os.path.exists(EXECUTABLE):
        print(f"[-] Ошибка: Исполняемый файл '{EXECUTABLE}' не найден. Скомпилируйте проект!")
        sys.exit(1)

    if not os.path.exists(TESTS_DIR):
        print(f"[-] Ошибка: Папка с тестами '{TESTS_DIR}' не найдена.")
        sys.exit(1)

    os.makedirs(RESULTS_DIR, exist_ok=True)

    csv_files = [f for f in os.listdir(TESTS_DIR) if f.lower().endswith(".csv")]
    
    if not csv_files:
        print(f"[-] В папке '{TESTS_DIR}' не найдено .csv файлов.")
        sys.exit(0)

    print(f"[+] Найдено CSV-файлов: {len(csv_files)}")
    print("=" * 60)

    passed_tests = 0
    total_tests = 0

    for csv_file in csv_files:
        csv_path = os.path.join(TESTS_DIR, csv_file)
        base_name = os.path.splitext(csv_file)[0]
        
        bin_path = os.path.join(RESULTS_DIR, f"{base_name}.bin")
        report_txt = os.path.join(RESULTS_DIR, f"{base_name}_report.txt")
        report_bin_txt = os.path.join(RESULTS_DIR, f"{base_name}_bin_report.txt")

        print(f"\n📁 Файл: {csv_file}")
        print("-" * 40)

        test_cases = [
            ("schema", [EXECUTABLE, "schema", csv_path], 0, True),
            ("stats", [EXECUTABLE, "stats", csv_path], 0, True),
            ("stats (--scientific --multiline)", [EXECUTABLE, "stats", csv_path, "--scientific", "--multiline"], 0, True),
            ("report", [EXECUTABLE, "report", csv_path, report_txt], 0, False),
            ("bin-import", [EXECUTABLE, "bin-import", csv_path, bin_path, "--scientific", "--multiline"], 0, False),
        ]

        for test_name, args, expected_code, print_output in test_cases:
            total_tests += 1
            code, out, err = run_command(args)
            
            if code == expected_code:
                print(f"  ✅ [{test_name}] — УСПЕХ")
                passed_tests += 1
                if print_output and out.strip():
                    print("     --- Вывод программы ---")
                    for line in out.strip().splitlines():
                        print(f"     | {line}")
                    print("     -----------------------")
            else:
                print(f"  ❌ [{test_name}] — ОШИБКА (код {code}, ожидался {expected_code})")
                if err:
                    print(f"     stderr: {err.strip()}")

        if os.path.exists(bin_path):
            bin_test_cases = [
                ("bin-schema", [EXECUTABLE, "bin-schema", bin_path], 0, True),
                ("bin-stats", [EXECUTABLE, "bin-stats", bin_path], 0, True),
                ("bin-report", [EXECUTABLE, "bin-report", bin_path, report_bin_txt], 0, False),
            ]

            for test_name, args, expected_code, print_output in bin_test_cases:
                total_tests += 1
                code, out, err = run_command(args)
                
                if code == expected_code:
                    print(f"  ✅ [{test_name}] — УСПЕХ")
                    passed_tests += 1
                    if print_output and out.strip():
                        print("     --- Вывод программы ---")
                        for line in out.strip().splitlines():
                            print(f"     | {line}")
                        print("     -----------------------")
                else:
                    print(f"  ❌ [{test_name}] — ОШИБКА (код {code}, ожидался {expected_code})")
                    if err:
                        print(f"     stderr: {err.strip()}")

    print("\n" + "=" * 60)
    print(f"🏁 ИТОГИ:")
    print(f"   Успешно: {passed_tests} / {total_tests}")
    print(f"   📂 Все сгенерированные отчёты и `.bin` файлы сохранены в папку: ./{RESULTS_DIR}/")

if __name__ == "__main__":
    main()