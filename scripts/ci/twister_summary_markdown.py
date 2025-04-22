import json
import sys
import re


def load_twister_json(json_path):
    with open(json_path, 'r') as f:
        return json.load(f)


def slugify(text):
    """
    Convert suite names into anchor slugs.
    """
    slug = text.lower()
    slug = re.sub(r"[^\w\s-]", "", slug)
    slug = slug.replace(" ", "-")
    return slug


def suite_summary_table(results):
    """
    Generate a summary of all test results in table format.
    """
    lines = []
    header = "| Suite | Passed | Failed | Skipped | Duration (s) |"
    separator = "|---|---|---|---|---|"
    lines.append(header)
    lines.append(separator)

    total_passed = total_failed = total_skipped = 0
    total_duration = 0.0

    for suite_name, result in results.items():
        passed = len(result["passed"])
        failed = len(result["failed"])
        skipped = len(result["skipped"])
        duration = result["duration"]

        total_passed += passed
        total_failed += failed
        total_skipped += skipped
        total_duration += duration

        anchor = slugify(suite_name)
        lines.append(
            f"| [{suite_name}](#{anchor}) | {passed} | {failed} | {skipped} "
            f"| {duration:.2f} |"
        )

    lines.append(f"| **TOTAL:** | **{total_passed}** | **{total_failed}** |"
                 f" **{total_skipped}** | **{total_duration:.2f}** |")

    return "\n".join(lines)


def suite_details_markdown(suite_name, results):
    passed = results["passed"]
    failed = results["failed"]
    skipped = results["skipped"]
    duration = results["duration"]

    total = len(passed) + len(failed) + len(skipped)
    anchor = slugify(suite_name)

    lines = [f"<a id=\"{anchor}\"></a>"]
    lines.append(f"<details><summary><strong>{suite_name}</strong>"
                 "</summary>\n")
    lines.append("")
    lines.append(
        f"✅ {len(passed)} Passed, ❌ {len(failed)} Failed, ⚠️ {len(skipped)} "
        f"Skipped — total {total} tests in {duration:.2f}s\n"
    )

    def section(title, emoji, cases):
        if not cases:
            return
        lines.append(f"#### {emoji} {title} ({len(cases)})")
        lines.append("")
        lines.append("| Identifier | Execution Time (s) |")
        lines.append("|------------|-------------------|")
        for tc in cases:
            lines.append(f"| {tc['identifier']} | {tc['execution_time']} |")
        lines.append("")

    section("Passed", "✅", passed)
    section("Failed", "❌", failed)
    section("Skipped", "⚠️", skipped)

    lines.append("</details>\n")
    return "\n".join(lines)


def get_suite_summary(suite):
    """
    Generate a dictionary summary of a test suite.
    """
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


def main(json_path):
    try:
        data = load_twister_json(json_path)
    except FileNotFoundError:
        print(f"Unable to locate '{json_path}'")
        sys.exit(1)

    suites = data.get("testsuites", [])
    all_results = {}

    for suite in suites:
        result = get_suite_summary(suite)
        all_results[suite["name"]] = result

    print("## 📋 Test Summary\n")
    print(suite_summary_table(all_results))
    print("\n---\n")
    print("## 🔍 Test Suite Details\n")
    for suite_name, result in all_results.items():
        print(suite_details_markdown(suite_name, result))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <path-to-twister.json>")
        sys.exit(1)

    main(sys.argv[1])
