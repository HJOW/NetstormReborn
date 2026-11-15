// RawCampaignHitBindings를 실제 World/사제/HP/Host로 조립하는 소스 계약 검사다. 전용 원본 조립 fixture나 cpppj 전용 시험은 없다.
// CampaignInstallSupport.h의 연결과 원문 가상 진입별 고정 HP·효과 순서를 검증하며 실제 교전/특정 적 공격 관측은 하지 않는다. (2026-11-16 WB04-E564)
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignHitScene } from './campaign-hit-support.mjs';
import { put, get, floatToBits } from './raw-support.mjs';
import { SquidProcess } from '../www/core/squid-process.js';
import { SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';

// 같은 피격 장면을 초기화하고 빈 지형 로드를 마친다. (2026-11-16 WB04-E564)
function scene(options = {}) { const value = new CampaignHitScene(options); if (options.bind !== false) value.world.load([]); return value; }
// 실제 생성자의 슬롯에 명시 HP/좌표 입력만 둔다. Pop/교전 성공을 이 함수가 대신하지 않는다. (2026-11-16 WB04-E564)
function target(value, type, hp = 10000, owner = 2, x = 30, y = 30) {
    const sid = value.create(type, owner); const raw = value.core.pool.allocated(sid);
    put(raw, 26, hp); put(raw, 14, floatToBits(x)); put(raw, 18, floatToBits(y)); return sid;
}

test('CampaignHitBindings: 다른 HP/사제 Pool과 하위 배열을 효과 전에 거부한다', () => {
    const value = scene({ bind: false }); const other = scene({ bind: false });
    assert.throws(() => value.makeHits({ hp: other.hp })); assert.throws(() => value.makeHits({ priests: other.priests }));
    assert.throws(() => value.makeHits({ speeds: [] }));
    assert.equal(value.hits.pool, value.core.pool);
    // 후속 소비자가 동일한 현재 모듈을 빌린다.
    for (const body of [value.hits.priestDamage, value.hits.generalDamage, value.hits.burning, value.hits.dudes, value.hits.impact]) assert.equal(body.pool, value.core.pool);
});

test('CampaignHitBindings: 사제 HP 변경·두 회복 사건 취소·현재 기본 프레임·소유자 통지', () => {
    const value = new CampaignHitScene(); value.ground(20, 20, 23, 23); value.priests.setLoadingDepth(1);
    const priest = value.create(158, 1); value.world.pop(priest, 21, 21, 3); value.priests.setLoadingDepth(0);
    const hp = value.priests.priestState.currentHitPoints(priest);
    // 취소 helper는 같은 사건의 첫 프로세스를 다시 찾아 중복까지 모두 지워야 한다.
    for (const event of [0x25c, 0x25d, 0x25d]) value.core.processes.addRegular(priest, event, 0.25);
    value.take(); value.hits.damageHooks().damage(priest, 3, 2, 0);
    assert.equal(value.priests.priestState.currentHitPoints(priest), hp - 3);
    assert.equal(value.core.processes.findEvent(priest, 0x25c), null); assert.equal(value.core.processes.findEvent(priest, 0x25d), null);
    assert.ok(value.core.processes.findEvent(priest, 0x25a));
    assert.equal(get(value.core.pool.slot(priest), 36), value.table.definitions[158 - 70].defaultFrame);
    assert.deepEqual(value.take(), [`priest:1:${priest}:0`]);
    assert.equal(value.hits.damageResult(priest, 1, 0), 1); assert.equal(value.priests.priestState.currentHitPoints(priest), hp - 4);
    assert.equal(value.hits.damageResult(priest, hp, 0), 0); assert.equal(value.priests.priestState.currentHitPoints(priest), 1);
});

test('CampaignHitBindings: Base/일반 진입의 HP·source owner BYTE와 미지원 VT fallback', () => {
    const value = scene(); const source = target(value, 80, 1, 7);
    const base = target(value, 80); const general = target(value, 75);
    assert.equal(value.hits.damageResult(base, 3, source), 1); assert.equal(get(value.core.pool.slot(base), 26), 9997);
    assert.deepEqual(value.take(), [`general:2:${base}:${source}`]);
    value.hits.damageHooks().damage(general, 9, 3, source); assert.equal(get(value.core.pool.slot(general), 26), 9991);
    assert.deepEqual(value.take(), [`general:2:${general}:${source}`]);
    put(value.core.pool.physical(general), 0, 0x12345678);
    assert.throws(() => value.hits.damageResult(general, 1, source), /반환형피해/);
    assert.throws(() => value.hits.damageHooks().damage(general, 1, 3, source), /피해/);
    const calls = []; const extra = () => 91;
    const hooks = value.hits.damageHooks({ extra, damage: (...args) => { calls.push(args); } });
    hooks.damage(general, 2, 4, source); assert.deepEqual(calls, [[general, 2, 4, source]]); assert.equal(hooks.extra, extra);
    const dead = target(value, 80, 1);
    assert.equal(value.hits.damageResult(dead, 5, source), 0); assert.equal(value.generalState.killedVictim, dead); assert.equal(value.generalState.killerOwner, 7);
    assert.ok(value.core.pool.slot(dead)[11] & 1);
});

test('CampaignHitBindings: 현재 지면·Player 주 섬·실제 시각 경고 장치가 일반 피해에 연결된다', () => {
    const value = new CampaignHitScene(); value.ground(20, 20, 23, 23);
    const victim = target(value, 75, 10000, 1, 21, 21); value.generalState.primaryIslands[1] = 127;
    value.core.processState.now = 12; assert.equal(value.hits.damageResult(victim, 1, 0), 1);
    assert.deepEqual(value.take(), ['warn-event', 'warn-sound', 'warn-text', `general:1:${victim}:0`]); assert.equal(value.generalState.warningTime, 42);
    value.core.processState.now = 13; value.hits.damageResult(victim, 1, 0);
    assert.deepEqual(value.take(), [`general:1:${victim}:0`]); assert.equal(value.generalState.warningTime, 43);
});

test('CampaignHitBindings: 불워크의 현재 비행 genus와 윈드 타워의 실제 방어 방향', () => {
    const value = scene(); const attacker = target(value, 80, 1, 3, 30, 20); const bulwark = target(value, 79);
    value.core.types[80].flags2 |= 0x100000;
    assert.equal(value.hits.damageResult(bulwark, 5, attacker), 0); assert.equal(get(value.core.pool.slot(bulwark), 26), 10000);
    value.hits.damageHooks().damage(bulwark, 5, 3, attacker); assert.equal(get(value.core.pool.slot(bulwark), 26), 10000);
    value.core.types[80].flags2 &= ~0x100000;
    assert.equal(value.hits.damageResult(bulwark, 5, attacker), 1); assert.equal(get(value.core.pool.slot(bulwark), 26), 9995);
    const tower = target(value, 142); put(value.core.pool.allocated(tower), 36, value.frames[142].run('L').first);
    value.hits.damageHooks().damage(tower, 5, 3, attacker); assert.equal(get(value.core.pool.slot(tower), 26), 10000);
    put(value.core.pool.allocated(tower), 36, value.frames[142].run('M').first);
    value.hits.damageHooks().damage(tower, 5, 3, attacker); assert.equal(get(value.core.pool.slot(tower), 26), 9995);
});

test('CampaignHitBindings: 제단 방향 11/12의 무피해 반환과 다이스의 다른 VT를 구분한다', () => {
    const value = scene();
    // 각 실제 생성자의 L/M과 나머지 방향을 독립 고정 기대값으로 검사한다.
    for (const type of [164, 165]) {
        const victim = target(value, type);
        for (const letter of ['L', 'M']) {
            put(value.core.pool.physical(victim), 36, value.frames[type].run(letter).first);
            // 실제 448b70은 TYPE164의 VT에만 있다. TYPE165는 별도 일반 Damageable이며 완성 다이스에는 HP가 없다.
            assert.equal(value.hits.damageResult(victim, 10, 0), type === 164 ? 1 : 0); value.hits.damageHooks().damage(victim, 10, 0, 0);
            assert.equal(get(value.core.pool.slot(victim), 26), 10000);
        }
        put(value.core.pool.physical(victim), 36, 0);
        value.core.pool.physical(victim)[40] = 0;
        // TYPE165는 완성 다이스라 HP 보유 판정 자체가 거짓이다. TYPE164 제단만 일반 HP 피해를 받는다.
        assert.equal(value.hits.damageResult(victim, 10, 0), type === 164 ? 1 : 0); value.hits.damageHooks().damage(victim, 10, 0, 0);
        assert.equal(get(value.core.pool.slot(victim), 26), type === 164 ? 9980 : 10000);
    }
});

test('CampaignHitBindings: 건설 중 신전은 실제 내용물 Form을 찾아 끝 시각/표시만 알린다', () => {
    const calls = []; const value = scene({ effects: { vortex: {
        constructionType: () => 30, resetEnd: (form, bits) => { calls.push(['end', form, bits]); }, refresh: (sid) => { calls.push(['refresh', sid]); },
    } } });
    const victim = target(value, 123); value.core.pool.physical(victim)[40] |= 1;
    assert.equal(value.hits.damageResult(victim, 23, 0), 1); assert.deepEqual(calls, []);
    const process = new SquidProcess(); value.core.processes.attach(process, 30, victim, 0x50);
    assert.equal(value.hits.damageResult(victim, 23, 0), 1);
    assert.deepEqual(calls, [['end', process.form, 23], ['refresh', victim]]); assert.equal(get(value.core.pool.slot(victim), 26), 10000);
    calls.length = 0; value.hits.damageHooks().damage(victim, 13, 0, 0); assert.deepEqual(calls, [['end', process.form, 13], ['refresh', victim]]);
    value.core.pool.physical(victim)[40] &= ~1;
    assert.equal(value.hits.damageResult(victim, 3, 0), 1); assert.equal(get(value.core.pool.slot(victim), 26), 9997);
});

test('CampaignHitBindings: burning의 현재 Form 조회와 실제 Host 삭제를 같은 풀에서 수행한다', () => {
    const made = []; const value = scene({ effects: { burning: { create: (sid) => { made.push(sid); return 42; } } } });
    const victim = target(value, 75, 10000); value.core.types[75].damageEffect = 1;
    const process = new SquidProcess(); value.core.processes.attach(process, 14, victim, 0x50); const form = process.form;
    assert.equal(value.hits.burning.refresh(victim), 0); assert.ok(value.core.pool.slot(form)[11] & 1); assert.equal(value.core.kernel.size, 0);
    put(value.core.pool.physical(victim), 26, 1); assert.equal(value.hits.burning.refresh(victim), 42); assert.deepEqual(made, [victim]);
});

test('CampaignHitBindings: Dude 통지는 현재 Host의 task Regular와 외부 burning 요청을 연결한다', () => {
    const calls = []; const value = scene({ effects: { dudes: { createBurning: (sid) => { calls.push(sid); } } } });
    const dude = target(value, 70); value.core.processState.now = 10; value.hits.dudes.notice(dude);
    const regular = value.core.processes.findEvent(dude, 0x5107); assert.ok(regular); assert.equal(regular.parent, dude);
    assert.equal(get(value.core.pool.slot(dude), 12, 2), 0x5107); assert.equal(regular.time, 10); assert.deepEqual(calls, [dude]); assert.equal(value.core.kernel.size, 1);
    value.hits.dudes.notice(dude); assert.equal(value.core.kernel.size, 1); assert.deepEqual(calls, [dude]);
    // 경로 미복원은 성공으로 바꾸지 않는다.
    assert.throws(() => value.hits.dudes.move(dude, [40, 41]));
});

test('CampaignHitBindings: 일반 피해의 다른 가상 방어는 명시 장치만 사용한다', () => {
    const calls = []; const value = scene({ effects: { general: { rejectDamage: (...args) => { calls.push(args); return false; } } } });
    const victim = target(value, 75); put(value.core.pool.physical(victim), 0, 0x12345678);
    assert.equal(value.hits.generalDamage.hit(victim, 1, 0, 0), true); assert.deepEqual(calls, [[victim, 0]]); assert.equal(get(value.core.pool.slot(victim), 26), 9999);
    const missing = scene(); const other = target(missing, 75); put(missing.core.pool.physical(other), 0, 0x12345678);
    assert.throws(() => missing.hits.generalDamage.hit(other, 1, 0, 0), /방어 미연결/);
    const altar = target(missing, 164); missing.core.pool.physical(altar)[10] = 255;
    assert.throws(() => missing.hits.damageResult(altar, 1, 0), /프레임 물리 부족/);
});

test('CampaignHitBindings: 등록된 방어/삭제 함수는 C++ 값 캡처의 사본이다', () => {
    const calls = []; const value = scene({ effects: { general: { rejectDamage: () => { calls.push('defense'); return false; } } } });
    value.hitEffects.general.rejectDamage = () => { throw new Error('교체 후 함수'); };
    const victim = target(value, 75); put(value.core.pool.physical(victim), 0, 0x12345678);
    assert.equal(value.hits.generalDamage.hit(victim, 1, 0, 0), true); assert.deepEqual(calls, ['defense']);
    const fallback = { emit: (event) => { calls.push(event); } }; const hooks = value.hits.destroyHooks(fallback);
    fallback.emit = () => { throw new Error('교체 후 함수'); };
    const event = { sid: victim, flags: 0, effect: SQUID_DESTROY_EFFECT.preDestroy }; hooks.emit(event); assert.equal(calls[1], event);
});

test('CampaignHitBindings: 충돌 anim은 실제 Factory·Pop·Regular·프레임·가상 삭제를 공유한다', () => {
    const value = scene(); value.hits.impact.visual(30, 30, 0, 0.05); assert.equal(value.core.kernel.size, 1);
    const regular = value.core.kernel.get(1);
    const animation = regular.parent; assert.equal(value.core.pool.slot(animation)[10], 160); assert.equal(value.hits.impact.handles(animation), true);
    assert.equal(value.hits.handler()(animation, 0, 0, 0.05), 0.05);
    assert.equal(get(value.core.pool.slot(animation), 36), value.frames[160].run('A').first);
    assert.equal(value.hits.handler()(animation, 0, value.frames[160].run('A').count, 0.05), -1);
    assert.ok(value.core.pool.slot(animation)[11] & 1); assert.equal(value.core.kernel.size, 0); assert.equal(value.core.bookkeeping.depth, 0);
});

test('CampaignHitBindings: Missile은 현재 시각·피해·충돌 효과를 공유하고 다른 풀을 거부한다', () => {
    const value = scene(); const calls = []; const clock = { now: 12 };
    const hit = { pool: value.core.pool, hit: (...args) => calls.push(['hit', ...args]), setTime: (...args) => calls.push(['time', ...args]) };
    const hooks = value.hits.missileHooks(hit, clock, { other: 3 }); hooks.hit(7, 8); clock.now = 13; hooks.refresh(9);
    assert.deepEqual(calls, [['hit', 7, 8, 12], ['time', 9, 13]]); assert.equal(hooks.other, 3);
    hooks.impact(40, 41, 0, 0.1); assert.equal(value.core.kernel.size, 1);
    value.impactState.suppressed = 1; const seed = value.random.state; hooks.hitEffect(250, 250);
    assert.notEqual(value.random.state, seed); assert.equal(value.core.kernel.size, 1);
    assert.throws(() => value.hits.missileHooks({ pool: {} }, clock));
});

test('CampaignHitBindings: 확인된 공통 자산만 삭제를 분배하고 나머지 효과/fallback을 보존한다', () => {
    const value = scene();
    // 실제 생성자가 같은 공통 Pre/Post를 사용하는 자산들이다.
    for (const type of [80, 81, 83, 130, 132]) {
        const sid = target(value, type); assert.equal(value.world.destroy(sid, 0), true); assert.ok(value.core.pool.slot(sid)[11] & 1);
    }
    const other = target(value, 75); const events = []; const marker = () => 7;
    const hooks = value.hits.destroyHooks({ selected: marker, emit: (event) => { events.push(event); } });
    const event = { effect: SQUID_DESTROY_EFFECT.preDestroy, sid: other, flags: 3 }; hooks.emit(event);
    assert.deepEqual(events, [event]); assert.equal(hooks.selected, marker); assert.throws(() => value.hits.destroy()(other, 0, hooks));
    assert.equal(value.hits.destroy((sid, flags, input) => { assert.deepEqual([sid, flags, input], [other, 3, hooks]); return false; })(other, 3, hooks), false);
    assert.throws(() => value.hits.destroyHooks().emit(event));
    assert.equal(value.hits.handler((sid, event, count, payload) => payload + 1)(other, 7, 3, 4), 5);
    assert.throws(() => value.hits.handler()(other, 7, 3, 4));
});
