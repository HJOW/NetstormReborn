// 썬 바리케이드 기존 표식 삭제 raw-fence-marker-clear.js(RawFenceMarkerClear)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-marker-clear-x86.tsv 259행 — 입력 4열(이웃 위치 장면·QA 일치 mask·packed QB 일치 mask·삭제 효과 변이 종류) → 검색 영역 `왼쪽,위,오른쪽,아래`, 삭제 요청 순서(`D:SID:flags`), 여섯 슬롯 전체 300바이트 raw.
//    원본 실제 finder/태그와 같은 현재 해시/삭제 순서를 비교한다(삭제 효과 뒤 next 와 태그 변경 포함).
//  - 계약: FenceMarkerClear_RequiredDeletionAndCurrentType(선택된 삭제만 필수이며 다른 타입은 현재 태그가 같아도 삭제하지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E83: 썬 바리케이드 기존 표식 삭제 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { RawFenceMarkerClear } from '../www/core/raw-fence-marker-clear.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E83)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-marker-clear-x86.tsv'));

// 두 끝점/세 표식/다른 타입의 원본 입력 SID, 독립 입력 수이다. (2026-11-08 WB04-E83)
const IDS = Object.freeze([50, 51, 60, 61, 62, 63]);
const ROWS = 259;
const fround = Math.fround;

// 예외가 나는지 본다. (2026-11-08 WB04-E83)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E83)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

const freshType = () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0 });
const freshTarget = () => ({ range: 0, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 });
const freshShape = () => ({ loaded: false, frameCount: 0, frames: [] });

// 단일 raw hash/finder/중심과 명시 삭제 변경 입력을 구성하는 장면이다. (2026-11-08 WB04-E83)
class MarkerClearScene {
    // 결과를 재계산하지 않고 동일한 입력 슬롯/버킷/태그만 설치한다.
    constructor(text) {
        this.pool = new SidPool(24000, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, freshType);
        this.markerType = 90;
        this.trace = [];
        this.deletions = 0;
        this.input = text.split(',').map(Number);
        assert.equal(this.input.length, 4);
        this.target = new RawGunTarget(this.pool, this.hash, this.types, Array.from({ length: 188 }, freshTarget), Array.from({ length: 188 }, freshShape), {
            friendlyFire: false, checking: false, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148,
            windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {}
        });
        // 필요한 실제 번호까지 확보한다.
        for (let sid = 5; sid <= 63; sid += 1) assert.equal(this.pool.allocate(2), sid);
        // 같은 원본 1×1 발자국을 공급한다.
        for (const type of [90, 91, 136]) { this.types[type].footX = 1; this.types[type].footY = 1; }
        const first = [50.25, 50.5];
        const second = this.input[0] === 0 ? [56.25, 50.5] : this.input[0] === 1 ? [50.25, 56.5] : this.input[0] === 2 ? [56.25, 56.5] : [44.25, 50.5];
        // native setup 과 같은 역순 머리 삽입이다.
        for (const sid of [...IDS].reverse()) {
            const point = sid === 50 ? first : sid === 51 ? second : [fround(fround(fround(first[0] + second[0]) * 0.5) + fround((sid - 61) * 0.25)), fround(fround(first[1] + second[1]) * 0.5)];
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            put(raw, 0, 0x11018000);
            raw[10] = sid < 60 ? 136 : sid === 63 ? 91 : 90;
            put(raw, 14, floatToBits(point[0]));
            put(raw, 18, floatToBits(point[1]));
            raw[33] = 1;
            // 입력한 packed 형식의 일치 여부만 기록한다.
            const packed = sid >= 60 && (this.input[2] & (1 << (sid - 60)));
            put(raw, 12, sid === 63 || (sid >= 60 && (this.input[1] & (1 << (sid - 60)))) ? 50 : 77, 2);
            put(raw, 26, packed ? 0 : 1);
            raw[30] = packed ? 50 : 77;
            put(raw, 4, this.hash.bucket(1, point[0], point[1]), 2);
            this.hash.setBucket(1, point[0], point[1], sid);
        }
        this.body = new RawFenceMarkerClear(this.pool, this.hash, this.types, this.target, () => this.markerType, (sid, flags) => {
            assert.equal(flags, 64);
            this.trace.push(`D:${sid}:${flags}`);
            this.deletions += 1;
            if (this.input[3] === 1) put(this.pool.allocated(sid), 12, 77, 2);
            if (this.input[3] === 2 && this.deletions === 1) put(this.pool.allocated(61), 12, 50, 2);
            if (this.input[3] === 3 && this.deletions === 1) put(this.pool.allocated(sid), 4, 0, 2);
        });
    }

    // 원본 여섯 슬롯의 전체 300 BYTE 이다.
    objects() { return IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }
}

test('FenceMarkerClear: 원본 10.78 259입력의 중심/Rect/finder/태그/현재 삭제 효과가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 결과는 원본 관찰로부터만 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const scene = new MarkerClearScene(row[0]);
        const area = scene.body.bounds(50, 51);
        const label = `입력 ${row[0]}`;
        assert.equal(`${area.left},${area.top},${area.right},${area.bottom}`, row[1], label);
        scene.body.clear(50, 51);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[2], label);
        assert.equal(scene.objects(), row[3], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('FenceMarkerClear: 선택된 삭제만 필수이며 다른 타입은 현재 태그가 같아도 삭제하지 않고 판본/풀 혼합을 거부한다', () => {
    const scene = new MarkerClearScene('0,0,0,0');
    const missing = new RawFenceMarkerClear(scene.pool, scene.hash, scene.types, scene.target, () => scene.markerType, undefined);
    missing.clear(50, 51);
    assert.equal(scene.trace.length, 0);
    put(scene.pool.allocated(60), 12, 50, 2);
    assert.ok(throws(() => missing.clear(50, 51)));
    scene.markerType = 91;
    scene.body.clear(50, 51);
    assert.equal(scene.trace.join(';'), 'D:63:64');
    assert.ok(throws(() => new RawFenceMarkerClear({ edition: 'cd1072' }, scene.hash, scene.types, scene.target, () => 90, undefined)));
    assert.ok(throws(() => new RawFenceMarkerClear(new SidPool(24000), scene.hash, scene.types, scene.target, () => 90, undefined)));
});
