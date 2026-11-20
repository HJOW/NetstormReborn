"""CPP의 읽기 전용 SelectionVtables.inc를 같은83개 원문 주소의 C# 표로 옮긴다."""
from pathlib import Path
import argparse
import hashlib
import re

# 실행 위치와 무관한 저장소 루트다.
ROOT = Path(__file__).resolve().parents[2]
# 기존 CPP 원문은 읽기만 한다.
SOURCE = ROOT / 'cpppj/src/o/SelectionVtables.inc'
# Core의 데이터 전용 생성 결과다.
OUTPUT = ROOT / 'dotnetpj/src/Netstorm.Core/Original/OriginalSelectionVtables.Data.cs'
# 보호10.78에서 관측한 생성자 VT의 정확한 수다.
ENTRY_COUNT = 83


def render():
    """원문 세 주소의 순서/전체 비트를 유지한 UTF-8/LF C# 표를 만든다."""
    raw = SOURCE.read_bytes().replace(b'\r\n', b'\n')
    records = re.findall(r'\{(0x[0-9a-fA-F]+)U,(0x[0-9a-fA-F]+)U,(0x[0-9a-fA-F]+)U\}', raw.decode('utf-8'))
    assert len(records) == ENTRY_COUNT and len({record[0] for record in records}) == ENTRY_COUNT
    lines = ['// gen_original_selection_vtables.py가 만든다. 직접 수정하지 않고 원문을 재대조한 뒤 생성한다.',
             '// 읽기 전용 CPP SelectionVtables.inc SHA-256(LF): ' + hashlib.sha256(raw).hexdigest(),
             '// 원본 VT 식별자이며 호스트 함수 포인터로 역참조하지 않는다.', 'namespace Netstorm.Core.Original;', '',
             '/// <summary>선택+7c/+88의 보호10.78 생성자83개 기록이다.</summary>',
             'public static partial class OriginalSelectionVtables', '{',
             '    /// <summary>원문 VT·선택 통지·표시 갱신의 세 식별 주소다.</summary>',
             '    /// <param name="Table">원문 VT다.</param>', '    /// <param name="Notice">가상+7c다.</param>', '    /// <param name="Refresh">가상+88이다.</param>',
             '    private readonly record struct Entry(uint Table, uint Notice, uint Refresh);',
             '    /// <summary>원문 선언 순서의83개 주소이며 최초 한 번만 구성한다.</summary>',
             '    private static readonly Entry[] Entries =', '    [']
    # 원문 각 행의 세 주소를 계산이나 추정 없이 그대로 출력한다.
    for record in records:
        lines.append('        new(' + ', '.join(value + 'u' for value in record) + '),')
    lines += ['    ];', '}', '']
    return '\n'.join(lines)


def main():
    """기본 실행은 생성하고 --check는 현재 결과와 비교만 한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    expected = render()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8') != expected:
            print('선택 VT 생성 자료가 원문과 다름'); return 1
        print('선택 VT83개 생성 자료 일치'); return 0
    OUTPUT.write_text(expected, encoding='utf-8', newline='\n')
    print('선택 VT83개 생성 완료'); return 0


if __name__ == '__main__':
    raise SystemExit(main())
