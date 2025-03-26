import json
import os
import shutil
import re
import argparse

TWISTER_TESTPLAN_JSON = "testplan.json"

TWISTER_OUT_DIR = "twister-out-control"
OUTPUT_DIR = "build"


def parse_test_plan(twister_dir):
    """
    Parse testplan.json to extract test configurations and platforms.
    """

    testplan_path = os.path.join(TWISTER_OUT_DIR, TWISTER_TESTPLAN_JSON)

    with open(testplan_path, "r") as f:
        testplan = json.load(f)

    test_images = []
    for suite in testplan["testsuites"]:
        name = suite["name"]
        platform = suite["platform"]

        platform_as_path = platform.replace("/", "_")

        build_path = os.path.join(twister_dir, platform_as_path, name)

        if os.path.exists(build_path):
            test_images.append({
                "name": name,
                "platform": platform,
                "path": build_path
            })
        else:
            raise FileNotFoundError(
                f"Build path '{build_path}' for {name} does not exist.")

    return test_images


def extract_kconfig_options(config_path, config_prefix):
    """
    Extract relevant Kconfig options from the .config file.
    """

    config_regex = re.compile(rf"^({config_prefix}_[A-Z0-9_]+)=(.+)$")
    config_options = {}

    with open(config_path, "r") as f:
        for line in f:
            match = config_regex.match(line.strip())
            if match:
                key, value = match.groups()
                config_options[key] = value

    return config_options


def generate_build_map(test_images, app_name, config_prefix):
    """
    Generate build_map.json by parsing the twister image build artifacts.
    """

    build_map = {"firmware_images": []}

    for image in test_images:
        config_path = os.path.join(image["path"], app_name, "zephyr",
                                   ".config")

        binary_path = os.path.join(image["path"], "merged.hex")

        if not os.path.exists(config_path):
            raise FileNotFoundError(f"""
                Unable to locate config for image: '{image["name"]}.
                File: '{config_path}' does not exist.""")

        if not os.path.exists(binary_path):
            raise FileNotFoundError(f"""
                Unable to locate binary for image: '{image["name"]}.
                File: '{binary_path}' does not exist.""")

        config_options = extract_kconfig_options(config_path, config_prefix)

        build_map["firmware_images"].append({
            "name": image["name"],
            "platform": image["platform"],
            "binary": binary_path,
            "config": config_options
        })

    return build_map


def collect_binaries(output_dir, build_map, clean=False):
    """
    Store the image binaries and build_map in output directory.
    """

    if clean and os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    # NOTE: This will raise an OSError if the output dir already exists.
    os.makedirs(output_dir, exist_ok=False)

    for i, fw in enumerate(build_map["firmware_images"]):
        # Copy the binary
        src_bin = fw["binary"]
        dest_bin = os.path.join(output_dir, f"{fw['name']}.hex")
        shutil.copy2(src_bin, dest_bin)

        # Update the binary record
        build_map["firmware_images"][i]["binary"] = dest_bin

    with open(os.path.join(output_dir, "build_map.json"), "w") as f:
        json.dump(build_map, f, indent=4)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate build_map.json and collect firmware binaries."
    )
    parser.add_argument(
        "--app-name",
        required=True,
        help="Application name used in the build directory (e.g. 'control')."
    )
    parser.add_argument(
        "--config-prefix",
        required=True,
        help="The Kconfig prefix to match (e.g. 'CONFIG_APP')."
    )
    parser.add_argument(
        "--output-dir",
        default="build",
        help="Directory where binaries and build map will be stored."
    )
    parser.add_argument(
        "--twister-out-dir",
        required=True,
        help="Path to Twister output directory."
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean the output directory before collecting binaries."
    )

    return parser.parse_args()


def main():
    args = parse_args()

    print("Parsing testplan.json...")
    test_images = parse_test_plan(args.twister_out_dir)

    print(f"Found {len(test_images)} test images.")

    print("Extracting configurations...")
    build_map = generate_build_map(test_images, args.app_name,
                                   args.config_prefix)

    print("Collecting firmware binaries...")
    collect_binaries(args.output_dir, build_map, clean=args.clean)

    print(f"Complete. Build artifacts stored in '{args.output_dir}/'.")


if __name__ == "__main__":
    main()
