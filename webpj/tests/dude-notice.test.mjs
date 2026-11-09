// Dude 충돌 반응 raw-dude-notice.js(RawDudeNotice·makeDudeNoticeHostHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다(cpppj DudeNoticeTests).
//  - dudenotice-x86.tsv originals 행: 종류(around/nearest/point)·상태 바이트·이전 task·기존 Regular·종속 Burning/경로 표식·거주 구역 구성·경로 결과·속도/프레임/시각 비트 입력과,
//    반환·조회/생성/부착/Burning/경로/프레임 호출 순서·슬롯 50바이트·최종 Regular(시각 비트,event,payload 비트,count)·속도 비트·진입점 8바이트 출력. originalCD 행은 범위 밖이다.
//  - 장면은 실제 Kernel/SquidProcessHost 의 Regular 와 RawSquidFinder 를 쓰고 Burning/경로/프레임은 원본과 같은 관찰 경계로 둔다(cpppj DudeScene, E76 dude-task 장면과 같은 방식).
//  - 계약: DudeNotice_MissingBurningAndPathFactoriesRejected(미복원 생성 경계를 묵시 성공으로 바꾸지 않음)와 풀/판본 혼합 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E207: Dude 충돌 반응 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { RawDudeNotice, createDudeNoticeState, makeDudeNoticeHostHooks } from '../www/core/raw-dude-notice.js';

// 원본 관측 10.78 행이다. (2026-11-09 WB04-E207)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dudenotice-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 원본 관찰 SID 이다. (2026-11-09 WB04-E207)
const SOURCE = 50;

// 예외가 나는지 본다. (2026-11-09 WB04-E207)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-09 WB04-E207)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// double 의 64비트 패턴을 10진 문자열로 읽고 쓰는 공용 DataView 이다. (2026-11-09 WB04-E207)
const view = new DataView(new ArrayBuffer(8));
const doubleToBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };
const bitsToDouble = (text) => { view.setBigUint64(0, BigInt(text), true); return view.getFloat64(0, true); };

// 내장 Form 네 생성자 주소와 Dude 70/거주 구역 89 의 genus·발자국 입력을 넣은 188칸 TYPE 표이다(cpppj DudeTypes). (2026-11-09 WB04-E207)
function dudeTypes() {
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    Object.assign(types[70], { flags2: 0x400, footX: 1, footY: 1 });
    Object.assign(types[89], { flags2: 0x40000000, footX: 3, footY: 3 });
    return types;
}

// 같은 raw/현재 타입/실제 Regular 와 명시 Burning/경로 요청 경계를 소유하는 장면이다(cpppj DudeScene). 기대값은 계산하지 않는다. (2026-11-09 WB04-E207)
class DudeScene {
    constructor(row) {
        assert.equal(row.length, 18);
        this.row = row;
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        const spots = new Uint8Array(65536);
        this.types = dudeTypes();
        this.frames = this.types.map(() => new FrameCodeTable([]));
        this.speeds = this.types.map(() => ({ speed: 0 }));
        this.residences = new SquidPostPopList();
        const unpop = new SquidUnpop(this.pool, this.hash, spots);
        const destroy = new RawSquidDestroy(this.pool, unpop, this.types);
        this.clock = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, this.types, new Kernel(16), destroy, this.clock, { regular: () => 1 });
        this.events = [];
        // 같은 관찰 번호 앞의 슬롯만 입력 배치로 확보한다.
        for (let id = 5; id <= 82; id += 1) assert.equal(this.pool.allocate(2), id);
        this.frames[70] = new FrameCodeTable([{ side: 0x41, variant: 0x50, flags: 0 }, { side: 0x49, variant: 0x50, flags: 0 }]);
        this.speeds[70].speed = bitsToFloat(Number(row[9]));
        // 현재 source 와 두 거주 구역의 raw 입력만 공급한다.
        for (const [sid, type, x, y, owner] of [[50, 70, 20.75, 21.9, 1], [60, 89, 24.75, 24.5, Number(row[6]) === 2 ? 2 : 1], [61, 89, 16.75, 19.3, 1]]) {
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            raw[10] = type;
            raw[34] = owner;
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            put(raw, 12, sid === SOURCE ? Number(row[3]) : 0, 2);
            put(raw, 36, sid === SOURCE ? Number(row[10]) : 0);
        }
        let head = 0;
        if (Number(row[7])) { this.marker(78, 24, 0); head = 78; }
        if (Number(row[5])) { this.marker(77, 14, head); head = 77; }
        put(this.pool.allocated(SOURCE), 6, head, 2);
        this.hash.setBucket(1, 20.75, 21.9, SOURCE);
        if (Number(row[4])) {
            this.clock.now = 20;
            const regular = this.host.addRegular(SOURCE, 0x5104, 1, 0x50);
            assert.ok(regular && regular.form === 83);
            // 기존 count7/time27 은 기대값 대입 없이 실제 Regular 의 7회 반복으로 준비한다.
            for (let count = 0; count < 7; count += 1) {
                this.clock.now = regular.time;
                regular.runFrame();
            }
            assert.ok(regular.count === 7 && regular.time === 27 && regular.payload === 1);
        }
        this.pool.allocated(SOURCE)[11] = Number(row[2]);
        this.clock.now = bitsToDouble(row[11]);
        const mode = Number(row[6]);
        this.residences.entries = mode === 0 ? [] : mode === 3 ? [60, 61] : [60];
        this.residences.count = this.residences.entries.length;
        if (Number(row[1]) === 2) this.types[89].flags2 = [0, 0x40000000, 0x10000000, 0x40000][mode];
    }

    // 관찰된 기존 typed Form 또는 Burning 생성 경계의 체인 효과만 공급한다. (2026-11-09 WB04-E207)
    marker(sid, type, next) {
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        // cpppj 장면은 가상 표 0 을 남겨 d678fcad 뒤 엄격한 공통 링크 선택에서 실패한다. 실제 Burning(14)/경로(24)는 Process Form 이므로 그 가상 표를 준다(웹 장면 교정, 단위 문서 2절).
        put(raw, 0, SQUID_FORM_VTABLES.process);
        raw[10] = type;
        raw[11] = 8;
        put(raw, 4, next, 2);
    }

    // 실제 Host Regular 만 연결하고 Burning/경로/Frame 은 native 와 같은 명시 관찰 경계다. (2026-11-09 WB04-E207)
    hooks() {
        const base = makeDudeNoticeHostHooks(this.pool, this.host);
        const row = this.row;
        return {
            taskRegular: (sid) => { this.events.push('R:50:65520:20736'); return base.taskRegular(sid); },
            addTaskRegular: (sid) => {
                this.events.push('N:40');
                const regular = base.addTaskRegular(sid);
                this.events.push('B:46:50:0:80');
                return regular;
            },
            createBurning: () => {
                this.events.push('N:144', 'D:50:0');
                assert.equal(this.pool.allocate(2), 84);
                this.marker(84, 14, get(this.pool.slot(SOURCE), 6, 2));
                put(this.pool.allocated(SOURCE), 6, 84, 2);
            },
            pathEndpoint: () => (Number(row[7]) === 1 ? [25, 22] : [100, 100]),
            startPath: (sid, x, y) => {
                // Patch 8개 실제 명령 인자만 직렬화하며 임의 reserved 값을 추가하지 않는다.
                this.events.push(`P:${sid},${floatToBits(x)},${floatToBits(y)},0,0,0,0,0:${floatToBits(this.speeds[70].speed)}`);
                return Number(row[8]) !== 0;
            },
            setFrame: (sid, frame, flags) => {
                this.events.push(`F:50:${frame}:${flags}`);
                put(this.pool.allocated(sid), 36, frame >>> 0);
            },
        };
    }

    // 현재 처리기를 실행한 뒤의 숫자/순서/raw/실제 Regular 상태를 원본 출력 순서로 직렬화한다. (2026-11-09 WB04-E207)
    run() {
        const logic = new RawDudeNotice(this.pool, this.hash, this.types, this.frames, this.speeds, this.residences, createDudeNoticeState(this.clock), this.hooks());
        let result = 'return';
        const point = new Uint8Array(8);
        switch (Number(this.row[1])) {
            case 0: logic.around(Math.fround(20.75), Math.fround(21.9)); break;
            case 1: result = String(logic.nearestResidence(SOURCE)); break;
            case 2: {
                const [x, y] = logic.residencePoint(60);
                put(point, 0, floatToBits(x));
                put(point, 4, floatToBits(y));
                break;
            }
            default: throw new Error('Dude notice fixture 종류 오류');
        }
        const current = this.host.findEventMasked(SOURCE, 0xfff0, 0x5100);
        const regular = current ? `${doubleToBits(current.time)},${current.event},${floatToBits(current.payload)},${current.count}` : '-';
        return [result, this.events.join(';'), hex(this.pool.slot(SOURCE)), regular, String(floatToBits(this.speeds[70].speed)), hex(point)];
    }
}

test('DudeNotice: 원본 10.78 의 notice/Regular 쓰기/nearest/표적 진입점과 대피 호출이 같다', { skip: rows.length === 0 && 'dudenotice-x86.tsv 없음' }, () => {
    let count = 0;
    // 원본 출력은 마지막 6칸만 쓰며 입력에서 기대값을 계산하지 않는다.
    for (const row of rows) {
        const output = new DudeScene(row).run();
        assert.deepEqual(output, row.slice(12), `행 ${count} 종류 ${row[1]}`);
        count += 1;
    }
    assert.ok(count > 0);
    assert.equal(count, rows.length);
});

test('DudeNotice: 실제 Burning/Path 생성이 없는 호출을 묵시 성공으로 바꾸지 않는다', { skip: rows.length === 0 && 'dudenotice-x86.tsv 없음' }, () => {
    const scene = new DudeScene(rows[0]);
    const hooks = scene.hooks();
    hooks.createBurning = null;
    hooks.startPath = null;
    const logic = new RawDudeNotice(scene.pool, scene.hash, scene.types, scene.frames, scene.speeds, scene.residences, createDudeNoticeState(scene.clock), hooks);
    assert.ok(throws(() => logic.notice(SOURCE)));
    assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 0x5107);
    assert.ok(scene.host.findEvent(SOURCE, 0x5107));
    assert.ok(throws(() => logic.move(SOURCE, [24.5, 21.5])));
});

test('DudeNotice: 판본/배열 크기·Host 풀 혼합을 거부한다(웹 계약 검사)', { skip: rows.length === 0 && 'dudenotice-x86.tsv 없음' }, () => {
    const scene = new DudeScene(rows[0]);
    const state = createDudeNoticeState(scene.clock);
    assert.ok(throws(() => new RawDudeNotice(scene.pool, scene.hash, scene.types.slice(0, 171), scene.frames, scene.speeds, scene.residences, state), TypeError));
    assert.ok(throws(() => new RawDudeNotice({ edition: 'cd1072' }, scene.hash, scene.types, scene.frames, scene.speeds, scene.residences, state), TypeError));
    assert.ok(throws(() => makeDudeNoticeHostHooks(new SidPool(24000), scene.host), TypeError));
    // 해제/contained 슬롯과 SID 범위 밖은 자산 계약 밖이다.
    const logic = new RawDudeNotice(scene.pool, scene.hash, scene.types, scene.frames, scene.speeds, scene.residences, state, scene.hooks());
    assert.ok(throws(() => logic.nearestResidence(4), RangeError));
    assert.ok(throws(() => logic.nearestResidence(24000), RangeError));
});
