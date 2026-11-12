// 더스트 데빌(윈드 플라이어) raw-wind-flyer.js(RawWindFlyer)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: wind-flyer-x86.tsv 334행 — 생성자 뒤 전용 표 쓰기 C 9행(기본 부착 뒤)·초기화 I 9행(최초 시각과 20초 대기)·애니메이션 A 180행(기한/정지/부호 있는 프레임 경계)·주기 처리 R 72행(공격 횟수·수명·소리 장부)·
//    범위 공격 D 64행(실제 해시 순회/후보 필터·다리 약화·피해)이 필드 104바이트·반환 비트·소리 장부·효과 순서까지 같다. cpppj WindFlyerTests.cpp 의 WindFlyerScene 을 같은 순서로 옮겼다.
//  - 웹 계약: 생성자 검사, 훅 누락 문구, 표 밖 TYPE 거부, 칸 계산(거의 올림→float 저장→절삭)의 경계, 피해 후보 10개 한도, idiv 무효 경로, 소리 장부의 한 프레임 한 번.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E358: 7-P1 RawWindFlyer 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { FlyerProcessFields, RawFlyerProcess } from '../www/core/raw-flyer-process.js';
import { RawWindFlyer } from '../www/core/raw-wind-flyer.js';

// 원본 관측 행이다(첫 행은 열 이름). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E358)
const rows = (() => {
    const all = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'wind-flyer-x86.tsv'));
    return all ? all.slice(1) : null;
})();

// 예외가 나는지 본다. (2026-11-13 WB04-E358)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E358)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본 실행기와 동일한 입력만 설치하고 결과를 계산하지 않는 장면이다(cpppj WindFlyerScene). (2026-11-13 WB04-E358)
class WindFlyerScene {
    constructor(kind, input) {
        this.pool = new SidPool(24000, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.stamps = new Uint32Array(188);
        this.sounds = new Uint32Array(188);
        this.fields = new FlyerProcessFields();
        this.now = 100.25;
        this.boss = true;
        this.frame = 77;
        this.runCount = 22;
        this.mask = 3;
        this.trace = [];
        // 실제 슬롯을 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 70) break;
            if (!sid || sid > 70) throw new Error('WindFlyer SID 입력 준비 실패');
        }
        // 원본과 동일한 50바이트 공격자·다리·후보를 설치한다.
        for (const sid of [50, 60, 61, 62]) {
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            put(raw, 0, sid === 50 ? 0x50a050 : 0x501ea0);
            raw[10] = sid === 50 ? 138 : sid === 60 ? 82 : sid === 61 ? 78 : 79;
            put(raw, 14, floatToBits(sid === 50 ? 50.25 : 51));
            put(raw, 18, floatToBits(sid === 50 ? 50.5 : 51));
            raw[34] = sid === 50 ? 1 : 2;
            put(raw, 36, sid === 50 ? 13 : 0);
        }
        this.types[82].flags2 = 4;
        this.sounds[138] = 881;
        this.fields.bytes.fill(0xa5);
        put(this.fields.bytes, 0, 0x509f98);
        put(this.fields.bytes, 48, 0x509f78);
        put(this.fields.bytes, 52, 70);
        put(this.fields.bytes, 56, 50);
        this.timeAt(96, 100);
        // 실제 finder 가 표면 → 두 일반 후보 순서로 반환하는 현재 배치이다.
        this.hash.setCell(0, 51, 51, 60);
        this.hash.setCell(1, 25, 25, 61);
        put(this.pool.allocated(61), 4, 62, 2);
        if (kind === 'C' || kind === 'I') {
            this.now = Number(input[0]);
            this.fields.bytes.fill(Number(input[1]) & 0xff);
        } else if (kind === 'A') {
            this.now = Number(input[0]);
            this.timeAt(96, Number(input[1]));
            put(this.pool.allocated(50), 36, Number(input[2]) >>> 0);
            this.pool.allocated(50)[40] = Number(input[3]) & 0xff;
            this.runCount = Number(input[4]) | 0;
        } else {
            this.boss = Number(input[1]) !== 0;
            this.stamps[138] = Number(input[3]);
            this.mask = Number(input[4]);
            this.pool.allocated(61)[11] = Number(input[5]) & 0xff;
            this.types[79].flags2 = Number(input[6]);
        }
        this.logic = this.makeLogic();
    }

    // 현재 전역과 훅을 묶어 본체를 만든다. (2026-11-13 WB04-E358)
    makeLogic(change = () => {}) {
        const hooks = {
            runCount: () => { this.event('Count'); return this.runCount; },
            assertion: (line) => { this.event(`Assert,${line}`); },
            destroy: (sid, flags) => { this.event(`Destroy,${flags}`); },
            sound: (x, y, token) => { this.event(`Sound,${token}`); },
            mayTarget: (source, target) => { this.event(`May,${target}`); return target >= 61 && target <= 62 && ((this.mask >> (target - 61)) & 1) !== 0; },
            weaken: (sid) => { this.event(`Weaken,${sid}`); },
            damageRate: () => 41.5,
            damage: (sid, amount, owner, source) => { this.event(`Damage,${sid},${amount},${owner},${source}`); },
        };
        change(hooks);
        return new RawWindFlyer(this.pool, this.hash, this.types, this.stamps, this.sounds, { now: () => this.now, boss: () => this.boss, frame: () => this.frame }, hooks);
    }

    // raw double 입력을 비트로 설치한다. (2026-11-13 WB04-E358)
    timeAt(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(this.fields.bytes, at, view.getUint32(0, true));
        put(this.fields.bytes, at + 4, view.getUint32(4, true));
    }

    // 실제 효과 순서만 기록한다. (2026-11-13 WB04-E358)
    event(value) { this.trace.push(value); }
}

// 해당 원문 범위의 모든 행을 재생하고 반환 비트·전체 원문·stamp·효과를 비교한다. (2026-11-13 WB04-E358)
function compareWindFlyer(kind, expected) {
    let seen = 0;
    // 다른 몸체의 관측은 별도 검사에 맡긴다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        if (row[0] !== kind) continue;
        const input = row[1].split(',');
        const scene = new WindFlyerScene(kind, input);
        let result = 0;
        seen += 1;
        if (kind === 'C') {
            const base = new RawFlyerProcess(scene.pool, scene.types, { server: () => true, now: () => scene.now, processType: () => 31 }, {
                attach: (fields, parent, processType, flags) => {
                    scene.event(`Attach,${processType},${parent},${flags}`);
                    put(fields.bytes, 52, 70);
                    put(fields.bytes, 56, 50);
                },
            });
            base.construct(scene.fields, 50, 17, floatToBits(4), 0.5);
            RawWindFlyer.specialize(scene.fields);
        } else if (kind === 'I') scene.logic.initialize(scene.fields);
        else if (kind === 'A') result = scene.logic.animation(scene.fields) >>> 0;
        else if (kind === 'R') result = floatToBits(scene.logic.handle(50, 0, Number(input[0]) >>> 0, Math.fround(Number(input[2]))));
        else scene.logic.attack(50, Math.fround(Number(input[2])));
        const where = `${kind}/${row[1]}`;
        assert.equal(hex(scene.fields.bytes.subarray(0, 104)), row[2], `${where} 필드`);
        assert.equal(result, Number(row[3]), `${where} 반환`);
        assert.equal(scene.stamps[138], Number(row[4]), `${where} 소리 장부`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[5], `${where} 효과 순서`);
    }
    assert.equal(seen, expected);
}

test('WindFlyer: 원본 10.78 생성자 9행 — 기본 부착 뒤 전용 표 쓰기가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareWindFlyer('C', 9); });
test('WindFlyer: 원본 10.78 초기화 9행 — 최초 시각과 20초 대기가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareWindFlyer('I', 9); });
test('WindFlyer: 원본 10.78 애니메이션 180행 — 기한/정지/부호 있는 프레임 경계가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareWindFlyer('A', 180); });
test('WindFlyer: 원본 10.78 주기 처리 72행 — 공격 횟수·수명·소리 장부가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareWindFlyer('R', 72); });
test('WindFlyer: 원본 10.78 범위 공격 64행 — 실제 해시 순회/후보 필터·다리 약화·피해가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareWindFlyer('D', 64); });

test('WindFlyer: 생성자는 PATCH 풀·188칸 TYPE/소리 표·현재 전역 조회기 세 개를 요구하고 TYPE 표는 살아 있는 참조이며 훅 누락은 원인 문구로 거부한다', () => {
    const scene = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    assert.equal(scene.logic.pool, scene.pool);
    const globals = () => ({ now: () => scene.now, boss: () => scene.boss, frame: () => scene.frame });
    const build = (pool, types, stamps, sounds, given) => () => new RawWindFlyer(pool, scene.hash, types, stamps, sounds, given, {});
    assert.throws(build({ edition: 'cd1072' }, scene.types, scene.stamps, scene.sounds, globals()), /PATCH188/);
    assert.throws(build(null, scene.types, scene.stamps, scene.sounds, globals()), TypeError);
    assert.throws(build(scene.pool, scene.types.slice(1), scene.stamps, scene.sounds, globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.stamps.subarray(1), scene.sounds, globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.stamps, scene.sounds.subarray(1), globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.stamps, scene.sounds, null), TypeError);
    for (const name of ['now', 'boss', 'frame']) {
        const given = globals();
        delete given[name];
        assert.throws(build(scene.pool, scene.types, scene.stamps, scene.sounds, given), (error) => error instanceof TypeError && error.message.endsWith(name), name);
        given[name] = 5;
        assert.throws(build(scene.pool, scene.types, scene.stamps, scene.sounds, given), TypeError, `${name} 값`);
    }
    // 훅 묶음을 생략해도 만들 수 있고 도달한 위치에서 같은 문구로 거부한다.
    const bare = new RawWindFlyer(scene.pool, scene.hash, scene.types, scene.stamps, scene.sounds, globals());
    assert.throws(() => bare.handle(50, 5, 0, 0.25), (error) => error.message === '더스트 데빌 실제 하위 미연결');
    assert.throws(() => bare.attack(50, 0.25), (error) => error.message === '더스트 데빌 실제 하위 미연결');
    // 후보의 현재 genus 는 살아 있는 TYPE 표에서 읽는다(만든 뒤 후보 TYPE 의 flags2 를 바꿔도 반영).
    scene.types[78].flags2 = 4;
    scene.trace.length = 0;
    scene.logic.attack(50, 0.25);
    assert.ok(scene.trace.some((line) => line.startsWith('Weaken,61')));
    // 표 밖 TYPE 번호를 가진 후보는 일반 finder 가 거부한다.
    const wild = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    wild.pool.physical(61)[10] = 200;
    assert.throws(() => wild.logic.attack(50, 0.25), RangeError);
    // 앞선 피해 효과가 뒤 피해 대상의 TYPE 번호를 표 밖으로 바꾸면 그 대상 조회를 명시 거부한다.
    const late = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    assert.throws(() => late.makeLogic((given) => {
        const damage = given.damage;
        given.damage = (sid, amount, owner, source) => { damage(sid, amount, owner, source); late.pool.physical(62)[10] = 200; };
    }).attack(50, 0.25), (error) => error instanceof RangeError && error.message === '더스트 데빌 TYPE 번호가 표 밖입니다');
});

test('WindFlyer 칸 계산: 위치는 거의 올림(+0.9999899864196777)→정수 절삭→float 저장→절삭으로 칸이 되고 비유한·범위 밖은 칸 0 이다', () => {
    // 일반 finder 가 처음 조회하는 표면 단계 칸이 곧 계산한 칸이다: 조회를 감시하는 해시로 읽는다.
    class SpyHash extends SquidHash {
        first = null;

        cell(level, x, y) {
            if (level === 0 && this.first === null) this.first = [x, y];
            return super.cell(level, x, y);
        }
    }
    const find = (x, y) => {
        const scene = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
        scene.hash = new SpyHash();
        scene.logic = scene.makeLogic();
        put(scene.pool.allocated(50), 14, floatToBits(x));
        put(scene.pool.allocated(50), 18, floatToBits(y));
        scene.logic.attack(50, 0.25);
        return scene.hash.first;
    };
    assert.deepEqual(find(50.25, 50.5), [51, 51]);
    // 49.0 은 49.99999 → 49, 49 + 1 ulp 은 49.99999762 → 49, 49 + 2 ulp(49.00001525878906)는 50.0000052 → 50.
    assert.deepEqual(find(49, 49), [49, 49]);
    assert.deepEqual(find(49.00000762939453, 49.00000762939453), [49, 49]);
    assert.deepEqual(find(49.00001525878906, 49.00001525878906), [50, 50]);
    // x 와 y 는 각각 독립 계산이다.
    assert.deepEqual(find(10.5, 20.5), [11, 21]);
    assert.deepEqual(find(20.5, 10.5), [21, 11]);
    // 1 미만에서는 float 상수와 배정도 0.99999 가 갈린다(1.0005e-5 + 0.99998998… < 1 이라 칸 0).
    assert.deepEqual(find(Math.fround(1.0005e-5), Math.fround(1.0005e-5)), [0, 0]);
    // 0 은 칸 0 이고 NaN 은 범위 밖이라 칸 0 이다.
    assert.deepEqual(find(0, 0), [0, 0]);
    assert.deepEqual(find(Number.NaN, 5.5), [0, 6]);
});

test('WindFlyer 범위 공격: 후보는 최대 10개이고 flags2 비트 4 후보는 중복 저장되며 약화/피해는 현재 free 비트와 owner 를 다시 읽고 피해량은 정수 절삭이다', () => {
    const scene = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    // 칸 하나의 체인에 후보 12개(SID 100..111, 모두 TYPE 78/발사 가능)를 올린다.
    for (let index = 0; index < 12; index += 1) {
        const raw = scene.pool.physical(100 + index);
        raw.fill(0);
        raw[10] = 78;
        put(raw, 14, floatToBits(51.5));
        put(raw, 18, floatToBits(51.5));
        put(raw, 4, index < 11 ? 101 + index : 0, 2);
    }
    scene.hash.setCell(0, 51, 51, 100);
    const hooks = (change = () => {}) => scene.makeLogic((given) => { given.mayTarget = (source, target) => { scene.event(`May,${target}`); return target >= 100; }; change(given); });
    hooks().attack(50, 0.25);
    const damaged = scene.trace.filter((line) => line.startsWith('Damage,'));
    assert.equal(damaged.length, 10);
    assert.equal(damaged[0], 'Damage,100,10,1,50');
    assert.equal(damaged[9], 'Damage,109,10,1,50');
    // flags2 비트 4 이면서 발사 가능한 후보는 두 번 저장되어 약화도 두 번이다.
    const twice = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    twice.types[82].flags2 = 4;
    twice.makeLogic((given) => { given.mayTarget = (source, target) => { twice.event(`May,${target}`); return target === 60; }; }).attack(50, 0.25);
    assert.equal(twice.trace.filter((line) => line === 'Weaken,60').length, 2);
    // 피해 효과가 뒤 후보의 free 비트를 바꾸면 그 후보는 건너뛴다(현재 값을 다시 읽음).
    const freed = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    freed.makeLogic((given) => {
        const damage = given.damage;
        given.damage = (sid, amount, owner, source) => { damage(sid, amount, owner, source); freed.pool.physical(62)[11] = 1; };
    }).attack(50, 0.25);
    assert.deepEqual(freed.trace.filter((line) => line.startsWith('Damage,')), ['Damage,61,10,1,50']);
    // 피해량 = 정수 절삭(DPS × 주기): 음수는 0 방향, NaN·범위 밖은 0, 2^32 를 넘는 값은 하위 DWORD.
    const amount = (rate) => {
        const probe = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
        probe.makeLogic((given) => { given.damageRate = () => rate; }).attack(50, 0.5);
        return probe.trace.filter((line) => line.startsWith('Damage,61')).map((line) => Number(line.split(',')[2]));
    };
    assert.deepEqual(amount(41.5), [20]);
    assert.deepEqual(amount(-41.5), [-20]);
    assert.deepEqual(amount(Number.NaN), [0]);
    assert.deepEqual(amount(1e30), [0]);
    assert.deepEqual(amount(6e9), [-1294967296]);
});

test('WindFlyer 주기 처리: 80 미만은 공격(주기 0.25 가 아니면 진단 0x19e), 80 이상은 boss 만 삭제하고 40 의 배수에서 한 프레임에 한 번만 소리를 낸다', () => {
    const handle = (count, { boss = true, period = 0.25, frame = 77, stamp = 0 } = {}) => {
        const scene = new WindFlyerScene('R', [String(count), '1', String(period), String(stamp), '3', '0', '0']);
        scene.boss = boss;
        scene.frame = frame;
        scene.stamps[138] = stamp;
        const result = scene.logic.handle(50, 0, count >>> 0, Math.fround(period));
        return { scene, result };
    };
    // 79 는 공격(주기 확인 후), 80 은 boss 면 삭제(−1)하고 boss 가 아니면 공격 없이 소리 조건으로 넘어간다.
    const attacking = handle(79);
    assert.ok(attacking.scene.trace.some((line) => line.startsWith('May,')) && attacking.result === 0.25);
    const dying = handle(80);
    assert.deepEqual(dying.scene.trace, ['Destroy,0']);
    assert.equal(dying.result, -1);
    const quiet = handle(80, { boss: false });
    assert.ok(!quiet.scene.trace.some((line) => line.startsWith('May,') || line.startsWith('Destroy,')));
    assert.equal(quiet.result, 0.25);
    assert.ok(quiet.scene.trace.includes('Sound,881'));
    // 주기가 0.25 가 아니면 진단 0x19e 뒤에도 0.25 로 공격한다.
    const odd = handle(10, { period: 0.5 });
    assert.equal(odd.scene.trace[0], 'Assert,414');
    assert.ok(odd.scene.trace.some((line) => line.startsWith('May,')));
    // 소리: 40 의 배수(음수·0 포함)에서만, 같은 프레임에 이미 낸 타입은 건너뛰고 장부를 현재 프레임으로 쓴다.
    for (const [count, sound] of [[40, true], [0, true], [41, false], [39, false], [(-40) >>> 0, true], [(-41) >>> 0, false]]) {
        const scene = handle(count, { boss: false }).scene;
        assert.equal(scene.trace.some((line) => line.startsWith('Sound,')), sound, `count ${count | 0}`);
        assert.equal(scene.stamps[138], sound ? 77 : 0, `장부 ${count | 0}`);
    }
    const repeat = handle(40, { boss: false, stamp: 77 });
    assert.ok(!repeat.scene.trace.some((line) => line.startsWith('Sound,')));
    // 공통 피해 사건(event 0 이 아님)은 훅으로 넘기고 그 반환을 그대로 쓴다.
    const event = new WindFlyerScene('R', ['0', '1', '0.25', '0', '3', '0', '0']);
    const forwarded = [];
    const logic = event.makeLogic((given) => { given.damageable = (sid, kind, count, period) => { forwarded.push([sid, kind, count, period]); return 3.5; }; });
    assert.equal(logic.handle(50, 7, 9, 0.75), 3.5);
    assert.deepEqual(forwarded, [[50, 7, 9, 0.75]]);
});

test('WindFlyer 애니메이션: 부모의 종속 head 가 비어 있을 때만 읽을 때마다 진단 0xa4 를 내고, 글자 수 0 과 INT_MIN / −1 은 idiv 무효 경로로 거부한다', () => {
    const run = (setup) => {
        const scene = new WindFlyerScene('A', ['100.25', '100', '13', '0', '22']);
        setup(scene);
        let error = null;
        let result = null;
        try { result = scene.logic.animation(scene.fields); } catch (caught) { error = caught; }
        return { scene, result, error };
    };
    // 기한이 되면 읽기 3회(정지 검사·프레임 읽기 전후)마다 진단하고, 기한 전이면 2회이다.
    const due = run((scene) => { put(scene.pool.allocated(50), 6, 0, 2); });
    assert.deepEqual(due.scene.trace.filter((line) => line.startsWith('Assert,')), ['Assert,164', 'Assert,164']);
    const waiting = run((scene) => { put(scene.pool.allocated(50), 6, 0, 2); scene.timeAt(96, 101); });
    assert.deepEqual(waiting.scene.trace.filter((line) => line.startsWith('Assert,')), ['Assert,164', 'Assert,164']);
    // 낮은 바이트만 0 이어도(head 256) 진단하지 않는다.
    const wide = run((scene) => { put(scene.pool.allocated(50), 6, 256, 2); });
    assert.deepEqual(wide.scene.trace.filter((line) => line.startsWith('Assert,')), []);
    const high = run((scene) => { put(scene.pool.allocated(50), 6, 1, 2); });
    assert.deepEqual(high.scene.trace.filter((line) => line.startsWith('Assert,')), []);
    // 글자 수 0 은 0 나눗셈, INT_MIN / −1 은 오버플로이다(프레임 0x7fffffff + 1).
    assert.ok(run((scene) => { scene.runCount = 0; }).error instanceof RangeError);
    assert.ok(run((scene) => { put(scene.pool.allocated(50), 36, 0x7fffffff); scene.runCount = -1; }).error instanceof RangeError);
    // 그 밖의 −1 은 나머지가 0 이고, 음수 프레임은 피제수 부호를 따른다.
    assert.equal(run((scene) => { scene.runCount = -1; }).result, 0);
    assert.equal(run((scene) => { put(scene.pool.allocated(50), 36, 0xfffffff0); scene.runCount = 22; }).result, -15);
    // 정지(+40 의 0x20)는 기한과 무관하게 현재 프레임을 돌려주고 기한을 바꾸지 않는다.
    const paused = run((scene) => { scene.pool.allocated(50)[40] = 0x20; });
    assert.equal(paused.result, 13);
    assert.ok(!paused.scene.trace.includes('Count'));
});

test('WindFlyer: 전용 표 쓰기(specialize)는 E352 의 specializeWind 와 같은 쓰기이다', () => {
    const fields = new FlyerProcessFields();
    fields.bytes.fill(0xa5);
    RawWindFlyer.specialize(fields);
    const expected = new FlyerProcessFields();
    expected.bytes.fill(0xa5);
    put(expected.bytes, 0, 0x509f98);
    put(expected.bytes, 48, 0x509f78);
    put(expected.bytes, 96, 0);
    put(expected.bytes, 100, 0);
    put(expected.bytes, 84, 0);
    assert.deepEqual(fields.bytes, expected.bytes);
});

test('WindFlyer 보강: 피해량 범위(±2^63 근처의 낮은 DWORD)·피해 훅 인자·10개 한도(비트 4 경로)·칸 하나의 사각형을 원문대로 처리한다', () => {
    // 2^63 이상/−2^63 미만은 정수 범위 밖이라 0 이고(낮은 DWORD 가 0 이 아닌 9.3e18 에서 구별), 범위 안은 낮은 DWORD 이다.
    const amount = (rate) => {
        const probe = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
        probe.makeLogic((given) => { given.damageRate = () => rate; }).attack(50, 0.5);
        return probe.trace.filter((line) => line.startsWith('Damage,61')).map((line) => Number(line.split(',')[2]));
    };
    assert.deepEqual(amount(1.86e19), [0]);
    assert.deepEqual(amount(-1.86e19), [0]);
    assert.deepEqual(amount(1.6e19), [Number(BigInt.asIntN(32, BigInt(8e18)))]);
    // 피해율 훅은 (공격자, 대상) 순서로 받는다.
    const rates = [];
    const scene = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    scene.makeLogic((given) => { given.damageRate = (source, target) => { rates.push([source, target]); return 41.5; }; }).attack(50, 0.25);
    assert.deepEqual(rates, [[50, 61], [50, 62]]);
    // 칸 하나의 사각형만 탐색한다: 오른쪽 아래 이웃 칸(52, 52)의 후보는 피해를 받지 않는다.
    const wide = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    const neighbor = wide.pool.physical(63);
    neighbor.fill(0);
    neighbor[10] = 79;
    put(neighbor, 14, floatToBits(52.5));
    put(neighbor, 18, floatToBits(52.5));
    wide.hash.setCell(0, 52, 52, 63);
    wide.makeLogic((given) => { given.mayTarget = (source, target) => { wide.event(`May,${target}`); return true; }; }).attack(50, 0.25);
    assert.ok(!wide.trace.some((line) => line.endsWith(',63') || line.startsWith('May,63')));
    // flags2 비트 4 후보만 12개이면 약화는 10번에서 멈춘다(10개 한도는 두 경로 모두에 적용).
    const weak = new WindFlyerScene('D', ['0', '1', '0.25', '0', '3', '0', '0']);
    for (let index = 0; index < 12; index += 1) {
        const raw = weak.pool.physical(100 + index);
        raw.fill(0);
        raw[10] = 82;
        put(raw, 14, floatToBits(51.5));
        put(raw, 18, floatToBits(51.5));
        put(raw, 4, index < 11 ? 101 + index : 0, 2);
    }
    weak.hash.setCell(0, 51, 51, 100);
    weak.makeLogic((given) => { given.mayTarget = () => false; }).attack(50, 0.25);
    assert.equal(weak.trace.filter((line) => line.startsWith('Weaken,')).length, 10);
});

test('WindFlyer 보강: 주기 처리의 소리 좌표·40 의 배수 경계, 애니메이션의 부모 인자·부호 있는 반환·정수 최솟값 처리', () => {
    // 소리 훅은 (x, y, 토큰) 순서로 현재 위치를 받는다.
    const scene = new WindFlyerScene('R', ['40', '0', '0.25', '0', '3', '0', '0']);
    scene.boss = false;
    const sounds = [];
    scene.makeLogic((given) => { given.sound = (x, y, token) => { sounds.push([x, y, token]); }; }).handle(50, 0, 40, 0.25);
    assert.deepEqual(sounds, [[50.25, 50.5, 881]]);
    // 40 의 배수가 아닌 20·60 은 소리를 내지 않는다.
    for (const count of [20, 60, 100]) {
        const other = new WindFlyerScene('R', [String(count), '0', '0.25', '0', '3', '0', '0']);
        other.boss = false;
        other.logic.handle(50, 0, count, 0.25);
        assert.ok(!other.trace.some((line) => line.startsWith('Sound,')), `count ${count}`);
    }
    // 애니메이션: 글자 수 훅은 현재 부모를 받고, 기한 전 반환은 부호 있는 프레임이며, 0x7fffffff + 1(INT_MIN)은 글자 수 −1 만 거부한다.
    const animate = (setup, prepare = () => {}) => {
        const given = new WindFlyerScene('A', ['100.25', '100', '13', '0', '22']);
        setup(given);
        const parents = [];
        const logic = given.makeLogic((hooks) => { hooks.runCount = (sid) => { parents.push(sid); return given.runCount; }; });
        prepare(given);
        return { result: logic.animation(given.fields), parents };
    };
    assert.deepEqual(animate(() => {}, (given) => { put(given.fields.bytes, 56, 60, 2); }).parents, [60]);
    const waiting = animate((given) => { put(given.pool.allocated(50), 36, 0xfffffff0); given.timeAt(96, 101); });
    assert.equal(waiting.result, -16);
    const wrapped = animate((given) => { put(given.pool.allocated(50), 36, 0x7fffffff); });
    assert.equal(wrapped.result, -2147483648 % 22);
    // 음수 프레임이 글자 수의 배수이면 부호 없는 0 이다(−0 이 아님).
    const zero = animate((given) => { put(given.pool.allocated(50), 36, 0xffffffe9); });
    assert.ok(Object.is(zero.result, 0));
});
