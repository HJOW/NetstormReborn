// 운반체 도착 raw-carrier-arrival.js(RawCarrierArrival)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrierarrival-x86.tsv 의 `originals` 203행 — 부모 운반체(사제 genus)의 현재 경로/깊이/내용물/다이스/귀환·간헐천 분기와 끝의 회피 검색을 입력 JSON(권한·제어·깊이·genus·helper·소유자·경로 유무·회피 반환·장면 6종)으로
//    실제 PE 에서 실행해 부모 도착 반환값, 큰 회피 경계의 요청 기록(`E:` 인자 비트·사건·반환), 부모 슬롯 전체 raw 가 같다. 선택하지 않은 큰 하위 효과(포획·전달·경제·프로토콜·공간)가 실행되면 시험이 실패한다.
//    `originalCD` 행은 다른 판본이라 사용하지 않는다(이 fixture 에는 10.37 행이 없다).
//  - 계약: 값형 Player AI 표 대신 실제 현재 Player 조회 getter 를 매 판정 읽음(생성 뒤 값이 바뀌어도 반대 표가 분기를 결정하지 않음), 같은 capture DWORD 의 0/음수 비0 가 건강한 동맹 사제 pickup 을 즉시 바꿈,
//    필수 하위 경계 누락·다른 풀 거부.
//  - 한 칸 genus 조회(genusAt)는 cpppj 시험 장면과 같이 실제 RawDamageablePostDestroy.genusAt 몸체를 쓴다(2026-11-10 WB04-E253: 시험 안의 대역 구현을 실제 몸체로 교체).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E160: 운반체 도착 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { RawPriestState } from '../www/core/raw-priest-state.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawContainedCreate } from '../www/core/contained-create.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { DamageablePostDestroyState, RawDamageablePostDestroy } from '../www/core/raw-damageable-post-destroy.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor } from '../www/core/raw-player-placement-anchor.js';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathCleanup, createPathCleanupState } from '../www/core/raw-path-cleanup.js';
import { RawCarrierArrival, createCarrierArrivalState } from '../www/core/raw-carrier-arrival.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E160)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrierarrival-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 풀 크기, 타입 수, 부모/후보 번호이다. (2026-11-09 WB04-E160)
const CAPACITY = 24000;
const TYPE_COUNT = 188;
const PARENT = 50;
const CANDIDATE = 60;

// 예외가 나는지 본다. (2026-11-09 WB04-E160)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E160)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 선택하지 않은 큰 포획/전달/경제/프로토콜/공간 효과가 실행되면 명시 예외로 드러낸다. (2026-11-09 WB04-E160)
const forbidden = () => () => { throw new Error('Carrier 도착의 미선택 큰 하위 효과'); };

// 같은 풀의 기존 helper 를 직접 공급하는 도착 시험 장면이다(독립 Native 의 Path 입력 경계). (2026-11-09 WB04-E160)
class ArrivalScene {
    // 입력에 맞는 실제 슬롯과 기존 helper 를 구성하고 Native 결과를 재계산하지 않는다. controlled 는 실제 현재 Player 조회 getter(선택)이다.
    constructor(json, controlled = null) {
        this.input = JSON.parse(json);
        this.pool = new SidPool(CAPACITY, { server: true });
        this.hash = new SquidHash();
        this.types = Array.from({ length: TYPE_COUNT }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
        this.frames = Array.from({ length: TYPE_COUNT }, () => []);
        this.spots = new Uint8Array(65536);
        this.effects = [];
        this.book = new SquidPostPopState();
        this.rewardState = new SquidRewardState();
        this.containedState = new ContainedFinderState();
        this.cleanupState = createPathCleanupState();
        this.anchorState = new PlayerPlacementAnchorState();
        this.state = createCarrierArrivalState();
        this.runtime = new RawPathRuntime({ priest: true });
        this.actualPath = null;
        // 기록된 부모 50/후보 60 번호까지 슬롯 입력 바이트를 0 으로 채운다.
        for (let sid = 5; sid <= CANDIDATE; sid += 1) this.pool.physical(sid).fill(0);
        this.add(PARENT, 158, 20.75, 21.9, 1);
        const scene = this.value('scene');
        if (scene >= 1 && scene <= 4) this.add(CANDIDATE, scene === 2 ? 84 : 150, 20.2, scene === 4 ? 23.2 : 21.3, 1);
        else if (scene === 5) this.add(CANDIDATE, 165, 23.75, 24.9, 3);
        const raw = this.pool.physical(PARENT);
        put(raw, 26, 200, 4);
        raw[34] = this.value('owner');
        this.types[158].maxHitPoints = 200;
        this.types[158].flags2 = this.value('genus');
        if (scene >= 1 && scene <= 4) this.types[scene === 2 ? 84 : 150].flags2 = [0, 0x10000, 0, 0x4200, 0x200][scene];
        else if (scene === 5) this.types[165].flags2 = 0x400000;
        this.spots[22 * 256 + 21] = 6;
        this.spots[21 * 256 + 20] = 6;
        this.state.boss = this.value('boss') !== 0;
        this.state.controlledOwners[this.value('owner')] = this.value('controlled') !== 0;
        this.cleanupState.boss = this.state.boss;
        this.containedState.boss = this.state.boss;
        this.runtime.setMoveAwayDepth(this.value('depth'));
        this.runtime.setCommandFields(this.value('helper'), 0, 0, { x: 0, y: 0 });
        this.reward = new SquidReward(this.pool, this.types, this.book, this.rewardState, new ScrambledSpStore(new GameRandom(), () => 1));
        this.priest = new RawPriestState(this.pool, this.reward, this.types, this.spots);
        this.carrier = new RawCarrierPreDestroy(this.pool, this.containedState, { validateDamageable: forbidden(), damageablePre: forbidden(), dependentFound: forbidden() });
        this.cleanup = new RawPathCleanup(this.pool, this.hash, this.types, this.cleanupState, {
            reserveShared: forbidden(), addShared: forbidden(), contained: forbidden(), graphFor: forbidden(), ritual: forbidden(), refund: forbidden(), destroy: forbidden()
        });
        this.anchor = new RawPlayerPlacementAnchor(this.pool, this.types, this.hash.view(0), this.book, new SquidPostPopList(), this.containedState, this.anchorState);
        this.create = new RawContainedCreate(this.pool, undefined, { create: forbidden(), notifySurface: forbidden(), attach: forbidden() });
        this.footprint = new RawSquidNeighbors(this.pool, this.hash, this.spots, this.types, this.frames);
        // 이미 이식한 공통 한 칸 genus 조회(004b2070) 몸체를 같은 풀·해시에서 공유한다(cpppj CarrierArrivalSupport 의 post). 삭제 후처리의 큰 하위 효과는 도달하면 실패한다. (2026-11-10 WB04-E253)
        this.post = new RawDamageablePostDestroy(this.pool, this.hash, this.types, this.frames, this.reward, this.footprint, new DamageablePostDestroyState(), {
            dropDude: forbidden(), dropIsland: forbidden(), regular: forbidden(), hasBridgeFall: forbidden(), notifySurface: forbidden(), displayUpdate: forbidden(), destroy: forbidden(), base: forbidden()
        });
        this.arrival = new RawCarrierArrival(this.pool, this.hash, this.types, this.carrier, this.cleanup, this.priest, this.anchor, this.create, this.footprint, this.state, {
            ...(controlled ? { currentControlled: controlled } : {}),
            path: (sid) => { assert.equal(sid, PARENT); return this.actualPath ? this.actualPath(sid) : (this.value('path') ? this.runtime : null); },
            genusAt: (x, y, genus) => this.post.genusAt(x, y, genus),
            pickup: forbidden(),
            escape: (sid, delay, event) => {
                assert.equal(sid, PARENT);
                this.effects.push(`E:50:${floatToBits(delay)}:${event}:${this.value('reply')}`);
                return this.value('reply');
            },
            chooseReturn: forbidden(), chooseGeyser: forbidden(), goReturn: forbidden(), goGeyser: forbidden(), minedPacket: forbidden(), destroy: forbidden(), robust: forbidden()
        });
    }

    // 입력값만 읽고 예상 분기/반환을 계산하지 않는다. (2026-11-09 WB04-E160)
    value(key) { return Number(this.input[key]) >>> 0; }

    // Native 입력과 같은 합성 raw/발자국/버킷 등록이다. 전체 actual Pop 완료 근거가 아니다. (2026-11-09 WB04-E160)
    add(sid, type, x, y, width) {
        const raw = this.pool.physical(sid);
        put(raw, 0, 0x11018000);
        raw[10] = type;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        this.types[type].footX = this.types[type].footY = width;
        put(raw, 4, this.hash.bucket(1, x, y), 2);
        this.hash.setBucket(1, x, y, sid);
    }

    // 요청된 큰 회피 효과의 순서/비트/반환만 읽는다. (2026-11-09 WB04-E160)
    effectsText() { return this.effects.length === 0 ? '-' : this.effects.join(';'); }
}

test('운반체 도착: 원본 10.78 관측 203행의 도착 반환·회피 요청 기록·부모 슬롯 raw 가 같다', { skip: rows.length === 0 }, () => {
    assert.ok(rows.length > 0);
    let checked = 0;
    const results = new Set();
    // 각 고유 입력을 자체 실제 풀/hash/HP/contained/Graph helper 에 연결한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const scene = new ArrivalScene(row[1]);
        const result = scene.arrival.arrival(PARENT, 0, 0);
        const where = row.slice(0, 3).join(' ');
        assert.equal(result, Number(row[2]), where);
        assert.equal(scene.effectsText(), row[3], where);
        assert.equal(hex(scene.pool.slot(PARENT)), row[4], where);
        results.add(result);
        checked += 1;
    }
    assert.equal(checked, 203);
    // 종료(1000)와 회피 반환이 모두 관찰되었는지 확인한다.
    assert.ok(results.size > 1, [...results].join(','));
});

test('운반체 도착: 값형 Player AI 표 대신 실제 현재 Player 조회 getter 를 매 판정 읽는다', () => {
    let checked = 0;
    let queries = 0;
    // 생성 뒤 현재 값을 바꿔도 매 조회에서 같은 현재 저장소를 사용해야 한다.
    for (const row of rows) {
        let brain = 0;
        const scene = new ArrivalScene(row[1], (owner) => { assert.ok(owner > 0 && owner < 9); queries += 1; return brain !== 0; });
        // 반대 값의 예전 bool 표가 분기를 결정하지 않아야 한다.
        scene.state.controlledOwners[scene.value('owner')] = scene.value('controlled') === 0;
        brain = scene.value('controlled') ? 0x12345678 : 0;
        assert.equal(scene.arrival.arrival(PARENT, 0, 0), Number(row[2]), row[1]);
        assert.equal(scene.effectsText(), row[3], row[1]);
        assert.equal(hex(scene.pool.slot(PARENT)), row[4], row[1]);
        checked += 1;
    }
    assert.ok(checked === 203 && queries > 0, `${checked}/${queries}`);
});

test('운반체 도착: 같은 capture DWORD 의 0/음수 비0 가 건강한 동맹 사제 pickup 을 즉시 바꾼다', () => {
    const row = rows.find((candidate) => JSON.parse(candidate[1]).scene === 1);
    assert.ok(row);
    const scene = new ArrivalScene(row[1]);
    let capture = 0;
    scene.state.sharedAllowAlliedPickup = () => capture;
    scene.state.allowAlliedPickup = true;
    scene.types[158].flags2 = 0;
    scene.types[150].flags2 = 0x200000;
    scene.pool.physical(PARENT)[34] = 2;
    const candidate = scene.pool.physical(CANDIDATE);
    candidate[34] = 1;
    put(candidate, 26, 200);
    scene.types[150].maxHitPoints = 200;
    scene.spots.fill(6);
    scene.state.alliances[2 * 9 + 1] = 1;
    assert.equal(scene.priest.groundImmobile(CANDIDATE), false);
    capture = 0;
    assert.equal(scene.arrival.pickupCandidate(PARENT, 20, 21).sid, 0);
    capture = -7;
    assert.equal(scene.arrival.pickupCandidate(PARENT, 20, 21).sid, CANDIDATE);
    assert.ok(capture === -7 && scene.state.allowAlliedPickup);
});

test('운반체 도착: 필수 하위 경계 누락과 다른 풀 연결을 첫 효과 전에 거부한다', () => {
    const scene = new ArrivalScene(rows[0]?.[1] ?? '{"boss":0,"controlled":0,"depth":1,"genus":65536,"helper":0,"owner":0,"path":0,"reply":1000,"scene":0}');
    const make = (override = {}) => new RawCarrierArrival(override.pool ?? scene.pool, scene.hash, scene.types, scene.carrier, scene.cleanup, scene.priest, scene.anchor, scene.create, scene.footprint, scene.state, override.hooks ?? {
        path: () => null, genusAt: () => 0, pickup: () => {}, escape: () => 0, chooseReturn: () => 0, chooseGeyser: () => 0, goReturn: () => {}, goGeyser: () => {}, minedPacket: () => {}, destroy: () => {}, robust: () => true
    });
    assert.ok(!throws(() => make()));
    assert.ok(throws(() => make({ hooks: { path: () => null } })));
    assert.ok(throws(() => make({ pool: new SidPool(CAPACITY, { server: true }) })));
    // 운반체가 아닌 부모는 패치의 isCarrier assert 로 거부한다.
    scene.types[158].flags2 = 0;
    assert.ok(throws(() => scene.arrival.arrival(PARENT, 0, 0)));
});
