#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 동결 기대를 바꾸지 않고 두 x87 정밀도에서 검색 하위와 전체 재개를 다시 검증한다."""
import json
import struct

from decomp_ai_bridge_search import ROOT, FIXTURES, SearchOracle, digest
from unicorn.x86_const import UC_X86_REG_FPCW

REPORT = ROOT / "cpppj/ai-bridge-search-precision-20261022.json"  # 동결 후의 별도 교차 검증 근거다.


def rows(name):
    """기존 관측 행을 읽기만 하며 기대와 입력을 재작성하지 않는다."""
    return [line.split("\t") for line in (FIXTURES / f"ai-bridge-{name}-x86.tsv").read_text(encoding="utf-8").splitlines() if line and not line.startswith("#")]


def main():
    """53/64비트 유효숫자의 실제 명령 반환을 동결된 전 바이트와 대조한다."""
    assert not REPORT.exists()
    results = []
    for precision in (0x27F, 0x37F):  # 실제 CRT가 잠시 바꾼 제어워드도 다시 복구되는지 확인한다.
        oracle = SearchOracle()
        oracle.mu.reg_write(UC_X86_REG_FPCW, precision)
        counts = {}
        for name in ("shape", "clear", "score", "search"):  # 모양→점유→평가→전체 검색의 기존 모든 입력을 재생한다.
            samples = rows(name)
            for row in samples:  # C++ 구현/기대 생성식을 사용하지 않는다.
                if name == "shape":
                    actual = oracle.shape(*map(int, row[:3])).hex()
                    assert actual == row[3]
                elif name == "clear":
                    assert oracle.clear(*map(int, row[:5])) == int(row[5])
                elif name == "score":
                    pattern, direction, x, y, tx, ty, excluded, fill, score = map(int, row[:9])
                    shape = oracle.shape(pattern, direction, 0)
                    assert oracle.score(shape, (x, y, pattern, direction), (tx, ty), excluded, fill) == (score, row[9])
                else:
                    observed = oracle.search(bytes.fromhex(row[4]), int(row[2]), bytes.fromhex(row[3]), 4, int(row[1]))
                    encoded = ";".join(",".join(map(str, value)) for value in observed)
                    assert encoded == row[5]
                assert oracle.mu.reg_read(UC_X86_REG_FPCW) == precision
            counts[name] = len(samples)
        results.append({"control_word": precision, "rows": counts, "search_returns": counts["search"] * 4})
    report = {"precision_runs": results, "fixture_modified": False, "stubs": [], "game_started": False, "sources": {str(path.relative_to(ROOT)): digest(path) for path in (ROOT / "tools/decomp_ai_bridge_search.py", ROOT / "tools/decomp_ai_bridge_search_verify.py", ROOT / "originals/Netstorm.exe", ROOT / "originalCD/NETSTORM.EXE")}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(results))


if __name__ == "__main__":
    main()
