// cpppj의 실제 Host 수명 시험과 다섯 애니메이션 공장의 원문 계약을 대조한다. 게임 실행/교전 검사가 아니다. (2026-11-16 WB04-E566)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { GunProcess, GunProcessState, RawGunProcess, createGunProcess } from '../www/core/raw-gun-process.js';
import { RawGunAnimation, createGunAnimation, makeGunAnimationHostHooks, makeGunAnimationProcessHooks } from '../www/core/raw-gun-animation.js';
import { ArcherAnimationState, RawArcherAnimation, createArcherAnimation, makeArcherAnimationHostHooks, makeArcherAnimationProcessHooks, makeArcherMissileHooks } from '../www/core/raw-archer-animation.js';
import { CannonFireState, RawCannonFire, createCannonFire, makeCannonFireHostHooks } from '../www/core/raw-cannon-fire.js';
import { CrossbowAnimationState, RawCrossbowAnimation, createCrossbowAnimation, makeCrossbowAnimationHostHooks } from '../www/core/raw-crossbow-animation.js';
import { RawIceCannonFire, createIceCannonFire, makeIceCannonFireHostHooks } from '../www/core/raw-ice-cannon-fire.js';
import { put, get } from './raw-support.mjs';

// cpppj의 SID50 부모·SID60 표적·실제 Kernel/Form/파괴/전송 장면을 10.78로 구성한다. (2026-11-16 WB04-E566)
function scene(type = 128) {
    // 실제 공유 풀과 내장 ProcessForm 생성자 표이다.
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, group: 0 }));
    // 원문의 공통 Form 네 종류만 명시한다.
    for (const form of [1, 2, 3, 6]) types[form].constructorAddress = typeConstructorAddress(form);
    types[type].group = type === 71 ? 0 : 1;
    // cpppj와 같은 실제 SID까지 할당한다.
    for (let id = 5; id <= 60; id += 1) assert.equal(pool.allocate(2), id);
    pool.physical(50).fill(0); pool.physical(50)[10] = type;
    pool.physical(60).fill(0); pool.physical(60)[10] = 158; pool.physical(60)[11] = 2;
    // 실제 프로세스 수명과 전송 관찰 장치이다.
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel(64);
    const hostState = new SquidProcessState(); hostState.now = 10;
    const gunState = new GunProcessState(); gunState.now = 10; gunState.boss = false;
    const packet = new Uint8Array(64); const packets = [];
    // 현재 Form 재조회와 삭제 인수를 관찰하는 외부 장치이다.
    const host = new SquidProcessHost(pool, types, kernel, destroy, hostState, {
        transmit: {
            // 로컬 애니메이션은 전송하지 않으며 실제 Gun 갱신만 관찰한다.
            writeData: (process, changed) => { assert.ok(process instanceof GunProcess); return process.writePacket(packet, changed); },
            transmit: (size) => packets.push(Uint8Array.from(packet.subarray(0, size))),
        },
    });
    // 실제 Gun 애니메이션 처리기이다.
    const gun = createGunProcess(host, new RawGunProcess(pool, types, gunState.globals(), {}), 50);
    const frames = types.map(() => ({ frames: new FrameCodeTable([{ side: 65, variant: 0, flags: 0 }, { side: 65, variant: 0, flags: 0 }]) }));
    const data = types.map(() => ({ directions: 20, frameCount: 40 }));
    // 현재 TYPE과 시각을 읽는 처리기 상태이다.
    const state = { now: 10, sunCannonType: type };
    // 명시 외부 좌표/공간 장치이며 Host/Kernel을 대체하지 않는다.
    const common = { center: (sid) => sid === 50 ? [50, 50] : [50, 40], unpop: () => {}, repop: () => {} };
    return { pool, types, destroy, kernel, hostState, gunState, host, gun, frames, data, state, common, packets };
}

// 완료 뒤 실제 Gun 단계·Form 반환·Kernel과 파괴 깊이를 검사한다. (2026-11-16 WB04-E566)
function finished(s, process, stage) {
    // 삭제 전 보관한 실제 Form 번호이다.
    const form = process.form;
    assert.equal(s.kernel.size, 2);
    process.runFrame();
    assert.equal(s.gun.fields.stage, stage);
    assert.equal(s.host.fromForm(form), null);
    assert.equal(s.kernel.size, 1);
    assert.ok(s.pool.slot(form)[11] & 3);
    assert.ok(s.packets.length > 0);
    assert.deepEqual([s.destroy.preDepth, s.destroy.postDepth], [0, 0]);
    s.host.kill(s.gun, 0x40); assert.equal(s.kernel.size, 0);
}

test('GunAnimation cpppj PATCH Host 시험: 회전 완료가 Gun 단계4를 쓰고 실제 Form/Kernel을 반환한다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene();
    s.common.center = (sid) => sid === 50 ? [50, 50] : [50, 60];
    // 시험하는 실제 파생 프레임 처리기이다.
    const logic = new RawGunAnimation(s.pool, s.types, s.state, makeGunAnimationHostHooks(s.host, s.common));
    put(s.gun.fields.bytes, 28, 3);
    // 실제 공장으로 부착한 애니메이션이다.
    const animation = createGunAnimation(s.host, logic, 2, s.gun.form, 60);
    assert.deepEqual([animation.fields.gunForm, animation.parent, animation.fields.time, s.pool.slot(animation.form)[10]], [s.gun.form, 50, 10, 36]);
    finished(s, animation, 4);
});

test('Archer cpppj PATCH Host 시험: 회전 완료와 부착 뒤 발사 초기화·완료가 실제 Gun/Form을 갱신한다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene(71); const surface = [];
    // cpppj의 살아 있는 표적과 출력 초기 비트를 재현한다.
    s.pool.physical(60)[11] = 0; s.types[158].flags1 = 0x10; s.pool.physical(50)[12] = 0xa5;
    // 현재 TYPE과 시각을 읽는 처리기 상태이다.
    const state = new ArcherAnimationState({ now: 10, delta: 0 });
    // 시험하는 실제 파생 프레임 처리기이다.
    const logic = new RawArcherAnimation(s.pool, s.types, s.data, state, makeArcherAnimationHostHooks(s.host, { common: s.common, notifySurface: (sid) => surface.push(sid) }));
    put(s.gun.fields.bytes, 28, 3);
    // 실제 회전 프로세스이다.
    const rotate = createArcherAnimation(s.host, logic, 2, s.gun.form, 60);
    rotate.runFrame();
    assert.deepEqual([s.gun.fields.stage, s.host.fromForm(rotate.form), s.kernel.size], [4, null, 1]);
    put(s.gun.fields.bytes, 28, 6);
    // 실제 발사 프로세스이다.
    const fire = createArcherAnimation(s.host, logic, 5, s.gun.form, 60);
    assert.deepEqual([surface, s.pool.slot(50)[12] & 1, get(fire.fields.bytes, 64)], [[50], 0, 0x43b40000]);
    put(fire.fields.bytes, 64, 0x43870000); put(fire.fields.bytes, 68, 0); put(fire.fields.bytes, 80, 1);
    finished(s, fire, 7);
});

test('CannonFire cpppj PATCH Host 시험: 썬더의 죽은 표적 완료가 Gun 단계7과 실제 자기 삭제로 이어진다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene(131);
    s.frames[131].frames = new FrameCodeTable([{ side: 65, variant: 0, flags: 0 }]);
    // 시험하는 실제 파생 프레임 처리기이다.
    const logic = new RawCannonFire(s.pool, s.types, s.frames, new CannonFireState({ now: 10 }), makeCannonFireHostHooks(s.host, s.common));
    put(s.gun.fields.bytes, 28, 6);
    finished(s, createCannonFire(s.host, logic, s.gun.form, 60), 7);
});

test('IceCannonFire cpppj PATCH Host 시험: 표적 판정 실패가 실제 Gun 단계7·자기 삭제로 이어진다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene(129);
    // 현재 TYPE과 시각을 읽는 처리기 상태이다.
    const state = new CannonFireState({ now: 10, type: 129, displayType: 130, delay: Math.fround(0.08) });
    // 시험하는 실제 파생 프레임 처리기이다.
    const logic = new RawIceCannonFire(s.pool, s.types, s.frames, state, makeIceCannonFireHostHooks(s.host, s.common), { context: () => new Uint32Array(8), mayTarget: () => false });
    put(s.gun.fields.bytes, 28, 6);
    finished(s, createIceCannonFire(s.host, logic, s.gun.form, 60), 7);
});

test('Crossbow Host 소스 계약: 죽은 표적의 내림 완료도 실제 Gun 단계7·Form 반환으로 이어진다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene(140);
    // 시험하는 실제 파생 프레임 처리기이다.
    const logic = new RawCrossbowAnimation(s.pool, s.types, s.data, new CrossbowAnimationState({ now: 10 }), makeCrossbowAnimationHostHooks(s.host, s.common));
    put(s.gun.fields.bytes, 28, 6);
    finished(s, createCrossbowAnimation(s.host, logic, s.gun.form, 60), 7);
});

// 다섯 공장의 같은 풀·현재 TYPE·실제 파생/부착 실패 검사를 공유한다. (2026-11-16 WB04-E566)
function factories(s) {
    // Archer의 현재 TYPE 입력이다.
    const archerState = new ArcherAnimationState({ now: 10, type: 128 });
    // 공통 캐논/아이스의 현재 TYPE 입력이다.
    const cannonState = new CannonFireState({ now: 10, type: 128 });
    // 크로스보우의 현재 TYPE 입력이다.
    const crossbowState = new CrossbowAnimationState({ now: 10, type: 128 });
    // 실제 Gun 애니메이션 처리기이다.
    const gun = new RawGunAnimation(s.pool, s.types, s.state);
    // 현재 TYPE을 읽는 실제 Archer 처리기이다.
    const archer = new RawArcherAnimation(s.pool, s.types, s.data, archerState, { notifySurface: () => { throw new Error('부착 실패 후 각 초기화에 도달'); } });
    // 실제 공통 캐논 처리기이다.
    const cannon = new RawCannonFire(s.pool, s.types, s.frames, cannonState);
    // 실제 크로스보우 처리기이다.
    const crossbow = new RawCrossbowAnimation(s.pool, s.types, s.data, crossbowState);
    // 실제 아이스 캐논 처리기이다.
    const ice = new RawIceCannonFire(s.pool, s.types, s.frames, cannonState);
    return [
        (host) => createGunAnimation(host, gun, 5, s.gun.form, 60),
        (host) => createArcherAnimation(host, archer, 5, s.gun.form, 60),
        (host) => createCannonFire(host, cannon, s.gun.form, 60),
        (host) => createCrossbowAnimation(host, crossbow, s.gun.form, 60),
        (host) => createIceCannonFire(host, ice, s.gun.form, 60),
    ];
}

test('다섯 공장 소스 계약: 다른 풀·잘못된 실제 Gun/부모를 거부하고 부착 실패는 null이며 각 초기화를 건너뛴다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene(); const calls = factories(s);
    // 먼저 풀을 검사하고 실제 Gun 형식과 현재 부모 TYPE을 확인한다.
    for (const call of calls) {
        assert.throws(() => call({ pool: {} }), /불일치/);
        assert.throws(() => call({ pool: s.pool, fromForm: () => new SquidProcess() }), /GunProcess|부모/);
        s.pool.physical(50)[10] = 127;
        assert.throws(() => call(s.host), /부모|파생/);
        s.pool.physical(50)[10] = 128;
        // 원문 발사 TYPE 조회·부모·flags를 실제 필드 초기화 후 명시 부착 경계에서 관찰한다.
        const attached = [];
        // 부착 실패를 명시 반환하는 관찰 장치이다.
        const refused = { pool: s.pool, fromForm: () => s.gun, animationProcessType: (fire) => { assert.equal(fire, true); return 39; }, attach: (process, type, parent, flags) => { attached.push([type, parent, flags]); return null; } };
        assert.equal(call(refused), null);
        assert.deepEqual(attached, [[39, 50, 0x10]]);
        // dynamic_cast와 같이 null·다른 파생 결과를 null로 반환한다.
        for (const result of [null, new SquidProcess()]) {
            assert.equal(call({ pool: s.pool, fromForm: () => s.gun, animationProcessType: () => 35, attach: () => result }), null);
        }
    }
    // 시험하는 실제 파생 프레임 처리기이다.
    const logic = new RawGunAnimation(s.pool, s.types, s.state);
    assert.throws(() => createGunAnimation(s.host, logic, 3, s.gun.form, 60), RangeError);
    // 현재 TYPE을 읽는 실제 Archer 처리기이다.
    const archer = new RawArcherAnimation(s.pool, s.types, s.data, new ArcherAnimationState({ type: 128 }));
    assert.throws(() => createArcherAnimation(s.host, archer, 8, s.gun.form, 60), /단계/);
});

test('Gun 공장 소스 계약: 세 단계의 현재 장기 프로세스 TYPE·현재 부모 TYPE을 다음 생성에서 다시 읽는다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene(); const aux = new GameAuxState();
    s.hostState.bindGameAux(aux);
    // 현재 TYPE과 시각을 읽는 처리기 상태이다.
    const state = { ...s.state, currentSunCannonType: { value: 128 } };
    // 시험하는 실제 파생 프레임 처리기이다.
    const logic = new RawGunAnimation(s.pool, s.types, state);
    // 원문 단계2/5/8을 서로 다른 현재 TYPE에 부착한다.
    for (const [stage, type, vt] of [[2, 36, 0x50b260], [5, 35, 0x50b318], [8, 37, 0x50b2c8]]) {
        aux.rotateAnimationProcessType = stage === 8 ? 37 : 36;
        // 실제 공장으로 부착한 애니메이션이다.
        const animation = createGunAnimation(s.host, logic, stage, s.gun.form, 60);
        assert.deepEqual([s.pool.slot(animation.form)[10], get(animation.fields.bytes, 0), animation.fields.parent], [type, vt, 50]);
    }
    aux.fireAnimationProcessType = 38;
    state.currentSunCannonType.value = 127;
    assert.throws(() => createGunAnimation(s.host, logic, 5, s.gun.form, 60), /파생/);
    s.pool.physical(50)[10] = 127;
    assert.equal(s.pool.slot(createGunAnimation(s.host, logic, 5, s.gun.form, 60).form)[10], 38);
});

test('완료 어댑터 소스 계약: 입력 훅 사본·현재 Form 재조회·실제 파생 형식 검사를 보존한다', () => {
    // 실제 조회·통지/삭제와 fallback 호출 기록이다.
    const events = []; let current = null;
    // 현재 Form 재조회와 삭제 인수를 관찰하는 외부 장치이다.
    const host = { fromForm: (form) => { events.push(['조회', form]); return current; }, kill: (process, flags) => events.push(['삭제', process, flags]) };
    // 호출자 훅 보존을 식별하는 함수이다.
    const marker = () => 7;
    // 값 사본 보존을 검사할 호출자 훅 묶음이다.
    const input = { sound: marker, common: { sound: marker }, notifySurface: marker };
    // 다섯 실제 Host 훅 생성 함수이다.
    const adapters = [makeGunAnimationHostHooks, makeCannonFireHostHooks, makeCrossbowAnimationHostHooks, makeIceCannonFireHostHooks, makeArcherAnimationHostHooks];
    // 다섯 완료 콜백은 실제 Gun 및 각 파생 외의 객체를 빈 성공으로 처리하지 않는다.
    for (const make of adapters) {
        // 원문 값 인수처럼 복사하여 만든 훅 묶음이다.
        const output = make(host, input); const common = output.common && make === makeArcherAnimationHostHooks ? output.common : output;
        assert.equal(common.sound, marker); assert.equal(output.notifySurface, marker);
        current = null; assert.throws(() => common.notify(1, 2, 3, 4), /GunProcess/);
        assert.throws(() => common.kill(1, 0x10), /process|animation|Process/);
        current = new SquidProcess(); assert.throws(() => common.kill(1, 0x10));
        assert.notEqual(output, input);
        if (make === makeArcherAnimationHostHooks) assert.notEqual(output.common, input.common);
    }
    assert.equal(input.sound, marker); assert.equal(input.common.sound, marker);
});

test('생성 어댑터 소스 계약: Gun 실제 부착 실패·Archer 현재 TYPE 분배와 값으로 캡처한 fallback을 유지한다', () => {
    // 실제 부모·Gun·Host·Kernel을 공유하는 시험 장면이다.
    const s = scene(); const events = [];
    // 같은 풀의 실제 Gun 애니메이션 처리기이다.
    const gunLogic = new RawGunAnimation(s.pool, s.types, s.state);
    // Archer의 현재 TYPE 입력이다.
    const archerState = new ArcherAnimationState({ type: 71 });
    // 현재 TYPE을 읽는 실제 Archer 처리기이다.
    const archer = new RawArcherAnimation(s.pool, s.types, s.data, archerState);
    // 값 사본 보존을 검사할 호출자 훅 묶음이다.
    const input = { startAnimation: (...args) => events.push(args), marker: 7 };
    // 원문 값 인수처럼 복사하여 만든 훅 묶음이다.
    const output = makeArcherAnimationProcessHooks(s.host, archer, input);
    input.startAnimation = () => { throw new Error('입력 재대입이 누출됨'); };
    output.startAnimation(8, s.gun.form, 60); assert.deepEqual(events, [[8, s.gun.form, 60]]); assert.equal(output.marker, 7);
    assert.throws(() => makeArcherAnimationProcessHooks(s.host, archer).startAnimation(8, s.gun.form, 60), /미복원/);
    // 현재 Gun 조회 실패를 명시하는 장치이다.
    const missing = { pool: s.pool, fromForm: () => null };
    assert.throws(() => makeArcherAnimationProcessHooks(missing, archer).startAnimation(2, s.gun.form, 60), /GunProcess/);
    // 실제 필드 초기화 뒤 부착 실패를 반환하는 장치이다.
    const failed = { pool: s.pool, fromForm: () => s.gun, animationProcessType: () => 36, attach: () => null };
    assert.throws(() => makeGunAnimationProcessHooks(failed, gunLogic).startAnimation(2, s.gun.form, 60), /factory 실패/);
    archerState.currentType = { value: 128 };
    assert.throws(() => makeArcherAnimationProcessHooks(failed, archer).startAnimation(2, s.gun.form, 60), /factory 실패/);
    // 다른 풀은 어댑터를 만드는 시점에 거부한다.
    assert.throws(() => makeGunAnimationProcessHooks({ pool: {} }, gunLogic), /불일치/);
    assert.throws(() => makeArcherAnimationProcessHooks({ pool: {} }, archer), /불일치/);
    assert.throws(() => makeArcherMissileHooks({ pool: {} }, { pool: s.pool }), /불일치/);
    // 같은 Pool/Host의 실제 Missile 공장을 빌린 Archer 훅이다.
    const missileHooks = makeArcherMissileHooks(s.host, { pool: s.pool }, { common: s.common, marker: 9 });
    assert.equal(missileHooks.marker, 9); assert.equal(missileHooks.common.center, s.common.center);
    assert.notEqual(missileHooks.common, s.common); assert.equal(typeof missileHooks.common.createProjectile, 'function');
});

test('생성 어댑터 소스 계약: Gun과 현재 TYPE Archer 분기가 실제 Host에 회전을 붙인다', () => {
    // Gun 분기는 입력의 다른 훅을 보존하면서 실제 회전 Form을 만든다.
    const s = scene();
    // 같은 풀의 실제 Gun 애니메이션 처리기이다.
    const gunLogic = new RawGunAnimation(s.pool, s.types, s.state);
    // 실제 Gun 회전 공장으로 연결한 훅이다.
    const gunHooks = makeGunAnimationProcessHooks(s.host, gunLogic, { marker: 3 });
    gunHooks.startAnimation(2, s.gun.form, 60);
    assert.deepEqual([s.kernel.size, gunHooks.marker], [2, 3]);
    // Archer는 다음 호출의 현재 TYPE을 읽고 fallback 대신 실제 공장을 선택한다.
    const state = new ArcherAnimationState({ type: 71, currentType: { value: 128 } });
    // 현재 TYPE을 읽는 실제 Archer 처리기이다.
    const archer = new RawArcherAnimation(s.pool, s.types, s.data, state);
    // 현재 TYPE Archer 우선 분배로 연결한 훅이다.
    const hooks = makeArcherAnimationProcessHooks(s.host, archer, { startAnimation: () => { throw new Error('실제 Archer 분기 대신 fallback 선택'); } });
    hooks.startAnimation(2, s.gun.form, 60);
    assert.equal(s.kernel.size, 3);
});
