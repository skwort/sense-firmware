import json
import sys
import shutil
from colorama import Fore, Style

TERM_WIDTH = shutil.get_terminal_size((80, 20)).columns


def load_twister_json(json_path):
    with open(json_path, 'r') as f:
        return json.load(f)


def print_suite_test_summary(suite_name, results):
    """
    Format and print a structured summary of test results for a
    single suite.
    """
    num_passed = len(results["passed"])
    num_failed = len(results["failed"])
    num_skipped = len(results["skipped"])
    total_tests = num_passed + num_failed + num_skipped

    print(f"{Style.BRIGHT}  {suite_name}  {Style.RESET_ALL}"
          .center(TERM_WIDTH + 8, "="))
    if num_passed:
        test_str = "tests" if num_passed > 1 else "test"
        print(f"{Fore.GREEN}✔ PASSED: {num_passed} {test_str}")
        for tc in results["passed"]:
            print(f"    - {tc['identifier']} ({tc['execution_time']}s)")
        print(Style.RESET_ALL, end="")

    if num_failed:
        test_str = "tests" if num_passed > 1 else "test"
        print(f"{Fore.RED}✘ FAILED: {num_failed} {test_str}")
        for tc in results["failed"]:
            print(f"    - {tc['identifier']} ({tc['execution_time']}s)")
        print(Style.RESET_ALL, end="")

    if num_skipped:
        test_str = "tests" if num_passed > 1 else "test"
        print(f"{Fore.YELLOW}⚠ SKIPPED: {num_skipped} {test_str}")
        for tc in results["skipped"]:
            print(f"    - {tc['identifier']} ({tc['execution_time']}s)")
        print(Style.RESET_ALL, end="")

    print("." * TERM_WIDTH)
    print(
        f"{Style.BRIGHT}TOTAL: {total_tests} {Style.RESET_ALL}| "
        f"{Style.BRIGHT}{Fore.GREEN}✔ {num_passed} Passed{Style.RESET_ALL} | "
        f"{Style.BRIGHT}{Fore.RED}✘ {num_failed} Failed{Style.RESET_ALL} | "
        f"{Style.BRIGHT}{Fore.YELLOW}⚠ {num_skipped} "
        f"Skipped{Style.RESET_ALL}"
    )
    print("" + "=" * TERM_WIDTH)


def get_suite_summary(suite, print_summary=True):
    exec_time = float(suite.get("execution_time", 0))
    testcases = suite["testcases"]

    passed = [tc for tc in testcases if tc["status"] == "passed"]
    failed = [tc for tc in testcases if tc["status"] == "failed"]
    skipped = [tc for tc in testcases if tc["status"] == "skipped"]

    return {
        "passed": passed,
        "failed": failed,
        "skipped": skipped,
        "duration": exec_time
    }


def print_overall_summary(test_results):
    """
    Print an overall summary of all test results in a table format.
    """
    print()
    print(f"{Style.BRIGHT}  Overall Results  {Style.RESET_ALL}"
          .center(TERM_WIDTH + 8, "="))
    print()

    header = f"{'Suite':<30} | " \
             f"{'Passed':<7} | " \
             f"{'Failed':<7} | " \
             f"{'Skipped':<7} | " \
             f"{'Duration (s)':<12}"

    print(header)
    print("-" * len(header))

    for suite_name, result in test_results.items():
        passed = len(result["passed"])
        failed = len(result["failed"])
        skipped = len(result["skipped"])
        duration = result["duration"]

        passed_str = f"{Fore.GREEN}{passed:<7}{Style.RESET_ALL}"
        failed_str = f"{Fore.RED}{failed:<7}{Style.RESET_ALL}"
        skipped_str = f"{Fore.YELLOW}{skipped:<7}{Style.RESET_ALL}"
        duration_str = f"{Fore.CYAN}{duration:<12.2f}{Style.RESET_ALL}"

        print(f"{suite_name:<30} | "
              f"{passed_str} | "
              f"{failed_str} | "
              f"{skipped_str} | "
              f"{duration_str}")

    total_passed = sum(
        len(result["passed"]) for result in test_results.values()
    )
    total_failed = sum(
        len(result["failed"]) for result in test_results.values()
    )
    total_skipped = sum(
        len(result["skipped"]) for result in test_results.values()
    )
    total_duration = sum(
        result["duration"] for result in test_results.values()
    )

    total_passed_str = f"{Fore.GREEN}{total_passed:<7}{Style.RESET_ALL}"
    total_failed_str = f"{Fore.RED}{total_failed:<7}{Style.RESET_ALL}"
    total_skipped_str = f"{Fore.YELLOW}{total_skipped:<7}{Style.RESET_ALL}"
    total_duration_str = f"{Fore.CYAN}{total_duration:<12.2f}{Style.RESET_ALL}"

    print("-" * len(header))
    print(f"{'TOTAL:':<30} | "
          f"{total_passed_str} | "
          f"{total_failed_str} | "
          f"{total_skipped_str} | "
          f"{total_duration_str}")
    print()
    print("=" * TERM_WIDTH)


def main(json_path):
    try:
        data = load_twister_json(json_path)
    except FileNotFoundError:
        print(f"Unable to locate '{sys.argv[1]}'")
        sys.exit(1)

    suites = data.get("testsuites", [])

    all_results = {}
    for suite in suites:
        result = get_suite_summary(suite, print_summary=False)
        print_suite_test_summary(suite["name"], result)
        all_results[suite["name"]] = result

    print_overall_summary(all_results)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <path-to-twister.json>")
        sys.exit(1)

    main(sys.argv[1])
