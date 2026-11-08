// Carrier 내용물 통지 raw-carrier-content-notice.js(RawCarrierContentNotice)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-content-notice-x86.tsv(10.78 전용) 1731행 — 고정 입력 21열(저장 게임·로딩·폭탄 억제·소유자·로컬·kind·flags2·dead·+20 WORD·reason·좌표·확보·생성·genus·typeAt·타입 추가 DWORD·소리·변이·카메라) →
//    외부 효과 호출 순서(C·B·S·A·N·T·P·H·G·I·F), 효과 뒤 현재 전역 9칸, 부모/내용물 슬롯 50바이트 둘 전체.
//  - 계약: CarrierContentNotice_ContractsAndActualDispatch(실제 파생 VT 만 분배하고 읽지 않아야 할 분기와 선택된 필수 효과 누락을 구별).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E74: Carrier 내용물 통지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCarrierContentNotice } from '../www/core/raw-carrier-content-notice.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E74)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-content-notice-x86.tsv'));

// 원문 고정 입력 21열의 의미와 독립 입력 수, 부모/내용물 SID 이다. (2026-11-08 WB04-E74)
const COLUMN = Object.freeze({ saved: 0, loading: 1, blocked: 2, owner: 3, local: 4, kind: 5, flags: 6, dead: 7, word20: 8, reason: 9, x: 10, y: 11, allocate: 12, create: 13, genus: 14, typeAt: 15, effect: 16, sound: 17, mutation: 18, cameraX: 19, cameraY: 20 });
const ROWS = 1731;
const PARENT = 50;
const FORM = 60;

// 예외가 나는지 본다. (2026-11-08 WB04-E74)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E74)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 명시 경계 효과와 실제 공유 raw/전역 입력만 소유하는 장면이다. (2026-11-08 WB04-E74)
class NoticeScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.effects = new Array(188).fill(0);
        this.camera = [0, 0];
        this.saved = false;
        this.loading = 0;
        this.blocked = 0;
        this.local = 1;
        this.priestType = 158;
        this.blockerType = 156;
        this.input = [];
        this.trace = [];
    }

    // 원문 입력/명시 효과에만 쓰는 raw 접근이며 실제 생성의 대체가 아니다.
    raw(sid) {
        return this.pool.physical(sid);
    }

    // 매 행 전체 입력을 다시 공급한다.
    prepare(values) {
        assert.equal(values.length, 21);
        this.input = values;
        this.trace = [];
        this.saved = values[COLUMN.saved] !== 0;
        this.loading = values[COLUMN.loading];
        this.blocked = values[COLUMN.blocked];
        this.local = values[COLUMN.local];
        this.camera = [values[COLUMN.cameraX] | 0, values[COLUMN.cameraY] | 0];
        const parent = this.raw(PARENT);
        const form = this.raw(FORM);
        parent.fill(0xa5);
        form.fill(0x5a);
        put(parent, 0, 0x50f210);
        parent[10] = 158;
        parent[34] = values[COLUMN.owner];
        put(parent, 14, values[COLUMN.x]);
        put(parent, 18, values[COLUMN.y]);
        form[11] = values[COLUMN.dead];
        put(form, 18, values[COLUMN.kind], 2);
        put(form, 20, values[COLUMN.word20], 2);
        // 모든 미지정 TYPE 은 고정 입력 0 이며 추가 DWORD 만 지정한 서로 다른 값을 갖는다.
        for (let type = 0; type < this.types.length; type += 1) {
            this.types[type].flags2 = 0;
            this.effects[type] = 0x10000000 + type;
        }
        this.types[values[COLUMN.kind]].flags2 = values[COLUMN.flags];
        this.effects[values[COLUMN.kind]] = values[COLUMN.effect];
    }

    // 원본 경계가 받았던 DWORD 만 같은 순서로 기록한다.
    event(name, values = []) {
        this.trace.push([name, ...values].join(','));
    }

    // 원문 경계에 공급한 재진입 변경만 수행한다.
    mutate(event) {
        const mode = this.input[COLUMN.mutation];
        const parent = this.raw(PARENT);
        const form = this.raw(FORM);
        if (event === 'B' && mode === 1) { put(form, 18, 158, 2); form[11] = 2; }
        if (event === 'S' && mode === 2) { parent[34] = 2; put(parent, 14, floatToBits(99.0)); put(parent, 18, floatToBits(77.0)); }
        if (event === 'A' && mode === 3) { this.local = 2; put(form, 18, 123, 2); }
        if (event === 'N' && mode === 4) this.loading = 1;
        if (event === 'H' && mode === 5) { put(form, 18, 158, 2); put(parent, 14, floatToBits(25.5)); put(parent, 18, floatToBits(30.25)); }
        if (event === 'G' && mode === 6) { put(parent, 14, floatToBits(55.5)); put(parent, 18, floatToBits(66.25)); }
        if (event === 'P' && mode === 7) { form[11] = 2; this.loading = 1; }
        if (event === 'B' && mode === 8) { this.blocked = 1; put(form, 18, 123, 2); this.types[123].flags2 = 0; }
    }

    // 실제 외부 인수를 기록하며 가상 통지의 게임 분기는 제품이 수행한다.
    hooks() {
        const view = new DataView(new ArrayBuffer(4));
        const floatAt = (sid, offset) => { view.setUint32(0, get(this.pool.slot(sid), offset), true); return view.getFloat32(0, true); };
        return {
            center: (parent) => { this.event('C', [parent]); return [floatAt(parent, 14), floatAt(parent, 18)]; },
            bolt: (parent, from, to, time) => { this.event('B', [parent, from[0] >>> 0, from[1] >>> 0, to[0] >>> 0, to[1] >>> 0, floatToBits(time)]); this.mutate('B'); },
            soundAt: (x, y, name) => { assert.equal(name, 'thunderCrack.wav'); this.event('S', [floatToBits(x), floatToBits(y)]); this.mutate('S'); },
            typeWord: (type, offset) => { assert.ok(type < this.types.length && (offset === 0x6c || offset === 0x7c)); return offset === 0x6c ? this.effects[type] : this.input[COLUMN.sound]; },
            allocate: (bytes) => { this.event('A', [bytes]); this.mutate('A'); return this.input[COLUMN.allocate] ? 1 : 0; },
            create: (token, parent, value, first, second) => { this.event('N', [token, parent, value, first, second]); this.mutate('N'); return this.input[COLUMN.create] ? 1 : 0; },
            duration: (token, value) => { this.event('T', [token, floatToBits(value)]); },
            sound: (value) => { this.event('P', [value]); this.mutate('P'); },
            checkCarrier: (parent) => { this.event('H', [parent]); this.mutate('H'); },
            genusAt: (x, y, genus) => { this.event('G', [floatToBits(x), floatToBits(y), genus]); this.mutate('G'); return this.input[COLUMN.genus] ? FORM : 0; },
            typeAt: (x, y, type) => { this.event('I', [floatToBits(x), floatToBits(y), type]); return this.input[COLUMN.typeAt] ? 61 : 0; },
            freeSound: (name) => { assert.equal(name, 'priestFree.wav'); this.event('F'); }
        };
    }

    // 같은 현재 전역을 읽는 getter 묶음이다. (2026-11-08 WB04-E74)
    state() {
        const scene = this;
        return {
            get savedGame() { return scene.saved; },
            get loadingDepth() { return scene.loading; },
            get bombBlocked() { return scene.blocked; },
            get localOwner() { return scene.local; },
            get priestType() { return scene.priestType; },
            get cameraX() { return scene.camera[0]; },
            get cameraY() { return scene.camera[1]; },
            get freeSoundBlockerType() { return scene.blockerType; }
        };
    }

    // 원문의 9 DWORD 를 계산 없이 현재 값으로 직렬화한다.
    snapshot() {
        return [this.saved ? 1 : 0, this.loading, this.blocked, this.local, this.types[123].flags2, this.types[158].flags2, this.types[168].flags2, this.camera[0] >>> 0, this.camera[1] >>> 0].join(',');
    }
}

test('CarrierContentNotice: 원본 10.78 1731행의 전체 효과 순서·재진입 뒤 현재 전역·부모/내용물 슬롯 50바이트 둘이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new NoticeScene();
    let count = 0;
    // 이전 호출의 상태를 새 행에 넘기지 않는다.
    for (const row of rows) {
        scene.prepare(row[0].split(',').map(Number));
        const notice = new RawCarrierContentNotice(scene.pool, scene.types, scene.state(), scene.hooks());
        assert.ok(notice.handles(PARENT));
        notice.notice(PARENT, FORM, scene.input[COLUMN.reason]);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[1], `행 ${count}: 입력 ${row[0]}`);
        assert.equal(scene.snapshot(), row[2], `행 ${count}: 입력 ${row[0]}`);
        assert.equal(hex(scene.pool.slot(PARENT)), row[3], `행 ${count}: 입력 ${row[0]}`);
        assert.equal(hex(scene.pool.slot(FORM)), row[4], `행 ${count}: 입력 ${row[0]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierContentNotice: 실제 파생 VT 와 읽지 않아야 할 분기, 선택된 필수 효과의 누락을 구별한다', () => {
    const scene = new NoticeScene();
    scene.prepare('0,0,0,1,1,123,0,0,32,4,1109450752,1112211456,1,1,0,0,1,2,0,4294967279,31'.split(',').map(Number));
    const empty = new RawCarrierContentNotice(scene.pool, scene.types, scene.state(), {});
    empty.notice(PARENT, FORM, 4);
    scene.saved = true;
    scene.types[123].flags2 = 0x100;
    empty.notice(PARENT, FORM, 1);
    scene.saved = false;
    scene.loading = 1;
    empty.notice(PARENT, FORM, 1);
    scene.loading = 0;
    assert.ok(throws(() => empty.notice(PARENT, FORM, 1)));
    put(scene.raw(PARENT), 0, 0x501dd0);
    assert.ok(!empty.handles(PARENT));
    put(scene.raw(PARENT), 0, 0xdeadbeef);
    assert.ok(throws(() => empty.handles(PARENT)));
    assert.ok(throws(() => new RawCarrierContentNotice(new SidPool(24000, { server: false }), scene.types.slice(1), scene.state(), {})));
    assert.ok(throws(() => new RawCarrierContentNotice({ edition: 'cd1072' }, scene.types, scene.state(), {})));
});
