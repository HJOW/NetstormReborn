#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""정밀 디컴파일 결과(extracted/refined)에 판본 간 함수 대응과 신뢰도 등급을 붙인다.

입력은 tools/ghidra/refine_decomp.ps1 이 판본마다 만든 raw.c, functions.tsv, conventions.tsv, missing.tsv 와
원본 exe 다. 원본 exe 는 읽기만 한다. 하는 일:

1. 함수 특징 추출: exe 바이트를 역어셈블해 함수마다 참조 문자열, 임포트 API, 상수, 호출 순서, 니모닉 분포를 모은다.
2. 판본 간 함수 대응: 패치판(originals)과 CD판(originalCD)의 함수를 짝짓는다.
   이름 → 유일 문자열 → 호출 순서·호출자 전파 → 모듈 내 국소 구간 순으로 확정하고, 호출 그래프 문맥이 맞지 않는 쌍은 취소한다.
3. 호출 규약 힌트(--hints): 한 판본에서 근거가 부족한 함수를 상대 판본의 짝이 가진 확실한 근거로 보완한다.
4. 함수 포인터 표 색인: .rdata 의 함수 포인터 줄을 표로 묶어 "표 주소·슬롯 → 함수"를 만든다.
5. 신뢰도 등급: 함수마다 존재 근거(호출자·포인터·판본 간 대응)와 조각 의심 징후를 모아 A~D 를 매긴다.
6. 최종 C 파일: raw.c 의 함수 머리줄 아래에 등급·근거·대응 함수를 주석으로 붙인다.

사용 예 (저장소 루트에서, 보통은 tools/ghidra/refine_all.ps1 이 순서대로 부른다):
    python tools/decomp_refine.py --hints    # 대응을 계산해 판본별 hints.tsv 만 쓴다
    python tools/decomp_refine.py            # 대응표·등급·함수 포인터 표·최종 C 파일을 쓴다
    python tools/decomp_refine.py --probe    # 대응 품질 통계(홀드아웃 검증 포함)만 출력한다
    python tools/decomp_refine.py --show 4b1e80                       # 패치판 함수와 CD판의 짝을 차례로 출력
    python tools/decomp_refine.py --show 4735a0 --edition originalCD  # CD판 함수와 패치판의 짝

결과 (모두 extracted/refined 아래, Git 제외):
    match.tsv                    패치판↔CD판 함수 대응표
    <판본>/hints.tsv             상대 판본 근거로 보완한 호출 규약 힌트
    <판본>/grades.tsv            함수별 근거와 등급
    <판본>/vtables.tsv           함수 포인터 표 색인
    originals/Netstorm.c, originalCD/NETSTORM.c   등급·근거 주석이 붙은 최종 디컴파일
"""
import argparse
import bisect
import collections
import csv
import difflib
import hashlib
import json
import math
import re
import sys
from pathlib import Path

import capstone
import pefile

# 저장소 루트 (이 스크립트 기준 한 단계 위)
ROOT = Path(__file__).resolve().parents[1]
# 정밀 디컴파일 결과 폴더
REFINED = ROOT / 'extracted' / 'refined'
# 판본별 원본 exe 경로와 최종 C 파일 이름
EDITIONS = {
    'originals': {'exe': ROOT / 'originals' / 'Netstorm.exe', 'out': 'Netstorm.c', 'label': '패치판'},
    'originalCD': {'exe': ROOT / 'originalCD' / 'NETSTORM.EXE', 'out': 'NETSTORM.c', 'label': 'CD판'},
}

# 문자열로 인정하는 최소 길이 (짧은 문자열은 우연히 겹치기 쉽다)
MIN_STRING_LEN = 5
# 상수 특징으로 쓰는 값의 최소 크기 (작은 값은 어느 함수에나 있어 구별력이 없다)
MIN_CONST = 0x100
# 함수 하나에서 모으는 상수의 최대 개수
MAX_CONSTS = 96
# 호출 순서 전파에서 한 쌍을 확정하는 데 필요한 최소 유사도
PROPAGATE_MIN_SIM = 0.30
# 국소 구간에서 양쪽 함수 수가 같아 순서대로 짝지을 때 요구하는 최소 유사도
GAPFILL_MIN_SIM = 0.45
# 국소 구간으로 인정하는 최대 함수 수 (이보다 길면 같은 오브젝트 파일 구간이라고 보기 어렵다)
MAX_GAP_FUNCS = 40
# 문맥 검증: 짝지어진 이웃이 이 수 이상인데 겹치는 비율이 PRUNE_MAX_RATIO 미만이면 대응을 취소한다
PRUNE_MIN_CONTEXT = 3
PRUNE_MAX_RATIO = 0.34
# 전파 → 국소 구간 → 검증 반복의 최대 횟수
MAX_MATCH_ROUNDS = 12
# 표 하나 없이 유사도만으로 확정할 때 필요한 최소 유사도 (국소 구간 안에서만 적용)
NEIGHBOR_MIN_SIM = 0.72
# 유사도만으로 확정할 때 2위 후보보다 앞서야 하는 최소 차이
NEIGHBOR_MIN_MARGIN = 0.08
# Ghidra 가 이름을 붙이지 못한 함수 이름의 접두사 (이 접두사로 시작하면 이름 근거로 쓰지 않는다)
ANON_PREFIXES = ('FUN_', 'thunk_FUN_', 'Unwind@', 'Catch@', 'Catch_All@', 'FID_conflict')
# Ghidra 가 예외 처리 funclet(정리·catch 블록)에 붙이는 이름의 접두사
FUNCLET_PREFIXES = ('Unwind@', 'Catch@', 'Catch_All@')
# 양쪽 근거(피호출 함수의 레지스터 읽기 + 호출 지점의 레지스터 전달)가 모두 있는 호출 규약 판정 근거
TWO_SIDED_BASES = ('ecx-both-sides', 'ecx-pass-through', 'ecx+edx-both-sides')
# ECX 를 먼저 읽지만 호출 쪽 근거가 엇갈려 규약을 지정하지 못한 판정 근거 (상대 판본의 근거로 보완할 수 있는 경우)
FALLBACK_BASES = ('ambiguous-ecx',)
# 디컴파일 결과에 나타나는 호출 규약 표기
CONVENTIONS = ('__thiscall', '__fastcall', '__stdcall', '__cdecl')
# 16진수 피연산자 추출용 정규식
HEX_RE = re.compile(r'0x([0-9a-f]+)')
# 디컴파일 결과의 함수 블록 머리줄
HEADER_RE = re.compile(r'^// ==== (\S+) @ ([0-9a-fA-F]+)', re.M)
# 조각(다른 함수의 일부)으로 의심하게 하는 디컴파일 징후: 호출자의 스택·프레임 레지스터를 그대로 쓰는 변수
FRAGMENT_RE = re.compile(r'\b(in_stack_[0-9a-f]+|unaff_EBP|unaff_retaddr)\b')
# 인자를 추정하지 못했음을 뜻하는 디컴파일 징후: 표준 호출 규약으로 설명되지 않는 레지스터 입력·출력
UNSURE_SIG_RE = re.compile(r'\b(unaff_E[A-Z]{2}|in_E[A-Z]{2}|extraout_E[A-Z]{2}|in_ST\d|in_FPUStatusWord)\b')
# 디컴파일 실패·깨진 흐름을 뜻하는 징후
BROKEN_RE = re.compile(r'(디컴파일 실패|halt_baddata|Bad instruction|Control flow encountered bad instruction)')


def read_tsv(path, key):
    """TSV 파일을 '키 열의 16진수 값 → 행' 사전으로 읽는다. 파일이 없으면 빈 사전."""
    table = {}
    if not path.exists():
        return table
    with open(path, encoding='utf-8', newline='') as fp:
        # 머리줄을 열 이름으로 써서 한 줄씩 읽는다
        for row in csv.DictReader(fp, delimiter='\t'):
            table[int(row[key], 16)] = row
    return table


def split_blocks(text):
    """디컴파일 텍스트를 (주소, 블록 문자열) 목록으로 나눈다."""
    marks = [m.start() for m in re.finditer(r'^// ==== ', text, re.M)]
    marks.append(len(text))
    blocks = []
    # 머리줄 위치 사이를 한 함수 블록으로 자른다
    for begin, end in zip(marks, marks[1:]):
        block = text[begin:end]
        head = HEADER_RE.match(block)
        if head:
            blocks.append((int(head.group(2), 16), block))
    return blocks


def parse_proto(block):
    """디컴파일 블록에서 함수 정의의 호출 규약 표기와 인자 수를 뽑는다. 못 찾으면 ('', None)."""
    body_at = block.find('\n{')
    if body_at < 0:
        return '', None
    # 함수 정의 앞의 주석 줄(머리줄, WARNING)을 빼고 한 줄로 잇는다
    lines = [ln.strip() for ln in block[:body_at].replace('\r', '').split('\n')]
    sig = ' '.join(ln for ln in lines if ln and not ln.startswith('//') and not ln.startswith('/*'))
    left, right = sig.find('('), sig.rfind(')')
    if left < 0 or right < left:
        return '', None
    conv = next((c for c in CONVENTIONS if c in sig[:left]), '')
    params = sig[left + 1:right].strip()
    if params in ('', 'void'):
        return conv, 0
    # 괄호 깊이 0 의 쉼표만 인자 구분자로 센다 (함수 포인터 인자의 괄호 안 쉼표는 제외)
    depth = count = 0
    for ch in params:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        elif ch == ',' and depth == 0:
            count += 1
    return conv, count + 1


class Edition:
    """한 판본의 exe 와 함수 사실, 함수 특징을 담는다."""

    def __init__(self, key):
        self.key = key
        self.label = EDITIONS[key]['label']
        self.dir = REFINED / key
        self.pe = pefile.PE(str(EDITIONS[key]['exe']))
        self.img = self.pe.get_memory_mapped_image()
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.image_end = self.base + len(self.img)
        # 구역별 주소 범위: (시작, 끝, 이름, 실행 가능 여부)
        self.sections = []
        for sec in self.pe.sections:
            start = self.base + sec.VirtualAddress
            end = start + max(sec.Misc_VirtualSize, sec.SizeOfRawData)
            name = sec.Name.rstrip(b'\0').decode('latin1')
            self.sections.append((start, end, name, bool(sec.Characteristics & 0x20000000)))
        # 임포트 주소 표: IAT 주소 → API 이름
        self.imports = {}
        for desc in getattr(self.pe, 'DIRECTORY_ENTRY_IMPORT', []):
            for imp in desc.imports:
                dll = desc.dll.decode('latin1')
                self.imports[imp.address] = imp.name.decode('latin1') if imp.name else f'{dll}#{imp.ordinal}'
        # 함수 사실 (DumpFunctions.java 출력): 진입 주소 → 행
        self.funcs = read_tsv(self.dir / 'functions.tsv', 'entry')
        self.entries = sorted(self.funcs)
        # 누락 함수 복구 출처 (RecoverMissing.java 출력): 주소 → 출처 문자열
        self.recovered = {addr: row['source'] for addr, row in read_tsv(self.dir / 'missing.tsv', 'address').items()}
        # 호출 규약 판정 근거 (ApplyConventions.java 출력): 진입 주소 → 행
        self.conv = read_tsv(self.dir / 'conventions.tsv', 'entry')
        self.feat = {}
        self.blocks = []
        self.proto = {}

    def load_decomp(self):
        """raw.c 를 읽어 함수 블록과 함수 정의(호출 규약 표기, 인자 수)를 준비한다."""
        raw = self.dir / 'raw.c'
        if not raw.exists():
            return
        self.blocks = split_blocks(raw.read_text(encoding='utf-8'))
        # 블록마다 함수 정의를 해석한다
        for entry, block in self.blocks:
            self.proto[entry] = parse_proto(block)

    def is_data(self, addr):
        """주소가 실행 불가능한 구역(.rdata/.data 등) 안인지 확인한다."""
        return any(start <= addr < end and not execable for start, end, _, execable in self.sections)

    def cstring(self, addr):
        """데이터 구역의 주소에서 널로 끝나는 ASCII 문자열을 읽는다. 문자열이 아니면 None."""
        if not self.is_data(addr):
            return None
        off = addr - self.base
        end = self.img.find(b'\0', off, off + 256)
        if end < 0 or end - off < MIN_STRING_LEN:
            return None
        raw = self.img[off:end]
        if all(32 <= ch < 127 or ch in (9, 10, 13) for ch in raw):
            return raw.decode('latin1')
        return None

    def extract_features(self):
        """모든 함수의 특징(문자열·API·상수·호출 순서·니모닉 분포)을 뽑는다."""
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        entry_set = set(self.funcs)
        # 함수마다 몸체 범위를 순서대로 역어셈블한다
        for entry, row in self.funcs.items():
            strings, apis, consts = set(), set(), set()
            calls = []
            hist = collections.Counter()
            count = 0
            # 몸체의 주소 범위("시작-끝;시작-끝")를 하나씩 처리한다
            for part in row['ranges'].split(';'):
                lo, hi = (int(x, 16) for x in part.split('-'))
                data = self.img[lo - self.base:hi + 1 - self.base]
                # 범위 안의 명령어를 차례로 읽는다
                for _, _, mnem, ops in md.disasm_lite(data, lo):
                    count += 1
                    hist[mnem] += 1
                    # 피연산자의 16진수 값을 문자열·API·상수로 분류한다
                    for match in HEX_RE.finditer(ops):
                        value = int(match.group(1), 16)
                        if value in self.imports:
                            apis.add(self.imports[value])
                        elif self.base <= value < self.image_end:
                            if mnem == 'call' and value in entry_set:
                                calls.append(value)
                                continue
                            text = self.cstring(value)
                            if text:
                                strings.add(text)
                        elif value >= MIN_CONST and len(consts) < MAX_CONSTS:
                            consts.add(value)
            self.feat[entry] = {
                'strings': strings, 'apis': apis, 'consts': consts, 'calls': calls,
                'hist': hist, 'count': count,
                'norm': math.sqrt(sum(v * v for v in hist.values())) or 1.0,
            }

    def anonymous(self, entry):
        """Ghidra 가 의미 있는 이름을 붙이지 못한 함수인지 확인한다."""
        return self.funcs[entry]['name'].startswith(ANON_PREFIXES)


def jaccard(a, b):
    """두 집합의 자카드 유사도. 둘 다 비어 있으면 None(판단 불가)."""
    if not a and not b:
        return None
    return len(a & b) / len(a | b)


def similarity(fa, fb):
    """두 함수 특징의 유사도(0~1). 쓸 수 있는 성분만 가중 평균한다."""
    # 니모닉 분포의 코사인 유사도
    dot = sum(v * fb['hist'].get(k, 0) for k, v in fa['hist'].items())
    parts = [(dot / (fa['norm'] * fb['norm']), 3.0)]
    # 명령어 수 비율
    big = max(fa['count'], fb['count']) or 1
    parts.append((min(fa['count'], fb['count']) / big, 2.0))
    # 호출 횟수 비율
    calls_big = max(len(fa['calls']), len(fb['calls']))
    if calls_big:
        parts.append((min(len(fa['calls']), len(fb['calls'])) / calls_big, 1.5))
    # 집합 성분 (문자열·API·상수): 둘 다 비어 있으면 성분에서 뺀다
    for key, weight in (('strings', 3.0), ('apis', 2.0), ('consts', 2.0)):
        value = jaccard(fa[key], fb[key])
        if value is not None:
            parts.append((value, weight))
    total = sum(weight for _, weight in parts)
    return sum(value * weight for value, weight in parts) / total


def call_maps(ed):
    """판본의 호출 관계를 (함수 → 피호출 함수 집합, 함수 → 호출자 집합)으로 돌려준다."""
    callees = {}
    callers = collections.defaultdict(set)
    # 함수마다 직접 호출 대상을 집합으로 만들고 역방향 색인을 채운다
    for entry, feat in ed.feat.items():
        callees[entry] = set(feat['calls'])
        for callee in callees[entry]:
            callers[callee].add(entry)
    return callees, callers


def between(sorted_entries, lo, hi):
    """정렬된 주소 목록에서 lo 초과 hi 미만인 것들을 돌려준다."""
    return sorted_entries[bisect.bisect_right(sorted_entries, lo):bisect.bisect_left(sorted_entries, hi)]


class Matcher:
    """두 판본의 함수를 짝짓는다. a 쪽이 패치판, b 쪽이 CD판이다."""

    def __init__(self, a, b, use_reviewed=True):
        self.a, self.b = a, b
        self.use_reviewed = use_reviewed
        # 확정한 대응: a 주소 → (b 주소, 방법, 점수), 역방향 색인
        self.ab = {}
        self.ba = {}
        # 문맥 검증에서 취소한 쌍 (다시 확정하지 않는다)
        self.banned = set()
        # 판본별 호출 관계: 함수 → 피호출 함수 집합, 함수 → 호출자 집합
        self.callees_a, self.callers_a = call_maps(a)
        self.callees_b, self.callers_b = call_maps(b)

    def apply_reviewed(self):
        """기계어 차등 검증을 통과한 수동 대응을 우선하고, 확인된 다른 오버로드 대응은 금지한다."""
        manifest_path = ROOT / 'cpppj/recovery-manifest.json'
        evidence_path = ROOT / 'cpppj/recovery-evidence.json'
        if not self.use_reviewed or not manifest_path.exists() or not evidence_path.exists():
            return 0
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
        evidence = json.loads(evidence_path.read_text(encoding='utf-8'))
        if hashlib.sha256(manifest_path.read_bytes()).hexdigest() != evidence.get('manifest_sha256'):
            raise ValueError('검토 대응 목록이 검증 뒤 변경되었습니다. decomp_oracle.py를 다시 실행하세요.')
        # 다른 바이너리에 이전 주소의 수동 원형을 적용하지 않는다.
        for key, expected in evidence['binary_sha256'].items():
            actual = hashlib.sha256(EDITIONS[key]['exe'].read_bytes()).hexdigest()
            if actual != expected:
                raise ValueError('검토 대응의 원본 SHA-256이 다릅니다. decomp_oracle.py로 다시 검증하세요.')
        # 직접 확인한 오대응은 자동 전파에서도 다시 선택하지 않게 한다.
        for pair in manifest['rejected_pairs']:
            self.banned.add((int(pair['originals'], 16), int(pair['originalCD'], 16)))
        added = 0
        # 함수 존재와 양쪽 ret N까지 확인한 대응만 강한 앵커로 사용한다.
        for pair in manifest['reviewed_pairs']:
            x, y = int(pair['originals'], 16), int(pair['originalCD'], 16)
            if x not in self.a.funcs or y not in self.b.funcs:
                raise ValueError(f'검토된 함수가 정밀 결과에 없습니다: {pair["name"]}')
            if any(int(ed.conv[address]['purge']) != pair['purge'] for ed, address in ((self.a, x), (self.b, y))):
                raise ValueError(f'검토된 ret N과 덤프가 다릅니다: {pair["name"]}')
            added += self.accept(x, y, 'reviewed', 1.0)
        # 다리 검증은 별도 기대값으로 확인했다. 수명 함수의 부분 실행은 강한 앵커에서 제외되어 있다.
        bridge_path = ROOT / 'cpppj/recovery-bridge-evidence.json'
        if bridge_path.exists():
            bridge = json.loads(bridge_path.read_text(encoding='utf-8'))
            fixture = ROOT / 'cpppj/tests/fixtures/bridge-x86.tsv'
            generator = ROOT / 'tools/decomp_bridge_oracle.py'
            if hashlib.sha256(fixture.read_bytes()).hexdigest() != bridge['fixture_sha256'] or \
               hashlib.sha256(generator.read_bytes()).hexdigest() != bridge['generator_sha256']:
                raise ValueError('다리 검증 도구/기대값이 변경되었습니다. decomp_bridge_oracle.py로 다시 검증하세요.')
            # 원본 판본 해시가 다르면 주소를 적용하지 않는다.
            for key, expected in bridge['binary_sha256'].items():
                if hashlib.sha256(EDITIONS[key]['exe'].read_bytes()).hexdigest() != expected:
                    raise ValueError('다리 검토 대응의 원본 SHA-256이 다릅니다.')
            # 열린 끝 판단 전체와 한 방향 보조 검사의 오대응을 다시 전파하지 않는다.
            for pair in bridge['rejected_pairs']:
                self.banned.add((int(pair['originals'],16), int(pair['originalCD'],16)))
            # 정상 반환과 ret N까지 실행한 함수만 수동 앵커로 적용한다.
            for pair in bridge['reviewed_pairs']:
                x, y = int(pair['originals'],16), int(pair['originalCD'],16)
                if x not in self.a.funcs or y not in self.b.funcs or any(
                    int(ed.conv[address]['purge']) != pair['purge'] for ed,address in ((self.a,x),(self.b,y))):
                    raise ValueError(f'다리 검토 함수의 존재/ret N 불일치: {pair["name"]}')
                added += self.accept(x,y,'reviewed',1.0)
        # 표면 이웃/재귀 검증도 실제 입력·도구·판본이 그대로일 때만 강한 대응으로 적용한다.
        surface_path = ROOT / 'cpppj/recovery-surface-evidence.json'
        if surface_path.exists():
            surface = json.loads(surface_path.read_text(encoding='utf-8'))
            fixture = ROOT / 'cpppj/tests/fixtures/surface-x86.tsv'
            if hashlib.sha256(fixture.read_bytes()).hexdigest() != surface['fixture_sha256']:
                raise ValueError('표면 기대값이 변경되었습니다. decomp_surface_oracle.py로 다시 검증하세요.')
            # 다른 버전의 실행 도구 결과를 같은 근거로 적용하지 않는다.
            for name, expected in surface['source_sha256'].items():
                if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
                    raise ValueError('표면 검증 도구가 변경되었습니다. decomp_surface_oracle.py로 다시 검증하세요.')
            # 주소가 같아도 바이너리 내용이 다른 판본은 거부한다.
            for key, expected in surface['binary_sha256'].items():
                if hashlib.sha256(EDITIONS[key]['exe'].read_bytes()).hexdigest() != expected:
                    raise ValueError('표면 검토 대응의 원본 SHA-256이 다릅니다.')
            # 다음 함수와 생성자의 오대응을 다시 전파하지 않는다.
            for pair in surface['rejected_pairs']:
                self.banned.add((int(pair['originals'],16), int(pair['originalCD'],16)))
            # 실행 범위와 두 판본 ret N을 확인한 함수만 검토 앵커가 된다.
            for pair in surface['reviewed_pairs']:
                x, y = int(pair['originals'],16), int(pair['originalCD'],16)
                if x not in self.a.funcs or y not in self.b.funcs or any(
                    int(ed.conv[address]['purge']) != pair['purge'] for ed,address in ((self.a,x),(self.b,y))):
                    raise ValueError(f'표면 검토 함수의 존재/ret N 불일치: {pair["name"]}')
                added += self.accept(x,y,'reviewed',1.0)
        # 공간 해시 앵커는 x87 상태까지 명시한 정상 반환 검증 결과만 사용한다.
        hash_path = ROOT / 'cpppj/recovery-hash-evidence.json'
        if hash_path.exists():
            evidence = json.loads(hash_path.read_text(encoding='utf-8'))
            if evidence['x87_checked_control_words'] != ['0x27f','0x37f'] or any(evidence['assert_reports'].values()):
                raise ValueError('공간 해시의 x87 상태/assert 검증이 다릅니다.')
            fixture = ROOT / 'cpppj/tests/fixtures/hash-x86.tsv'
            if hashlib.sha256(fixture.read_bytes()).hexdigest() != evidence['fixture_sha256']:
                raise ValueError('공간 해시 기대값이 변경되었습니다. decomp_hash_oracle.py로 다시 검증하세요.')
            # 기대값을 실행한 생성/공통 도구가 바뀌면 검토 대응을 적용하지 않는다.
            for name, expected in evidence['source_sha256'].items():
                if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
                    raise ValueError('공간 해시 검증 도구가 변경되었습니다. 다시 검증하세요.')
            # 다른 내용의 EXE에는 같은 주소를 근거로 적용하지 않는다.
            for key, expected in evidence['binary_sha256'].items():
                if hashlib.sha256(EDITIONS[key]['exe'].read_bytes()).hexdigest() != expected:
                    raise ValueError('공간 해시 검토 대응의 원본 SHA-256이 다릅니다.')
            # Init은 malloc 경로를 제외했으므로 목록에 넣지 않는다. 정상 함수 5개만 확인한다.
            for pair in evidence['reviewed_pairs']:
                x, y = int(pair['originals'],16), int(pair['originalCD'],16)
                if x not in self.a.funcs or y not in self.b.funcs or any(
                    int(ed.conv[address]['purge']) != pair['purge'] for ed,address in ((self.a,x),(self.b,y))):
                    raise ValueError(f'공간 해시 함수의 존재/ret N 불일치: {pair["name"]}')
                added += self.accept(x,y,'reviewed',1.0)
        return added

    def accept(self, x, y, method, score):
        """한 쌍을 확정한다. 이미 다른 상대와 짝지어졌거나 금지된 쌍이면 무시하고 False."""
        if x in self.ab or y in self.ba or (x, y) in self.banned:
            return False
        self.ab[x] = (y, method, score)
        self.ba[y] = x
        return True

    def drop(self, x):
        """확정했던 쌍을 취소하고 금지 목록에 올린다."""
        y = self.ab.pop(x)[0]
        del self.ba[y]
        self.banned.add((x, y))

    def match_names(self):
        """Ghidra 가 라이브러리 함수로 식별한 같은 이름끼리 짝짓는다 (양쪽에서 유일한 이름만)."""
        def unique_names(ed):
            groups = collections.defaultdict(list)
            # 이름이 붙은 함수를 이름별로 모은다
            for entry in ed.funcs:
                if not ed.anonymous(entry):
                    groups[ed.funcs[entry]['name']].append(entry)
            return {name: v[0] for name, v in groups.items() if len(v) == 1}
        names_a, names_b = unique_names(self.a), unique_names(self.b)
        # 양쪽에 다 있는 이름을 확정한다
        for name in sorted(set(names_a) & set(names_b)):
            self.accept(names_a[name], names_b[name], 'name', 1.0)

    def match_unique_strings(self, hide=None):
        """양쪽에서 각각 한 함수만 참조하는 문자열로 짝짓는다 (서로가 서로의 1위일 때만).

        hide 가 주어지면 그 함수(패치판 주소)에 대한 표는 버린다 (홀드아웃 검증에서 정답을 숨길 때 쓴다)."""
        def owners(ed):
            groups = collections.defaultdict(set)
            # 문자열마다 그것을 참조하는 함수를 모은다
            for entry, feat in ed.feat.items():
                for text in feat['strings']:
                    groups[text].add(entry)
            return {text: next(iter(v)) for text, v in groups.items() if len(v) == 1}
        own_a, own_b = owners(self.a), owners(self.b)
        votes = collections.Counter()
        # 양쪽에서 유일한 문자열마다 그 주인 쌍에 한 표
        for text in set(own_a) & set(own_b):
            if hide and own_a[text] in hide:
                continue
            votes[(own_a[text], own_b[text])] += 1
        self._accept_mutual_best(votes, 'string', min_sim=0.0)

    def _accept_mutual_best(self, votes, method, min_sim):
        """표를 가장 많이 받은 상대가 서로 일치하고 동률이 없으면 확정한다. 확정한 쌍 수를 돌려준다."""
        best_a = collections.defaultdict(list)
        best_b = collections.defaultdict(list)
        # 함수마다 후보를 표 수로 모은다
        for (x, y), n in votes.items():
            if x in self.ab or y in self.ba or (x, y) in self.banned:
                continue
            best_a[x].append((n, y))
            best_b[y].append((n, x))
        accepted = 0
        # a 쪽 함수마다 1위 후보를 확인한다
        for x, cands in sorted(best_a.items()):
            cands.sort(reverse=True)
            if len(cands) > 1 and cands[0][0] == cands[1][0]:
                continue
            n, y = cands[0]
            back = sorted(best_b[y], reverse=True)
            if back[0][1] != x or (len(back) > 1 and back[0][0] == back[1][0]):
                continue
            sim = similarity(self.a.feat[x], self.b.feat[y])
            if sim < min_sim:
                continue
            if self.accept(x, y, method, round(sim, 3)):
                accepted += 1
        return accepted

    def context(self, x, y):
        """한 쌍의 호출 그래프 문맥 일치도를 계산한다.

        짝지어진 피호출 함수·호출자를 공통 번호(CD판 주소)로 바꿔 두 함수의 이웃 집합이 얼마나 겹치는지 본다.
        돌려주는 값: (겹치는 이웃 수, 이웃 합집합 크기). 합집합이 0 이면 판단할 근거가 없다."""
        shared = total = 0
        # 피호출 함수와 호출자를 각각 비교한다
        for near_a, near_b in ((self.callees_a.get(x, ()), self.callees_b.get(y, ())),
                               (self.callers_a.get(x, ()), self.callers_b.get(y, ()))):
            mapped_a = {self.ab[n][0] for n in near_a if n in self.ab and n != x}
            mapped_b = {n for n in near_b if n in self.ba and n != y}
            shared += len(mapped_a & mapped_b)
            total += len(mapped_a | mapped_b)
        return shared, total

    def propagate_calls(self):
        """이미 짝지은 함수 쌍의 호출 순서를 맞대어, 같은 자리에 있는 미확정 피호출 함수끼리 표를 준다."""
        votes = collections.Counter()
        # 확정된 쌍마다 양쪽의 호출 순서를 정렬한다
        for x, (y, _, _) in self.ab.items():
            calls_a = self.a.feat[x]['calls']
            calls_b = self.b.feat[y]['calls']
            if not calls_a or not calls_b:
                continue
            # 확정된 피호출 함수는 쌍 번호로, 미확정은 'U' 로 바꿔 순서를 맞춘다
            tok_a = [('M', self.ab[c][0]) if c in self.ab else ('U', 0) for c in calls_a]
            tok_b = [('M', c) if c in self.ba else ('U', 0) for c in calls_b]
            sm = difflib.SequenceMatcher(None, tok_a, tok_b, autojunk=False)
            # 일치 구간에서 양쪽이 모두 미확정인 자리를 후보 쌍으로 삼는다
            for i, j, size in sm.get_matching_blocks():
                for k in range(size):
                    if tok_a[i + k][0] == 'U':
                        votes[(calls_a[i + k], calls_b[j + k])] += 1
        return self._accept_mutual_best(votes, 'callseq', PROPAGATE_MIN_SIM)

    def propagate_callers(self):
        """호출자가 짝지어진 미확정 함수끼리 표를 준다 (호출자 쌍을 공유할수록 표가 많다)."""
        # CD판의 미확정 함수를 "짝지어진 호출자" 기준으로 색인한다
        index_b = collections.defaultdict(set)
        for y, callers in self.callers_b.items():
            if y in self.ba:
                continue
            for caller in callers:
                if caller in self.ba:
                    index_b[caller].add(y)
        votes = collections.Counter()
        # 패치판의 미확정 함수마다 호출자의 상대가 부르는 미확정 함수에 표를 준다
        for x, callers in self.callers_a.items():
            if x in self.ab:
                continue
            for caller in callers:
                if caller in self.ab:
                    for y in index_b.get(self.ab[caller][0], ()):
                        votes[(x, y)] += 1
        # 표가 같은 후보가 여럿이면 유사도로 가른다
        scored = collections.Counter()
        for (x, y), n in votes.items():
            sim = similarity(self.a.feat[x], self.b.feat[y])
            if sim >= PROPAGATE_MIN_SIM:
                scored[(x, y)] = n + sim
        return self._accept_mutual_best(scored, 'callers', PROPAGATE_MIN_SIM)

    def local_gaps(self):
        """주소 순으로 이웃한 두 확정 쌍이 상대 판본에서도 가까이 순서대로 놓인 구간을 찾는다.

        오브젝트 파일의 링크 순서는 판본마다 다르지만 한 오브젝트 파일 안의 함수 순서는 유지된다.
        돌려주는 값: [(패치판 미확정 함수 목록, CD판 미확정 함수 목록)]"""
        anchors = sorted((x, y) for x, (y, _, _) in self.ab.items())
        gaps = []
        # 이웃한 확정 쌍 사이의 구간을 하나씩 본다
        for (xa, ya), (xb, yb) in zip(anchors, anchors[1:]):
            if yb <= ya:
                continue
            mid_a = between(self.a.entries, xa, xb)
            mid_b = between(self.b.entries, ya, yb)
            if not mid_a or not mid_b or len(mid_a) > MAX_GAP_FUNCS or len(mid_b) > MAX_GAP_FUNCS:
                continue
            # 구간 안에 다른 곳과 짝지어진 함수가 끼어 있으면 같은 모듈 구간이 아니다
            if any(e in self.ab for e in mid_a) or any(e in self.ba for e in mid_b):
                continue
            gaps.append((mid_a, mid_b))
        return gaps

    def match_gaps(self):
        """국소 구간 안의 미확정 함수를 짝짓는다.

        양쪽 개수가 같으면 순서대로(gapfill), 다르면 유사도가 높고 서로 1위일 때만(neighbor) 확정한다."""
        accepted = 0
        votes = collections.Counter()
        # 국소 구간마다 양쪽 미확정 함수를 비교한다
        for mid_a, mid_b in self.local_gaps():
            if len(mid_a) == len(mid_b):
                sims = [similarity(self.a.feat[x], self.b.feat[y]) for x, y in zip(mid_a, mid_b)]
                # 순서대로 짝지었을 때 모두 최소 유사도를 넘어야 구간 전체를 확정한다
                if min(sims) >= GAPFILL_MIN_SIM:
                    for x, y, sim in zip(mid_a, mid_b, sims):
                        if self.accept(x, y, 'gapfill', round(sim, 3)):
                            accepted += 1
                    continue
            # 개수가 다르거나 순서 짝이 안 맞으면 유사도 1위끼리만 후보로 삼는다
            for x in mid_a:
                sims = sorted(((similarity(self.a.feat[x], self.b.feat[y]), y) for y in mid_b), reverse=True)
                margin = sims[0][0] - (sims[1][0] if len(sims) > 1 else 0.0)
                if sims[0][0] >= NEIGHBOR_MIN_SIM and (margin >= NEIGHBOR_MIN_MARGIN or len(mid_b) == 1):
                    votes[(x, sims[0][1])] = sims[0][0]
        return accepted + self._accept_mutual_best(votes, 'neighbor', NEIGHBOR_MIN_SIM)

    def prune(self):
        """문맥이 맞지 않는 쌍을 취소한다. 이름·문자열 근거가 있는 쌍은 건드리지 않는다."""
        doomed = []
        # 전파·이웃으로 확정한 쌍마다 문맥 일치도를 다시 계산한다
        for x, (y, method, _) in self.ab.items():
            if method in ('name', 'string', 'reviewed'):
                continue
            shared, total = self.context(x, y)
            if total >= PRUNE_MIN_CONTEXT and shared / total < PRUNE_MAX_RATIO:
                doomed.append(x)
        # 문맥이 맞지 않는 쌍을 한꺼번에 취소한다
        for x in doomed:
            self.drop(x)
        return len(doomed)

    def run(self, log=print, hide=None):
        """대응 단계를 근거가 강한 순서로 실행한다. hide 는 문자열 앵커에서 숨길 패치판 함수 집합이다."""
        reviewed = self.apply_reviewed()
        if reviewed:
            log(f'  기계어 검토 대응: {reviewed}쌍')
        self.match_names()
        log(f'  이름 일치: {len(self.ab)}쌍')
        before = len(self.ab)
        self.match_unique_strings(hide)
        log(f'  유일 문자열: +{len(self.ab) - before}쌍')
        # 전파 → 국소 구간 → 검증(취소)을 더 늘지 않을 때까지 반복한다
        for round_no in range(1, MAX_MATCH_ROUNDS + 1):
            added = 0
            # 호출 순서·호출자 전파를 수렴할 때까지 돈다
            while True:
                step = self.propagate_calls() + self.propagate_callers()
                added += step
                if step == 0:
                    break
            gap_added = self.match_gaps()
            pruned = self.prune()
            log(f'  {round_no}회차: 전파 +{added}쌍, 국소 구간 +{gap_added}쌍, 문맥 불일치 취소 -{pruned}쌍 '
                f'(누계 {len(self.ab)})')
            if added + gap_added == 0 or (added + gap_added) <= pruned:
                break

    def confidence(self, x):
        """확정한 쌍의 신뢰 수준을 정한다: strong / medium / weak."""
        y, method, _ = self.ab[x]
        if method in ('name', 'string', 'reviewed'):
            return 'strong'
        shared, total = self.context(x, y)
        if shared >= 3 and shared / total >= 0.6:
            return 'strong'
        if shared >= 1 and shared / total >= 0.5:
            return 'medium'
        if method == 'gapfill':
            return 'medium'
        return 'weak'

    def local_order(self):
        """주소 순으로 이웃한 확정 쌍 가운데 CD판에서도 순서가 유지되는 비율을 돌려준다 (모듈 내 순서 검증)."""
        anchors = sorted((x, y) for x, (y, _, _) in self.ab.items())
        kept = sum(1 for (_, ya), (_, yb) in zip(anchors, anchors[1:]) if yb > ya)
        return kept, max(1, len(anchors) - 1)


def holdout_check(a, b, log=print):
    """전파의 정밀도를 잰다: 문자열 앵커의 절반을 숨기고 대응을 돌려, 숨긴 함수가 정답과 같은 상대를 찾는지 본다."""
    # 수동 정답을 평가에 섞지 않아 기존 자동 매칭 정밀도와 직접 비교할 수 있게 한다.
    truth = Matcher(a, b, use_reviewed=False)
    truth.match_names()
    truth.match_unique_strings()
    answers = sorted((x, y) for x, (y, method, _) in truth.ab.items() if method == 'string')
    # 주소 순으로 하나 걸러 하나씩 숨긴다 (난수를 쓰지 않아 결과가 재현된다)
    hidden = {x: y for index, (x, y) in enumerate(answers) if index % 2 == 0}
    trial = Matcher(a, b, use_reviewed=False)
    trial.run(log=lambda *_: None, hide=set(hidden))
    found = right = 0
    by_level = collections.Counter()
    # 숨긴 함수마다 전파가 찾아낸 상대를 정답과 비교한다
    for x, y in hidden.items():
        if x not in trial.ab:
            continue
        found += 1
        level = trial.confidence(x)
        if trial.ab[x][0] == y:
            right += 1
            by_level[(level, 'ok')] += 1
        else:
            by_level[(level, 'wrong')] += 1
    log(f'  홀드아웃: 숨긴 앵커 {len(hidden)}개 중 {found}개를 다시 찾음, 그중 정답 {right}개 '
        f'(정밀도 {right / max(1, found):.1%}, 재현율 {right / max(1, len(hidden)):.1%})')
    # 신뢰 수준별 정답·오답 수
    for level in ('strong', 'medium', 'weak'):
        ok, wrong = by_level[(level, 'ok')], by_level[(level, 'wrong')]
        if ok + wrong:
            log(f'    {level}: 정답 {ok}, 오답 {wrong} (정밀도 {ok / (ok + wrong):.1%})')


def convention_check(a, b, pairs, levels, log=print):
    """짝지어진 함수의 호출 규약 근거를 서로 대조한다.

    같은 소스에서 나온 함수는 두 판본에서 호출 규약과 ret N 이 같아야 한다. 일치율은 규약 판정과 함수 대응의
    독립된 검증이 된다. 돌려주는 값: (패치판 힌트, CD판 힌트) — 근거가 부족한 쪽을 상대의 양쪽 근거로 보완한다."""
    hints_a, hints_b = {}, {}
    both = same = purge_both = purge_same = 0
    conflicts = collections.Counter()
    # 신뢰할 수 있는 쌍만 대조한다
    for x, y, _, _ in pairs:
        if levels[x] == 'weak' or x not in a.conv or y not in b.conv:
            continue
        ra, rb = a.conv[x], b.conv[y]
        # 규약을 지정하지 못한 쪽(unknown)이 있으면 일치율 계산에서 뺀다
        if 'unknown' not in (ra['decision'], rb['decision']):
            both += 1
            if ra['decision'] == rb['decision']:
                same += 1
            else:
                conflicts[(ra['decision'], rb['decision'])] += 1
        # ret N 의 N 은 양쪽 다 알려진 경우에만 비교한다
        if ra['purge'] != '-1' and rb['purge'] != '-1':
            purge_both += 1
            purge_same += ra['purge'] == rb['purge']
        # 한쪽은 양쪽 근거로 __thiscall, 다른 쪽은 ECX 를 먼저 읽지만 호출 쪽 근거가 없어 기본값인 경우 보완한다
        for strong, weak, key, hints in ((ra, rb, y, hints_b), (rb, ra, x, hints_a)):
            if strong['decision'] != '__thiscall' or strong['basis'] not in TWO_SIDED_BASES:
                continue
            if weak['basis'] not in FALLBACK_BASES or weak['ecx_in'] != '1':
                continue
            sites, ecx_set = int(weak['call_sites']), int(weak['ecx_set'])
            # 호출 지점이 있는데 절반도 ECX 를 설정하지 않으면 상충하는 근거이므로 보완하지 않는다
            if sites == 0 or ecx_set * 2 >= sites:
                hints[key] = '__thiscall'
    if both:
        log(f'  호출 규약 일치: {same}/{both} ({same / both:.1%}), '
            f'ret N 일치: {purge_same}/{purge_both} ({purge_same / max(1, purge_both):.1%})')
        log(f'  규약 불일치 상위: {conflicts.most_common(6)}')
        log(f'  상대 판본 근거로 보완할 수 있는 함수: {a.label} {len(hints_a)}개, {b.label} {len(hints_b)}개')
    return hints_a, hints_b


def find_vtables(ed):
    """데이터 구역에서 함수 진입 주소가 연달아 놓인 줄을 함수 포인터 표(가상 함수 표·콜백 표) 후보로 모은다.

    돌려주는 값: [(표 주소, [슬롯별 함수 주소])], 그리고 함수 주소 → [(표 주소, 슬롯 번호)]"""
    entry_set = set(ed.funcs)
    tables = []
    slots_of = collections.defaultdict(list)
    # 실행 불가능한 구역을 4바이트 단위로 훑는다
    for start, end, _, execable in ed.sections:
        if execable:
            continue
        addr = start
        limit = min(end, ed.image_end) - 3
        while addr < limit:
            run = []
            # 함수 진입 주소가 이어지는 동안 한 표로 묶는다
            while addr + 4 * len(run) < limit:
                off = addr + 4 * len(run) - ed.base
                value = int.from_bytes(ed.img[off:off + 4], 'little')
                if value not in entry_set:
                    break
                run.append(value)
            if run:
                tables.append((addr, run))
                for slot, target in enumerate(run):
                    slots_of[target].append((addr, slot))
                addr += 4 * len(run)
            else:
                addr += 4
    return tables, slots_of


def grade_edition(ed, other, pairs, slots_of):
    """함수마다 근거를 모아 등급을 매긴다. 돌려주는 값: 주소 → 근거 사전.

    A: 호출자·포인터 근거가 있고 상대 판본에 신뢰할 수 있는 짝이 있으며 ret N 이 어긋나지 않는다.
    B: 호출자·포인터 근거 또는 신뢰할 수 있는 짝 가운데 하나가 있다.
    C: 그런 근거는 없지만 자동 분석이 찾았거나 패딩 뒤 프롤로그로 복구한 함수다.
    D: 디컴파일이 깨졌거나, 근거 없이 함수 끝·ret 직후에서 복구했거나, 조각 징후가 있는데 근거가 없다."""
    result = {}
    body_of = dict(ed.blocks)
    # 함수마다 존재 근거와 의심 징후를 계산한다
    for entry, row in ed.funcs.items():
        body = body_of.get(entry, '')
        callers = int(row['call_refs'])
        pointers = int(row['data_refs'])
        partner = pairs.get(entry)
        trusted = partner if partner and not partner[1].endswith('/weak') else None
        source = ed.recovered.get(entry, 'auto')
        # 예외 처리 funclet 은 부모 함수의 EBP 를 그대로 쓰는 것이 정상이므로 조각 징후로 보지 않는다
        funclet = row['name'].startswith(FUNCLET_PREFIXES)
        fragment = bool(FRAGMENT_RE.search(body)) and not funclet
        unsure = len(set(UNSURE_SIG_RE.findall(body)))
        broken = bool(BROKEN_RE.search(body)) or not body
        conv_row = ed.conv.get(entry)
        # 상대 판본의 짝과 호출 규약, ret N 이 같은지 본다
        conv_agree = purge_agree = ''
        if partner and conv_row and partner[0] in other.conv:
            other_row = other.conv[partner[0]]
            if 'unknown' not in (conv_row['decision'], other_row['decision']):
                conv_agree = '1' if conv_row['decision'] == other_row['decision'] else '0'
            if conv_row['purge'] != '-1' and other_row['purge'] != '-1':
                purge_agree = '1' if conv_row['purge'] == other_row['purge'] else '0'
        evidence = bool(callers or pointers or row['thunk'] == '1')
        # 호출·포인터·패딩 뒤 프롤로그 근거 없이 함수 끝이나 ret 직후에서만 복구한 함수인지
        heuristic_only = source != 'auto' and not ({'pointer', 'call', 'prologue'} & set(source.split('+')))
        if broken:
            grade = 'D'
        elif heuristic_only and not evidence and not trusted:
            grade = 'D'
        elif fragment and not evidence:
            grade = 'D'
        elif evidence and trusted and not fragment and purge_agree != '0':
            grade = 'A'
        elif evidence or trusted:
            grade = 'B'
        else:
            grade = 'C'
        result[entry] = {
            'grade': grade, 'callers': callers, 'pointers': pointers, 'jump_in': int(row['jump_refs']),
            'source': source, 'partner': partner, 'conv': conv_row, 'conv_agree': conv_agree,
            'purge_agree': purge_agree, 'fragment': fragment, 'unsure': unsure, 'broken': broken,
            'funclet': funclet, 'vtables': slots_of.get(entry, []),
        }
    return result


def annotate(ed, other, grades, other_grades):
    """raw.c 의 함수 블록마다 머리줄 아래에 등급·근거 주석을 넣은 최종 C 파일을 쓴다."""
    out_path = ed.dir / EDITIONS[ed.key]['out']
    with open(out_path, 'w', encoding='utf-8', newline='') as fp:
        # 주소 순으로 함수 블록을 기록한다
        for entry, block in sorted(ed.blocks):
            info = grades.get(entry)
            head, _, rest = block.partition('\n')
            fp.write(head.rstrip('\r') + '\n')
            if info:
                notes = [f'[신뢰도 {info["grade"]}]', f'호출자 {info["callers"]}', f'포인터 {info["pointers"]}']
                if info['source'] != 'auto':
                    notes.append(f'복구 출처 {info["source"]}')
                if info['partner']:
                    partner, method, score = info['partner']
                    notes.append(f'{other.label} {other.funcs[partner]["name"]}@{partner:08x} ({method} {score})')
                else:
                    notes.append(f'{other.label} 대응 없음')
                if info['funclet']:
                    notes.append('예외 처리 funclet(부모 함수의 EBP 사용)')
                if info['fragment']:
                    notes.append('조각 의심(in_stack/unaff_EBP)')
                if info['unsure']:
                    notes.append(f'미추정 레지스터 {info["unsure"]}종')
                # 상대 판본의 짝이 더 깨끗하게 디컴파일됐으면 그쪽을 참고하라고 알린다
                mate = other_grades.get(info['partner'][0]) if info['partner'] else None
                if mate and not mate['broken'] and (mate['unsure'] < info['unsure']
                                                    or (info['fragment'] and not mate['fragment'])):
                    notes.append(f'{other.label} 쪽이 더 깨끗함(미추정 레지스터 {mate["unsure"]}종)')
                fp.write('// ' + ' · '.join(notes) + '\n')
                conv = info['conv']
                if conv:
                    # 호출 규약의 근거와 상대 판본과의 일치 여부
                    text = (f'// 호출 규약 {conv["decision"]} (근거 {conv["basis"]}: 호출 지점 {conv["call_sites"]}곳 중 '
                            f'{conv["ecx_set"]}곳 ECX 설정, ECX 먼저 읽음 {conv["ecx_in"]}, ret {conv["purge"]})')
                    if info['conv_agree'] == '0':
                        text += f' · {other.label}과 규약 다름'
                    if info['purge_agree'] == '0':
                        text += f' · {other.label}과 ret N 다름'
                    fp.write(text + '\n')
                # 함수 포인터 표에 실린 함수는 표 주소와 슬롯을 적는다 (간접 호출을 풀 때 쓴다)
                if info['vtables']:
                    text = ', '.join(f'{table:08x}[{slot}]' for table, slot in info['vtables'][:8])
                    fp.write(f'// 함수 포인터 표: {text}\n')
            fp.write(rest.replace('\r\n', '\n'))
    return out_path


def show_pair(addr_text, edition):
    """함수 하나와 상대 판본의 짝을 차례로 출력한다 (한쪽이 모호할 때 다른 쪽을 바로 대조하기 위한 것).

    주소는 함수 진입 주소가 아니어도 된다. 그 주소보다 앞에서 시작하는 가장 가까운 함수를 고른다."""
    addr = int(addr_text, 16)
    other_key = 'originalCD' if edition == 'originals' else 'originals'
    blocks = {}
    # 두 판본의 최종 C 파일을 함수 블록으로 나눈다
    for key in (edition, other_key):
        path = REFINED / key / EDITIONS[key]['out']
        if not path.exists():
            sys.exit(f'{path} 가 없습니다. 먼저 tools/ghidra/refine_all.ps1 을 실행하세요.')
        blocks[key] = dict(split_blocks(path.read_text(encoding='utf-8')))
    entries = sorted(blocks[edition])
    index = bisect.bisect_right(entries, addr) - 1
    if index < 0:
        sys.exit(f'{addr:08x} 앞에서 시작하는 함수가 없습니다.')
    entry = entries[index]
    # 대응표에서 짝을 찾는다
    partner = None
    with open(REFINED / 'match.tsv', encoding='utf-8', newline='') as fp:
        for row in csv.DictReader(fp, delimiter='\t'):
            if int(row[edition], 16) == entry:
                partner = int(row[other_key], 16)
                print(f'대응: {row["method"]} / {row["level"]} (유사도 {row["score"]}, '
                      f'문맥 {row["ctx_shared"]}/{row["ctx_total"]})')
                break
    print(f'===== {EDITIONS[edition]["label"]} {entry:08x} =====')
    print(blocks[edition][entry].rstrip())
    if partner is None:
        print(f'===== {EDITIONS[other_key]["label"]}: 대응하는 함수 없음 =====')
        return
    print(f'===== {EDITIONS[other_key]["label"]} {partner:08x} =====')
    print(blocks[other_key].get(partner, '(디컴파일 결과에 없음)').rstrip())


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--show', metavar='주소', help='그 주소의 함수와 상대 판본의 짝을 출력하고 끝낸다 (16진수)')
    parser.add_argument('--edition', choices=list(EDITIONS), default='originals',
                        help='--show 의 주소가 속한 판본 (기본 originals)')
    parser.add_argument('--probe', action='store_true', help='대응 품질 통계(홀드아웃 검증 포함)만 출력하고 파일은 쓰지 않는다')
    parser.add_argument('--hints', action='store_true', help='대응을 계산해 판본별 hints.tsv 만 쓰고 끝낸다')
    args = parser.parse_args()
    if args.show:
        show_pair(args.show, args.edition)
        return

    # 두 판본을 읽고 특징을 뽑는다
    eds = {}
    for key in EDITIONS:
        if not (REFINED / key / 'functions.tsv').exists():
            sys.exit(f'{REFINED / key / "functions.tsv"} 가 없습니다. 먼저 tools/ghidra/refine_all.ps1 을 실행하세요.')
        eds[key] = Edition(key)
        eds[key].extract_features()
        print(f'{eds[key].label}: 함수 {len(eds[key].funcs)}개 특징 추출')
    a, b = eds['originals'], eds['originalCD']

    # 판본 간 대응
    print('판본 간 함수 대응:')
    matcher = Matcher(a, b)
    matcher.run()
    pairs = sorted((x, y, method, score) for x, (y, method, score) in matcher.ab.items())
    methods = collections.Counter(method for _, _, method, _ in pairs)
    levels = {x: matcher.confidence(x) for x, _, _, _ in pairs}
    kept, total = matcher.local_order()
    print(f'  합계 {len(pairs)}쌍 — 방법별 {dict(methods)}')
    print(f'  신뢰 수준: {dict(collections.Counter(levels.values()))}')
    print(f'  국소 순서 유지(이웃한 쌍이 CD판에서도 같은 순서): {kept}/{total} ({kept / total:.1%})')
    print(f'  대응률: {a.label} {len(pairs) / len(a.funcs):.1%}, {b.label} {len(pairs) / len(b.funcs):.1%}')
    hints_a, hints_b = convention_check(a, b, pairs, levels)
    if args.probe:
        holdout_check(a, b)
        return

    # 힌트 기록 (--hints 면 여기서 끝낸다)
    if args.hints:
        for ed, hints in ((a, hints_a), (b, hints_b)):
            with open(ed.dir / 'hints.tsv', 'w', encoding='utf-8', newline='') as fp:
                fp.write('entry\tconvention\n')
                # 주소 순으로 힌트를 적는다
                for entry in sorted(hints):
                    fp.write(f'{entry:x}\t{hints[entry]}\n')
            print(f'{ed.label}: 힌트 {len(hints)}개 → {ed.dir / "hints.tsv"}')
        return

    # 대응표 기록
    with open(REFINED / 'match.tsv', 'w', encoding='utf-8', newline='') as fp:
        fp.write('originals\toriginalCD\tmethod\tscore\tlevel\tctx_shared\tctx_total\tname_a\tname_b\t'
                 'size_a\tsize_b\tconv_a\tconv_b\tpurge_a\tpurge_b\n')
        # 확정한 쌍마다 근거와 양쪽 함수의 크기·호출 규약을 적는다
        for x, y, method, score in pairs:
            ra, rb = a.funcs[x], b.funcs[y]
            ca, cb = a.conv.get(x, {}), b.conv.get(y, {})
            shared, ctx_total = matcher.context(x, y)
            fields = [f'{x:08x}', f'{y:08x}', method, str(score), levels[x], str(shared), str(ctx_total),
                      ra['name'], rb['name'], ra['size'], rb['size'], ca.get('decision', ''),
                      cb.get('decision', ''), ca.get('purge', ''), cb.get('purge', '')]
            fp.write('\t'.join(fields) + '\n')

    # 판본마다 함수 포인터 표, 등급, 최종 C 파일을 만든다
    pair_ab = {x: (y, f'{method}/{levels[x]}', score) for x, y, method, score in pairs}
    pair_ba = {y: (x, f'{method}/{levels[x]}', score) for x, y, method, score in pairs}
    all_grades = {}
    table_counts = {}
    # 먼저 두 판본의 등급을 모두 계산한다 (머리말에서 상대 판본의 상태를 참고하기 때문)
    for ed, other, pair_map in ((a, b, pair_ab), (b, a, pair_ba)):
        ed.load_decomp()
        tables, slots_of = find_vtables(ed)
        table_counts[ed.key] = len(tables)
        with open(ed.dir / 'vtables.tsv', 'w', encoding='utf-8', newline='') as fp:
            fp.write('table\tslot\tfunction\tname\n')
            # 표마다 슬롯 순서대로 적는다
            for table, run in tables:
                for slot, target in enumerate(run):
                    fp.write(f'{table:08x}\t{slot}\t{target:08x}\t{ed.funcs[target]["name"]}\n')
        all_grades[ed.key] = grade_edition(ed, other, pair_map, slots_of)
    # 판본마다 등급표와 최종 C 파일을 쓴다
    for ed, other in ((a, b), (b, a)):
        grades = all_grades[ed.key]
        with open(ed.dir / 'grades.tsv', 'w', encoding='utf-8', newline='') as fp:
            fp.write('entry\tname\tgrade\tcallers\tpointers\tjump_in\tsource\tpartner\tmethod\tscore\t'
                     'convention\tconv_basis\tconv_agree\tpurge_agree\tparams\tfragment\tunsure_regs\tbroken\ttables\n')
            # 주소 순으로 함수별 근거를 적는다
            for entry in ed.entries:
                info = grades[entry]
                partner = info['partner'] or ('', '', '')
                conv = info['conv'] or {}
                proto = ed.proto.get(entry, ('', None))
                fields = [f'{entry:08x}', ed.funcs[entry]['name'], info['grade'], str(info['callers']),
                          str(info['pointers']), str(info['jump_in']), info['source'],
                          f'{partner[0]:08x}' if info['partner'] else '', partner[1], str(partner[2]),
                          conv.get('decision', ''), conv.get('basis', ''), info['conv_agree'], info['purge_agree'],
                          '' if proto[1] is None else str(proto[1]), str(int(info['fragment'])),
                          str(info['unsure']), str(int(info['broken'])),
                          ','.join(f'{t:08x}[{s}]' for t, s in info['vtables'][:8])]
                fp.write('\t'.join(fields) + '\n')
        out_path = annotate(ed, other, grades, all_grades[other.key])
        dist = collections.Counter(info['grade'] for info in grades.values())
        print(f'{ed.label}: 등급 {dict(sorted(dist.items()))}, 함수 포인터 표 {table_counts[ed.key]}개 → {out_path}')


if __name__ == '__main__':
    main()
