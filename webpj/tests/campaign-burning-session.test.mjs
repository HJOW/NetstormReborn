// cpppj DamageableBurningChecking_ActualClientSessionRecoveryAndLifetime를 실제 웹 Session에서 재현한다. (WB04-E1315)
// 피해/전투 Kernel은 실행하지 않고 HP 입력에 대한 연소 생성·중복 진단·회복 정리만 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { BurningVisualProcess } from '../www/core/burning-visual-system.js';
import { visualSessionScene } from './campaign-visual-session-support.mjs';
import { get, put } from './raw-support.mjs';

test('DamageableBurningChecking: 실제 Client 진단 복귀와 Session 연소 수명', () => {
    // 진단은 원문 검사와 같이 HP·checking·두 번째 Form만 바꾸는 외부 입력이다.
    let current, gameplayOwner, maximum = 0, second = 0;
    const temple = 15002;
    const { session: s, gameplay, diagnoses } = visualSessionScene({ assertion: (expression) => {
        assert.equal(expression, 'findProcess (mySid, burningSystemProcessType, 1 ) == 0');
        gameplayOwner.aiDebug().checking().value = 0;
        put(current.core().pool.allocated(temple), 26, maximum); current.world().destroy(second, 0);
    } });
    current = s; gameplayOwner = gameplay;
    try {
        s.load(); const c = s.core(), raw = c.pool.allocated(temple), particles = s.particles();
        assert.equal(raw[10], 124); assert.notEqual(c.types[124].damageEffect, 0);
        maximum = get(raw, 26); put(raw, 26, Math.trunc(maximum / 4));
        const a = particles.addBurning(temple, 1), b = particles.addBurning(temple, 1);
        const tokenA = a.fields.registration.token, tokenB = b.fields.registration.token;
        const lookup = new RawContainedLookup(c.pool, (sid, flags) => s.world().destroy(sid, flags));
        const first = lookup.find(temple, 14); second = lookup.find(temple, 14, 1);
        assert.ok(first && second && first !== second);
        gameplay.aiDebug().checking().value = 0x80000000;
        s.priests().damageableRefresh(temple);
        assert.equal(diagnoses.length, 1); assert.equal(gameplay.aiDebug().checking().value, 0); assert.equal(get(raw, 26), maximum);
        assert.equal(lookup.find(temple, 14), first); assert.equal(lookup.find(temple, 14, 1), 0);
        assert.ok(c.processes.fromForm(first)); assert.equal(c.processes.fromForm(second), null);
        // 이번 호출은 진단 전에 읽은 HP와 첫 Form을 보존하고, 다음 갱신에서 현재 회복 HP로 정리해야 한다.
        s.priests().damageableRefresh(temple);
        assert.equal(lookup.find(temple, 14), 0); assert.equal(c.processes.fromForm(first), null);
        assert.equal(particles.rendering.lookup(tokenA), null); assert.equal(particles.rendering.lookup(tokenB), null);
        assert.deepEqual([c.destroy.preDepth, c.destroy.postDepth, c.bookkeeping.depth], [0, 0, 0]);
        put(raw, 26, Math.trunc(maximum / 4)); s.priests().damageableRefresh(temple);
        const created = lookup.find(temple, 14), effect = c.processes.fromForm(created);
        assert.ok(effect instanceof BurningVisualProcess); assert.equal(effect.fields.mode, 1); assert.equal(effect.fields.stage, 0);
        put(raw, 26, maximum); s.priests().damageableRefresh(temple);
        assert.equal(c.processes.fromForm(created), null); assert.equal(lookup.find(temple, 14), 0); assert.equal(diagnoses.length, 1);
    } finally { s.dispose(); }
});
