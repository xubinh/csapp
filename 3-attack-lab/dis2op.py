#!/usr/bin/env python3
import sys
import re


def extract_operation_code(disas_file_path: str) -> str:
    with open(disas_file_path, encoding="utf8") as file:
        content_lines = file.readlines()

    operation_code_strings = []
    for line in content_lines:
        match_result = re.fullmatch(r"^[^\t]+\t([0-9a-fA-F ]+)\t.+\n$", line)
        if not match_result:
            # print(line[:-1])
            continue
        assert type(match_result.group(1)) == str
        operation_code_strings.append(match_result.group(1))

    final_operation_code_string = " ".join(operation_code_strings)
    final_operation_code_string = re.sub(r" +", " ", final_operation_code_string)
    final_operation_code_string = final_operation_code_string.strip()
    return final_operation_code_string


def post_process(final_operation_code_string: str, padding_size: int) -> str:
    final_operation_code_string = "00 " * padding_size + final_operation_code_string
    return final_operation_code_string


def dis2op(disas_file_path: str, out_file_path: str, padding_size: int) -> None:
    final_operation_code_string = extract_operation_code(disas_file_path)
    # final_operation_code_string = post_process(final_operation_code_string, padding_size)
    with open(out_file_path, "w", encoding="utf8") as file:
        file.write(final_operation_code_string)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise RuntimeError()

    disas_file_path = sys.argv[1]
    out_file_path = sys.argv[2]
    padding_size = int(sys.argv[3])
    dis2op(disas_file_path, out_file_path, padding_size)
