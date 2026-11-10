// Damageable 삭제 후처리 raw-damageable-post-destroy.js(RawDamageablePostDestroy·DamageablePostDestroyState)를 원본 10.78 관측 damageablepostdestroy-x86.tsv 와 cpppj `DamageablePostDestroyTests.cpp`·`DamageablePostDestroyHostTests.cpp` 계약으로 대조한다(WB04-E253).
//  - 관측 2,246행(originals; Post 1,606·Drops 640): 부모 50 과 주변 후보 여섯(HP 대상 60·61, 무 HP 62, 다리 63·64, 잔해 65)을 실제 4단계 해시에 올린 장면에서 하위 효과의 호출 순서와 인자
//    (드롭 D·섬 정리 I·피해 Regular R·낙하 조회 F·표면 통지 N·표시 갱신 U·가상 삭제 X·공통 post B)와 일곱 슬롯 전체 바이트를 대조한다. 효과 도중 TYPE·소유자·extra·dead·프레임·WORD 가 바뀌는 여섯 변이를 포함한다.
//  - 계약(cpppj): packed count 의 DWORD 덧셈(carry)·payload 0, 필수 생성 경계 누락 거부, 다리 사이의 공유 boss 재조회, 현재 population(signed 20 경계)·battle·피해 사건·제외 TYPE 다섯(전체 DWORD)·특별 TYPE 의 공유 소유자,
//    실제 Host(SquidFactory·SquidProcessHost·Kernel)에서 Regular 의 초기 count 와 첫 Kernel 호출.
//  - 경계 계약: fixture 가 구별하지 못해 변형 시험에서 살아남은 입력을 닫는다(마지막 검사의 주석).
// CD 10.72(originalCD) 행은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E253)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { Kernel } from '../www/core/kernel.js';
import { GameRandom } from '../www/core/random.js';
import { hasScheduledBridgeFall } from '../www/core/squid-process.js';
import { DamageablePostDestroyState, RawDamageablePostDestroy } from '../www/core/raw-damageable-post-destroy.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, bitsToFloat, get, put, readRows } from './raw-support.mjs';

// 원본 격자에서 관찰한 부모/주변 후보 번호, 합성 가상 표 기록값, 10.78 관측 행 수(Post·Drops)이다. (2026-11-10 WB04-E253)
const POST_IDS = [50, 60, 61, 62, 63, 64, 65];
const POST_VTABLE = 0x11307000;
const POST_ROWS = 1606;
const DROP_ROWS = 640;
const SOURCE = 50;

// 10.78(originals) 관측 행만 쓴다. 파일이 없으면 검사를 건너뛴다. (2026-11-10 WB04-E253)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'damageablepostdestroy-x86.tsv'));
const rows = allRows?.filter((row) => row[0] === 'originals') ?? null;

// 10진 문자열 DWORD 를 부호 없는 수로 읽는다. (2026-11-10 WB04-E253)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 188칸 TYPE 입력이다(cpppj 기본 RiftTypeRecord: 모든 칸 0). (2026-11-10 WB04-E253)
const blankTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));

// 금 간 검색에 쓰는 원본과 같은 네 개 합성 프레임 코드('A','P', 번호, flags)이다. (2026-11-10 WB04-E253)
const postFrames = () => [
    { side: 0x41, variant: 0x50, number: 1, flags: 0 }, { side: 0x41, variant: 0x50, number: 11, flags: 32 },
    { side: 0x41, variant: 0x50, number: 21, flags: 64 }, { side: 0x41, variant: 0x50, number: 11, flags: 96 },
];

// cpppj 여러 시험이 함께 쓰는 기본 입력이다: 전투·권한, 부모 TYPE 89(HP 보유, 최대 HP 131073, 발자국 3×3), 좌표 (50.5, 50.5). (2026-11-10 WB04-E253)
const baseInput = (overrides = {}) => {
    const input = [0x110000, 1, 1, 0, 89, 0x10, 0, 0, 131073, 3, 3, 0, 0, 6699, 5, 0, floatToBits(50.5), floatToBits(50.5)];
    // 지정한 칸만 바꾼다.
    for (const [index, value] of Object.entries(overrides)) input[Number(index)] = value;
    return input;
};

// cpppj `DamageablePostReplayScene`: 원본 fixture 의 효과/전체 raw 저장소를 계산 없이 문자열로 보존하는 장면이다. 풀 번호 5..127 을 확보한다. (2026-11-10 WB04-E253)
class PostScene {
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        this.hash = new SquidHash();
        this.types = blankTypes();
        this.frames = this.types.map(() => []);
        this.spots = new Uint8Array(65536);
        this.bookkeeping = new SquidPostPopState();
        this.hpState = new SquidRewardState();
        this.random = new GameRandom(12345);
        this.store = new ScrambledSpStore(this.random, () => 12345);
        this.state = new DamageablePostDestroyState();
        this.events = [];
        this.input = new Array(18).fill(0);
        // 원본 분석 장면의 대상을 실제 할당 영역으로 준비한다.
        for (let sid = 5; sid <= 127; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 다음 행의 free/dead 값이 할당 API 를 막지 않도록 검사 입력만 전체 슬롯에 직접 쓴다(cpppj PostInputSlot).
    slot(sid) { return this.pool.physical(sid); }

    // opcode 와 같은 입력 raw/버킷을 구성하며 HP/finder 결과를 미리 만들지 않는다.
    prepare(values) {
        const input = values.map((value) => value >>> 0);
        this.input = input;
        this.hash.reset();
        this.events = [];
        // TYPE 표는 같은 배열을 유지하고 레코드만 기본값으로 되돌린다.
        for (let index = 0; index < this.types.length; index += 1) this.types[index] = { cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 };
        // 프레임은 같은 실제 코드 표 순서로 금 간 첫 검색/단단한 판정을 공급한다.
        for (let index = 0; index < this.frames.length; index += 1) this.frames[index] = postFrames();
        // 각 원본 후보의 좌표/상태/footprint/해시 단계를 입력 순서로 설정한다.
        for (const sid of POST_IDS) {
            const type = sid === 50 ? input[4] : (sid < 62 ? 90 : (sid === 62 ? 92 : (sid < 65 ? 82 : 94)));
            const raw = this.slot(sid);
            raw.fill(0);
            put(raw, 0, POST_VTABLE);
            raw[10] = type & 0xff;
            raw[11] = sid === 50 ? 2 : 0;
            raw[40] = (sid === 50 ? input[3] : (sid === 61 ? 1 : 0)) & 0xff;
            raw[34] = 2;
            put(raw, 12, 0x7355, 2);
            const x = sid === 50 ? bitsToFloat(input[16]) : (sid === 60 ? 49.25 : (sid === 61 ? 49.5 : (sid === 63 ? 49 : (sid === 65 ? 50.5 : 50))));
            const y = sid === 50 ? bitsToFloat(input[17]) : (sid === 60 ? 49.25 : (sid === 61 ? 49.5 : (sid === 63 || sid === 64 ? 49 : (sid === 65 ? 50.5 : 50))));
            put(raw, 14, sid === 50 ? input[16] : floatToBits(x));
            put(raw, 18, sid === 50 ? input[17] : floatToBits(y));
            put(raw, 36, sid === 63 ? input[11] % 4 : (sid === 64 ? 1 : 0), 4);
            const record = this.types[type];
            record.flags1 = type === 90 ? 0x10 : 0;
            record.flags2 = type === 82 ? 4 : (type === 94 ? 0x1000000 : 0);
            record.maxHitPoints = 100;
            record.group = 0;
            record.footX = sid === 50 ? input[9] | 0 : (sid === 65 ? 3 : 1);
            record.footY = sid === 50 ? input[10] | 0 : (sid === 65 ? 3 : 1);
            if (sid !== 50) {
                const level = sid === 63 || sid === 64 ? 0 : 1;
                put(raw, 4, this.hash.bucket(level, x, y), 2);
                this.hash.setBucket(level, x, y, sid);
            }
        }
        Object.assign(this.types[input[4]], { flags1: input[5], flags2: input[6], group: input[7] | 0, maxHitPoints: input[8] | 0 });
        this.state = new DamageablePostDestroyState();
        Object.assign(this.state, { battle: input[1] !== 0, boss: input[2] !== 0, dudeCount: input[12] | 0, damageEvent: input[13], bridgeCrackLife: input[14] });
        this.hp = new SquidReward(this.pool, this.types, this.bookkeeping, this.hpState, this.store);
        this.footprint = new RawSquidNeighbors(this.pool, this.hash, this.spots, this.types, this.frames);
        this.post = this.build(this.hooks());
    }

    // 같은 자료를 참조하는 부모 몸체를 주어진 훅으로 만든다.
    build(hooks) { return new RawDamageablePostDestroy(this.pool, this.hash, this.types, this.frames, this.hp, this.footprint, this.state, hooks); }

    // 이 격자는 외부 몸체의 실행 완료를 주장하지 않고 필수 원본 경계의 호출/인자/지정 변경만 대조한다.
    hooks() {
        const { input } = this;
        return {
            dropDude: (sid, index, randomPoint) => {
                this.events.push(`D:${sid}:${index}:${randomPoint ? 1 : 0}`);
                if (input[15] === 1) this.slot(50)[10] = 91;
                if (input[15] === 6) this.slot(50)[34] = 3;
            },
            dropIsland: (x, y, flags, owner) => {
                this.events.push(`I:${floatToBits(x)}:${floatToBits(y)}:${flags}:${owner}`);
                if (input[15] === 2) this.slot(50)[40] = 1;
            },
            regular: (sid, event, payload, count) => {
                this.events.push(`R:${sid}:${event}:${floatToBits(payload)}:${count}`);
                if (input[15] === 4) this.slot(61)[11] = 2;
            },
            hasBridgeFall: (sid) => { this.events.push(`F:${sid}`); return input[11] === 3; },
            notifySurface: (sid) => { this.events.push(`N:${sid}`); if (input[15] === 3) put(this.slot(sid), 36, 2, 4); },
            displayUpdate: (sid) => {
                this.events.push(`U:${sid}:${get(this.slot(sid), 36, 4)}`);
                if (input[15] === 5) put(this.slot(sid), 12, 0xfedc, 2);
            },
            destroy: (sid, flags) => this.events.push(`X:${sid}:${flags}`),
            base: (sid, flags) => this.events.push(`B:${sid}:${flags}`),
        };
    }

    // 관찰한 모든 경계와 현재 raw 후보를 원본 행 형식 그대로 직렬화한다.
    eventsText() { return this.events.length ? this.events.join(';') : '-'; }
    rawText() { return POST_IDS.map((sid) => `${sid}:${Buffer.from(this.pool.slot(sid)).toString('hex')}`).join(';'); }
}

// 정상 입력의 전체 raw·경계 순서를 원본 10.78 과 대조한다(cpppj DamageablePostDestroy_ReplaysBothNativeFullEffects).
test('damageable-post-destroy: 원본 10.78 2,246행(Post 1,606·Drops 640)의 하위 효과 순서·인자와 일곱 슬롯 전체가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new PostScene();
    const counts = { Post: 0, Drops: 0 };
    // 지정한 원본 DWORD 입력만 파싱하고 기대 출력을 재계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const input = row[2].split(',').map(number);
        assert.equal(input.length, 18);
        scene.prepare(input);
        if (row[1] === 'Post') scene.post.postDestroy(SOURCE, input[0]);
        else scene.post.drops(SOURCE, input[6], input[0] !== 0);
        const label = `${row[1]} 행 ${counts[row[1]]} 입력 ${row[2]}`;
        assert.equal(scene.eventsText(), row[3], `${label} 사건`);
        assert.equal(scene.rawText(), row[4], `${label} 슬롯`);
        assert.equal(row[5], '0');
        assert.ok(scene.post.pool === scene.pool && scene.random.state === 12345 && !scene.store.initialized);
        counts[row[1]] += 1;
    }
    assert.deepEqual(counts, { Post: POST_ROWS, Drops: DROP_ROWS });
});

// count 의 낮은 WORD 를 mask 하지 않고 실제 ADD 의 carry 와 payload 0 을 확인한다.
test('damageable-post-destroy: packed count 는 DWORD 덧셈이고 필수 생성 경계가 없으면 구성을 거부한다', () => {
    const scene = new PostScene();
    const input = baseInput();
    scene.prepare(input);
    scene.post.postDestroy(SOURCE, input[0]);
    // 원본 count 는 owner 1×65536 + 65537 = 131073 이며 payload 는 0 으로 유지된다.
    assert.ok(scene.events.includes('R:60:6699:0:131073'));
    for (const name of ['dropDude', 'dropIsland', 'regular', 'hasBridgeFall', 'notifySurface', 'displayUpdate', 'destroy', 'base']) {
        assert.throws(() => scene.build({ ...scene.hooks(), [name]: undefined }), TypeError, name);
    }
});

// 첫 다리 약화의 표면 통지가 권한을 바꾸면 뒤의 금 간 다리 삭제는 현재 권한을 따른다.
test('damageable-post-destroy: 공유 boss 는 다리 후보 사이에서 다시 읽는다', () => {
    const scene = new PostScene();
    scene.prepare([0, 1, 1, 0, 90, 0x10, 0, 2, 100, 1, 1, 0, 0, 6699, 5, 0, floatToBits(50), floatToBits(50)]);
    const boss = { value: true };
    scene.state.bindBoss(boss);
    // 두 좌표의 원래 독립 버킷에 같은 SID 를 중복 연결하지 않는다.
    assert.ok(scene.hash.bucket(0, 49, 49) === 63 && scene.hash.bucket(0, 50, 49) === 64 && get(scene.pool.slot(63), 4, 2) === 0 && get(scene.pool.slot(64), 4, 2) === 0);
    const hooks = scene.hooks();
    const notify = hooks.notifySurface;
    const counts = { notified: 0, destroyed: 0 };
    hooks.notifySurface = (sid) => { notify(sid); counts.notified += 1; boss.value = false; };
    hooks.destroy = () => { counts.destroyed += 1; };
    const post = scene.build(hooks);
    post.postDestroy(SOURCE, 0);
    assert.deepEqual(counts, { notified: 1, destroyed: 0 });
    assert.equal(boss.value, false);
    assert.equal(get(scene.pool.slot(63), 36, 4), 1);
    // 이미 약화된 두 다리는 현재 boss 가 모두 삭제한다(독립 필드는 쓰지 않음).
    boss.value = true;
    scene.state.boss = false;
    post.postDestroy(SOURCE, 0);
    assert.equal(counts.destroyed, 2);
});

// 현재 population 의 signed 20명 경계는 오래된 state 사본 대신 같은 장기 count 를 읽는다.
test('damageable-post-destroy: 현재 Dude population(signed)이 드롭 수를 정한다', () => {
    const scene = new PostScene();
    scene.prepare(baseInput());
    const population = new SquidPostPopList();
    scene.state.bindDudePopulation(population);
    scene.state.dudeCount = 999;
    for (const [count, drops] of [[20, 5], [21, 2], [0xffffffff, 5]]) {
        population.count = count;
        scene.events = [];
        scene.post.drops(SOURCE, 0x4200, true);
        assert.equal(scene.events.length, drops, `population ${count}`);
    }
    assert.equal(scene.state.dudeCount, 999);
    assert.throws(() => scene.state.bindDudePopulation({}), TypeError);
});

// 실제 Post 의 현재 battle 이 false 면 예약하지 않고 true 면 같은 현재 사건 DWORD 를 각 예약에 쓴다.
test('damageable-post-destroy: 현재 battle 과 피해 사건 DWORD 가 실제 Regular 예약을 정한다', () => {
    const scene = new PostScene();
    const input = baseInput();
    scene.prepare(input);
    const battle = { value: false };
    const event = { value: 0x12345678 };
    scene.state.bindBattle(battle);
    scene.state.bindDamageEvent(event);
    scene.post.postDestroy(SOURCE, input[0]);
    assert.ok(!scene.events.some((value) => value.startsWith('R:')));
    assert.ok(scene.events.includes('B:50:1114112'));
    scene.events = [];
    battle.value = true;
    scene.post.postDestroy(SOURCE, input[0]);
    assert.ok(scene.events.includes('R:60:305419896:0:131073'));
    assert.equal(scene.state.damageEvent, 6699);
});

// 현재 다섯 제외 DWORD 만 단락 판정에 쓰며 전체 DWORD 가 BYTE 와 다르면 제외하지 않는다.
test('damageable-post-destroy: 제외 TYPE 다섯 개는 공유 소유자의 전체 DWORD 로 비교한다', () => {
    const scene = new PostScene();
    scene.prepare(baseInput());
    const roles = [200, 201, 202, 203, 204].map((value) => ({ value }));
    scene.state.excludedDropTypes.fill(89);
    // 원문 비교 순서의 서로 다른 실제 주소를 빌린다.
    roles.forEach((role, index) => scene.state.bindExcludedDropType(index, role));
    // 다섯 각 현재 주소와 DWORD 전체 비교를 같은 드롭 몸체에서 관측한다.
    roles.forEach((role, index) => {
        role.value = 89;
        scene.post.drops(SOURCE, 0x4200, true);
        assert.deepEqual(scene.events, [], `제외 ${index}`);
        role.value = 0x10059;
        scene.post.drops(SOURCE, 0x4200, true);
        assert.equal(scene.events.length, 5, `전체 DWORD ${index}`);
        scene.events = [];
        role.value = 200 + index;
    });
    assert.throws(() => scene.state.bindExcludedDropType(5, { value: 0 }), RangeError);
});

// 현재 벌프 TYPE 이 수량 3 을 선택하며 생성 효과 중 바뀐 TYPE 으로 반복 수를 재계산하지 않는다.
test('damageable-post-destroy: 특별 TYPE 공유 소유자가 수량 3 을 정하고 반복 수는 처음 값을 유지한다', () => {
    const scene = new PostScene();
    scene.prepare(baseInput({ 15: 1 }));
    const bulf = { value: 89 };
    scene.state.specialDropType = 999;
    scene.state.bindSpecialDropType(bulf);
    scene.post.drops(SOURCE, 0x4200, true);
    assert.ok(scene.events.length === 3 && scene.slot(50)[10] === 91);
    scene.events = [];
    scene.slot(50)[10] = 89;
    bulf.value = 0x10059;
    scene.post.drops(SOURCE, 0x4200, true);
    assert.ok(scene.events.length === 5 && scene.state.specialDropType === 999);
});

// packed count/event/payload/time/Form 을 실제 Host 가 보존하고 첫 Kernel 호출에 그대로 전달한다(cpppj DamageablePostDestroy_ActualHostInitialCountAndFirstKernelCall).
// 각 피해량은 다른 실제 장면이다: 정상 피해(최대 HP 100 → 51)와 DWORD carry(131073 → 65537).
for (const maximum of [100, 131073]) {
    test(`damageable-post-destroy: 실제 Host 가 Regular 의 초기 count 를 보존하고 첫 Kernel 호출에 전달한다(최대 HP ${maximum})`, () => {
        const pool = new SidPool(24000, { server: true });
        const hash = new SquidHash();
        const spots = new Uint8Array(65536);
        const types = blankTypes();
        const frames = types.map(() => []);
        // native 와 같은 합성 group 0 이 주변 피해 Regular 분기를 선택한다.
        Object.assign(types[89], { constructorAddress: typeConstructorAddress(89), flags1: 0x10, maxHitPoints: maximum, footX: 3, footY: 3, group: 0 });
        // 내장 Form 생성자 주소(공통 ProcessForm 생성이 읽음)이다.
        for (const form of [1, 2, 3, 6]) types[form].constructorAddress = typeConstructorAddress(form);
        const factory = new SquidFactory(pool, types);
        // 실제 짧은 거주 구역 생성자와 공통 HP postCreate 로 관찰 번호 5..60 을 확보한다.
        for (let id = 5; id <= 60; id += 1) assert.equal(factory.create(89, 2), id);
        const victim = 60;
        const parent = pool.allocated(SOURCE);
        parent[11] = 2;
        put(parent, 14, floatToBits(20.5));
        put(parent, 18, floatToBits(20.5));
        const target = pool.allocated(victim);
        target[11] = 0;
        target[40] = 0;
        put(target, 14, floatToBits(21));
        put(target, 18, floatToBits(21));
        put(target, 4, 0, 2);
        hash.setBucket(1, 21, 21, victim);
        const random = new GameRandom(12345);
        const sp = new ScrambledSpStore(random, () => 1);
        const hp = new SquidReward(pool, types, new SquidPostPopState(), new SquidRewardState(), sp);
        const footprint = new RawSquidNeighbors(pool, hash, spots, types, frames);
        const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, hash, spots), types);
        const kernel = new Kernel();
        const clock = new SquidProcessState();
        clock.now = 17.25;
        const calls = [];
        // 명시 handler 관찰 경계이며 가상 HP/지면 피해 완료가 아니다.
        const host = new SquidProcessHost(pool, types, kernel, destroy, clock, {
            regular: (sid, event, count, payload) => { calls.push([sid, event, count, floatToBits(payload)]); return 0; },
        });
        const state = new DamageablePostDestroyState();
        state.battle = true;
        const regularForms = [];
        let bases = 0;
        const missing = () => { throw new Error('선택 Post Host 검사의 미복원 하위효과 요청'); };
        const post = new RawDamageablePostDestroy(pool, hash, types, frames, hp, footprint, state, {
            dropDude: missing,
            dropIsland: missing,
            regular: (sid, event, payload, count) => {
                const process = host.addRegular(sid, event, payload);
                assert.ok(process);
                process.setInitialCount(count);
                assert.ok(process.event === 6699 && process.payload === 0 && process.time === 17.25 && process.count === count);
                assert.ok(process.parent === victim && host.fromForm(process.form) === process);
                regularForms.push(process.form);
            },
            hasBridgeFall: (sid) => hasScheduledBridgeFall(host, sid),
            notifySurface: missing,
            displayUpdate: missing,
            destroy: missing,
            // 마지막 전체 공통 삭제는 Path 소비자에서 별도 연결한다.
            base: (sid, flags) => { assert.ok(sid === SOURCE && flags === 0x10000); bases += 1; },
        });
        post.postDestroy(SOURCE, 0x10000);
        assert.ok(bases === 1 && regularForms.length === 1 && kernel.size === 1 && post.pool === pool);
        const process = host.findEvent(victim, 6699);
        assert.ok(process);
        const expected = maximum === 100 ? 65587 : 131073;
        assert.ok(process.count === expected && process.payload === 0 && process.time === 17.25);
        clock.now = 17.24;
        kernel.runFrame();
        assert.ok(calls.length === 0 && kernel.size === 1);
        clock.now = 17.25;
        kernel.runFrame();
        assert.deepEqual(calls, [[60, 6699, expected, 0]]);
        assert.ok(kernel.size === 0 && (pool.slot(regularForms[0])[11] & 1) && get(pool.slot(victim), 6, 2) === 0);
        assert.ok(hash.bucket(1, 21, 21) === victim && random.state === 12345 && !sp.initialized);
        // 실제 ProcessForm 삭제 깊이도 복구된다.
        assert.ok(destroy.preDepth === 0 && destroy.postDepth === 0);
    });
}

// 경계 계약용 배치 도우미이다: 해시를 비운 뒤 지정한 후보만 다시 올린다. 각 항목은 [sid, level, x, y]이다. (2026-11-10 WB04-E253)
function replace(scene, placements) {
    scene.hash.reset();
    // 후보마다 좌표를 쓰고 그 버킷의 머리에 잇는다.
    for (const [sid, level, x, y] of placements) {
        const raw = scene.slot(sid);
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 4, scene.hash.bucket(level, x, y), 2);
        scene.hash.setBucket(level, x, y, sid);
    }
}

// 사건 목록에서 주어진 표식으로 시작하는 것만 고른다. (2026-11-10 WB04-E253)
const only = (events, marker) => events.filter((event) => event.startsWith(`${marker}:`));

// fixture 가 구별하지 못하는 경계(변형 시험에서 살아남은 입력)를 닫는 계약이다. (2026-11-10 WB04-E253)
//  - 드롭 genus 여섯 비트(0x100000·0x40000·0x20000·0x10000·0x4000·0x200) 각각의 수량과 그 밖 비트의 무효, SID 0 의 조용한 반환, buried(8) 부모의 드롭 허용.
//  - 섬 정리 조건: 잔해가 그 칸에 있어야 하고, 부모 genus 0x10000000·abstract/buried·조용한 표면 TYPE(붕괴 flag 없이)은 제외한다. 조용한 표면 TYPE 은 공유 소유자의 전체 DWORD 로 비교한다.
//  - 다리 후처리 제외 genus 세 비트(0x100000·0x20000·0x10000) 각각, 공격 소유자 8(4비트 마스크), 음수 최대 HP 의 0 방향 절삭과 count 의 DWORD 감김.
//  - 주변 피해 후보: 표면 단계(해시 0단계)의 HP 객체와 dead 후보는 예약하지 않는다.
//  - 탐색 영역: 중심에 단정도 0.99999 를 더해 절삭한다(중심 50+3ulp 는 51칸). 아래쪽 반경 2 의 다리는 찾고 3 은 찾지 않으며, 세로 중심은 Y 좌표로 정한다.
//  - 다리 프레임이 코드 표 밖이면 거부하고, 약화의 프레임 쓰기는 DWORD 전체다(256 번 프레임 → 5).
//  - genusAt: 좌표를 그대로 절삭한 한 칸, genus 에 6 비트가 있을 때만 표면 단계 포함, genus 가 겹치는 첫 후보, 2^31 은 감긴 음수 칸(없음)·±2^63 밖은 거부.
//  - 구성: 다른 풀의 HP/중심 몸체·다른 해시·짧은 프레임/TYPE 표 거부. SID 0·풀 밖 SID·TYPE 188 은 범위 오류.
// 동등 변형(결과가 같아 구별하지 않음)은 단위 문서 3절에 적었다.
test('damageable-post-destroy: 드롭 genus 비트·섬 정리 조건·다리 제외 genus·피해 후보·탐색 영역·프레임 범위·genusAt·구성 검사', () => {
    const scene = new PostScene();
    // 드롭 genus 비트별 수량.
    scene.prepare(baseInput());
    for (const [genus, count] of [[0x100000, 1], [0x40000, 1], [0x20000, 2], [0x10000, 1], [0x4000, 5], [0x200, 5], [0x24200, 2], [0x80000, 0], [0x8000, 0], [0x400, 0], [0x1000000, 0]]) {
        scene.events = [];
        scene.post.drops(SOURCE, genus, false);
        assert.equal(scene.events.length, count, `드롭 genus ${genus.toString(16)}`);
    }
    scene.events = [];
    scene.post.drops(0, 0x4200, true);
    assert.deepEqual(scene.events, []);
    scene.slot(50)[40] = 8;
    scene.post.drops(SOURCE, 0x4200, true);
    assert.equal(scene.events.length, 5);

    // 섬 정리: 부모 TYPE flags1 0x400, 전투 아님. 잔해 65(발자국 3, (50.5,50.5))가 부모 칸 (50,50)을 덮는다.
    const island = (overrides = {}, flags = 0, tweak = null) => {
        scene.prepare(baseInput({ 0: flags, 1: 0, 5: 0x400, ...overrides }));
        if (tweak) tweak();
        scene.post.postDestroy(SOURCE, flags);
        assert.equal(scene.events.at(-1), `B:50:${flags}`);
        return only(scene.events, 'I');
    };
    const dropped = [`I:${floatToBits(50.5)}:${floatToBits(50.5)}:0:2`];
    assert.deepEqual(island(), dropped);
    assert.deepEqual(island({ 6: 0x10000000 }), []);
    assert.deepEqual(island({ 6: 0x20000000 }), dropped);
    assert.deepEqual(island({ 3: 8 }), []);
    assert.deepEqual(island({ 3: 1 }), []);
    assert.deepEqual(island({ 16: floatToBits(120.5), 17: floatToBits(120.5) }), []);
    assert.deepEqual(island({ 4: 147 }), []);
    assert.deepEqual(island({ 4: 147 }, 0x200000), [`I:${floatToBits(50.5)}:${floatToBits(50.5)}:2097152:2`]);
    // 조용한 표면 TYPE 공유 소유자: 현재 DWORD 가 부모 TYPE 과 같을 때만 제외하고 독립 필드(147)는 쓰지 않는다.
    assert.deepEqual(island({}, 0, () => scene.state.bindSilentSurfaceType({ value: 89 })), []);
    assert.deepEqual(island({}, 0, () => scene.state.bindSilentSurfaceType({ value: 0x100 + 89 })), dropped);
    assert.deepEqual(island({ 4: 147 }, 0, () => scene.state.bindSilentSurfaceType({ value: 89 })), dropped);
    assert.equal(scene.state.silentSurfaceType, 147);

    // 다리 후처리 제외 genus: 부모 TYPE 90(HP, group 2 라 주변 피해 없음), 좌표 (50,50).
    for (const [genus, visited] of [[0, true], [0x40000, true], [0x100000, false], [0x20000, false], [0x10000, false]]) {
        scene.prepare([0, 1, 1, 0, 90, 0x10, genus, 2, 100, 1, 1, 0, 0, 6699, 5, 0, floatToBits(50), floatToBits(50)]);
        scene.post.postDestroy(SOURCE, 0);
        assert.equal(only(scene.events, 'N').length > 0, visited, `다리 제외 genus ${genus.toString(16)}`);
    }

    // 공격 소유자 8 과 음수 최대 HP.
    scene.prepare(baseInput({ 0: 0x80000 }));
    scene.post.postDestroy(SOURCE, 0x80000);
    assert.ok(scene.events.includes(`R:60:6699:0:${8 * 65536 + 65537}`));
    for (const [maximum, count] of [[-4, 65535], [-3, 65536], [-2, 65536], [7, 65536 + 4]]) {
        scene.prepare(baseInput({ 0: 0x10000, 8: maximum >>> 0 }));
        scene.post.postDestroy(SOURCE, 0x10000);
        assert.deepEqual(only(scene.events, 'R'), [`R:60:6699:0:${count}`], `최대 HP ${maximum}`);
    }

    // 주변 피해 후보: 표면 단계의 HP 객체(다리 TYPE 에 HP 를 줌)와 dead 후보는 예약하지 않는다.
    scene.prepare(baseInput({ 0: 0x10000 }));
    scene.types[82].flags1 = 0x10;
    // HP 몸체는 구성 때 TYPE 표를 고정하므로 바꾼 표로 다시 만든다.
    scene.hp = new SquidReward(scene.pool, scene.types, scene.bookkeeping, scene.hpState, scene.store);
    scene.post = scene.build(scene.hooks());
    assert.ok(scene.hp.hasHitPoints(63) && scene.hp.hasHitPoints(64));
    scene.post.postDestroy(SOURCE, 0x10000);
    assert.deepEqual(only(scene.events, 'R'), ['R:60:6699:0:131073']);
    scene.prepare(baseInput({ 0: 0x10000 }));
    scene.slot(60)[11] = 2;
    scene.post.postDestroy(SOURCE, 0x10000);
    assert.deepEqual(only(scene.events, 'R'), []);

    // 탐색 영역: 부모 TYPE 90(발자국 1, group 2), 다리 63 하나만 0단계에 둔다. 중심 = 좌표 − 0.5.
    const bridgeAt = (parentXBits, parentY, bridgeX, bridgeY) => {
        scene.prepare([0, 1, 1, 0, 90, 0x10, 0, 2, 100, 1, 1, 0, 0, 6699, 5, 0, parentXBits, floatToBits(parentY)]);
        replace(scene, [[63, 0, bridgeX, bridgeY]]);
        scene.post.postDestroy(SOURCE, 0);
        return only(scene.events, 'N');
    };
    // 중심 50 은 50칸(영역 48..52), 중심 50+3ulp 는 51칸(영역 49..53)이다.
    assert.deepEqual(bridgeAt(floatToBits(50.5), 20.5, 53, 20), []);
    assert.deepEqual(bridgeAt(floatToBits(50.5) + 3, 20.5, 53, 20), ['N:63']);
    assert.deepEqual(bridgeAt(floatToBits(50.5) + 3, 20.5, 48, 20), []);
    // 세로: 아래쪽 반경 2 는 찾고 3 은 찾지 않는다.
    assert.deepEqual(bridgeAt(floatToBits(50.5), 20.5, 50, 22), ['N:63']);
    assert.deepEqual(bridgeAt(floatToBits(50.5), 20.5, 50, 23), []);
    assert.deepEqual(bridgeAt(floatToBits(50.5), 20.5, 50, 18), ['N:63']);
    assert.deepEqual(bridgeAt(floatToBits(50.5), 20.5, 50, 17), []);
    // 영역 덧셈은 DWORD 로 감긴다: 중심 2^31 은 감긴 칸 −2^31 이라 영역이 비고, 범위 오류 없이 공통 post 까지 간다.
    assert.deepEqual(bridgeAt(0x4f000000, 20.5, 50, 20), []);
    assert.equal(scene.events.at(-1), 'B:50:0');

    // 다리 프레임이 코드 표 밖이면 거부한다.
    scene.prepare([0, 1, 1, 0, 90, 0x10, 0, 2, 100, 1, 1, 0, 0, 6699, 5, 0, floatToBits(50), floatToBits(50)]);
    put(scene.slot(63), 36, 4, 4);
    assert.throws(() => scene.post.postDestroy(SOURCE, 0), /다리 프레임 오류/);
    // 약화의 프레임 쓰기는 DWORD 전체다: 300칸 코드 표의 256 번 프레임(번호 1)이 금 간 5 번 프레임으로 바뀐다.
    scene.prepare([0, 1, 1, 0, 90, 0x10, 0, 2, 100, 1, 1, 0, 0, 6699, 5, 0, floatToBits(50), floatToBits(50)]);
    const long = Array.from({ length: 300 }, () => ({ side: 0x41, variant: 0x50, number: 1, flags: 0 }));
    long[5] = { side: 0x41, variant: 0x50, number: 11, flags: 0x20 };
    scene.frames[82] = long;
    replace(scene, [[63, 0, 49, 49]]);
    put(scene.slot(63), 36, 256, 4);
    scene.post.postDestroy(SOURCE, 0);
    assert.equal(get(scene.pool.slot(63), 36, 4), 5);
    assert.ok(scene.events.includes('U:63:5'));
    assert.equal(get(scene.pool.slot(63), 12, 2), 0x7325);

    // genusAt: 0단계에 다리 63(genus 4, (49,49))·TYPE 83(genus 2, (52,47))·TYPE 95(genus 0x1000000, (60,60)), 1단계에 잔해 65((50.5,50.5), 발자국 3)를 둔다.
    scene.prepare(baseInput());
    scene.slot(64)[10] = 83;
    scene.slot(62)[10] = 95;
    Object.assign(scene.types[83], { flags2: 2, footX: 1, footY: 1 });
    Object.assign(scene.types[95], { flags2: 0x1000000, footX: 1, footY: 1 });
    replace(scene, [[63, 0, 49, 49], [64, 0, 52, 47], [62, 0, 60, 60], [65, 1, 50.5, 50.5]]);
    const { post } = scene;
    assert.equal(post.genusAt(49.99999, 49.99999, 4), 63);
    assert.equal(post.genusAt(50, 49.5, 4), 0);
    assert.equal(post.genusAt(49.5, 49.5, 2), 0);
    assert.equal(post.genusAt(52.5, 47.5, 2), 64);
    assert.equal(post.genusAt(47.5, 52.5, 2), 0);
    assert.equal(post.genusAt(52.5, 47.5, 6), 64);
    assert.equal(post.genusAt(50.5, 50.5, 0x1000000), 65);
    assert.equal(post.genusAt(60, 60, 0x1000000), 0);
    assert.equal(post.genusAt(60, 60, 0x1000004), 62);
    assert.equal(post.genusAt(2147483648, 49, 4), 0);
    assert.equal(post.genusAt(-9223372036854775808, 49, 4), 0);
    assert.throws(() => post.genusAt(9223372036854775808, 49, 4), /CRT/);
    assert.throws(() => post.genusAt(49, -9223373136366403584, 4), /CRT/);
    assert.throws(() => post.genusAt(NaN, 49, 4), /CRT/);

    // 구성 검사: 다른 풀의 HP/중심 몸체, 다른 해시, 짧은 프레임/TYPE 표.
    scene.prepare(baseInput());
    const other = new SidPool(32768, { server: false });
    const build = (overrides) => {
        const parts = { pool: scene.pool, hash: scene.hash, types: scene.types, frames: scene.frames, hp: scene.hp, footprint: scene.footprint, ...overrides };
        return new RawDamageablePostDestroy(parts.pool, parts.hash, parts.types, parts.frames, parts.hp, parts.footprint, scene.state, scene.hooks());
    };
    assert.ok(build({}).pool === scene.pool);
    assert.throws(() => build({ hp: new SquidReward(other, scene.types, scene.bookkeeping, scene.hpState, scene.store) }), TypeError);
    assert.throws(() => build({ footprint: new RawSquidNeighbors(other, scene.hash, scene.spots, scene.types, scene.frames) }), TypeError);
    assert.throws(() => build({ footprint: new RawSquidNeighbors(scene.pool, new SquidHash(), scene.spots, scene.types, scene.frames) }), TypeError);
    assert.throws(() => build({ frames: scene.frames.slice(1) }), TypeError);
    assert.throws(() => build({ types: scene.types.slice(1), frames: scene.frames.slice(1) }), TypeError);
    // SID 0·풀 밖 SID·TYPE 188 은 범위 오류다.
    assert.throws(() => scene.post.postDestroy(0, 0), /SID 오류/);
    assert.throws(() => scene.post.postDestroy(32768, 0), /SID 오류/);
    scene.slot(50)[10] = 188;
    assert.throws(() => scene.post.postDestroy(SOURCE, 0), /타입 오류/);
    assert.deepEqual(scene.events, []);
});
