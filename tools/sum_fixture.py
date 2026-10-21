"""fixture 의 절 구성과 열 개수를 요약한다 (작업용 도구)."""
import sys
from collections import Counter
from pathlib import Path

root = Path(__file__).resolve().parents[1]
for name in sys.argv[1:]:
    path = root / "cpppj" / "tests" / "fixtures" / f"{name}-x86.tsv"
    rows = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            rows.append(line.split("\t"))
    print(f"===== {name} : {len(rows)} rows =====")
    counter = Counter((r[0], r[1]) for r in rows)
    for key, count in sorted(counter.items()):
        print(f"  section={key[0]!r} edition={key[1]!r} rows={count}")
    widths = Counter(len(r) for r in rows)
    print(f"  columns: {dict(widths)}")
    for key in sorted(set((r[0], r[1]) for r in rows)):
        sample = next(r for r in rows if (r[0], r[1]) == key)
        print(f"  --- sample {key} ---")
        for i, cell in enumerate(sample):
            text = cell if len(cell) <= 70 else cell[:70] + f"...(len {len(cell)})"
            print(f"    [{i}] {text}")
        break
