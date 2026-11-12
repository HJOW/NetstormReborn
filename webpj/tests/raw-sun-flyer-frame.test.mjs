// 훨리기그 전용 비행 raw-sun-flyer-frame.js(RawSunFlyerFrame)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: sun-flyer-frame-x86.tsv 410행 — 초기화 I 9행·장식 프레임 A 54행·통지 N 64행·홈 통지 P 36행·폭탄 정리 C 4행·종료 E 4행·전용 Frame F 237행·복귀 지연 R 1행·표적 가능 Z 1행이
//    필드 136바이트·객체 9개 450바이트·전역 20바이트·반환·효과 순서까지 같다. sun-flyer-clear-x86.tsv 30행은 전체 DWORD 폭탄 번호의 정리(장치 효과만 비교)를 대조한다.
//    cpppj SunFlyerFrameTests.cpp 의 SunFrameScene 을 같은 순서로 옮겼다(종료 E 는 E352 RawFlyerProcess, 표적 가능 Z 는 RawGunTarget, 공통 통지/홈 통지는 E355 RawFlyerFrame 을 쓴다).
//  - 웹 계약: 생성자 검사, 훅 누락 문구, SHP·TYPE 범위 오류, idiv 무효 경로, 장식 프레임의 정수화 범위 오류.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E357: 7-P1 RawSunFlyerFrame 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { FlyerProcessFields, RawFlyerProcess } from '../www/core/raw-flyer-process.js';
import { RawFlyerFrame } from '../www/core/raw-flyer-frame.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { RawSunFlyerFrame } from '../www/core/raw-sun-flyer-frame.js';

// 원본 관측 행이다(헤더는 # 주석). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E357)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(fixtures, 'sun-flyer-frame-x86.tsv'));
const clearRows = readRows(path.join(fixtures, 'sun-flyer-clear-x86.tsv'));

// 원문 관측에서 따로 보존한 9개 현재 객체이다. (2026-11-13 WB04-E357)
const IDS = [50, 51, 60, 61, 62, 63, 70, 80, 81];

// 예외가 나는지 본다. (2026-11-13 WB04-E357)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E357)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 64비트 double 비트(BigInt)를 double 로 바꾼다. (2026-11-13 WB04-E357)
function doubleOf(bitsValue) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(bitsValue), true);
    return view.getFloat64(0, true);
}

// 선언한 입력과 기존 공통 몸체를 같은 원문 136바이트/풀에 연결하는 장면이다(cpppj SunFrameScene). (2026-11-13 WB04-E357)
class SunFrameScene {
    constructor(kind, input) {
        this.pool = new SidPool(24000, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.shapes = Array.from({ length: 188 }, () => ({ loaded: false, frameCount: 0, frames: [] }));
        this.data = Array.from({ length: 188 }, () => ({ fallingFrames: 0, explosionFirst: 0, explosionEnd: 0, sound: 0 }));
        this.commonData = Array.from({ length: 188 }, () => ({ range: 0, sound: 0 }));
        this.stamps = new Uint32Array(188);
        this.fields = new FlyerProcessFields();
        this.server = true;
        this.boss = true;
        this.paused = false;
        this.now = 100;
        this.bombType = 163;
        this.mutation = 0;
        this.count = 9;
        this.may = 0;
        this.frameNumber = 7;
        this.sunType = 73;
        this.windType = 138;
        this.rainType = 144;
        this.sunProcess = 32;
        this.soundProcess = 47;
        this.processType = 31;
        this.events = [];
        // 관측 마지막 번호까지 실제 client 슬롯을 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 81) break;
            if (!sid || sid > 81) throw new Error('훨리기그 Frame 검사 SID 준비 오류');
        }
        // 모든 관측 객체의 초기 50바이트를 구별한다.
        for (const sid of IDS) {
            const raw = this.pool.allocated(sid);
            raw.fill(0xa5);
            put(raw, 0, sid < 60 ? 0x50a1e8 : sid >= 80 ? 0x509b98 : 0x501dd0);
            put(raw, 4, 0, 2);
            put(raw, 6, sid === 50 || sid === 51 ? 70 : 0, 2);
            put(raw, 8, 0, 2);
            raw[10] = sid < 60 ? 73 : sid >= 80 ? 163 : 72;
            raw[11] = 0;
            put(raw, 12, sid === 61 ? 50 : 0, 2);
            put(raw, 14, floatToBits(50.25));
            put(raw, 18, floatToBits(50.5));
            put(raw, 36, 0);
            raw[34] = 1;
            raw[40] = 0;
        }
        // getter 의 물리 헤더 폭은 TYPE 발자국 2 와 다른 2.25 부터 시작한다.
        this.shapes[73].loaded = true;
        this.shapes[73].frameCount = 20;
        for (let index = 0; index < 20; index += 1) this.shapes[73].frames.push({ cellWidth: Math.fround(2.25 + index * 0.125) });
        // 현재 원본 타입의 불변 폭탄 필드이다.
        for (let type = 0; type < 188; type += 1) {
            this.types[type].footX = this.types[type].footY = 2;
            this.data[type] = { fallingFrames: 3, explosionFirst: 4, explosionEnd: 6, sound: type === 163 ? 881 : 882 };
        }
        this.fields.bytes.fill(0xa5);
        this.fields.sunTail.fill(0xa5);
        put(this.fields.bytes, 0, 0x509f24);
        this.fields.bytes[8] = 0x10;
        put(this.fields.bytes, 10, 61, 2);
        put(this.fields.bytes, 12, 60, 2);
        put(this.fields.bytes, 48, 0x509f04);
        put(this.fields.bytes, 52, 70);
        put(this.fields.bytes, 56, 50);
        put(this.fields.bytes, 80, 1);
        put(this.fields.bytes, 84, 1);
        put(this.fields.bytes, 88, 0);
        put(this.fields.bytes, 92, 0);
        put(this.fields.bytes, 96, 0);
        this.timeAt(104, 100);
        this.timeAt(112, 0);
        this.timeAt(120, 100);
        this.write(128, floatToBits(-10000));
        this.write(132, floatToBits(-10000));
        const number = (index) => Number(input[index]);
        if (kind === 'I') {
            this.now = doubleOf(input[0]);
            this.fields.bytes.fill(number(1) & 0xff);
            this.fields.sunTail.fill(number(1) & 0xff);
        } else if (kind === 'A') {
            this.now = doubleOf(input[0]);
            put(this.fields.bytes, 52, number(1));
            this.count = number(2);
        } else if (kind === 'N') {
            this.server = number(0) !== 0;
            put(this.fields.bytes, 12, number(3), 2);
            this.mutation = number(5);
        } else if (kind === 'P') {
            this.server = number(0) !== 0;
            put(this.fields.bytes, 10, number(1), 2);
            this.pool.allocated(61)[11] = number(2) & 0xff;
            this.write(96, number(3));
        } else if (kind === 'C') {
            this.write(96, number(0));
            this.mutation = number(1);
        } else if (kind === 'E') {
            this.server = number(0) !== 0;
            this.mutation = number(1);
        } else if (kind === 'F') {
            this.write(84, number(0));
            this.pool.allocated(50)[40] = number(1) & 0xff;
            this.boss = number(2) !== 0;
            this.paused = (number(3) | 0) > 0;
            this.write(96, number(4));
            put(this.pool.allocated(80), 36, number(5));
            this.pool.allocated(80)[11] = number(6) & 0xff;
            this.timeAt(104, 100 - [0, 0.25, 1][number(7)]);
            this.timeAt(120, number(8) ? 99 : 101);
            this.may = number(9);
            this.timeAt(112, [0, 99, 100, 101][number(10)]);
            this.write(132, floatToBits(number(11) ? 49 : 51));
            this.mutation = number(12);
        }
        // 실제 공통 45b760/45b7a0 과 전용 통지 재진입의 경계이다(원본 관측은 개발자 추가 진단을 끈다).
        const base = {
            assertion: (line) => { this.event(`Assert,${line}`); },
            removeDepend: (form, target) => { this.event(`Remove,${form},${target},0`); },
            refreshForm: () => { this.event('Refresh'); },
            homeNotice: (home, first, second) => { this.event(`Home,${home},${first},${second}`); },
            sunNotify: (live, message, first, second) => { this.sun.notify(live, message, first, second); },
        };
        this.common = new RawFlyerFrame(this.pool, this.types, this.commonData, this.stamps, {
            server: () => this.server, boss: () => this.boss, checking: () => 0, now: () => this.now, frame: () => this.frameNumber,
            sunType: () => this.sunType, windType: () => this.windType, rainType: () => this.rainType, sunAviaryProcess: () => this.sunProcess, soundProcess: () => this.soundProcess,
        }, base);
        this.sun = new RawSunFlyerFrame(this.pool, this.shapes, this.data, {
            boss: () => this.boss, paused: () => this.paused, now: () => this.now, bombType: () => this.bombType,
        }, this.hooks());
    }

    // 실제 136바이트 원문에 DWORD 를 쓴다. (2026-11-13 WB04-E357)
    write(at, value) {
        if (at < 112) put(this.fields.bytes, at, value);
        else put(this.fields.sunTail, at - 112, value);
    }

    // 현재 double 저장 두 DWORD 의 명시 입력이다. (2026-11-13 WB04-E357)
    timeAt(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        this.write(at, view.getUint32(0, true));
        this.write(at + 4, view.getUint32(4, true));
    }

    // 원문 사건을 생략/중복 제거 없이 보존한다. (2026-11-13 WB04-E357)
    event(value) { this.events.push(value); }

    // 원문 observer 와 같은 외부 효과/변경 입력을 공급한다. (2026-11-13 WB04-E357)
    hooks() {
        const pool = this.pool;
        return {
            assertion: (line) => { this.event(`Assert,${line}`); },
            robust: (sid, checking) => { this.event(`Sid,${sid},${checking ? 1 : 0}`); return sid > 0 && sid < 65535; },
            destroy: (sid, flags) => { this.event(`Destroy,${sid},${flags}`); if (this.mutation === 6) this.write(96, 81); },
            unpop: (sid, flags) => {
                this.event(`Unpop,${sid},${flags}`);
                if (this.mutation === 1 && sid === 50) {
                    put(pool.allocated(50), 14, floatToBits(71.25));
                    put(pool.allocated(50), 18, floatToBits(72.5));
                    this.write(56, 51);
                    this.now = 200;
                }
            },
            pop: (sid, x, y, flags) => {
                this.event(`Pop,${sid},${floatToBits(x)},${floatToBits(y)},${flags}`);
                put(pool.allocated(sid), 14, floatToBits(x));
                put(pool.allocated(sid), 18, floatToBits(y));
                if (this.mutation === 4 && sid === 80) put(pool.allocated(50), 18, floatToBits(52.5));
            },
            repop: (sid, flags) => { this.event(`Repop,${sid},${flags}`); if (this.mutation === 5) this.write(96, 0); },
            mayTarget: (parent, target) => {
                this.event(`May,${parent},${target}`);
                if (this.mutation === 3) {
                    this.write(56, 51);
                    put(this.fields.bytes, 12, 61, 2);
                }
                return this.may !== 0;
            },
            create: (type, flags) => {
                this.event(`Create,${type},${flags}`);
                pool.allocated(80)[10] = type & 0xff;
                if (this.mutation === 7) this.write(96, 81);
                if (this.mutation === 8) pool.allocated(80)[10] = 164;
                return 80;
            },
            sound: (x, y, sound) => { this.event(`Sound,${floatToBits(x)},${floatToBits(y)},${sound}`); if (this.mutation === 2) this.now = 200; },
            runCount: (parent) => { this.event(`Count,${parent}`); return this.count | 0; },
            baseFrame: () => { this.event('Base'); },
            baseNotify: (live, message, first, second) => { this.common.baseNotify(live, message, first, second); },
            baseParentDying: (live) => { this.common.parentDying(live); },
        };
    }

    // 실제 관측 전체 136바이트를 읽는다. (2026-11-13 WB04-E357)
    image() { return hex(this.fields.bytes) + hex(this.fields.sunTail); }

    // 현재 9개 객체 450바이트를 같은 순서로 읽는다. (2026-11-13 WB04-E357)
    objects() { return IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 실제 원문 20바이트 전역을 현재 값으로 읽는다. (2026-11-13 WB04-E357)
    globalsImage(pause) {
        const raw = new Uint8Array(20);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.now, true);
        put(raw, 0, this.server ? 1 : 0);
        put(raw, 4, this.boss ? 1 : 0);
        put(raw, 8, pause);
        put(raw, 12, view.getUint32(0, true));
        put(raw, 16, view.getUint32(4, true));
        return hex(raw);
    }
}

// 표적 가능 Z 행의 실제 Gun 전역 기본 상태이다. (2026-11-13 WB04-E357)
function gunState() {
    return {
        friendlyFire: false, checking: false, showRanges: false,
        sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148,
        windHalfAngle: 30, scaleX: 16, scaleY: 11,
        allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {},
    };
}

test('SunFlyerFrame: 원본 10.78 전용 비행 410행 — 초기화/장식 프레임/통지/홈 통지/폭탄 정리/종료/Frame/복귀 지연/표적 가능이 전체 상태·반환·효과 순서까지 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 원본 기대를 제품 계산으로 만들지 않고 전수 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const kind = row[0][0];
        const input = row[1] === '' ? [] : row[1].split(',').map((part) => BigInt(part));
        const scene = new SunFrameScene(kind, input);
        let result = '-';
        if (kind === 'I') scene.sun.initialize(scene.fields);
        else if (kind === 'A') result = String(scene.sun.animation(scene.fields) >>> 0);
        else if (kind === 'N') scene.sun.notify(scene.fields, Number(input[1]), Number(input[2]), Number(input[4]));
        else if (kind === 'P') scene.sun.parentDying(scene.fields);
        else if (kind === 'C') scene.sun.clearBomb(scene.fields);
        else if (kind === 'R') {
            const view = new DataView(new ArrayBuffer(8));
            view.setFloat64(0, RawSunFlyerFrame.returnDelay(), true);
            result = String(view.getBigUint64(0, true));
        } else if (kind === 'E') {
            const hooks = {
                assertion: (line) => { scene.event(`Assert,${line}`); },
                pieces: (parent) => { scene.event(`Pieces,${parent}`); if (scene.mutation === 9) scene.write(56, 51); },
                destroy: (parent, flags) => { scene.event(`Destroy,${parent},${flags}`); },
            };
            new RawFlyerProcess(scene.pool, scene.types, { server: () => scene.server, now: () => scene.now, processType: () => scene.processType }, hooks).finish(scene.fields);
        } else if (kind === 'Z') {
            const targetTypes = Array.from({ length: 188 }, () => ({ range: 0, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 }));
            const gunTypes = scene.types.map((type) => ({ ...type, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0 }));
            const logic = new RawGunTarget(scene.pool, scene.hash, gunTypes, targetTypes, scene.shapes, gunState());
            result = String(logic.mayTarget({ x: 50.25, y: 50.5, centerX: 49.25, centerY: 49.5, type: 73, direction: 0, owner: 1, frame: 0 }, 0) ? 1 : 0);
        } else scene.sun.frame(scene.fields);
        const actual = [scene.image(), scene.objects(), scene.globalsImage(kind === 'F' ? Number(input[3]) : 0), result, scene.events.length === 0 ? '-' : scene.events.join(';')];
        const where = `${kind} ${row[1]}`;
        const names = ['필드', '객체', '전역', '반환', '효과 순서'];
        // 차이가 있는 입력/열을 기록하고 정상 사례로 대체하지 않는다.
        for (let index = 0; index < actual.length; index += 1) assert.equal(actual[index], row[index + 2], `${where} ${names[index]}`);
        count += 1;
    }
    assert.equal(count, 410);
});

test('SunFlyerFrame: 원본 10.78 전체 DWORD 폭탄 번호 정리 30행 — 첫 Frame 전 미정 값도 장치 효과 없이 0 으로 쓴다', { skip: !clearRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기존 410행과 분리한 새 30경계이다.
    for (const row of clearRows) {
        assert.equal(row.length, 7);
        const kind = row[0][0];
        const input = row[1].split(',').map((word) => BigInt(word));
        const scene = new SunFrameScene(kind, input);
        if (kind === 'P') scene.sun.parentDying(scene.fields);
        else scene.sun.clearBomb(scene.fields);
        // 새 원문은 순수 helper 도 기계어이므로 장치 효과만 대조한다.
        const effects = scene.events.filter((event) => event !== '' && !event.startsWith('Sid,')).join(';');
        assert.equal(scene.image(), row[2], `${row[1]} 필드`);
        assert.equal(scene.objects(), row[3], `${row[1]} 객체`);
        assert.equal(scene.globalsImage(0), row[4], `${row[1]} 전역`);
        assert.equal(effects === '' ? '-' : effects, row[6], `${row[1]} 효과`);
        count += 1;
    }
    assert.equal(count, 30);
});

// 폭탄 번호 80 이 낙하(프레임 0 < 3)하는 기본 장면이다: 투하 시각 0.5초 전, 부모 TYPE 73 의 SHP 폭 2.25. (2026-11-13 WB04-E357)
function dropScene() {
    const scene = new SunFrameScene('X', []);
    scene.write(96, 80);
    scene.timeAt(104, 99.5);
    return scene;
}

// 장면의 훅을 호출자가 바꾼 본체를 만든다(원본 장면과 같은 풀·표·전역을 쓴다). (2026-11-13 WB04-E357)
function bodyOf(scene, change = () => {}) {
    const hooks = scene.hooks();
    change(hooks);
    return new RawSunFlyerFrame(scene.pool, scene.shapes, scene.data, { boss: () => scene.boss, paused: () => scene.paused, now: () => scene.now, bombType: () => scene.bombType }, hooks);
}

// Pop 훅이 받은 수(double 그대로)를 모은다. (2026-11-13 WB04-E357)
function collectPops(scene) {
    const popped = [];
    const body = bodyOf(scene, (hooks) => {
        const pop = hooks.pop;
        hooks.pop = (sid, x, y, flags) => { popped.push([sid, x, y, flags]); pop(sid, x, y, flags); };
    });
    return { popped, body };
}

// 필드의 double 한 칸을 읽는다. (2026-11-13 WB04-E357)
function timeOf(scene, at) {
    const view = new DataView(new ArrayBuffer(8));
    const bytes = at < 112 ? scene.fields.bytes : scene.fields.sunTail;
    const base = at < 112 ? at : at - 112;
    view.setUint32(0, Buffer.from(bytes).readUInt32LE(base), true);
    view.setUint32(4, Buffer.from(bytes).readUInt32LE(base + 4), true);
    return view.getFloat64(0, true);
}

test('SunFlyerFrame: 생성자는 PATCH 풀·188칸 SHP/폭탄 표·현재 전역 조회기 네 개를 요구하고 SHP 는 살아 있는 참조·폭탄 표는 사본이다', () => {
    const scene = dropScene();
    assert.equal(scene.sun.pool, scene.pool);
    const globals = () => ({ boss: () => true, paused: () => false, now: () => 100, bombType: () => 163 });
    const build = (pool, shapes, data, given) => () => new RawSunFlyerFrame(pool, shapes, data, given, scene.hooks());
    assert.throws(build({ edition: 'cd1072' }, scene.shapes, scene.data, globals()), /PATCH/);
    assert.throws(build(null, scene.shapes, scene.data, globals()), TypeError);
    assert.throws(build(scene.pool, scene.shapes.slice(1), scene.data, globals()), /PATCH/);
    assert.throws(build(scene.pool, scene.shapes, scene.data.slice(1), globals()), /PATCH/);
    assert.throws(build(scene.pool, scene.shapes, scene.data, null), TypeError);
    for (const name of ['boss', 'paused', 'now', 'bombType']) {
        const given = globals();
        delete given[name];
        assert.throws(build(scene.pool, scene.shapes, scene.data, given), (error) => error instanceof TypeError && error.message.endsWith(name), name);
        given[name] = 5;
        assert.throws(build(scene.pool, scene.shapes, scene.data, given), TypeError, `${name} 값`);
    }
    assert.doesNotThrow(() => new RawSunFlyerFrame(scene.pool, scene.shapes, scene.data, globals()));
    // 훅 누락과 훅 묶음 생략은 같은 문구로 거부한다.
    const message = (error) => error.message === '훨리기그 실제 하위 몸체 미연결';
    assert.throws(() => new RawSunFlyerFrame(scene.pool, scene.shapes, scene.data, globals()).notify(scene.fields, 2, 0, 0), message);
    assert.throws(() => bodyOf(scene, (hooks) => { delete hooks.baseNotify; }).notify(scene.fields, 2, 0, 0), message);
    // 폭탄 표는 사본이라 만든 뒤 바꾼 낙하 프레임 수 0 을 무시하고, SHP 는 살아 있는 참조라 만든 뒤 적재를 끄면 낙하 위치 계산이 거부한다.
    scene.data[163].fallingFrames = 0;
    scene.sun.frame(scene.fields);
    assert.ok(scene.events.some((line) => line.startsWith('Pop,80,')));
    const live = dropScene();
    live.shapes[73].loaded = false;
    assert.throws(() => live.sun.frame(live.fields), (error) => error instanceof RangeError && /SHP/.test(error.message));
});

test('SunFlyerFrame 낙하: Pop 은 float 로 줄인 값을 받고 경과 시간의 float 저장이 위치를 가른다', () => {
    const run = (x, y, now, elapsed) => {
        const scene = dropScene();
        scene.now = now;
        scene.timeAt(104, now - elapsed);
        put(scene.pool.allocated(50), 14, floatToBits(x));
        put(scene.pool.allocated(50), 18, floatToBits(y));
        const { popped, body } = collectPops(scene);
        body.frame(scene.fields);
        return popped;
    };
    // 탐색으로 찾은 입력: 경과 0.3045초를 float 로 줄이면 폭탄 y 가 119.51392364501953 이고 줄이지 않으면 119.513916015625 이다.
    assert.deepEqual(run(83.31434631347656, 115.63443756103516, 106.057, 0.3045), [
        [80, 82.18934631347656, 119.51392364501953, 0],
        [50, 83.31434631347656, 115.58443450927734, 0],
    ]);
    // 모든 Pop 좌표는 float 값이다(천장 = 위치 + 4 도 float 로 줄인다: 0.1 + 4 는 float 가 아니다).
    for (const [x, y, now, elapsed] of [[20.5, 0.1, 100, 5], [20.5, 0.1, 100, 0.01], [0.1, 30.5, 100, 0.01]]) {
        for (const [, px, py] of run(x, y, now, elapsed)) assert.ok(Object.is(Math.fround(px), px) && Object.is(Math.fround(py), py), `${px},${py}`);
    }
    // 천장에 닿으면(큰 경과) 폭탄은 천장 높이에 놓이고 폭발 첫 프레임으로 간다.
    const ceiling = dropScene();
    ceiling.now = 100;
    ceiling.timeAt(104, 90);
    put(ceiling.pool.allocated(50), 18, floatToBits(0.1));
    const { popped, body } = collectPops(ceiling);
    body.frame(ceiling.fields);
    assert.equal(popped[0][2], Math.fround(Math.fround(0.1) + 4));
    assert.equal(Buffer.from(ceiling.pool.slot(80)).readUInt32LE(36), 4);
});

test('SunFlyerFrame 장식 프레임: 주기 상수·개수/값의 float 저장은 큰 개수에서 갈리고 NaN 은 거부하며 음수 값은 0 방향으로 정수화한다', () => {
    const run = (form, now, count) => {
        const scene = new SunFrameScene('X', []);
        put(scene.fields.bytes, 52, form);
        scene.now = now;
        scene.count = count;
        return scene.sun.animation(scene.fields);
    };
    // 탐색으로 찾은 입력(큰 개수 16777249 / 16777238 에서 float 저장 단계마다 정수가 달라진다).
    assert.equal(run(41700, 1201.451, 16777249), 8660872);
    assert.equal(run(20820, 956.65, 16777238), 2616357);
    // 보통 개수에서는 장식 값이 [0, 개수) 안이다.
    assert.ok(run(61, 100, 9) >= 0 && run(61, 100, 9) < 9);
    // 개수가 0 이면 값이 (−1, 0.5) 라 0 방향 절삭으로 항상 0 이다(내림이면 −1).
    for (const now of [100, 100.04, 100.08, 100.12]) assert.equal(run(61, now, 0), 0);
    // 비유한 값은 정수화 범위 오류로 거부한다.
    const nan = new SunFrameScene('X', []);
    assert.throws(() => bodyOf(nan, (hooks) => { hooks.runCount = () => Number.NaN; }).animation(nan.fields), (error) => error instanceof RangeError && /정수화/.test(error.message));
});

test('SunFlyerFrame 폭발·투하 경계: 폭발 프레임 기한은 같을 때도 진행하고, 정수 감김·음수 프레임·낙하 프레임 0 은 원문처럼 처리한다', () => {
    const explode = (frame, next) => {
        const scene = dropScene();
        put(scene.pool.allocated(80), 36, frame);
        scene.timeAt(120, next);
        scene.sun.frame(scene.fields);
        return scene;
    };
    // 폭발 구간(프레임 ≥ 3): 다음 프레임 기한 == now 이면 프레임 +1·바이트 35 = 0xed·기한 갱신, 이후면 Repop 만.
    const due = explode(4, 100);
    assert.equal(Buffer.from(due.pool.slot(80)).readUInt32LE(36), 5);
    assert.equal(due.pool.slot(80)[35], 0xed);
    const later = explode(4, 100.5);
    assert.equal(Buffer.from(later.pool.slot(80)).readUInt32LE(36), 4);
    assert.ok(later.events.includes('Repop,80,0'));
    // 프레임 0x7fffffff + 1 은 INT_MIN 으로 감기고 폭발 끝보다 작아 정리하지 않는다.
    const wrapped = explode(0x7fffffff, 100);
    assert.equal(Buffer.from(wrapped.pool.slot(80)).readUInt32LE(36), 0x80000000);
    assert.ok(!wrapped.events.some((line) => line.startsWith('Destroy,')));
    // 프레임 −1(0xffffffff)은 부호 있는 값이라 낙하 구간(−1 < 3)이다.
    const negative = dropScene();
    put(negative.pool.allocated(80), 36, 0xffffffff);
    negative.sun.frame(negative.fields);
    assert.ok(negative.events.some((line) => line.startsWith('Pop,80,')));
    // 낙하 프레임 수가 0 인 폭탄 표에서 음수 프레임은 원본 idiv 무효 경로(0 나눗셈)이다.
    const zero = dropScene();
    zero.data[163].fallingFrames = 0;
    put(zero.pool.allocated(80), 36, 0xffffffff);
    assert.throws(() => bodyOf(zero).frame(zero.fields), (error) => error instanceof RangeError && /idiv/.test(error.message));
});

test('SunFlyerFrame 부모 복원: 저장 위치보다 아래(y 가 클 때)일 때만 Unpop → Pop 하고 같은 값이면 하지 않으며 x 가 아니라 y 를 비교한다', () => {
    const run = (x, y, savedY) => {
        const scene = dropScene();
        scene.write(132, floatToBits(savedY));
        scene.timeAt(104, 99.999);
        put(scene.pool.allocated(50), 14, floatToBits(x));
        put(scene.pool.allocated(50), 18, floatToBits(y));
        scene.sun.frame(scene.fields);
        return scene.events.filter((line) => line.startsWith('Unpop,50') || line.startsWith('Pop,50,'));
    };
    assert.equal(run(60, 40, 50).length, 0);
    assert.equal(run(60, 50, 50).length, 0);
    assert.equal(run(60, 51, 50).length, 2);
});

test('SunFlyerFrame 새 폭탄 투하: 발사 가능에 현재 WORD 표적, 생성 폭탄의 flags·프레임 초기화, 투하 시각·소리, 전역 폭탄 TYPE 을 쓴다', () => {
    const make = (overrides = () => {}) => {
        const scene = new SunFrameScene('X', []);
        scene.may = 1;
        scene.timeAt(112, 100);
        put(scene.fields.bytes, 12, 300, 2);
        overrides(scene);
        return scene;
    };
    // 기한이 되고 폭탄이 없으면 만든다: 표적 WORD(300), 생성 폭탄 flags 에 8 을 더하고 프레임 0, 투하 시각 = now, 다음 투하 = now + 2.
    const scene = make();
    scene.pool.allocated(80)[40] = 0x31;
    put(scene.pool.allocated(80), 36, 7);
    scene.timeAt(104, 12);
    scene.data[163].sound = 881;
    const dropped = collectPops(scene);
    dropped.body.frame(scene.fields);
    assert.ok(scene.events.includes('May,50,300') && scene.events.includes('Create,163,2'));
    // 투하 Pop 의 좌표도 float 값이다(50.5 + 0.4 는 float 가 아니다).
    for (const [sid, px, py] of dropped.popped) assert.ok(Object.is(Math.fround(px), px) && Object.is(Math.fround(py), py), `${sid} ${px},${py}`);
    assert.equal(scene.pool.slot(80)[40], 0x39);
    assert.equal(Buffer.from(scene.pool.slot(80)).readUInt32LE(36), 0);
    assert.equal(timeOf(scene, 104), 100);
    assert.equal(timeOf(scene, 112), 102);
    // 낮은 DWORD 가 0 이 아닌 시각(now = 100.1)도 투하 시각/다음 투하에 그대로 쓴다.
    const fine = make();
    fine.now = 100.1;
    fine.timeAt(112, 100.05);
    fine.sun.frame(fine.fields);
    assert.equal(timeOf(fine, 104), 100.1);
    assert.equal(timeOf(fine, 112), 100.1 + 2);
    // 소리 토큰은 생성된 폭탄의 TYPE 소리이다(부모 TYPE 소리가 아님: 분리한 값 881 / 882).
    const sound = scene.events.find((line) => line.startsWith('Sound,'));
    assert.ok(sound.endsWith(',882') || sound.endsWith(',881'));
    // 전역 폭탄 TYPE 이 바뀌면 그 번호로 생성한다.
    const other = make((target) => { target.bombType = 164; target.data[164] = { fallingFrames: 3, explosionFirst: 4, explosionEnd: 6, sound: 777 }; });
    bodyOf(other).frame(other.fields);
    assert.ok(other.events.includes('Create,164,2'));
    assert.ok(other.events.some((line) => line.endsWith(',777')));
    // 다음 투하 기한이 아직이면 만들지 않는다(낮은 DWORD 만 0 으로 읽으면 기한이 된 것처럼 보이는 100 + 1e-11 도 포함).
    const barely = make((target) => { target.timeAt(112, 100 + 1e-11); });
    bodyOf(barely).frame(barely.fields);
    assert.ok(!barely.events.some((line) => line.startsWith('Create,')));
    const early = make((target) => { target.timeAt(112, 100.5); });
    early.sun.frame(early.fields);
    assert.ok(!early.events.some((line) => line.startsWith('Create,')));
    // 폭탄이 이미 있으면 새로 만들지 않는다(정지 중에도 같다).
    const busy = make((target) => { target.write(96, 81); target.paused = true; });
    busy.sun.frame(busy.fields);
    assert.ok(!busy.events.some((line) => line.startsWith('Create,')));
    // 생성 폭탄 TYPE 이 표 밖(200)이면 소리 조회가 거부한다.
    const wild = make();
    assert.throws(() => bodyOf(wild, (hooks) => { const create = hooks.create; hooks.create = (type, flags) => { const sid = create(type, flags); wild.pool.allocated(sid)[10] = 200; return sid; }; }).frame(wild.fields), (error) => error instanceof RangeError && /TYPE/.test(error.message));
});

test('SunFlyerFrame 통지/폭탄 정리/범위 오류: 메시지 1 의 정지는 다음 투하를 지우고, 폭탄 번호는 0..65535 까지만 정리하며 표 밖 SHP/TYPE/폭탄 번호는 거부한다', () => {
    // 메시지 1: info1 0 이면 공격 해제·다음 투하 0(이미 값이 있어도 지움), 아니면 다음 투하 = now + 2.
    const stop = new SunFrameScene('X', []);
    stop.timeAt(112, 105);
    stop.sun.notify(stop.fields, 1, 0, 0);
    assert.equal(timeOf(stop, 112), 0);
    assert.equal(stop.fields.bytes[80], 0);
    const resume = new SunFrameScene('X', []);
    resume.now = 100.1;
    resume.sun.notify(resume.fields, 1, 1, 0);
    assert.equal(timeOf(resume, 112), 100.1 + 2);
    assert.equal(resume.fields.bytes[80], 1);
    // 폭탄 정리: 65535 는 유효성 검사 훅을 거치고 65536 이상은 건너뛰며 어느 쪽이든 번호를 0 으로 쓴다.
    for (const [bomb, called] of [[65535, true], [70000, false], [0, true]]) {
        const scene = new SunFrameScene('X', []);
        scene.write(96, bomb);
        scene.sun.clearBomb(scene.fields);
        assert.equal(scene.events.some((line) => line.startsWith('Sid,')), called, `폭탄 ${bomb}`);
        assert.equal(Buffer.from(scene.fields.bytes).readUInt32LE(96), 0);
    }
    // 폭탄 번호가 65536 이상이면 Frame 이 범위 오류로 거부한다(풀 접근 오류가 아님).
    for (const bomb of [65536, 70000]) {
        const scene = dropScene();
        scene.write(96, bomb);
        assert.throws(() => scene.sun.frame(scene.fields), (error) => error.message === '훨리기그 폭탄 SID 범위 오류');
    }
    const edge = dropScene();
    edge.write(96, 65535);
    assert.throws(() => edge.sun.frame(edge.fields), (error) => error.message !== '훨리기그 폭탄 SID 범위 오류');
    // 전역 폭탄 TYPE 이 표 밖이면 Frame 시작에서 거부한다.
    const wildType = dropScene();
    wildType.bombType = 188;
    assert.throws(() => wildType.sun.frame(wildType.fields), (error) => error instanceof RangeError && /폭탄 TYPE/.test(error.message));
    // SHP: 부모 TYPE 이 188 이상이거나 적재 전이거나 프레임이 표 밖이면 각각 거부한다.
    const shapeError = (setup) => {
        const scene = dropScene();
        setup(scene);
        return () => scene.sun.frame(scene.fields);
    };
    assert.throws(shapeError((scene) => { scene.pool.physical(50)[10] = 188; }), (error) => error instanceof RangeError && /SHP TYPE/.test(error.message));
    assert.throws(shapeError((scene) => { scene.pool.physical(50)[10] = 255; }), (error) => error instanceof RangeError && /SHP TYPE/.test(error.message));
    assert.throws(shapeError((scene) => { scene.pool.physical(50)[10] = 72; }), (error) => error instanceof RangeError && /SHP 프레임/.test(error.message));
    assert.throws(shapeError((scene) => { put(scene.pool.physical(50), 36, 20); }), (error) => error instanceof RangeError && /SHP 프레임/.test(error.message));
    assert.throws(shapeError((scene) => { put(scene.pool.physical(50), 36, 21); }), (error) => error instanceof RangeError && /SHP 프레임/.test(error.message));
});
