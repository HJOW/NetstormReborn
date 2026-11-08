// 썬 바리케이드 표적 raw-fence-target.js(RawFenceTarget)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-target-x86.tsv 856행(원본 PATCH 전용 단일 판본) — 종류 A(Across 626)·P(Point 전체 finder 48)·M(일반 포구 24)·R(시야 사각형 24)·I(선분 교차 134)의 입력 28열(발사자/표적/배리어 끝점 비트·발자국·group·소유자·동맹·픽셀 배율·핫스폿·포구 픽셀·프레임·해시 단계·표식 등록 수)
//    → 출력 네 float 의 원본 비트(정렬/소수 반올림 없음). A/P 는 값형 동맹 표와 현재 방향별 동맹 조회기(allianceRead, 반대 fallback)로 두 번 재생한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E86: 썬 바리케이드 표적 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { RawFenceTarget } from '../www/core/raw-fence-target.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E86)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-target-x86.tsv'))?.filter((row) => row[0] !== 'kind') ?? null;

// 원본 관찰과 같은 실제 server SID 의 역할 순서(발사자·표적·배리어 끝점 둘·표식 둘·끝점 이웃 둘)이다. (2026-11-08 WB04-E86)
const IDS = Object.freeze([15000, 15001, 15002, 15003, 15004, 15005, 15006, 15007]);
const TYPES_BY_ROLE = Object.freeze([71, 158, 136, 136, 90, 90, 136, 136]);
const fround = Math.fround;

// 예외가 나는지 본다. (2026-11-08 WB04-E86)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const freshType = () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0 });
const freshTarget = () => ({ range: 0, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 });
const freshShape = () => ({ loaded: false, frameCount: 0, frames: [] });
const freshAllies = () => Array.from({ length: 9 }, () => new Array(9).fill(false));

// 고정 주소의 현재 raw/TYPE/배율/동맹을 같은 실제 분배기에 공급하는 검사 입력이다. (2026-11-08 WB04-E86)
class FenceFixture {
    constructor() {
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, freshType);
        this.geometry = Array.from({ length: 188 }, () => ({ shotOffsets: [] }));
        // 현재 변경되는 실제 픽셀 배율과 각도 진단을 담는 상태이며 중심 조회에도 같은 객체를 쓴다.
        this.targeting = { friendlyFire: false, checking: false, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148, windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {} };
        this.process = { allies: freshAllies() };
        this.target = new RawGunTarget(this.pool, this.hash, this.types, Array.from({ length: 188 }, freshTarget), Array.from({ length: 188 }, freshShape), this.targeting);
        this.fence = new RawFenceTarget(this.pool, this.hash, this.types, this.geometry, this.target, this.targeting, this.process);
        // 원본과 같은 순서로 server 슬롯을 한 번 확보한다.
        for (const sid of IDS) assert.equal(this.pool.allocate(), sid);
    }

    // 각 관찰 행의 슬롯/타입/SHP/체인 입력만 복원하며 결과를 계산하지 않는다.
    setup(row) {
        this.hash.reset();
        this.hash.select(Number(row[23]));
        this.process.allianceRead = undefined;
        this.process.allies = freshAllies();
        this.targeting.scaleX = bitsToFloat(Number(row[16]));
        this.targeting.scaleY = bitsToFloat(Number(row[17]));
        const owner = Number(row[14]);
        this.process.allies[1][owner] = Number(row[15]) !== 0;
        const types = this.types;
        types[71].footX = Number(row[9]) | 0;
        types[71].footY = Number(row[10]) | 0;
        types[71].group = Number(row[13]) | 0;
        types[158].footX = Number(row[11]) | 0;
        types[158].footY = Number(row[12]) | 0;
        types[136].footX = 1; types[136].footY = 1; types[90].footX = 1; types[90].footY = 1;
        types[71].hotspotX = Number(row[18]) | 0;
        types[71].hotspotY = Number(row[19]) | 0;
        const ox = Number(row[20]);
        const oy = Number(row[21]);
        // 원본 SHP 헤더에 넣은 네 signed WORD 쌍을 그대로 입력한다.
        this.geometry[71].shotOffsets = Array.from({ length: 4 }, (_, frame) => [ox + frame * 3, oy - frame]);
        const a = [bitsToFloat(Number(row[5])), bitsToFloat(Number(row[6]))];
        const b = [bitsToFloat(Number(row[7])), bitsToFloat(Number(row[8]))];
        const mid = [fround(fround(a[0] + b[0]) * 0.5), fround(fround(a[1] + b[1]) * 0.5)];
        const positions = [
            [bitsToFloat(Number(row[1])), bitsToFloat(Number(row[2]))], [bitsToFloat(Number(row[3])), bitsToFloat(Number(row[4]))], a, b,
            mid, [fround(mid[0] + 4), mid[1]], [fround(a[0] + 4), a[1]], [fround(b[0] + 4), b[1]]
        ];
        // 같은 server 순서로 확보한 슬롯에 원본과 같은 raw 필드를 쓴다.
        IDS.forEach((sid, index) => {
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            raw[10] = TYPES_BY_ROLE[index];
            raw[34] = index === 0 ? 1 : owner;
            put(raw, 14, floatToBits(positions[index][0]));
            put(raw, 18, floatToBits(positions[index][1]));
            put(raw, 36, index === 0 ? Number(row[22]) : 0);
            if (index === 4 || index === 5) {
                const first = IDS[index === 4 ? 2 : 6];
                const second = IDS[index === 4 ? 3 : 7];
                put(raw, 12, first, 2);
                put(raw, 26, second >>> 8);
                raw[30] = second & 255;
            }
        });
        const marks = Number(row[24]);
        const registered = [0, 1, 2, 3];
        if (Math.abs(marks) >= 1) registered.push(4);
        if (Math.abs(marks) === 2) registered.push(5);
        if (marks < 0) registered.reverse();
        // 입력으로 정한 현재 버킷에 머리 삽입하며 finder 결과는 계산하지 않는다.
        for (const index of registered) {
            const cx = Math.trunc(Math.trunc(positions[index][0]) / 2);
            const cy = Math.trunc(Math.trunc(positions[index][1]) / 2);
            put(this.pool.allocated(IDS[index]), 4, this.hash.cell(1, cx, cy), 2);
            this.hash.setCell(1, cx, cy, IDS[index]);
        }
    }
}

// 선택한 실제 루트 종류의 모든 DWORD 를 비교한다. 실패할 때 행 번호와 실제/기대 비트를 남긴다. (2026-11-08 WB04-E86)
function checkRows(kind, expectedCount, currentAlliances = false) {
    const fixture = new FenceFixture();
    let count = 0;
    let index = 0;
    // 한 행은 Native 고유 루트 한 개이며 반복 재실행을 새 관찰로 세지 않는다.
    for (const row of rows) {
        index += 1;
        if (row[0][0] !== kind) continue;
        assert.equal(row.length, 29);
        fixture.setup(row);
        // 현재 한 방향 DWORD 만 native 입력과 같고 fallback 은 반대로 둔다.
        const current = new Array(81).fill(0);
        if (currentAlliances) {
            current[9 + Number(row[14])] = Number(row[15]) ? 7 : 0;
            fixture.process.allianceRead = (owner, target) => current[owner * 9 + target];
            fixture.process.allies[1][Number(row[14])] = Number(row[15]) === 0;
        }
        const point = (x, y) => [bitsToFloat(Number(row[x])), bitsToFloat(Number(row[y]))];
        let output = [0, 0, 0, 0];
        if (kind === 'R') {
            const rect = fixture.fence.sightBounds(IDS[0]);
            output = [rect.left, rect.top, rect.right, rect.bottom];
        } else {
            const result = kind === 'A' ? fixture.fence.across(IDS[4], IDS[0], IDS[1])
                : kind === 'P' ? fixture.fence.point(IDS[0], IDS[1])
                    : kind === 'M' ? fixture.fence.muzzle(IDS[0])
                        : RawFenceTarget.intersection(point(5, 6), point(7, 8), point(1, 2), point(3, 4));
            output = [result[0], result[1], 0, 0];
        }
        const actual = output.map(floatToBits);
        const expected = [25, 26, 27, 28].map((column) => Number(row[column]));
        assert.deepEqual(actual, expected, `${kind} 행 ${index}`);
        count += 1;
    }
    assert.equal(count, expectedCount);
}

test('FenceTarget: 원본 10.78 배리어 Across 626개(방향·끝점·발자국·동맹·Cannon/대각/픽셀 배율 보정)가 float 전체 비트까지 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    checkRows('A', 626);
});

test('FenceTarget: 전체 finder 와 현재 해시 단계·후보 유무/가까운 후보 선택 원본 48개가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    checkRows('P', 48);
});

test('FenceTarget: 같은 입력을 현재 방향별 동맹 조회기(비대칭 표)로 재생해도 Across/Point 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    checkRows('A', 626, true);
    checkRows('P', 48, true);
});

test('FenceTarget: 일반 포구 24개·시야 사각형 24개·선분 교차(평행/수직/사선/끝점/소수) 134개가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    checkRows('M', 24);
    checkRows('R', 24);
    checkRows('I', 134);
});

test('FenceTarget: 판본/같은 풀/TYPE 크기 혼합과 잘못된 배율·SHP 프레임·동맹 범위를 거부한다', () => {
    const fixture = new FenceFixture();
    assert.ok(throws(() => new RawFenceTarget({ edition: 'cd1072' }, fixture.hash, fixture.types, fixture.geometry, fixture.target, fixture.targeting, fixture.process)));
    assert.ok(throws(() => new RawFenceTarget(new SidPool(24000), fixture.hash, fixture.types, fixture.geometry, fixture.target, fixture.targeting, fixture.process)));
    assert.ok(throws(() => new RawFenceTarget(fixture.pool, fixture.hash, fixture.types.slice(1), fixture.geometry, fixture.target, fixture.targeting, fixture.process)));
    // 포구 SHP 프레임 목록이 비어 있으면 거부한다.
    const raw = fixture.pool.allocated(IDS[0]);
    raw.fill(0);
    raw[10] = 71;
    assert.ok(throws(() => fixture.fence.muzzle(IDS[0])));
    assert.ok(throws(() => fixture.fence.sightBounds(IDS[0])));
    // 평행한 두 수직 선분은 끝점 b 를 후보로 삼지만 b 가 첫 사각형의 max 대각 끝점이라 제외되어 invalid 이다.
    assert.deepEqual(RawFenceTarget.intersection([1, 1], [1, 5], [1, 2], [1, 4]), [-10000, -10000]);
});
