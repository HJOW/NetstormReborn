// Carrier 지면 조회 raw-carrier-check.js(RawCarrierCheck·makeCarrierCheckPostPopHooks/SpawnHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carriercheck-x86.tsv 10.78(originals) 676행 — Check(Carrier +0xcc 00426fc0: genus 제외→좌표 거의 올림 칸 spot&6→가상 +0xc8 분배)와 Try(사제 낙하 요청 004941f0/00494e80)의 입력(genus·지면·
//    프레임 방향·권한·할당·변경 단계·가상 표·응답·좌표·extra·상태) → 반환·호출 기록·슬롯 50바이트·현재 권한. 실제 Carrier 조회/사제 낙하 모듈을 쓰고 보호막/예약/공간/소리/프레임만 명시 대체한다.
//    CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: CarrierCheck_ComposesSurfaceFallAndProcessScheduling(표면 삭제 대신 같은 가상 +0xc8 을 직접 호출해 실제 공유 Form/Kernel 재예약과 Carrier postPop 의 조회 연결)·
//    CarrierCheck_RejectsInvalidBindingsAndReadsCurrentTables(현재 표 재조회·제외 genus 의 좌표 생략·같은 풀 연결 거부). 섬 생명주기(RawIslandLifecycle)·사제 나선 생성(RawPriestSpawn)
//    경로는 해당 파일을 옮길 때 이어서 검사한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E65: Carrier 지면 조회 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { RawPriestState } from '../www/core/raw-priest-state.js';
import { RawCarrierPostPop, CarrierPostPopState, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { RawPriestFall, PriestFallState, PRIEST_FALL_EVENT, makePriestFallHandler, makePriestFallWalker } from '../www/core/raw-priest-fall.js';
import { RawCarrierCheck, makeCarrierCheckPostPopHooks, makeCarrierCheckSpawnHooks } from '../www/core/raw-carrier-check.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E65)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carriercheck-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 관찰 SID, 판본당 독립 행 수, 합성 가상 표(호스트 함수 주소가 아님)이다. (2026-11-08 WB04-E65)
const SOURCE = 50;
const ROWS = 676;
const SYNTHETIC_VTABLE = 0x11313000;

// 예외가 나는지 본다. (2026-11-08 WB04-E65)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E65)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-E65)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 원본 입력과 같은 raw 자산/표/외부 효과를 소유하는 검사 장면이다. (2026-11-08 WB04-E65)
class CheckScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        // 원본 번호 정책을 유지하며 예약 슬롯을 건너뛰고 SID 50 까지 확보한다.
        for (let sid = 5; sid <= SOURCE; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.store = new ScrambledSpStore(new GameRandom(), () => 1);
        this.book = new SquidPostPopState();
        this.hpMode = new SquidRewardState();
        this.types = formTypeInputs();
        this.frames = this.types.map(() => new FrameCodeTable([]));
        this.choices = this.types.map(() => ({ falling: 0, landed: 0 }));
        this.spots = new Uint8Array(65536);
        this.surfaces = new Uint16Array(65536);
        this.mode = new PriestFallState();
        this.events = [];
        this.allocated = false;
        this.change = 0;
        this.reply = 0;
        this.types[PRIEST_TYPE].maxHitPoints = 200;
        this.types[PRIEST_TYPE].flags1 = 0x69012;
        this.types[PRIEST_TYPE].flags2 = 0x210000;
        this.types[PRIEST_TYPE].footX = 1;
        this.types[PRIEST_TYPE].footY = 1;
        this.hp = new SquidReward(this.pool, this.types, this.book, this.hpMode, this.store);
    }

    // 한 행의 raw/전역/지면 입력만 공급한다. 낙하 여부/반환은 생산 코드가 결정한다.
    prepare(row) {
        assert.equal(row.length, 19);
        this.events = [];
        this.types[PRIEST_TYPE].flags2 = Number(row[2]) >>> 0;
        this.mode.authority = Number(row[6]) !== 0;
        this.allocated = Number(row[7]) !== 0;
        this.change = Number(row[8]);
        this.reply = Number(row[10]);
        this.frames[PRIEST_TYPE] = new FrameCodeTable([{ side: Number(row[5]), variant: 80, flags: 0 }]);
        this.spots.fill(0);
        this.spots[21 * 256 + 20] = Number(row[3]);
        this.spots[22 * 256 + 21] = Number(row[4]);
        const raw = this.pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, Number(row[9]) ? PRIEST_VTABLE : SYNTHETIC_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[11] = Number(row[14]);
        raw[34] = 1;
        raw[40] = Number(row[13]);
        put(raw, 26, 20);
        put(raw, 14, Number(row[11]));
        put(raw, 18, Number(row[12]));
    }

    // 보호막/확보/공간 효과에 공급한 권한/좌표 변이만 실행한다.
    mutate(stage) {
        if ((stage === 'shield' && this.change === 1) || (stage === 'new' && this.change === 2)) this.mode.authority = !this.mode.authority;
        if (stage === 'repop' && this.change === 3) {
            const raw = this.pool.allocated(SOURCE);
            put(raw, 14, floatToBits(30.75));
            put(raw, 18, floatToBits(31.9));
        }
    }

    // 낙하 요청의 명시 효과 인자를 기록한다. 0x25b 처리 효과는 이 fixture 에서 실행하지 않는다.
    fallHooks() {
        return {
            ensureShield: (sid) => { this.events.push(`S:${sid}`); this.mutate('shield'); },
            reserveRegular: () => { this.events.push('A:40'); this.mutate('new'); return this.allocated; },
            addSharedRegular: (sid, event, payload) => { this.events.push(`R:${sid}:${event}:${floatToBits(payload)}`); },
            unpop: (sid, flags) => { this.events.push(`U:${sid}:${flags}`); },
            repop: (sid, flags) => { this.events.push(`P:${sid}:${flags}`); this.mutate('repop'); },
            sound: (sid, x, y) => { this.events.push(`W:${sid}:${floatToBits(x)}:${floatToBits(y)}`); },
            setFrame: () => assert.fail('조회 장면에서 프레임 지정에 도달하면 안 된다'),
            advanceFrame: () => assert.fail('조회 장면에서 프레임 진행에 도달하면 안 된다'),
            clearShield: () => assert.fail('조회 장면에서 보호막 해제에 도달하면 안 된다')
        };
    }

    // 합성 가상 +0xc8 의 DWORD 입력을 기록하고 0 아님으로 반환한다.
    otherWalker(sid) {
        this.events.push(`V:${sid}:${this.reply}`);
        return this.reply !== 0;
    }

    // 원본과 같은 구분자로 모든 사건의 순서를 보존한다.
    text() {
        return this.events.length === 0 ? '-' : this.events.join(';');
    }

    // 현재 장면의 사제 상태 조회기이다.
    priest() {
        return new RawPriestState(this.pool, this.hp, this.types, this.spots);
    }

    // 현재 장면의 낙하 요청기이다.
    fall() {
        return new RawPriestFall(this.pool, this.priest(), this.frames, this.choices, this.surfaces, this.mode, this.fallHooks());
    }
}

test('CarrierCheck: 원본 10.78 676행의 반환·호출 기록·슬롯 50바이트·현재 권한이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CheckScene();
    let count = 0;
    // 기대값을 계산하지 않고 실제 모듈을 각 입력에 연결한다.
    for (const row of rows) {
        scene.prepare(row);
        const fall = scene.fall();
        const check = new RawCarrierCheck(scene.pool, scene.types, scene.spots, makePriestFallWalker(scene.pool, fall, (sid) => scene.otherWalker(sid)));
        const result = row[1] === 'Try' ? fall.tryFall(SOURCE) : check.check(SOURCE);
        assert.equal(Number(result), Number(row[15]), `행 ${count}`);
        assert.equal(scene.text(), row[16], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[17], `행 ${count}`);
        assert.equal(scene.mode.authority, Number(row[18]) !== 0, `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
    assert.equal(scene.store.initialized, false);
});

test('CarrierCheck: 같은 가상 +0xc8 이 공유 Regular 를 재예약하고 Carrier postPop 의 조회는 반환을 버린다', () => {
    const scene = new CheckScene();
    scene.prepare(rows ? rows[0] : ['originals', 'Check', '0', '0', '0', '65', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0']);
    scene.types[PRIEST_TYPE].flags2 = 0x210000;
    scene.change = 0;
    scene.allocated = true;
    scene.mode.authority = true;
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 0, PRIEST_VTABLE);
    raw[11] = 0;
    raw[40] = 32;
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    scene.frames[PRIEST_TYPE] = new FrameCodeTable([{ side: 65, variant: 80, flags: 0 }, { side: 74, variant: 80, flags: 0 }]);
    scene.choices[PRIEST_TYPE] = { falling: 3, landed: 4 };
    const hash = new SquidHash();
    hash.setBucket(1, 20.75, 21.9, SOURCE);
    const unpop = new SquidUnpop(scene.pool, hash, scene.spots);
    const destroy = new RawSquidDestroy(scene.pool, unpop, scene.types);
    const kernel = new Kernel(16);
    const processMode = new SquidProcessState();
    processMode.now = 12.5;
    let fall = null;
    const host = new SquidProcessHost(scene.pool, scene.types, kernel, destroy, processMode, {
        regular: (sid, event, count, payload) => makePriestFallHandler(scene.pool, fall)(sid, event, count, payload)
    });
    const hooks = scene.fallHooks();
    hooks.addSharedRegular = (sid, event, payload) => { host.addSharedRegular(sid, event, payload); };
    hooks.setFrame = (sid, frame, flags) => {
        assert.ok(frame === 3 && flags === 0);
        put(scene.pool.allocated(sid), 36, 1);
    };
    hooks.advanceFrame = (sid, steps, flags) => { assert.ok(steps === 1 && flags === 0); };
    fall = new RawPriestFall(scene.pool, scene.priest(), scene.frames, scene.choices, scene.surfaces, scene.mode, hooks);
    const walker = makePriestFallWalker(scene.pool, fall);
    // 첫 요청은 공유 Regular 를 예약하고 첫 이벤트가 J 를 쓰기 전의 둘째 요청은 새 예약을 만든다.
    assert.equal(walker(SOURCE), true);
    assert.equal(kernel.size, 1);
    assert.equal(walker(SOURCE), true);
    assert.equal(kernel.size, 2);
    kernel.runFrame();
    const process = host.findEvent(SOURCE, PRIEST_FALL_EVENT);
    assert.ok(process && process.count === 1 && process.payload === Math.fround(0.1));
    // 프레임이 이미 J(낙하 방향)이면 새 요청이 없다.
    const before = scene.text();
    assert.ok(walker(SOURCE) === true && kernel.size === 2 && scene.text() === before);
    // 실제 사제 genus 는 +0xcc 에서 제외된다. 비권한 postPop 은 조회 뒤 공통 장부를 계속한다.
    const check = new RawCarrierCheck(scene.pool, scene.types, scene.spots, walker);
    assert.equal(check.check(SOURCE), false);
    const base = new SquidPostPop(scene.pool, scene.types, scene.book);
    const carrierMode = new CarrierPostPopState();
    carrierMode.boss = false;
    const postHooks = makeCarrierCheckPostPopHooks(scene.pool, check, makeCarrierPostPopHooks(scene.pool, base, () => assert.fail('조회 대체 전 훅'), () => assert.fail('지면 소유 미도달')));
    const carrier = new RawCarrierPostPop(scene.pool, scene.types, carrierMode, postHooks);
    scene.events = [];
    scene.book.depth = 1;
    carrier.postPop(SOURCE, 0);
    assert.ok(scene.book.depth === 0 && scene.events.length === 0 && kernel.size === 2);
    // 실제 form 삭제로 부모 체인을 정리한다. 삭제한 객체는 다시 읽지 않는다.
    for (let found = host.findEvent(SOURCE, PRIEST_FALL_EVENT); found; found = host.findEvent(SOURCE, PRIEST_FALL_EVENT)) host.kill(found, 0x10);
    assert.equal(kernel.size, 0);
    // 사제 나선 생성 훅은 같은 조회를 genus 래퍼 뒤에 연결한다. 생성 자체는 RawPriestSpawn 이식 때 검사한다.
    const spawn = makeCarrierCheckSpawnHooks(scene.pool, check, { keep: 1 });
    assert.equal(spawn.keep, 1);
    assert.doesNotThrow(() => spawn.checkCarrier(SOURCE));
});

test('CarrierCheck: 현재 표를 다시 읽고 제외 genus 는 좌표를 읽지 않으며 잘못된 연결은 거부한다', () => {
    const scene = new CheckScene();
    scene.prepare(rows ? rows[0] : ['originals', 'Check', '0', '0', '0', '65', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0']);
    let calls = 0;
    const check = new RawCarrierCheck(scene.pool, scene.types, scene.spots, (sid) => { assert.equal(sid, SOURCE); calls += 1; return true; });
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    scene.spots.fill(0);
    scene.types[PRIEST_TYPE].flags2 = 0;
    assert.ok(check.check(SOURCE) && calls === 1);
    scene.spots[22 * 256 + 21] = 4;
    assert.ok(!check.check(SOURCE) && calls === 1);
    scene.spots[22 * 256 + 21] = 0;
    assert.ok(check.check(SOURCE) && calls === 2);
    // 제외 genus 는 NaN/무한대 좌표를 읽지 않고 거짓이다.
    scene.types[PRIEST_TYPE].flags2 = 0x210000;
    put(raw, 14, 0x7fc12345);
    put(raw, 18, 0x7f800000);
    put(raw, 36, 0xffffffff);
    assert.ok(!check.check(SOURCE) && calls === 2);
    // 제외되지 않은 genus 는 비유한 좌표를 진단한다.
    scene.types[PRIEST_TYPE].flags2 = 0;
    assert.ok(throws(() => check.check(SOURCE)));
    assert.ok(throws(() => new RawCarrierCheck(scene.pool, [], scene.spots, () => true)));
    assert.ok(throws(() => new RawCarrierCheck(scene.pool, scene.types, scene.spots, null)));
    const other = new SidPool(24000, { server: false });
    assert.ok(throws(() => makeCarrierCheckPostPopHooks(other, check, {})));
    assert.ok(throws(() => makeCarrierCheckSpawnHooks(other, check, {})));
    // 사제가 아닌 vtable 은 명시 fallback 없이는 낙하 분배를 거부한다.
    const fall = scene.fall();
    put(raw, 0, SYNTHETIC_VTABLE);
    const walker = makePriestFallWalker(scene.pool, fall);
    assert.ok(throws(() => walker(SOURCE)));
    const fallback = makePriestFallWalker(scene.pool, fall, (sid) => { assert.equal(sid, SOURCE); return true; });
    assert.ok(fallback(SOURCE) && scene.events.length === 0);
});
