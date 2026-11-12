// 맨 오 워 풀(레인 아비어리) raw-rain-aviary-process.js(RainAviaryProcessFields·RawRainAviaryProcess·RainAviaryProcess)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: rain-aviary-x86.tsv 1,602행 — 생성자 C 18행(미정 바이트를 포함한 모든 쓰기), 후처리 P 386행(live/dead/free 후보와 개별 할당·fort/친군 사격 관문·부착 이미지), 프레임 F 174행(현재 효과 이후 읽기와 실제 AP 상태 경계),
//    상태 writer W 1,024행(전체 dirty/부모/force 와 미사용 버퍼)이 필드 48바이트·객체 250바이트·전역 28바이트·패킷 64바이트·호출 순서까지 같고, rain-home-notice-x86.tsv 36행(홈 가상 34 통지의 서버 관문·시각 저장·게임 난수 소비·
//    정수 초 나눗셈·Form 갱신)이 필드 48바이트·난수 상태·갱신 요청 이미지까지 같다. cpppj RainAviaryProcessTests.cpp 의 AviaryScene 과 RainHomeNoticeTests.cpp 를 같은 순서로 옮겼다.
//  - cpppj 시험 이식: 선택 누락과 부분 쓰기 보존(RainAviary_RequiredBoundaries).
//  - 웹 계약: Kernel 래퍼의 프레임/Form·부모 기록/writePacket, 상태 패킷의 쓰기 전 길이 경계, 프레임 상태별 효과, 홈 통지의 읽기 순서·부분 쓰기.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E354: 7-P1 RawRainAviaryProcess 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { GameRandom } from '../www/core/random.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { findAttachedProcess } from '../www/core/raw-sun-aviary-process.js';
import { RainAviaryProcess, RainAviaryProcessFields, RawRainAviaryProcess } from '../www/core/raw-rain-aviary-process.js';

// 원본 관측 행이다(레인 아비어리 파일의 첫 행은 열 이름, 홈 통지 파일은 # 주석). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E354)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = (() => {
    const all = readRows(path.join(fixtures, 'rain-aviary-x86.tsv'));
    return all ? all.slice(1) : null;
})();
const noticeRows = readRows(path.join(fixtures, 'rain-home-notice-x86.tsv'));

// 두 부모/플라이어/종속 Form 입력의 실제 SID 와 native 관찰의 외부 가상 표 기록값이다. (2026-11-13 WB04-E354)
const IDS = [50, 51, 60, 70, 71];
const INPUT_VTABLE = 0x1240f000;

// 예외가 나는지 본다. (2026-11-13 WB04-E354)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E354)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문과 같은 현재 입력만 준비하고 결과는 제품 몸체가 기록하게 하는 장면이다(cpppj AviaryScene). (2026-11-13 WB04-E354)
class AviaryScene {
    constructor(kind, input) {
        this.kind = kind;
        this.pool = new SidPool(24000, { server: false });
        this.fields = new RainAviaryProcessFields();
        this.fort = false;
        this.friendlyFire = false;
        this.server = true;
        this.checking = false;
        this.editor = 0;
        this.processType = 60;
        this.flyerType = 144;
        this.now = 100;
        this.mutation = 0;
        this.frameCalls = 0;
        this.allocated = true;
        this.trace = [];
        this.attached = null;
        // 대상 71까지 실제 슬롯을 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 71) break;
            if (!sid || sid > 71) throw new Error('아비어리 검사 SID 준비 오류');
        }
        // 원문과 같은 다섯 50바이트 입력을 준비한다.
        for (const value of IDS) {
            const raw = this.pool.allocated(value);
            raw.fill(0xa5);
            put(raw, 0, INPUT_VTABLE);
            put(raw, 4, value === 70 ? 71 : 0, 2);
            put(raw, 6, value === 50 || value === 51 ? 70 : 0, 2);
            put(raw, 8, 127, 2);
            raw[10] = value === 50 || value === 51 ? 143 : value === 60 ? 144 : value === 70 ? 60 : 31;
            raw[11] = 0;
            put(raw, 12, value === 50 ? 150 : value === 51 ? 151 : value === 60 ? 9 : 0, 2);
            put(raw, 14, floatToBits(value + 0.25));
            put(raw, 18, floatToBits(value + 0.5));
            raw[34] = value === 50 ? 1 : value === 51 ? 2 : 0;
            put(raw, 36, value === 51 ? 7 : value === 60 ? 42 : 0);
            raw[40] = 0;
        }
        this.fields.bytes.fill(0xa5);
        put(this.fields.bytes, 0, 0x509e20);
        this.fields.bytes[8] = 0x10;
        this.timeAt(100);
        put(this.fields.bytes, 24, 1);
        put(this.fields.bytes, 32, 0x509e00);
        put(this.fields.bytes, 36, 77);
        put(this.fields.bytes, 40, 50);
        put(this.fields.bytes, 44, 0);
        if (kind === 'C') {
            this.server = Number(input[0]) !== 0;
            this.now = [0, 100.25, -5.25][Number(input[1])];
            this.fields.bytes.fill(Number(input[2]) & 0xff);
        } else if (kind === 'P') {
            this.server = Number(input[0]) !== 0;
            this.fort = Number(input[1]) !== 0;
            this.friendlyFire = Number(input[2]) !== 0;
            this.allocated = Number(input[4]) !== 0;
            this.mutation = Number(input[6]);
            const layout = Number(input[5]);
            if (!layout) put(this.pool.allocated(50), 6, 0, 2);
            if (layout === 2) {
                this.pool.allocated(70)[11] = 2;
                this.pool.allocated(71)[10] = 60;
            }
            if (layout === 3) this.pool.allocated(70)[11] = 1;
        } else if (kind === 'F') {
            this.server = Number(input[0]) !== 0;
            this.editor = Number(input[1]);
            this.pool.allocated(50)[40] = Number(input[2]) & 0xff;
            put(this.fields.bytes, 24, Number(input[3]));
            put(this.pool.allocated(50), 36, Number.parseInt(input[4], 10) >>> 0);
            put(this.fields.bytes, 44, Number(input[5]));
            this.timeAt([99, 100, 101][Number(input[6])]);
            this.mutation = Number(input[7]);
        } else if (kind === 'W') {
            this.fields.bytes[8] = Number(input[0]) & 0xff;
            this.timeAt(123.25);
            put(this.fields.bytes, 24, 7);
            put(this.fields.bytes, 44, 5);
        }
        this.body = new RawRainAviaryProcess(this.pool, this.globals(), this.hooks());
    }

    // 같은 현재 전역을 매번 읽는 함수 묶음이다. (2026-11-13 WB04-E354)
    globals() {
        return {
            fort: () => this.fort,
            friendlyFire: () => (this.friendlyFire ? 1 : 0),
            server: () => this.server,
            editor: () => this.editor,
            checking: () => (this.checking ? 1 : 0),
            now: () => this.now,
            processType: () => this.processType,
            flyerType: () => this.flyerType,
        };
    }

    // raw double 을 조립한다. (2026-11-13 WB04-E354)
    timeAt(value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(this.fields.bytes, 16, view.getUint32(0, true));
        put(this.fields.bytes, 20, view.getUint32(4, true));
    }

    // 실제 선택된 호출 순서이다. (2026-11-13 WB04-E354)
    event(value) { this.trace.push(value); }

    // 반환값/변경 입력만 공급하고 제품의 다음 상태나 최종 패킷을 계산하지 않는다. (2026-11-13 WB04-E354)
    hooks() {
        return {
            findProcess: (sid, type) => {
                this.event(`Find,${sid},${type}`);
                if (this.kind === 'P' && this.mutation === 1) this.server = false;
                if (this.kind === 'P' && this.mutation === 2) this.processType = 59;
                return findAttachedProcess(this.pool, sid, type);
            },
            allocate: (size) => {
                this.event(`Allocate,${size}`);
                if (!this.allocated) return null;
                const value = new RainAviaryProcessFields();
                value.bytes.fill(0xa5);
                return value;
            },
            attach: (value, sid, type, flags) => {
                this.event(`Attach,${type},${sid},${flags},${value ? '1' : '0'}`);
                this.attached = value;
            },
            damageable: (sid, flags) => { this.event(`Base,${flags}`); },
            assertion: (line) => { this.event(`Assert,${line}`); },
            setFrame: (sid, frame) => {
                this.event(`Frame,${sid},${frame}`);
                put(this.pool.allocated(sid), 36, frame >>> 0);
                this.frameCalls += 1;
                if (this.frameCalls === 1 && this.mutation === 1) {
                    put(this.fields.bytes, 40, 51);
                    this.now = 200;
                }
                if (this.frameCalls === 1 && this.mutation === 5) put(this.fields.bytes, 24, 9);
            },
            sound: (x, y, name) => {
                assert.equal(name, 'manOWarBuild.wav');
                this.event(`Sound,${floatToBits(x)},${floatToBits(y)}`);
                if (this.mutation === 2) {
                    put(this.fields.bytes, 24, 9);
                    put(this.fields.bytes, 44, 10);
                }
            },
            checkFrame: (sid, frame, line) => { this.event(`Check,${frame},${line}`); },
            create: (type, flags) => { this.event(`Create,${type},${flags}`); return 60; },
            setOwner: (sid, owner) => {
                this.event(`Owner,${sid},${owner}`);
                this.pool.allocated(sid)[34] = owner & 0xff;
                if (this.mutation === 3) {
                    put(this.fields.bytes, 40, 51);
                    this.now = 200;
                }
            },
            pop: (sid, x, y, flags) => {
                this.event(`Pop,${sid},${floatToBits(x)},${floatToBits(y)},${flags}`);
                if (this.mutation === 4) {
                    put(this.fields.bytes, 40, 51);
                    this.now = 200;
                }
            },
            bindFlyer: (flyer, sid) => { this.event(`Bind,${flyer},${sid}`); },
        };
    }

    // 다섯 전체 기록을 원문 순서대로 읽는다. (2026-11-13 WB04-E354)
    objects() { return IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 현재 전역 28바이트이다. (2026-11-13 WB04-E354)
    currentGlobals() {
        const raw = new Uint8Array(28);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.now, true);
        put(raw, 0, this.server ? 1 : 0);
        put(raw, 4, this.editor);
        put(raw, 8, view.getUint32(0, true));
        put(raw, 12, view.getUint32(4, true));
        put(raw, 16, this.processType);
        put(raw, 20, this.fort ? 1 : 0);
        put(raw, 24, this.friendlyFire ? 1 : 0);
        return hex(raw);
    }
}

// 범위를 나누어 한 실패가 다른 원문 검사 행을 감추지 않게 한다. (2026-11-13 WB04-E354)
function compareScope(kind, expected) {
    let compared = 0;
    // TSV 헤더 이외의 해당 원문 입력만 독립적으로 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        if (row[0] !== kind) continue;
        compared += 1;
        const input = row[1].split(',');
        const scene = new AviaryScene(kind, input);
        let objects = '-';
        let packet = '-';
        let result = '-';
        if (kind === 'C') scene.body.initialize(scene.fields);
        else if (kind === 'P') scene.body.postPop(50, Number(input[3]));
        else if (kind === 'F') scene.body.frame(scene.fields);
        else {
            const bytes = new Uint8Array(64).fill(0xa7);
            result = String(RawRainAviaryProcess.writeProcess(scene.fields, bytes, Number(input[1]) !== 0, Number(input[2]) !== 0));
            packet = hex(bytes);
        }
        const image = kind === 'P' ? (scene.attached ? hex(scene.attached.bytes) : '-') : hex(scene.fields.bytes);
        if (kind === 'P' || kind === 'F') objects = scene.objects();
        const trace = scene.trace.length === 0 ? '-' : scene.trace.join(';');
        const where = `${kind}/${row[1]}`;
        assert.equal(image, row[2], `${where} 필드`);
        assert.equal(objects, row[3], `${where} 객체`);
        assert.equal(scene.currentGlobals(), row[4], `${where} 전역`);
        assert.equal(packet, row[5], `${where} 패킷`);
        assert.equal(result, row[6], `${where} 길이`);
        assert.equal(trace, row[7], `${where} 호출 순서`);
    }
    assert.equal(compared, expected);
}

test('RainAviary: 원본 10.78 생성자 18행 — 미정 바이트를 포함한 생성자의 모든 쓰기가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('C', 18); });
test('RainAviary: 원본 10.78 후처리 386행 — live/dead/free 후보와 개별 할당·fort/친군 사격 관문이 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('P', 386); });
test('RainAviary: 원본 10.78 프레임 174행 — 현재 효과 이후 읽기와 실제 AP 상태 경계가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('F', 174); });
test('RainAviary: 원본 10.78 상태 writer 1,024행 — 전체 dirty/부모/force 와 미사용 버퍼가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('W', 1024); });

test('RainAviary: 원본 10.78 홈 통지 36행 — 서버 관문·시각 저장·게임 난수 소비·정수 초 나눗셈·Form 갱신 요청이 같다', { skip: !noticeRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: false });
    let server = false;
    let now = 0;
    const logic = new RawRainAviaryProcess(pool, {
        fort: () => false, friendlyFire: () => 0, server: () => server, editor: () => 0, checking: () => 0, now: () => now, processType: () => 60, flyerType: () => 144,
    }, {});
    let count = 0;
    // 36개 실제 입력에서 전체 필드와 두 정밀도 공통 원문을 대조한다.
    for (const row of noticeRows) {
        assert.equal(row.length, 8);
        server = Number(row[0]) !== 0;
        const random = new GameRandom(Number(row[1]));
        const view = new DataView(new ArrayBuffer(8));
        view.setBigUint64(0, BigInt(row[2]), true);
        now = view.getFloat64(0, true);
        const before = Buffer.from(row[4], 'hex');
        assert.equal(before.length, 48);
        const fields = new RainAviaryProcessFields();
        fields.bytes.set(before);
        let refreshed = 0;
        let event = '';
        logic.flyerDied(fields, random, (current) => {
            assert.ok(current === fields && current.form === 80 && current.parent === 50);
            event = `R:${hex(current.bytes)}`;
            refreshed += 1;
        });
        assert.equal(hex(fields.bytes), row[5], `${row[3]} 필드`);
        assert.equal(random.state, Number(row[6]), `${row[3]} 난수 상태`);
        assert.equal(refreshed, server ? 1 : 0);
        if (server) {
            const offset = row[7].indexOf(';R:');
            assert.notEqual(offset, -1);
            assert.equal(event, row[7].slice(offset + 1));
        } else assert.ok(row[7] === '-' && event === '');
        count += 1;
    }
    assert.equal(count, 36);
});

test('RainAviary: 선택 누락은 앞선 쓰기를 남기고 거부하며 미선택 하위는 요구하지 않고 개발자 진단·짧은 패킷은 쓰기 전에 거부한다', () => {
    const scene = new AviaryScene('P', '1,0,0,1,1,0,0'.split(','));
    const hooks = scene.hooks();
    delete hooks.allocate;
    const missing = new RawRainAviaryProcess(scene.pool, scene.globals(), hooks);
    assert.ok(throws(() => missing.postPop(50, 1)));
    assert.ok(!scene.attached && scene.trace.join(';') === 'Find,50,60');
    scene.server = false;
    scene.trace.length = 0;
    missing.postPop(50, 1);
    assert.equal(scene.trace.join(';'), 'Base,1');
    const frame = new AviaryScene('F', '1,0,0,2,11,0,1,0'.split(','));
    frame.checking = true;
    const frameHooks = frame.hooks();
    delete frameHooks.checkFrame;
    const diagnostic = new RawRainAviaryProcess(frame.pool, frame.globals(), frameHooks);
    const before = Uint8Array.from(frame.fields.bytes);
    assert.ok(throws(() => diagnostic.frame(frame.fields)));
    assert.deepEqual(frame.fields.bytes, before);
    assert.equal(frame.trace.length, 0);
    assert.ok(throws(() => RawRainAviaryProcess.writeProcess(frame.fields, new Uint8Array(8), true)));
    assert.deepEqual(frame.fields.bytes, before);
});

test('RainAviary: 생성자는 PATCH 풀과 현재 전역 조회기 여덟 개를 요구하고 pool 접근자는 같은 풀을 돌려준다', () => {
    const scene = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
    assert.equal(scene.body.pool, scene.pool);
    // 풀 판이 다르거나 풀이 없으면 거부한다.
    assert.throws(() => new RawRainAviaryProcess({ edition: 'cd1072' }, scene.globals(), {}), /PATCH/);
    assert.throws(() => new RawRainAviaryProcess(null, scene.globals(), {}), TypeError);
    // 전역은 값이 아니라 조회 함수여야 하며 빠진 이름을 알려 준다.
    for (const name of ['fort', 'friendlyFire', 'server', 'editor', 'checking', 'now', 'processType', 'flyerType']) {
        const globals = scene.globals();
        delete globals[name];
        assert.throws(() => new RawRainAviaryProcess(scene.pool, globals, {}), (error) => error instanceof TypeError && error.message.endsWith(name), name);
        globals[name] = 5;
        assert.throws(() => new RawRainAviaryProcess(scene.pool, globals, {}), TypeError, `${name} 값`);
    }
    assert.throws(() => new RawRainAviaryProcess(scene.pool, null, {}), TypeError);
    // 훅 묶음을 생략해도 만들 수 있고 도달한 위치에서만 요구한다.
    assert.doesNotThrow(() => new RawRainAviaryProcess(scene.pool, scene.globals()));
});

test('RainAviary: 훅 누락은 원인을 알려 주는 오류 문구로 거부한다(일반 호출 오류와 구별)', () => {
    const scene = new AviaryScene('P', '1,0,0,1,1,0,0'.split(','));
    const hooks = scene.hooks();
    delete hooks.allocate;
    assert.throws(() => new RawRainAviaryProcess(scene.pool, scene.globals(), hooks).postPop(50, 1), (error) => error.message === '맨 오 워 풀 실제 하위 소비자 미연결');
    // 훅 묶음을 아예 생략해도 같은 문구로 거부한다(내부 빈 묶음으로 대체).
    assert.throws(() => new RawRainAviaryProcess(scene.pool, scene.globals()).postPop(50, 1), (error) => error.message === '맨 오 워 풀 실제 하위 소비자 미연결');
});

test('RainAviary 후처리: 확보 실패는 undefined 여도 NULL 부착 요청으로 정규화하고 초기화하지 않는다', () => {
    const scene = new AviaryScene('P', ['1', '0', '0', '1', '1', '0', '0']);
    const hooks = scene.hooks();
    const received = [];
    hooks.allocate = () => undefined;
    hooks.attach = (value, sid, type, flags) => { received.push([value, sid, type, flags]); };
    new RawRainAviaryProcess(scene.pool, scene.globals(), hooks).postPop(50, 1);
    assert.deepEqual(received, [[null, 50, 60, 8]]);
});

test('RainAviary 후처리: 서버·fort/친군 사격·flags 관문을 원문 순서로 읽고 부착 때 processType 을 다시 읽는다', () => {
    // [server, fort, friendlyFire, flags, 기대 호출] — fort 이면서 친군 사격이 아니면 편집 모드 fort 는 프로세스를 만들지 않는다.
    for (const [server, fort, friendly, flags, expected] of [
        [true, false, false, 1, 'Find,50,60;Allocate,48;Attach,60,50,8,1;Base,1'],
        [true, true, false, 1, 'Base,1'],
        [true, true, true, 1, 'Find,50,60;Allocate,48;Attach,60,50,8,1;Base,1'],
        [false, false, false, 1, 'Base,1'],
        [true, false, false, 2, 'Base,2'],
        [true, false, false, 0, 'Base,0'],
    ]) {
        const scene = new AviaryScene('P', ['1', '0', '0', '1', '1', '1', '0']);
        scene.server = server;
        scene.fort = fort;
        scene.friendlyFire = friendly;
        // 서로 다른 입력 객체이므로 pool 의 +6(종속 WORD)은 70 으로 이미 살아 있는 체인이 없게 비운다.
        put(scene.pool.allocated(50), 6, 0, 2);
        scene.body.postPop(50, flags);
        assert.equal(scene.trace.join(';'), expected, `server ${server} fort ${fort} friendly ${friendly} flags ${flags}`);
    }
    // 이미 같은 타입 프로세스가 있으면 중복 진단 0x257 을 내고도 계속 새로 만든다.
    const dup = new AviaryScene('P', ['1', '0', '0', '1', '1', '1', '0']);
    dup.body.postPop(50, 1);
    assert.deepEqual(dup.trace, ['Find,50,60', 'Assert,599', 'Allocate,48', 'Attach,60,50,8,1', 'Base,1']);
});

test('RainAviary 프레임: 상태별 효과 — 정지/편집 차단은 무동작, 상태 2 의 전환·상태 6 의 서버/비서버 경로·상태 10 의 대기', () => {
    // 정지(+40 의 0x20)·편집 차단은 모든 상태에서 무동작이다.
    for (const [guard, editor] of [[0x20, 0], [0, 1]]) {
        const scene = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
        scene.pool.allocated(50)[40] = guard;
        scene.editor = editor;
        const before = Uint8Array.from(scene.fields.bytes);
        scene.body.frame(scene.fields);
        assert.deepEqual(scene.fields.bytes, before);
        assert.equal(scene.trace.length, 0);
    }
    // 상태 1: 홈 프레임 0·AP 2·소리.
    const start = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
    start.body.frame(start.fields);
    assert.deepEqual(start.trace, ['Frame,50,0', `Sound,${floatToBits(50.25)},${floatToBits(50.5)}`]);
    assert.equal(start.fields.stage, 2);
    // 상태 2: 프레임 11 초과면 6, 기한 전이면 유지, 기한이면 프레임 +1.
    const over = new AviaryScene('F', '1,0,0,2,12,0,1,0'.split(','));
    over.body.frame(over.fields);
    assert.equal(over.fields.stage, 6);
    assert.equal(over.trace.length, 0);
    const wait = new AviaryScene('F', '1,0,0,2,5,0,2,0'.split(','));
    wait.body.frame(wait.fields);
    assert.equal(wait.fields.stage, 2);
    assert.equal(wait.trace.length, 0);
    const step = new AviaryScene('F', '1,0,0,2,11,0,1,0'.split(','));
    step.body.frame(step.fields);
    assert.deepEqual(step.trace, ['Frame,50,12']);
    // 상태 6: 서버이면 플라이어를 만들어 연결하고, 비서버도 AP 5·홈 프레임 0·3초 대기를 한다.
    const server = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    server.body.frame(server.fields);
    assert.deepEqual(server.trace.slice(0, 2), ['Create,144,0', 'Owner,60,1']);
    assert.ok(server.trace.includes('Bind,60,50'));
    assert.deepEqual(server.trace.slice(-1), ['Frame,50,0']);
    assert.equal(server.fields.stage, 5);
    assert.equal(server.fields.time, 103);
    const client = new AviaryScene('F', '0,0,0,6,5,0,1,0'.split(','));
    client.body.frame(client.fields);
    assert.deepEqual(client.trace, ['Frame,50,0']);
    assert.equal(client.fields.stage, 5);
    assert.equal(client.fields.time, 103);
    // 상태 10: 기한이면 AP 2 로 돌아가고 아니면 유지한다.
    const ready = new AviaryScene('F', '1,0,0,10,5,0,0,0'.split(','));
    ready.body.frame(ready.fields);
    assert.equal(ready.fields.stage, 2);
    const hold = new AviaryScene('F', '1,0,0,10,5,0,2,0'.split(','));
    hold.body.frame(hold.fields);
    assert.equal(hold.fields.stage, 10);
    // 범위 밖 상태는 개발자 진단이 켜졌을 때만 0x820 을 낸다.
    for (const [checking, stage, diagnosed] of [[true, 0, true], [true, 11, true], [true, 3, false], [true, 9, false], [false, 11, false]]) {
        const scene = new AviaryScene('F', `1,0,0,${stage},5,0,1,0`.split(','));
        scene.checking = checking;
        scene.body.frame(scene.fields);
        assert.equal(scene.trace.includes('Assert,2080'), diagnosed, `상태 ${stage} checking ${checking}`);
    }
});

test('RainAviary 프레임: 첫 부모 포인터는 살아 있는 보기라 효과가 바꾼 좌표·소유자를 이후 읽기가 그대로 본다', () => {
    // 상태 6: 플라이어를 만드는 동안 부모 좌표가 (0.1, 0.1)·소유자가 3 으로 바뀌면 Pop 은 현재 값 − 0.5 / − 2 를 float 로 줄인 값이다
    // (0.1f 는 결과 지수보다 아래 비트를 가져 float 로 줄이는 단계가 값을 바꾼다 — Pop 훅이 받은 수를 그대로 비교한다).
    const launch = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    const hooks = launch.hooks();
    const create = hooks.create;
    const pop = hooks.pop;
    const popped = [];
    hooks.create = (type, flags) => {
        const sid = create(type, flags);
        put(launch.pool.allocated(50), 14, floatToBits(0.1));
        put(launch.pool.allocated(50), 18, floatToBits(0.1));
        launch.pool.allocated(50)[34] = 3;
        return sid;
    };
    hooks.pop = (sid, x, y, flags) => { popped.push([x, y]); pop(sid, x, y, flags); };
    new RawRainAviaryProcess(launch.pool, launch.globals(), hooks).frame(launch.fields);
    assert.ok(launch.trace.includes('Owner,60,3'));
    const expectedX = Math.fround(Math.fround(0.1) - 0.5);
    const expectedY = Math.fround(Math.fround(0.1) - 2);
    assert.deepEqual(popped, [[expectedX, expectedY]]);
    assert.ok(launch.trace.includes(`Pop,60,${floatToBits(expectedX)},${floatToBits(expectedY)},0`));
    // 상태 1: 홈 프레임을 쓰는 동안 좌표가 바뀌면 소리는 현재 좌표에서 난다.
    const start = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
    const startHooks = start.hooks();
    const setFrame = startHooks.setFrame;
    startHooks.setFrame = (sid, frame) => {
        setFrame(sid, frame);
        put(start.pool.allocated(50), 14, floatToBits(9.5));
        put(start.pool.allocated(50), 18, floatToBits(7.25));
    };
    new RawRainAviaryProcess(start.pool, start.globals(), startHooks).frame(start.fields);
    assert.ok(start.trace.includes(`Sound,${floatToBits(9.5)},${floatToBits(7.25)}`));
});

test('RainAviary 프레임: 개발자 진단이 켜지면 프레임을 바꾸기 전에 새 프레임과 줄 번호(0x7fa)를 검사한다', () => {
    const up = new AviaryScene('F', '1,0,0,2,5,0,0,0'.split(','));
    up.checking = true;
    up.body.frame(up.fields);
    assert.deepEqual(up.trace, ['Check,6,2042', 'Frame,50,6']);
});

test('RainAviary 프레임: 상태 6 에서 부모의 종속 WORD(+6)가 0 이면 소유자 지정 뒤 진단 0xa4 를 내고 계속 연결한다', () => {
    const zero = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    put(zero.pool.allocated(50), 6, 0, 2);
    zero.body.frame(zero.fields);
    assert.deepEqual(zero.trace.slice(0, 3), ['Create,144,0', 'Owner,60,1', 'Assert,164']);
    assert.equal(get(zero.pool.slot(50), 12, 2), 60);
    assert.equal(zero.fields.stage, 5);
    // +6 이 이미 있으면 진단을 내지 않는다.
    const linked = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    linked.body.frame(linked.fields);
    assert.ok(!linked.trace.includes('Assert,164'));
});

test('RainAviary 홈 통지: 서버가 아니면 아무것도 하지 않고, 읽기 순서(AP 10 → 첫 시각 → 난수 → 둘째 시각 → 갱신)와 정수 초를 지킨다', () => {
    // 서버가 아니면 필드·난수·시각을 건드리지 않는다.
    const idle = new AviaryScene('F', '0,0,0,1,5,0,1,0'.split(','));
    const idleBefore = Uint8Array.from(idle.fields.bytes);
    const idleRandom = new GameRandom(55);
    new RawRainAviaryProcess(idle.pool, idle.globals(), {}).flyerDied(idle.fields, idleRandom, () => { throw new Error('호출되면 안 됨'); });
    assert.deepEqual(idle.fields.bytes, idleBefore);
    assert.equal(idleRandom.state, 55);
    // 서버: 시각 조회가 두 번이고 둘째는 난수 소비 뒤이며, 정수 초 = floor((난수(5000) + 10000) / 1000) 이다.
    const scene = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
    const sequence = [];
    const times = [100.25, 200.5];
    const random = new GameRandom(55);
    const probe = new GameRandom(55);
    const expectedSeconds = Math.floor((probe.next(5000) + 10000) / 1000);
    const dirtyBefore = scene.fields.bytes[8];
    RawRainAviaryProcess.flyerDiedIn(scene.fields, () => { sequence.push(`now:${random.state}`); return times.shift(); }, () => { sequence.push('random'); return random; }, (fields) => {
        sequence.push('refresh');
        assert.equal(fields.stage, 10);
        assert.equal(fields.dirty, dirtyBefore | 6);
    });
    assert.deepEqual(sequence, ['now:55', 'random', `now:${random.state}`, 'refresh']);
    assert.equal(scene.fields.time, 200.5 + expectedSeconds);
    assert.ok(expectedSeconds >= 10 && expectedSeconds <= 14);
    // 정수 초 나눗셈이므로 저장 시각의 소수부는 둘째 시각의 소수부와 같다.
    assert.equal((scene.fields.time * 2) % 1, 0);
});

test('RainAviary 홈 통지: 난수는 한계 5000 미만 값에 10000 을 더한 밀리초이고 정수 초는 1000 으로 나눈 몫이다(이웃 한계 5001 과 구별)', () => {
    // 한계 5000 과 5001 이 서로 다른 정수 초를 만드는 첫 시드를 찾는다(next(65536) 은 상위 16비트 그대로이다).
    let seed = 1;
    let sample = 0;
    for (; seed < 200000; seed += 1) {
        sample = new GameRandom(seed).next(65536);
        if (Math.floor((sample % 5000 + 10000) / 1000) !== Math.floor((sample % 5001 + 10000) / 1000)) break;
    }
    assert.ok(seed < 200000);
    const fields = new RainAviaryProcessFields();
    RawRainAviaryProcess.flyerDiedIn(fields, () => 1000.5, () => new GameRandom(seed), () => {});
    assert.equal(fields.time, 1000.5 + Math.floor((sample % 5000 + 10000) / 1000));
});

test('RainAviary 홈 통지: 선택 누락은 그 앞의 쓰기를 남기고 거부한다', () => {
    const times = () => { let value = 10.5; return () => (value += 1); };
    // 시각 조회기가 없으면 AP 10 과 dirty |= 4 만 남는다.
    const first = new RainAviaryProcessFields();
    first.bytes[8] = 0x10;
    assert.throws(() => RawRainAviaryProcess.flyerDiedIn(first, undefined, () => new GameRandom(55), () => {}), /미연결/);
    assert.deepEqual([first.stage, first.dirty], [10, 0x14]);
    // 난수가 없으면 첫 시각까지 저장된다(dirty |= 6).
    const second = new RainAviaryProcessFields();
    second.bytes[8] = 0x10;
    assert.throws(() => RawRainAviaryProcess.flyerDiedIn(second, times(), undefined, () => {}), /미연결/);
    assert.deepEqual([second.stage, second.dirty, second.time], [10, 0x16, 11.5]);
    // 갱신 요청이 없으면 둘째 시각까지 저장된 뒤 거부한다.
    const third = new RainAviaryProcessFields();
    third.bytes[8] = 0x10;
    const random = new GameRandom(55);
    assert.throws(() => RawRainAviaryProcess.flyerDiedIn(third, times(), () => random, undefined), /미연결/);
    assert.ok(third.time > 12.5 + 9);
    assert.equal(third.dirty, 0x16);
});

test('RainAviaryProcessFields: form/parent 는 DWORD 의 하위 WORD 이며 writer 도 하위 WORD 값을 쓴다', () => {
    const fields = new RainAviaryProcessFields();
    put(fields.bytes, 36, 0xabcd0005);
    put(fields.bytes, 40, 0x12340032);
    assert.equal(fields.form, 5);
    assert.equal(fields.parent, 0x32);
    const packet = new Uint8Array(64).fill(0xa7);
    assert.equal(RawRainAviaryProcess.writeProcess(fields, packet, true, false), 13);
    assert.equal(get(packet, 4), 5);
    assert.equal(get(packet, 9), 0x32);
});

test('RainAviary 상태 writer: 길이(9 + 부모 4 + 시각 8 + 상태 4)에 정확히 맞는 버퍼는 쓰고 한 바이트 모자라면 쓰기 전에 거부하며 force 는 모든 필드를 가정한다', () => {
    // [dirty, 부모 포함, force, 기대 총 길이] — 부모 입력은 필드의 현재 부모(50)이다.
    for (const [dirty, includeParent, force, length] of [[0, false, false, 9], [0, true, false, 13], [2, false, false, 17], [4, false, false, 13], [6, true, false, 25], [0, true, true, 25], [0, false, true, 25]]) {
        const scene = new AviaryScene('W', [String(dirty), '0', '0']);
        scene.fields.bytes[8] = dirty;
        const before = Uint8Array.from(scene.fields.bytes);
        // 한 바이트 모자라면 아무것도 바꾸지 않고 거부한다.
        assert.throws(() => RawRainAviaryProcess.writeProcess(scene.fields, new Uint8Array(length - 1), includeParent, force), RangeError, `dirty ${dirty} 부모 ${includeParent} force ${force}`);
        assert.deepEqual(scene.fields.bytes, before, `dirty ${dirty} 부모 ${includeParent} force ${force} 거부 뒤 필드`);
        // 정확히 맞으면 쓰고 반환 길이가 총 길이이며 필드 dirty 를 비운다.
        assert.equal(RawRainAviaryProcess.writeProcess(scene.fields, new Uint8Array(length), includeParent, force), length, `dirty ${dirty} 부모 ${includeParent} force ${force}`);
        assert.equal(scene.fields.dirty, 0);
    }
});

test('RainAviary 상태 writer: protocol/message 는 패킷 머리 첫 두 바이트에 그대로 쓰이고 기본은 1/0x73 이다', () => {
    const scene = new AviaryScene('W', ['2', '0', '0']);
    scene.fields.bytes[8] = 2;
    const custom = new Uint8Array(64).fill(0xa7);
    RawRainAviaryProcess.writeProcess(scene.fields, custom, false, false, 2, 0x40);
    assert.deepEqual([custom[0], custom[1]], [2, 0x40]);
    scene.fields.bytes[8] = 2;
    const normal = new Uint8Array(64).fill(0xa7);
    RawRainAviaryProcess.writeProcess(scene.fields, normal, false, false);
    assert.deepEqual([normal[0], normal[1]], [1, 0x73]);
});

test('RainAviaryProcess: Kernel 소유 래퍼가 프레임을 로직에 전달하고 부착 결과 Form/부모를 필드에 기록하며 writePacket 이 부착 전에도 현재 Host 번호를 쓴다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    const kernel = new Kernel(32);
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), new SquidProcessState(), { asset: { selected: () => 0, emit: () => {} } });
    const calls = [];
    const logic = new RawRainAviaryProcess(pool, {
        fort: () => false, friendlyFire: () => 0, server: () => true, editor: () => 0, checking: () => 0, now: () => 5, processType: () => 60, flyerType: () => 144,
    }, {
        setFrame: (sid, frame) => { calls.push(`frame:${sid},${frame}`); },
        sound: (x, y, name) => { calls.push(`sound:${name}`); },
    });
    const fields = new RainAviaryProcessFields();
    logic.initialize(fields);
    const process = new RainAviaryProcess(logic, fields);
    const packet = new Uint8Array(64);
    assert.equal(process.writePacket(packet, false, true), 9 + 8 + 4);
    // 기본 protocol/message 는 1/0x73 이다.
    assert.deepEqual([packet[0], packet[1]], [1, 0x73]);
    assert.deepEqual([get(fields.bytes, 36), get(fields.bytes, 40)], [0, 0]);
    const parent = pool.allocate(2);
    pool.allocated(parent)[10] = 143;
    try {
        assert.equal(host.attach(process, 60, parent, 0x10), process);
        assert.deepEqual([get(fields.bytes, 36), get(fields.bytes, 40)], [process.form, parent]);
        kernel.runFrame();
        // 초기 상태 1: 홈 프레임 0 과 AP 2, manOWarBuild 소리.
        assert.deepEqual(calls, [`frame:${parent},0`, 'sound:manOWarBuild.wav']);
        assert.equal(fields.stage, 2);
        process.notify(1, 2, 3);
        assert.equal(process.fields, fields);
        assert.equal(fields.form, process.form);
        assert.equal(fields.parent, parent);
        // 부착 뒤 필드 번호가 어긋나도 writer 는 Host 의 현재 Form/부모를 먼저 반영하고 protocol/message 를 그대로 전달한다.
        put(fields.bytes, 36, 999);
        put(fields.bytes, 40, 998);
        const other = new Uint8Array(64).fill(0xa7);
        process.writePacket(other, false, false, 2, 0x40);
        assert.deepEqual([get(fields.bytes, 36), get(fields.bytes, 40)], [process.form, parent]);
        assert.deepEqual([other[0], other[1]], [2, 0x40]);
        assert.equal(get(other, 4), process.form);
    } finally {
        kernel.clear();
    }
});
