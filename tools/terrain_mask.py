# -*- coding: utf-8 -*-
"""Terrainbuilder의 본섬 생성 규칙을 독립 재현하여 마스크·해시·캡처 대조 이미지를 만든다."""
import argparse
import collections
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw

from fort import SECTION_NAMES, split_sections
from taff import TaffArchive

# 원본 월드는 16칸 청크 16개로 구성된다.
WORLD_SIZE = 256
# 저장되지 않은 지면 칸은 해시 입력에서 255로 표시한다.
EMPTY = 255
# 원본 연결 방향 A~P의 북·동·남·서 비트 (VA 0x52f910).
CONNECTIONS = (15, 7, 14, 13, 11, 6, 12, 9, 3, 5, 10, 4, 8, 1, 2, 0)
# 원본 8방향 좌표표 (VA 0x52f83c/0x52f85c).
NEIGHBORS = ((0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1))
# 004bca20의 시드 0 대체 값과 004c117e의 청크별 시드 증가량.
DEFAULT_SEED = 0x0BAD0BAD
CHUNK_SEED_STEP = 0x10E3


def random_value(state, limit):
    """원본 정수 난수의 다음 상태와 출력을 반환한다 (32비트 오버플로)."""
    state = ((state or DEFAULT_SEED) * 0x10003 + 3) & 0xFFFFFFFF
    return state, (state >> 16) % limit


def territory_chunks(rows, patterns):
    """저장 영역을 회전한 뒤 원본 004c10a0의 월드 y·x 순서로 나열한다."""
    chunks = []
    # 활성 영역만 청크 소속 표에 등록한다.
    for region in range(20):
        shape, flags, position, seed, _, _ = rows[region * 6:region * 6 + 6]
        if not flags & 1 or flags & 2:
            continue
        pattern = patterns[shape & 63]
        width, height = pattern['Width'], pattern['Height']
        turns = (shape >> 6) // 2
        cells = []
        # 패턴의 빈 셀을 제외하고 연결 방향도 함께 회전한다.
        for index, letter in enumerate(pattern['Cells']):
            if letter == '.':
                continue
            x, y = index % width, index // width
            if turns == 1:
                x, y = height - 1 - y, x
            mask = CONNECTIONS[ord(letter) - ord('A')]
            # 현재 저장 방향의 회전 수는 0 또는 1이며 같은 수만큼 연결 비트도 회전한다.
            for _ in range(turns):
                mask = ((mask << 1) & 15) | (mask >> 3)
            cells.append((1 + (position & 15) + x, 1 + (position >> 4) + y, mask))
        cells.sort(key=lambda cell: (cell[1], cell[0]))
        # 영역 카운터는 해당 영역 청크 순번이며 밀도와 성장에서 서로 다른 시점의 값을 쓴다.
        for index, (x, y, mask) in enumerate(cells):
            chunks.append((x, y, region, mask, seed, index, len(cells)))
    return sorted(chunks, key=lambda cell: (cell[1], cell[0]))


def generate_mask(rows, patterns):
    """004c10a0·004c0800·004c0cf0를 본섬의 영역 바이트 마스크로 재현한다."""
    chunks = territory_chunks(rows, patterns)
    owners = {(x, y): region for x, y, region, *_ in chunks}
    land = bytearray([EMPTY]) * (WORLD_SIZE * WORLD_SIZE)

    def at(x, y):
        """월드 밖은 빈 칸으로 취급한다."""
        return land[y * WORLD_SIZE + x] if 0 <= x < WORLD_SIZE and 0 <= y < WORLD_SIZE else EMPTY

    def put(x, y, region, restricted=False):
        """004c0780처럼 성장 단계에서만 청크 소속을 검사한다."""
        if not (0 <= x < WORLD_SIZE and 0 <= y < WORLD_SIZE) or at(x, y) != EMPTY:
            return 0
        if restricted and owners.get((x // 16, y // 16)) != region:
            return 0
        land[y * WORLD_SIZE + x] = region
        return 1

    def rectangle(left, top, right, bottom, region):
        """004c0930의 반열린 사각형을 채우고 추가된 칸 수를 반환한다."""
        count = 0
        # 원본 사각형 작성 함수는 x를 바깥 반복문으로 사용한다.
        for x in range(left, right):
            # 세로 방향으로 범위 안의 지면을 채운다.
            for y in range(top, bottom):
                count += put(x, y, region)
        return count

    targets = {}
    # 모든 청크의 통로를 먼저 작성하고 청크마다 독립된 밀도 시드를 사용한다.
    for x, y, region, mask, seed, index, count in chunks:
        left, top = x * 16, y * 16
        cx, cy = left + 4 + (seed & 7), top + 4 + ((9999 - seed) & 7)
        added = rectangle(cx - 1, cy - 1, cx + 3, cy + 3, region)
        added += rectangle(left if mask & 8 else cx, top if mask & 1 else cy,
                           left + 16 if mask & 2 else cx + 2, top + 16 if mask & 4 else cy + 2, region)
        if index == 1:
            added += rectangle(left + 3, top + 3, left + 13, top + 13, region)
        _, value = random_value(index * CHUNK_SEED_STEP + seed, 30)
        targets[x, y] = max(20, value + 50 - added * 100 // 256) * 256 // 100
    # 성장 후보마다 난수를 세 번 소비하고, 실제로 추가된 칸만 목표에서 뺀다.
    for x, y, region, mask, seed, index, count in chunks:
        state = (count + index) * CHUNK_SEED_STEP + seed
        remaining, retries = targets[x, y], 1000
        # 실패 1,000회가 연속되거나 목표량을 채울 때까지 3×3 덩어리를 붙인다.
        while remaining > 0 and retries > 0:
            retries -= 1
            state, px = random_value(state, 15)
            state, py = random_value(state, 15)
            state, _ = random_value(state, 1)
            px, py = px + x * 16, py + y * 16
            if at(px, py) != region:
                continue
            added = 0
            # 원본 성장의 덩어리는 y·x 순서로 추가된다.
            for dy in (-1, 0, 1):
                # 청크 소속이 다른 칸에는 지면을 확장하지 않는다.
                for dx in (-1, 0, 1):
                    added += put(px + dx, py + dy, region, restricted=True)
            if added:
                remaining -= added
                retries = 1000
    # 원본은 두 번의 y·x 순회에서 빈 틈을 즉시 보정한다.
    for _ in range(2):
        # 월드 테두리를 제외한 행을 순회한다.
        for y in range(1, WORLD_SIZE - 1):
            # 직선 방향에 지면이 있는 빈 칸만 검사한다.
            for x in range(1, WORLD_SIZE - 1):
                if at(x, y) != EMPTY or all(at(x + dx, y + dy) == EMPTY for dx, dy in NEIGHBORS[::2]):
                    continue
                run, owner = 0, EMPTY
                # 북서·북 경계에 걸친 연속 지면도 조사한다.
                for direction in range(13):
                    dx, dy = NEIGHBORS[direction & 7]
                    value = at(x + dx, y + dy)
                    if value == EMPTY:
                        run = 0
                        continue
                    if run == 0:
                        owner = value
                    if value != owner:
                        run = 0
                        continue
                    run += 1
                    if run > 4 and direction & 1:
                        put(x, y, owner)
                        break
    return land


def main():
    """공식 맵의 본섬 마스크와 해시를 저장하고 선택적으로 원본 캡처에 윤곽을 겹친다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('map', help='아카이브 안 맵 이름 또는 느슨한 .fort 경로')
    parser.add_argument('--orig', type=Path, default=Path('originals'))
    parser.add_argument('--out', type=Path, default=Path('extracted/terrain'))
    parser.add_argument('--capture', type=Path)
    parser.add_argument('--client', type=int, nargs=4, metavar=('X', 'Y', 'W', 'H'))
    parser.add_argument('--offset', type=int, nargs=2, help='클라이언트 화면 좌표 = 월드 칸 × (16,11) + offset')
    args = parser.parse_args()
    if args.capture and (not args.client or not args.offset):
        parser.error('--capture에는 --client와 --offset이 필요합니다.')
    path = Path(args.map)
    if path.is_file():
        data = path.read_bytes()
    else:
        archive = TaffArchive(str(args.orig / 'netstorm.tarc'))
        name = 'd/' + path.stem.lower() + '.fort'
        entry = next(e for e in archive.entries if e.name.replace('\\', '/').lstrip('/').lower() == name)
        data = archive.read(entry)
    sections, _ = split_sections(data)
    rows = sections[SECTION_NAMES.index('Territory')]
    patterns = json.loads((Path(__file__).resolve().parent.parent / 'src/Netstorm.Assets/TerritoryPatterns.json').read_text(encoding='utf-8'))
    land = generate_mask(rows, patterns)
    report = {'map': path.stem, 'sha256': hashlib.sha256(land).hexdigest(),
              'regions': dict(sorted(collections.Counter(value for value in land if value != EMPTY).items()))}
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / f'{path.stem}.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    (args.out / f'{path.stem}.mask').write_bytes(land)
    Image.frombytes('L', (WORLD_SIZE, WORLD_SIZE), bytes(0 if value == EMPTY else 255 for value in land)).resize(
        (WORLD_SIZE * 3, WORLD_SIZE * 3), Image.Resampling.NEAREST).save(args.out / f'{path.stem}.png')
    if args.capture:
        image = Image.open(args.capture).convert('RGB')
        x, y, width, height = args.client
        image = image.crop((x, y, x + width, y + height))
        draw = ImageDraw.Draw(image)
        ox, oy = args.offset
        # 기본 isle 프레임의 xmin=-15, ymin=-10을 적용한다. 가장자리 그림의 돌출은 별도 비교 대상이다.
        for index, region in enumerate(land):
            if region == EMPTY:
                continue
            x, y = index % WORLD_SIZE, index // WORLD_SIZE
            sx, sy = x * 16 + ox - 15, y * 11 + oy - 10
            # 북·동·남·서 이웃이 다르면 해당 칸의 경계를 표시한다.
            for dx, dy, line in ((0, -1, (sx, sy, sx + 16, sy)), (1, 0, (sx + 16, sy, sx + 16, sy + 11)),
                                 (0, 1, (sx, sy + 11, sx + 16, sy + 11)), (-1, 0, (sx, sy, sx, sy + 11))):
                nx, ny = x + dx, y + dy
                if not (0 <= nx < WORLD_SIZE and 0 <= ny < WORLD_SIZE) or land[ny * WORLD_SIZE + nx] != region:
                    draw.line(line, fill=(255, 80, 80))
        image.save(args.out / f'{path.stem}-overlay.png')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
