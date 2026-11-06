#!/usr/bin/env python3
"""원본 PE의 색표 절대 주소 후보와 실제 명령 피연산자를 읽기 전용으로 분류한다."""
import csv
import hashlib
import json
import re
import socket
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 원본/정밀 자료만 읽는다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import capstone
import pefile

REGIONS = [('ownerHighlight', 0x556538, 0x556d38), ('ownerSolid', 0x556e38, 0x557638),
           ('map0', 0x555e38, 0x555f38), ('map8', 0x557838, 0x557938)]  # 원문 두8행과 이번 잔여9표의 첫/끝 행이다.


def digest(name):
    """실행 없이 원시 바이트의 SHA256을 기록한다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def main():
    """숫자 바이트 후보와 실제 코드 피연산자를 구분하고 수동 함수 경계를 명시한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    base = pe.OPTIONAL_HEADER.ImageBase
    image = pe.get_memory_mapped_image()
    ranges = []  # 현재 정밀 목록의 inclusive 코드 범위다. 세미콜론으로 나뉜 여러 구간도 보존한다.
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        for row in csv.DictReader(stream, delimiter='\t'):  # 엔트리와 모든 구간을 독립적으로 읽는다.
            for low, high in re.findall(r'([0-9a-f]+)-([0-9a-f]+)', row['ranges']):  # 쉼표 하나라고 가정하지 않는다.
                ranges.append((int(low, 16), int(high, 16), row['entry'], 'precise-metadata'))
    assert not any(low == 0x45bcd0 for low, _, _, _ in ranges)
    ranges.append((0x45bcd0, 0x45bcff, '45bcd0', 'manual-PE-VT-ret8'))  # VT509b98+a4와 실제ret8로 확인한 목록 밖 몸체다.
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    candidates = []  # 정렬 여부와 무관한4BYTE 후보다. 코드로 검증되지 않은 숫자도 삭제하지 않는다.
    for section in pe.sections:  # 파일에 실제 있는 모든 PE 섹션을 검사한다. BSS0을 코드로 만들지 않는다.
        data = section.get_data()
        for offset in range(len(data) - 3):  # 명령의 비정렬 immediate/disp도 포함한다.
            value = struct.unpack_from('<I', data, offset)[0]
            label = next((label for label, low, high in REGIONS if low <= value < high), None)
            if label is None:
                continue
            address = base + section.VirtualAddress + offset
            owners = [entry for entry in ranges if entry[0] <= address <= entry[1]]  # 현재 후보를 포함한 모든 함수 경계를 보존한다.
            decoded = []  # 같은 후보가 실제 주소 피연산자인 명령만 분리한다.
            for low, high, parent, origin in owners:  # 중첩/별도cleanup의 모든 현재 범위를 확인한다.
                for ins in md.disasm(image[low-base:high-base+1], low):  # 알려진 경계에서 시작하며 임의 중간 BYTE로 디코드하지 않는다.
                    if not (ins.address <= address and address + 4 <= ins.address + ins.size):
                        continue
                    if any((operand.type == capstone.x86.X86_OP_IMM and operand.imm == value) or
                           (operand.type == capstone.x86.X86_OP_MEM and operand.mem.disp == value) for operand in ins.operands):
                        decoded.append(dict(address=f'{ins.address:08x}', parent=parent, origin=origin,
                                            instruction=ins.mnemonic+' '+ins.op_str))
            candidates.append(dict(region=label, address=f'{address:08x}', value=f'{value:08x}',
                                   section=section.Name.rstrip(b'\0').decode(), decoded=decoded))
    body = list(md.disasm(image[0x45bcd0-base:0x45bd00-base], 0x45bcd0))
    assert struct.unpack_from('<I', image, 0x509c3c-base)[0] == 0x45bcd0
    assert body[-1].mnemonic == 'ret' and body[-1].op_str == '8'
    decoded_rows = [entry for entry in candidates if entry['decoded']]  # 명령 피연산자로 확인한 후보만 별도로 집계한다.
    for label in ('ownerHighlight', 'ownerSolid'):  # 두표의이번직접주소피연산자는초기화부모만가리킨다.
        assert {d['parent'] for entry in decoded_rows if entry['region'] == label for d in entry['decoded']} == {'43c4e0'}
    selected = [entry for entry in decoded_rows if entry['region'] == 'map0' and any(d['parent'] == '45bcd0' for d in entry['decoded'])]  # 목록 밖 실제 추가Draw의 주소 참조다.
    assert len(selected) == 1
    product = (ROOT / 'cpppj/src/client/RawCampaignFactoryDisplay.cpp').read_text(encoding='utf-8')
    enum = (ROOT / 'cpppj/src/client/SpriteDraw.h').read_text(encoding='utf-8')
    assert product.count('dispatch.additional==0x45bcd0U') == 1 and 'if (composite[11]&2U) return;const auto frame=DisplayDword(composite,36);' in product
    assert 'impl_->intenseColors=(*colorMaps)[0]' in product and 'mode==SpriteShapeMode::Bright160' in product
    assert 'Shadow, Bright160' in enum
    startup = (ROOT / 'cpppj/src/client/ClientMain.cpp').read_text(encoding='utf-8')
    original = (ROOT / 'extracted/refined/originals/Netstorm.c').read_text(encoding='utf-8')
    start = original.index('// ==== FUN_00438dc0 @')
    finish = original.index('// ==== ', start+8)
    assert 'FUN_004a4850(uVar10,1);' in original[start:finish]
    assert startup.index('screen_->LoadNativePaletteFile(files_,palettePath,1)') < startup.index('colorSortInput_->CaptureWithPrimary(')
    tracked = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv',
               'cpppj/src/client/ObjectHitVtables.inc', 'cpppj/src/client/RawCampaignFactoryDisplay.cpp', 'cpppj/src/client/SpriteDraw.h',
               'cpppj/src/client/ClientMain.cpp', 'cpppj/tests/TranslatedPixelsTests.cpp', 'tools/cpp_colormap_reference_audit.py']
    report = dict(unit=968, host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-11-03'),
                  kind='static PE bytes/instruction operands only; no emulation/game/flyer tests', candidates=candidates,
                  excludedNumericCandidates=[entry for entry in candidates if not entry['decoded']],
                  selected=dict(entry='0045bcd0', table='00509b98', slot='a4', reference='00509c3c', bytes=image[0x45bcd0-base:0x45bd00-base].hex(),
                                instructions=[dict(address=f'{ins.address:08x}', text=ins.mnemonic+' '+ins.op_str) for ins in body]),
                  scope='두8행의직접operand는초기화부모만확인. 간접별칭/임의계산주소/런타임 전체도달부재의증명은아니다. map8도계산/FILE부모밖직접operand미발견. map0의Whirligig폭탄추가Draw는원문코드복원대상이며전용검사는제외한다.',
                  product=dict(actualAdditionalDispatch='0045bcd0', deadByte=11, frameDword=36, colorMapIndex=0, mode='Bright160', flyerBodyExecutionTests=0),
                  startup=dict(parent='00438dc0', nativePalette='004a4850', clear=1, beforeInitialColorSortCapture=True),
                  sha256={name:digest(name) for name in tracked})
    (ROOT / 'cpppj/colormap-reference-static-20261106.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print('static:', len(candidates), 'candidates;', len(decoded_rows), 'decoded operands; map0 -> VT509b98+a4/0045bcd0')


if __name__ == '__main__':
    main()
