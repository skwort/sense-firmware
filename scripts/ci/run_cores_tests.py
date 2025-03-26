import json
import subprocess
import os
import argparse
import shutil
from junitparser import JUnitXml, TestCase, Failure, Error
from colorama import Fore, Style

TERM_WIDTH = shutil.get_terminal_size((80, 20)).columns


def run_test(fw_image, board, port, baud, serial_number, build_map,
             summary_dir, print_output=False):
    """
    Runs pytest for a given firmware image and collects results.
    """
    image_name = fw_image["binary"].split("/")[-1]
    result_file = f"{summary_dir}/{image_name}.xml"

    cmd = [
        "pytest", "-p", "no:cacheprovider", "tests/cores/control",
        "--rootdir", ".",
        "--board", board,
        "--port", port,
        "--baud", str(baud),
        "--fw-image", fw_image["binary"],
        "--serial-number", serial_number,
        "--build-map", build_map,
        f"--junit-xml={result_file}"
    ]

    if print_output:
        process = subprocess.run(cmd)
    else:
        process = subprocess.run(cmd, capture_output=True, text=True)

    return {
        "fw_image": fw_image["name"],
        "result_file": result_file,
        "returncode": process.returncode
    }


def parse_junit_results(xml_file):
    """
    Parses a JUnit XML report using junitparser and returns test
    results.
    """
    xml = JUnitXml.fromfile(xml_file)

    results = {
        "passed": [],
        "failed": [],
        "skipped": [],
        "total": 0
    }

    for suite in xml:
        for case in suite:

            if case.result:
                if any(isinstance(r, Failure)
                       or isinstance(r, Error)
                       for r in case.result):
                    results["failed"].append(case)

                elif any(isinstance(r, TestCase.Skipped) for r in case.result):
                    results["skipped"].append(case)
            else:
                results["passed"].append(case)

    results["total"] = (len(results["passed"])
                        + len(results["failed"])
                        + len(results["skipped"]))

    results["duration"] = sum(suite.time for suite in xml)

    return results


def get_test_name(case: TestCase):
    if case.classname:
        return f"{case.classname}.{case.name}"
    else:
        return case.name


def print_image_test_summary(results):
    """
    Format and print a structured summary of test results for a
    single image.
    """
    total_tests = results["total"]
    num_passed = len(results["passed"])
    num_failed = len(results["failed"])
    num_skipped = len(results["skipped"])

    print("." * TERM_WIDTH)
    if num_passed:
        test_str = "tests" if num_passed > 1 else "test"
        print(f"{Fore.GREEN}✔ PASSED: {num_passed} {test_str}")
        for case in results["passed"]:
            print(f"    - {get_test_name(case)}  ({round(case.time, 2)}s)")
        print(Style.RESET_ALL, end="")

    if num_failed:
        test_str = "tests" if num_passed > 1 else "test"
        print(f"{Fore.RED}✘ FAILED: {num_failed} {test_str}")
        for case in results["failed"]:
            print(f"    - {get_test_name(case)}  ({round(case.time, 2)}s)")
        print(Style.RESET_ALL, end="")

    if num_skipped:
        test_str = "tests" if num_passed > 1 else "test"
        print(f"{Fore.YELLOW}⚠ SKIPPED: {num_skipped} {test_str}")
        for case in results["skipped"]:
            print(f"    - {get_test_name(case)}  ({round(case.time, 2)}s)")
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


def print_overall_summary(test_results):
    """
    Print an overall summary of all test results in a table format.
    """
    print()
    print(f"{Style.BRIGHT}  Overall Results  {Style.RESET_ALL}"
          .center(TERM_WIDTH + 8, "="))
    print()

    header = f"{'Firmware Image':<30} | " \
             f"{'Passed':<7} | " \
             f"{'Failed':<7} | " \
             f"{'Skipped':<7} | " \
             f"{'Duration (s)':<12}"

    print(header)
    print("-" * len(header))

    for fw_image_name, result in test_results.items():
        passed = len(result["passed"])
        failed = len(result["failed"])
        skipped = len(result["skipped"])
        duration = result["duration"]

        passed_str = f"{Fore.GREEN}{passed:<7}{Style.RESET_ALL}"
        failed_str = f"{Fore.RED}{failed:<7}{Style.RESET_ALL}"
        skipped_str = f"{Fore.YELLOW}{skipped:<7}{Style.RESET_ALL}"
        duration_str = f"{Fore.CYAN}{duration:<12.2f}{Style.RESET_ALL}"

        print(f"{fw_image_name:<30} | "
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


def parse_args():
    """
    Collect and return command line arguments.
    """
    parser = argparse.ArgumentParser(
        description="Run tests against each of the images in the build map."
    )
    parser.add_argument(
        "--binary-dir",
        default="build",
        type=str,
        help="The location of the binaries to be used in testing."
    )
    parser.add_argument(
        "--build-map",
        default="build/build_map.json",
        type=str,
        help="The path to the build_map.json file."
    )
    parser.add_argument(
        "--board",
        type=str,
        required=True,
        help="The target board specifier."
    )
    parser.add_argument(
        "--port",
        type=str,
        required=True,
        help="The debug serial port of the target board."
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="The baud-rate of the the serial port."
    )
    parser.add_argument(
        "--serial-number",
        type=str,
        help="The serial number of the debugger attached to the target board."
    )
    parser.add_argument(
        "--summary-dir",
        type=str,
        default="summary",
        help="The path to store the JUnit XML files after a image is tested"
    )
    parser.add_argument(
        "--print-test-output",
        action="store_true",
        help="Print the output of each pytest exectution to stdout."
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean the summary directory before starting."
    )

    return parser.parse_args()


def main():
    args = parse_args()

    with open(args.build_map, "r") as f:
        build_map = json.load(f)

    if args.clean and os.path.exists(args.summary_dir):
        shutil.rmtree(args.summary_dir)

    test_results = {}

    for fw_image in build_map.get("firmware_images", []):
        print()
        print(f"{Style.BRIGHT}  {fw_image["name"]}  {Style.RESET_ALL}"
              .center(TERM_WIDTH + 8, "="))
        print(f"Running tests for {fw_image["name"]}...")

        result = run_test(
            fw_image=fw_image,
            board=args.board,
            port=args.port,
            baud=args.baud,
            serial_number=args.serial_number,
            build_map=args.build_map,
            summary_dir=args.summary_dir,
            print_output=args.print_test_output
        )

        summary = parse_junit_results(result["result_file"])
        print_image_test_summary(summary)
        test_results[fw_image["name"]] = summary
        test_results[fw_image["name"]]["returncode"] = result["returncode"]

    print_overall_summary(test_results)


if __name__ == "__main__":
    main()
