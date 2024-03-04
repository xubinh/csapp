#!/usr/bin/env python3
import sys
import re
from typing import Callable
from os.path import basename, dirname, join, split
from pathlib import Path


def get_content_lines(
    md_file_path: str,
    is_toc_entry: Callable[[str], bool],
    is_toc_end_entry: Callable[[str], bool],
) -> tuple[int, list[str]]:
    """按行读取文件内容, 切除目录部分, 返回剩余的所有内容与新目录的插入位置"""
    with open(md_file_path, encoding="utf8") as file:
        content_lines = [line[:-1] for line in file.readlines()]
    toc_begin_idx = None
    for idx, line in enumerate(content_lines):
        if is_toc_entry(line):
            toc_begin_idx = idx
            break
    assert toc_begin_idx, "没有找到目录"
    toc_end_idx = None
    for idx in range(toc_begin_idx + 1, len(content_lines)):
        line = content_lines[idx]
        if is_toc_end_entry(line):
            toc_end_idx = idx
            break
    assert toc_end_idx, "目录行以下需要另一个标题行来确定目录部分的范围"

    toc_insert_position = toc_begin_idx
    content_lines = content_lines[:toc_begin_idx] + content_lines[toc_end_idx:]

    return toc_insert_position, content_lines


def get_headers(content_lines: list[str]) -> tuple[list[str], list[int]]:
    """按顺序提取出所有标题行内容, 以及这些标题在原文中的行号"""
    headers = []
    header_positions = []
    for idx, line in enumerate(content_lines):
        if line and line[0] == "#":
            headers.append(line)
            header_positions.append(idx)
    return headers, header_positions


def clean_headers(headers: list[str]) -> list[str]:
    """清洗标题行中已有的锚点"""
    regex_clean_headers = re.compile(r"<a id=\"[0-9\.]+\"></a>")
    headers = [regex_clean_headers.sub(r"", header) for header in headers]
    return headers


def insert_anchors(headers: list[str]) -> tuple[list[str], list[str]]:
    """向标题行文本中插入锚点, 同时生成对应的目录条目"""
    regex_get_header_info = re.compile(r"(#+) +(.+)")

    def _get_header_info(header: str) -> tuple[int, str]:
        header_match = regex_get_header_info.fullmatch(header)
        assert header_match, "分割标题内容失败"
        header_level = len(header_match.group(1))
        header_text = header_match.group(2)
        return header_level, header_text

    header_path: list[int] = []

    def _get_current_header_id() -> str:
        assert header_path, "当前还未遍历到任何标题行"
        current_header_id = ".".join([str(idx) for idx in header_path[1:]])
        return current_header_id

    def _get_anchor(current_header_id: str):
        return f'<a id="{current_header_id}"></a>'

    def _get_reference(current_header_id: str, header_text: str) -> str:
        return f'<a href="#{current_header_id}">{header_text}</a>'

    def _insert_anchor(header: str) -> list[str]:
        header_level, header_text = _get_header_info(header)
        assert header_level <= len(header_path) + 1, "标题不能够一次下降两级以上"
        assert not (header_path and header_level == 1), "一级标题有且只能有一个"

        # 向上回溯标题树:
        while header_level < len(header_path):
            header_path.pop()

        # 如果是下级子标题:
        if len(header_path) < header_level:
            header_path.append(1)

        # 如果是同级兄弟标题:
        else:
            header_path[-1] += 1

        current_header_id = _get_current_header_id()
        anchor = _get_anchor(current_header_id)
        reference = _get_reference(current_header_id, header_text)

        anchor_header = f'{"#" * header_level} {anchor}{header_text}'
        toc_entry = f'{"  " * (header_level - 2)}- {reference}'

        if header_level == 1:
            anchor_header = header
            toc_entry = ""

        return [anchor_header, toc_entry]

    anchor_headers, toc_entries = list(zip(*[_insert_anchor(header) for header in headers]))
    toc_entries = [toc_entry for toc_entry in toc_entries if toc_entry]

    return list(anchor_headers), list(toc_entries)


def fill_in_new_headers_and_toc(
    content_lines: list[str],
    anchor_headers: list[str],
    header_positions: list[int],
    toc_insert_position: int,
    toc_entries: list[str],
    get_toc_entry: Callable[[], str],
) -> str:
    """替换标题行, 插入新目录, 返回新的 md 文本"""
    for anchor_header, header_position in zip(anchor_headers, header_positions):
        content_lines[header_position] = anchor_header
    toc_entries.insert(0, "")
    toc_entries.insert(0, get_toc_entry())
    toc_entries.append("")
    toc_entries.reverse()
    for toc_entry in toc_entries:
        content_lines.insert(toc_insert_position, toc_entry)

    content_lines.append("")
    content = "\n".join(content_lines)
    return content


def generate_toc(
    md_file_path: str,
    is_toc_entry: Callable[[str], bool],
    is_toc_end_entry: Callable[[str], bool],
    get_toc_entry: Callable[[], str],
):
    """主函数"""
    toc_insert_position, content_lines = get_content_lines(
        md_file_path=md_file_path,
        is_toc_entry=is_toc_entry,
        is_toc_end_entry=is_toc_end_entry,
    )

    headers, header_positions = get_headers(content_lines=content_lines)
    headers = clean_headers(headers=headers)
    anchor_headers, toc_entries = insert_anchors(headers=headers)
    content = fill_in_new_headers_and_toc(
        content_lines=content_lines,
        anchor_headers=anchor_headers,
        header_positions=header_positions,
        toc_insert_position=toc_insert_position,
        toc_entries=toc_entries,
        get_toc_entry=get_toc_entry,
    )

    def _get_copy_md_file_path(md_file_path: str) -> str:
        md_file_dir_name = dirname(md_file_path)
        md_file_base_name = basename(md_file_path)
        md_file_stem = Path(md_file_base_name).stem
        md_file_suffix = Path(md_file_base_name).suffix

        copy_md_file_path = (md_file_dir_name + "/" if md_file_dir_name else "") + f"{md_file_stem}-copy{md_file_suffix}"
        return copy_md_file_path

    copy_md_file_path = _get_copy_md_file_path(md_file_path=md_file_path)
    # with open(copy_md_file_path, "w", encoding="utf8") as file:
    with open(md_file_path, "w", encoding="utf8") as file:
        file.write(content)


if __name__ == "__main__":
    md_file_path = "./README.md"

    if len(sys.argv) > 1:
        md_file_path = sys.argv[1]

    print(f"parse file `{md_file_path}`")

    def is_toc_entry(line: str) -> bool:
        if line and line[0] == "#" and line.count("目录"):
            return True
        return False

    def is_toc_end_entry(line: str) -> bool:
        if line and line[0] == "#":
            return True
        return False

    def get_toc_entry() -> str:
        return "## 目录"

    generate_toc(
        md_file_path=md_file_path,
        is_toc_entry=is_toc_entry,
        is_toc_end_entry=is_toc_end_entry,
        get_toc_entry=get_toc_entry,
    )
