// 생성된 가상 표 자료 모듈 두 개(energy-provider-vtables.js·campaign-damage-vtables.js)를 cpppj 소스 EnergyProviderVtables.inc·CampaignDamageVtables.inc 와 대조하고 소비자 분배를 확인한다(WB04-E250).
//  - 대조: .inc 의 쌍을 tools/make-vtable-tables.mjs 의 같은 파싱 함수로 읽어 모듈 값과 순서까지 같은지, 생성기를 다시 돌린 결과가 현재 모듈 파일과 같은지(--check) 본다.
//  - 불변식: 세 표(+a0 에너지 공급·+98 피격·+9c 방어)는 같은 실제 PE 가상 표 83개를 같은 순서로 담고, 진입 함수 주소는 알려진 집합 안이다.
//  - 소비자: raw-energy-supply.js 의 energyProviderEntry 가 모든 쌍을 그대로 돌려주고 표 밖은 거부하며, raw-damageable-hit.js 의 makeDamageableGunHitHooks 가 이 표로 Base/Damageable 몸체를 분배한다.
// 값은 원본 PE 에서 cpppj 가 다시 확인하는 자료이며 이 검사는 새 원본 관측이 아니다. (2026-11-10 WB04-E250)
import assert from 'node:assert/strict';
import fs from 'node:fs';
import test from 'node:test';
import { CAMPAIGN_DAMAGE_DEFENSE_ENTRIES, CAMPAIGN_DAMAGE_HIT_ENTRIES } from '../www/core/campaign-damage-vtables.js';
import { ENERGY_PROVIDER_ENTRIES } from '../www/core/energy-provider-vtables.js';
import { energyProviderEntry } from '../www/core/raw-energy-supply.js';
import { makeDamageableGunHitHooks } from '../www/core/raw-damageable-hit.js';
import { SidPool } from '../www/core/sid-pool.js';
import { DAMAGE_INC, ENERGY_INC, ENERGY_MODULE, DAMAGE_MODULE, parsePairs, renderAll } from '../tools/make-vtable-tables.mjs';
import { put } from './raw-support.mjs';

// cpppj 소스가 없으면 검사를 건너뛴다. (2026-11-10 WB04-E250)
const present = fs.existsSync(ENERGY_INC) && fs.existsSync(DAMAGE_INC);

// 알려진 진입 함수 주소 집합이다: 에너지(기본·공급형), 피격(Base·Damageable·사제/신전/특수 넷), 방어(기본 false·TYPE 142 특별 방향). (2026-11-10 WB04-E250)
const ENERGY_ENTRIES = [0x0041c0f0, 0x0041c170];
const HIT_ENTRIES = [0x004add10, 0x0044c8e0, 0x00448b70, 0x0048ef20, 0x00494380, 0x004dba40];
const DEFENSE_ENTRIES = [0x0043d9b0, 0x0048ef70];

test('vtable-tables: 모듈 값이 cpppj .inc 의 쌍과 순서까지 같고 생성기 재생성 결과와 같다', { skip: !present && 'cpppj .inc 없음' }, () => {
    const energy = parsePairs(fs.readFileSync(ENERGY_INC, 'utf8'), 'kEnergyProviderEntries');
    const damage = fs.readFileSync(DAMAGE_INC, 'utf8');
    const hit = parsePairs(damage, 'kCampaignDamageHitEntries');
    const defense = parsePairs(damage, 'kCampaignDamageDefenseEntries');
    assert.deepEqual([energy.length, hit.length, defense.length], [83, 83, 83]);
    assert.deepEqual(ENERGY_PROVIDER_ENTRIES.map((pair) => [...pair]), energy);
    assert.deepEqual(CAMPAIGN_DAMAGE_HIT_ENTRIES.map((entry) => [entry.vtable, entry.address]), hit);
    assert.deepEqual(CAMPAIGN_DAMAGE_DEFENSE_ENTRIES.map((entry) => [entry.vtable, entry.address]), defense);
    // 생성기 --check 와 같은 비교: 줄바꿈 변환을 무시하고 현재 파일이 재생성 결과와 같다.
    const generated = renderAll();
    const normal = (text) => text.replace(/\r\n/g, '\n');
    assert.equal(normal(fs.readFileSync(ENERGY_MODULE, 'utf8')), generated.energy);
    assert.equal(normal(fs.readFileSync(DAMAGE_MODULE, 'utf8')), generated.damage);
});

test('vtable-tables: 세 표가 같은 PE 가상 표 83개를 같은 순서로 담고 진입 함수는 알려진 집합 안이다', () => {
    const vtables = ENERGY_PROVIDER_ENTRIES.map((pair) => pair[0]);
    assert.equal(new Set(vtables).size, 83);
    assert.deepEqual(CAMPAIGN_DAMAGE_HIT_ENTRIES.map((entry) => entry.vtable), vtables);
    assert.deepEqual(CAMPAIGN_DAMAGE_DEFENSE_ENTRIES.map((entry) => entry.vtable), vtables);
    assert.ok(ENERGY_PROVIDER_ENTRIES.every((pair) => ENERGY_ENTRIES.includes(pair[1])));
    assert.ok(CAMPAIGN_DAMAGE_HIT_ENTRIES.every((entry) => HIT_ENTRIES.includes(entry.address)));
    assert.ok(CAMPAIGN_DAMAGE_DEFENSE_ENTRIES.every((entry) => DEFENSE_ENTRIES.includes(entry.address)));
    // 공급형은 다섯 개뿐이고 모든 진입 종류가 실제로 쓰인다.
    assert.equal(ENERGY_PROVIDER_ENTRIES.filter((pair) => pair[1] === 0x0041c170).length, 5);
    for (const address of HIT_ENTRIES) assert.ok(CAMPAIGN_DAMAGE_HIT_ENTRIES.some((entry) => entry.address === address), address.toString(16));
    for (const address of DEFENSE_ENTRIES) assert.ok(CAMPAIGN_DAMAGE_DEFENSE_ENTRIES.some((entry) => entry.address === address), address.toString(16));
    // 표는 얼려 있어 소비자가 바꿀 수 없다.
    assert.ok(Object.isFrozen(ENERGY_PROVIDER_ENTRIES) && Object.isFrozen(ENERGY_PROVIDER_ENTRIES[0]) && Object.isFrozen(CAMPAIGN_DAMAGE_HIT_ENTRIES[0]));
});

test('vtable-tables: energyProviderEntry 가 모든 쌍을 그대로 돌려주고 표 밖 가상 표는 거부한다', () => {
    for (const [vtable, entry] of ENERGY_PROVIDER_ENTRIES) assert.equal(energyProviderEntry(vtable), entry);
    assert.throws(() => energyProviderEntry(0), /미복원/);
    assert.throws(() => energyProviderEntry(0x00501dd1), /미복원/);
});

// 현재 raw vtable 의 +98 진입 주소로 Base(0x004add10)/Damageable(0x0044c8e0) 몸체를 고르고 다른 진입은 기존 damage 훅으로 넘긴다.
test('vtable-tables: 피격 진입 표가 makeDamageableGunHitHooks 의 Base/Damageable/그 밖 분배를 결정한다', () => {
    const pool = new SidPool(24000, { server: false });
    const victim = pool.allocate(2);
    const calls = [];
    const damage = {
        pool,
        hit: (...args) => calls.push(['hit', ...args]),
        baseHit: (...args) => calls.push(['baseHit', ...args]),
    };
    const hooks = makeDamageableGunHitHooks(damage, CAMPAIGN_DAMAGE_HIT_ENTRIES, { damage: (...args) => calls.push(['other', ...args]) });
    const hitAt = (vtable) => {
        put(pool.allocated(victim), 0, vtable);
        calls.length = 0;
        hooks.damage(victim, 7, 2, 3);
        return calls.map((call) => call[0]);
    };
    // 일반 Base(0x00501dd0)·Damageable(0x00501ea0)·사제/신전/특수 진입(Base/Damageable 주소가 아닌 것)·표 밖은 기존 damage 훅으로 간다.
    assert.deepEqual(hitAt(0x00501dd0), ['baseHit']);
    assert.deepEqual(hitAt(0x00501ea0), ['hit']);
    const special = CAMPAIGN_DAMAGE_HIT_ENTRIES.find((entry) => ![0x004add10, 0x0044c8e0].includes(entry.address));
    assert.deepEqual(hitAt(special.vtable), ['other']);
    assert.deepEqual(hitAt(0x00501dd1), ['other']);
});
