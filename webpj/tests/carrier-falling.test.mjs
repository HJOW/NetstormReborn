// Carrier 낙하 raw-carrier-falling.js(RawCarrierFalling)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-falling-x86.tsv(10.78 전용; x87 53/64 두 관측 중 53비트 열) 9884행 — 모드(0 다리 판정 / 1 낙하 / 2 사제 해방 / 3 내부 좌표)·개수·좌표·권한·진단·검색 결과·내용물 WORD·복귀 좌표·지도 제한·genus·프레임·표면 패턴·변이 단계·사제 종류 →
//    반환 DWORD, 호출 순서(M·K·C·W·O/o·D/d·U·P/p·F/f·A), 부모/포획 Form/새 사제 raw, 권한, 사제 종류. carrier-falling-vtables.tsv 8행은 사제/캐리어/보호막의 서로 다른 낙하 슬롯 선택 근거를 원문 상수로 확인한다.
//  - 계약: CarrierFalling_LazyEffectsAndPhysicalFrameBounds(선택된 하위가 필요해질 때 미연결을 거부하고 물리 프레임 범위를 진단).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E73: Carrier 낙하 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawCarrierFalling } from '../www/core/raw-carrier-falling.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E73)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(FIXTURES, 'carrier-falling-x86.tsv'));
const tableRows = readRows(path.join(FIXTURES, 'carrier-falling-vtables.tsv'));

// 원문 격리 가상 표와 교체 표, 독립 입력 수, 가상 표 행 수, 부모/포획 Form/새 사제 SID 이다. (2026-11-08 WB04-E73)
const VT = 0x50f210;
const CHANGED_VT = 0x20000800;
const ROWS = 9884;
const TABLE_ROWS = 8;
const PARENT = 50;
const FORM = 51;
const BORN = 52;

// 예외가 나는지 본다. (2026-11-08 WB04-E73)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E73)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 큰 가상 효과만 명시하며 부모의 좌표/분기/캡처 계산은 제품 몸체가 수행하는 장면이다. (2026-11-08 WB04-E73)
class CarrierFallingScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.frames = [];
        this.surface = new Uint16Array(65536);
        this.players = new Uint8Array(128 * 0xb4);
        this.boss = false;
        this.checking = false;
        this.priestType = 0;
        this.mapLimit = 0;
        this.row = [];
        this.trace = [];
        this.surfaceKey = '';
    }

    // 원문은 할당 필터 없이 물리 raw 를 읽는다.
    raw(sid) {
        return this.pool.physical(sid);
    }

    // 가상 표 변경 뒤 현재 효과와 원래 효과를 구별한다.
    event(name, args) {
        this.trace.push([name, ...args].join(':'));
    }

    // 입력 자료만 설치하며 기대 좌표/반환/분기는 계산하지 않는다.
    prepare(input) {
        assert.equal(input.length, 19);
        this.row = input;
        this.boss = input[4] !== 0;
        this.checking = input[5] !== 0;
        this.priestType = input[18];
        this.mapLimit = input[11] | 0;
        this.trace = [];
        // 부모/포획 Form/새 사제의 실제 관측 전 raw 다.
        for (const sid of [PARENT, FORM, BORN]) {
            const raw = this.raw(sid);
            raw.fill(0xa5);
            put(raw, 0, VT);
            raw[10] = 158;
            raw[11] = 0;
            put(raw, 14, input[2]);
            put(raw, 18, input[3]);
        }
        put(this.raw(FORM), 12, input[8], 2);
        this.frames = [];
        // TYPE 번호와 원문의 8코드를 구성한다. 제품 프레임 조회 결과는 넣지 않는다.
        for (let kind = 0; kind < 188; kind += 1) {
            const codes = [];
            if (kind === 70 || kind === 71) {
                const sid = kind - 10;
                this.raw(sid)[10] = kind;
                put(this.raw(sid), 36, input[13]);
                this.types[kind].flags2 = (input[12] ^ (kind === 71 ? 4 : 0)) >>> 0;
                // 같은 고정 코드 패턴 입력을 제공한다.
                for (let index = 0; index < 8; index += 1) codes.push({ side: 65, variant: 0, flags: (input[14] ^ (index * 17) ^ (kind === 71 ? 32 : 0)) & 0xff });
            }
            this.frames.push(new FrameCodeTable(codes));
        }
        // 조회 좌표와 독립인 hash0 입력 패턴이다. 같은 입력이면 다시 채우지 않는다.
        const key = `${input[15]},${input[16]}`;
        if (this.surfaceKey !== key) {
            for (let index = 0; index < this.surface.length; index += 1) this.surface[index] = input[16] ? (index % 7 === 0 ? 0 : 60 + (index % 2)) : input[15] ? 60 : 0;
            this.surfaceKey = key;
        }
        // 원문의 모든 이전 소유자 후보에 같은 명시 위치 입력을 둔다.
        for (let owner = 0; owner < 128; owner += 1) {
            put(this.players, owner * 0xb4 + 0xa8, input[9]);
            put(this.players, owner * 0xb4 + 0xac, input[10]);
        }
        if (input[0] === 3) {
            put(this.raw(PARENT), 0, input[2]);
            put(this.raw(PARENT), 4, input[3]);
        }
    }

    // 기존 검색/생성/가상 부작용만 원문과 같은 경계로 공급한다.
    hooks() {
        return {
            find: (sid, mask, kind, index) => { this.event('M', [sid, mask, kind, index]); if (this.row[17] === 2) this.boss = false; return this.row[6]; },
            findKind: (sid, kind) => {
                this.event('K', [sid, kind, 0, 0]);
                if (this.row[17] === 3) { this.priestType = 159; put(this.raw(FORM), 12, 0x2342, 2); }
                return this.row[7];
            },
            create: (type, flags) => { this.event('C', [type, flags]); if (this.row[17] === 4) put(this.raw(FORM), 12, 0x3443, 2); return BORN; },
            setWord: (sid, word) => {
                this.event('W', [sid, word]);
                put(this.raw(sid), 12, word, 2);
                if (this.row[17] === 5) put(this.raw(sid), 0, CHANGED_VT);
            },
            setOwner: (sid, owner) => {
                this.event(get(this.raw(sid), 0) === CHANGED_VT ? 'o' : 'O', [sid, owner]);
                this.raw(sid)[34] = owner & 0xff;
                if (this.row[17] === 6) { this.raw(FORM)[12] = 4; put(this.raw(sid), 0, CHANGED_VT); }
            },
            destroy: (sid, flags) => { this.event(get(this.raw(sid), 0) === CHANGED_VT ? 'd' : 'D', [sid, flags]); this.raw(sid)[11] |= 2; },
            unpop: (sid, flags) => {
                this.event('U', [sid, flags]);
                this.raw(sid)[11] |= 4;
                if (this.row[17] === 1) { put(this.raw(sid), 0, CHANGED_VT); put(this.raw(sid), 14, floatToBits(42.5)); put(this.raw(sid), 18, floatToBits(43.5)); }
            },
            pop: (sid, x, y, flags) => {
                this.event(get(this.raw(sid), 0) === CHANGED_VT ? 'p' : 'P', [sid, floatToBits(x), floatToBits(y), flags]);
                put(this.raw(sid), 14, floatToBits(x));
                put(this.raw(sid), 18, floatToBits(y));
                this.raw(sid)[11] &= ~4 & 0xff;
                if (this.row[17] === 9) { put(this.raw(sid), 0, CHANGED_VT); put(this.raw(FORM), 0, CHANGED_VT); }
            },
            fallWalker: (sid) => {
                this.event(get(this.raw(sid), 0) === CHANGED_VT ? 'f' : 'F', [sid]);
                if (this.row[17] === 10) { put(this.raw(FORM), 0, CHANGED_VT); put(this.raw(PARENT), 0, CHANGED_VT); }
                return true;
            },
            assertion: (line) => {
                this.event('A', [line]);
                if (this.row[17] === 7) this.boss = true;
                if (this.row[17] === 8) { this.players.fill(0); put(this.raw(BORN), 0, CHANGED_VT); }
            }
        };
    }

    // 현재 전역 getter 로 만든 제품 몸체이다. (2026-11-08 WB04-E73)
    logic(hooks = this.hooks()) {
        return new RawCarrierFalling(this.pool, this.types, this.frames, this.surface, this.players, () => this.boss, () => this.checking, () => this.priestType, () => this.mapLimit, hooks);
    }
}

test('CarrierFalling: 원본 10.78 9884행의 다리 판정/낙하/사제 해방/내부 좌표와 효과 순서·부모/Form/새 사제 raw·권한·사제 종류가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierFallingScene();
    let count = 0;
    // 53비트 관측을 재생하며 64비트 관측도 원문에 별도 보존한다.
    for (const fields of rows) {
        assert.equal(fields.length, 11);
        scene.prepare(fields[0].split(',').map(Number));
        const logic = scene.logic();
        let result = 0;
        if (scene.row[0] === 0) result = logic.needsEscape(bitsToFloat(scene.row[2]), bitsToFloat(scene.row[3]));
        else if (scene.row[0] === 1) result = floatToBits(logic.fall(PARENT, scene.row[1]));
        else if (scene.row[0] === 2) logic.releasePriest(PARENT);
        else result = logic.validPoint(bitsToFloat(scene.row[2]), bitsToFloat(scene.row[3])) ? 1 : 0;
        const raws = `${hex(scene.raw(PARENT))},${hex(scene.raw(FORM))},${hex(scene.raw(BORN))}`;
        assert.equal(result, Number(fields[1]), `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[2], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(raws, fields[3], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.boss, Number(fields[4]) !== 0, `행 ${count}`);
        assert.equal(scene.priestType, Number(fields[5]), `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierFalling: 사제 보호막의 공통 수명과 사제/캐리어의 서로 다른 낙하 슬롯을 원본 가상 표 원문으로 확인한다', { skip: !tableRows && '원본 fixture 없음' }, () => {
    let tables = 0;
    // 공통 수명 분배와 낙하 슬롯 선택 근거를 원문 상수로 확인한다.
    for (const fields of tableRows) {
        assert.equal(fields.length, 8);
        const [table, destroy, pre, post, release, unpop, pop, walker] = fields.map(Number);
        assert.ok(destroy === 0x4af780 && release === 0x4ad4a0 && unpop === 0x4afe50 && pop === 0x4b02d0);
        if (table === 0x507770) assert.ok(pre === 0x4b0950 && post === 0x4b0840 && walker === 0x4437f0);
        else if (table === 0x50f210) assert.equal(walker, 0x494e80);
        else {
            assert.equal(pre, table === 0x5040c0 ? 0x42ae80 : 0x426890);
            assert.ok(post === 0x44b9e0 && walker === 0x4287d0);
        }
        tables += 1;
    }
    assert.equal(tables, TABLE_ROWS);
});

test('CarrierFalling: 선택된 하위가 실제 필요해지는 시점에 미연결을 거부하고 물리 프레임 범위를 진단한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierFallingScene();
    scene.prepare([0, 101, 0x41a40000, 0x41aa0000, 0, 0, 51, 51, 0x1281, 0x41f40000, 0x41fc0000, 256, 4, 0, 32, 1, 0, 0, 158]);
    const logic = scene.logic({});
    assert.equal(logic.fall(PARENT, 101), 1);
    logic.releasePriest(PARENT);
    assert.equal(logic.needsEscape(-2, 1), 0);
    assert.equal(logic.pool, scene.pool);
    scene.boss = true;
    assert.ok(throws(() => logic.fall(PARENT, 101)));
    put(scene.raw(60), 36, 8);
    assert.ok(throws(() => logic.needsEscape(20.5, 21.25)));
    assert.ok(throws(() => new RawCarrierFalling(scene.pool, scene.types, scene.frames.slice(1), scene.surface, scene.players, () => true, () => false, () => 158, () => 256, {})));
    assert.ok(throws(() => new RawCarrierFalling(scene.pool, scene.types, scene.frames, scene.surface, new Uint8Array(0xb4 + 1), () => true, () => false, () => 158, () => 256, {})));
});
