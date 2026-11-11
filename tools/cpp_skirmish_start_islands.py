# -*- coding: utf-8 -*-
"""스커미시 시작 섬 후보 표(.inc)를 만든다: 사제 1명만 있는 단일 소유자 영역 중 3x3 이하 패턴.

사용법: python tools/cpp_skirmish_start_islands.py <--dump-forts 출력 파일> <원본 d 디렉터리> <CanonDecoderTables.inc> <출력 .inc>
예: Netstorm.exe --dump-forts originals > forts.txt 뒤 python tools/cpp_skirmish_start_islands.py forts.txt originals/d cpppj/src/o/CanonDecoderTables.inc cpppj/src/client/SkirmishStartIslands.inc
12차 스커미시를 MP 규칙 월드로 바꾸면서(1300) 추가했다.
"""
import os, re, struct, sys

dump, root, tables, out = sys.argv[1:5]
dims = [(int(m.group(1)), int(m.group(2))) for m in re.finditer(r'\{1, (\d+), (\d+), \{\{', open(tables, encoding='utf-8').read())]
print('patterns', len(dims))


def sections_of(path):
    d = open(path, 'rb').read()
    pos, secs = 2, []
    while pos + 2 <= len(d) and len(secs) < 35:
        n = struct.unpack_from('<H', d, pos)[0]
        if n < 2:
            break
        secs.append(d[pos + 2:pos + n])
        pos += n
    return secs


fort = sec = None
data = {}
for ln in open(dump, encoding='utf-8', errors='replace').read().splitlines():
    if ln.startswith('FORT '):
        fort = ln.split()[1]
        data[fort] = {}
    elif ln.startswith('SEC '):
        sec = ln.split()[1]
        data[fort][sec] = []
    elif ln.startswith('O ') and fort and sec:
        data[fort][sec].append(ln)

rows = []
seen_names = set()
for fort, secs in sorted(data.items()):
    base = fort[:-5]
    # Imp1a/b/c 처럼 같은 틀을 되풀이한 파일은 한 개만 쓴다.
    if re.match(r'^Imp1[abc]$', base):
        continue
    for sec, objs in secs.items():
        if not sec.startswith('Terr'):
            continue
        live = [o for o in objs if 'noIsland' not in o]
        if len(live) != 1 or ' priest ' not in live[0]:
            continue
        index = int(sec[4:])
        s = sections_of(os.path.join(root, fort))
        if len(s) < 7 or len(s[6]) != 120:
            continue
        rec = s[6][index * 6:index * 6 + 6]
        if not (rec[1] & 1) or (rec[1] & 2):
            continue
        w, h = dims[rec[0] & 63]
        if w > 3 or h > 3 or w * h < 4:
            continue
        rows.append((base, index, w, h))
print(len(rows))
for r in rows:
    print(r)
with open(out, 'w', encoding='utf-8', newline='\n') as f:
    f.write('// 12차 스커미시 시작 섬 후보 표(자동 생성: 사제 1명만 있는 단일 소유자 영역 중 3x3 이하 패턴, 원본 d/*.fort 그대로).\n')
    f.write('// 각 항목 = {fort 이름, 영역 번호}. MP 전투섬 규칙처럼 참가자 수만큼 뽑아 소유자 1..N+1의 시작 섬으로 쓴다.\n')
    for base, index, w, h in rows:
        f.write('{"%s",%d},\n' % (base, index))
