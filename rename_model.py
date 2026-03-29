import re
import argparse
from pathlib import Path


def detect_model_name(content: str):
    match = re.search(r'ai_([a-zA-Z0-9_]+)_create', content)
    if not match:
        raise ValueError("Could not detect model name automatically.")
    return match.group(1)


def to_upper(name: str):
    return name.upper()


def replace_all(content: str, old: str, new: str):
    old_upper = to_upper(old)
    new_upper = to_upper(new)

    content = re.sub(rf'"{old}\.h"', f'"{new}.h"', content)
    content = re.sub(rf'"{old}_data\.h"', f'"{new}_data.h"', content)

    content = re.sub(rf'\bai_{old}_', f'ai_{new}_', content)
    content = re.sub(rf'\b{old}_handle\b', f'{new}_handle', content)

    content = re.sub(rf'\bAI_{old_upper}_', f'AI_{new_upper}_', content)

    return content


def main():
    parser = argparse.ArgumentParser(description="Replace STM32 X-CUBE-AI model name")
    parser.add_argument("new_model")
    parser.add_argument(
        "--file",
        default=r"./Core/Src/app_x-cube-ai.c",
        help="Path to app_x-cube-ai.c"
    )

    args = parser.parse_args()
    file_path = Path(args.file)
    content = file_path.read_text()

    old_model = detect_model_name(content)
    print(f"Detected model: {old_model}")

    new_model = args.new_model

    if old_model == new_model:
        print("Model name is already the same. No changes made.")
        return

    updated = replace_all(content, old_model, new_model)
    file_path.write_text(updated)
    print(f"Replaced '{old_model}' -> '{new_model}' successfully.")


if __name__ == "__main__":
    main()