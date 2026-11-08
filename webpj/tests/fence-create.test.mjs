// 썬 바리케이드 표식/포스 필드 생성 raw-fence-create.js(RawFenceMarkerCreate·RawFenceShieldCreate)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-create-x86.tsv 의 표식(M) 106행·포스 필드(S) 306행 — 입력(끝점 좌표 비트·소유자·SID·외부 효과 변이·차단 번째) → 부모/이웃 전체 50바이트 raw, 새로 만든 객체들의 전체 50바이트 raw, 외부 효과 호출 순서
//    (Create/Notify/Owner/Pop). 같은 fixture 의 재생성(R) 166행은 raw-fence-rebuild.js 시험이 소비한다.
//  - 계약: FenceCreate_RequiredBoundariesAndPartialWrites(선택하지 않은 소비자는 필요 없고 선택 실패는 앞선 새 객체/QA/Frame 쓰기를 보존).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E83: 썬 바리케이드 표식/포스 필드 생성 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { RawFenceMarkerCreate, RawFenceShieldCreate } from '../www/core/raw-fence-create.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E83)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-create-x86.tsv'));

// 관찰 부모 SID, 격리 원본 실행기의 가상 표 기록값, 종류별 독립 입력 수이다. (2026-11-08 WB04-E83)
const SOURCE = 50;
const VTABLE = 0x124f0000;
const MARKER_ROWS = 106;
const SHIELD_ROWS = 306;

// 예외가 나는지 본다. (2026-11-08 WB04-E83)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E83)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 TYPE 구간 계산기로 A 가변/J 8/K 8 의 입력 표를 만든다. (2026-11-08 WB04-E83)
function fenceCreateCodes(first) {
    const codes = [];
    // 입력 프레임 코드이며 기대 결과는 계산하지 않는다.
    for (let index = 0; index < first + 16; index += 1) codes.push({ side: index < first ? 0x41 : index < first + 8 ? 0x4a : 0x4b, variant: 0x41, flags: index });
    return new FrameCodeTable(codes);
}

const freshType = () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0 });
const freshTarget = () => ({ range: 0, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 });
const freshShape = () => ({ loaded: false, frameCount: 0, frames: [] });

// 같은 Pool/현재 중심과 명시 하위 효과 입력만 조립하는 장면이다. (2026-11-08 WB04-E83)
class FenceCreateScene {
    // 원본과 같은 입력 기록만 설치하고 생성 판단/순서는 제품 코드에 맡긴다.
    constructor(kind, values) {
        this.kind = kind;
        this.input = values.split(',').map(Number);
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, freshType);
        this.frames = Array.from({ length: 188 }, () => fenceCreateCodes(8));
        this.markerType = 90;
        this.shieldType = 170;
        this.created = 0;
        this.trace = [];
        this.other = 51;
        this.mutation = 0;
        this.blockAt = 0;
        if (kind === 'M') { this.other = this.input[7]; this.mutation = this.input[6]; } else { this.blockAt = this.input[8]; this.mutation = this.input[9]; }
        const targetTypes = Array.from({ length: 188 }, freshTarget);
        const shapes = Array.from({ length: 188 }, freshShape);
        this.target = new RawGunTarget(this.pool, this.hash, this.types, targetTypes, shapes, {
            friendlyFire: false, checking: false, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148,
            windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {}
        });
        const first = this.pool.layout.serverFirst;
        // 명시 생성이 반환할 60~159 와 높은 클라이언트 끝점을 입력으로 확보한다.
        const lastClient = this.other < first ? Math.max(this.other, 159) : 159;
        for (let sid = 5; sid <= lastClient; sid += 1) assert.equal(this.pool.allocate(2), sid);
        // 지정 take 는 원본 계약대로 서버 영역 끝점에만 사용한다.
        if (this.other >= first) new SquidFactory(this.pool, this.types).take(136, this.other);
        for (const type of [90, 136, 170, 171]) { this.types[type].footX = 1; this.types[type].footY = 1; }
        this.frames[171] = fenceCreateCodes(13);
        this.parents = [SOURCE, this.other];
        // 부모의 모든 보존 바이트를 원본 입력과 동일하게 준비한다.
        for (const sid of this.parents) {
            const raw = this.pool.allocated(sid);
            raw.fill(0xa5);
            put(raw, 0, VTABLE);
            raw[10] = 136;
            raw[11] = 0;
            raw[40] = 0;
            raw[34] = 1;
            put(raw, 14, this.input[sid === SOURCE ? 0 : 2]);
            put(raw, 18, this.input[sid === SOURCE ? 1 : 3]);
            raw[34] = this.input[sid === SOURCE ? 4 : 5];
        }
        this.markers = new RawFenceMarkerCreate(this.pool, this.target, () => this.markerType, this.hooks());
        this.shields = new RawFenceShieldCreate(this.pool, this.frames, () => this.shieldType, this.hooks());
    }

    // 새 객체/표시의 큰 몸체는 원본 관찰과 동일한 입력/현재 변경 경계이다.
    hooks() {
        const pool = this.pool;
        const event = (text) => this.trace.push(text);
        return {
            create: (type, flags) => {
                assert.equal(flags, 2);
                const sid = 60 + this.created;
                this.created += 1;
                event(`Create,${type},${flags},${sid}`);
                const raw = pool.allocated(sid);
                raw.fill(0xa5);
                put(raw, 0, VTABLE);
                raw[10] = this.kind === 'S' && this.mutation === 1 ? 171 : type;
                raw[11] = 0;
                raw[34] = 0;
                raw[40] = this.kind === 'S' && this.blockAt === this.created ? 32 : 0;
                put(raw, 36, 17);
                if (this.kind === 'S' && this.mutation === 3 && this.created === 1) this.shieldType = 171;
                return sid;
            },
            notifySurface: (sid) => {
                event(`Notify,${sid}`);
                if (this.kind === 'M' && this.mutation === 1) pool.allocated(SOURCE)[34] = 7;
            },
            setOwner: (sid, owner) => {
                event(`Owner,${sid},${owner}`);
                pool.allocated(sid)[34] = owner;
                if (this.kind === 'M' && this.mutation === 2) pool.allocated(SOURCE)[34] = 6;
                if (this.kind === 'S' && this.mutation === 2) put(pool.allocated(SOURCE), 14, floatToBits(199.0));
            },
            pop: (sid, x, y, flags) => {
                assert.equal(flags, 64);
                event(`Pop,${sid},${floatToBits(x)},${floatToBits(y)},${flags}`);
                put(pool.allocated(sid), 14, floatToBits(x));
                put(pool.allocated(sid), 18, floatToBits(y));
                if (this.kind === 'M' && this.mutation === 3) {
                    put(pool.allocated(SOURCE), 14, floatToBits(199.0));
                    put(pool.allocated(this.other), 18, floatToBits(200.0));
                }
            }
        };
    }

    // 종류에 맞는 생성 부모를 실행한다.
    run() {
        if (this.kind === 'M') this.markers.create(SOURCE, this.other);
        else this.shields.create([bitsToFloat(this.input[0]), bitsToFloat(this.input[1]), bitsToFloat(this.input[2]), bitsToFloat(this.input[3])], this.input[4], this.input[5], this.input[6] | 0, this.input[7] | 0);
    }

    // 원본이 기록한 전체 부모 바이트이다.
    parentsHex() { return this.parents.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 선택한 새 객체들의 전체 50 BYTE 이다.
    createdHex() {
        const value = Array.from({ length: this.created }, (_, index) => hex(this.pool.slot(60 + index))).join('');
        return value === '' ? '-' : value;
    }
}

// 별도 범위마다 원본 결과를 전수 비교해 한 실패가 다른 생성 경로를 감추지 않게 한다. (2026-11-08 WB04-E83)
function compare(kind, expected) {
    let count = 0;
    // 기대값은 실제 원본에서만 읽는다.
    for (const row of allRows.filter((candidate) => candidate[0] === kind)) {
        assert.equal(row.length, 5);
        const scene = new FenceCreateScene(kind, row[1]);
        scene.run();
        const label = `${kind} 입력 ${row[1]}`;
        assert.equal(scene.parentsHex(), row[2], label);
        assert.equal(scene.createdHex(), row[3], label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[4], label);
        count += 1;
    }
    assert.equal(count, expected);
}

test('FenceCreate: 원본 10.78 표식 106입력의 현재 중심/4칸 구간/high8·low8 태그/후속 owner 가 같다', { skip: !allRows && '원본 fixture 없음' }, () => {
    compare('M', MARKER_ROWS);
});

test('FenceCreate: 원본 10.78 포스 필드 306입력의 좌표/J·K 직접 Frame/100상한/차단/signed SID owner 가 같다', { skip: !allRows && '원본 fixture 없음' }, () => {
    compare('S', SHIELD_ROWS);
});

test('FenceCreate: 선택되지 않은 소비자는 필요 없고 선택 실패는 앞선 새 객체/QA/Frame 쓰기를 보존하며 판본/풀을 구별한다', () => {
    const markers = new FenceCreateScene('M', '1112080384,1112145920,1114308608,1112145920,1,2,0,51');
    const hooks = markers.hooks();
    delete hooks.notifySurface;
    const missing = new RawFenceMarkerCreate(markers.pool, markers.target, () => markers.markerType, hooks);
    assert.ok(throws(() => missing.create(SOURCE, 51)));
    assert.ok(markers.created === 1 && get(markers.pool.slot(60), 12, 2) === 50);
    const blocked = new FenceCreateScene('S', '1112080384,1112145920,1114308608,1112145920,1,2,50,51,1,0');
    const early = blocked.hooks();
    delete early.setOwner;
    delete early.pop;
    new RawFenceShieldCreate(blocked.pool, blocked.frames, () => blocked.shieldType, early).create([50.25, 50.5, 58.75, 50.5], 1, 2, 50, 51);
    assert.ok(blocked.created === 1 && get(blocked.pool.slot(60), 36) === 17);
    assert.ok(throws(() => new RawFenceMarkerCreate({ edition: 'cd1072' }, markers.target, () => 90, {})));
    assert.ok(throws(() => new RawFenceMarkerCreate(new SidPool(24000), markers.target, () => 90, {})));
    assert.ok(throws(() => new RawFenceShieldCreate(blocked.pool, blocked.frames.slice(1), () => 170, {})));
    assert.ok(throws(() => RawFenceMarkerCreate.points([Number.NaN, 0, 0, 0])));
});
