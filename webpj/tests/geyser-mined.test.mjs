// 간헐천 채집 통지 raw-geyser-mined.js(RawGeyserMined)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: geysermined-x86.tsv 의 `originals` 1156행 — 입력 7 값(수량 WORD·cost 비트·난수 seed·free·boss·표면 타입·소리 콜백 변이 0~12) → 게임 난수 상태, 효과 순서(S 소리[좌표 비트·번호·flags]/D 삭제/C 생성/P 배치/A assert),
//    간헐천 슬롯(6)과 생성물 슬롯(7) 50바이트(가상 표 DWORD 제외 전체). (`originalCD` 1156행은 10.72 라 사용하지 않는다.)
//  - 계약: 필수 효과/타입 표 누락·짧은 패킷·CD 판본을 raw 수량이나 난수를 바꾸기 전에 거부하고, 소리 뒤 현재 boss 로 고갈 삭제 여부를 정한다.
//  - 연결(WB04-E323, cpppj `rawGeyserMined_real_packet_factory_destroy_and_space`·`rawGeyserMined_shared_boss_rechecked_after_sound`·`NuggetTypeNumber_MiningReadsOwnerChangedBySound`):
//    registerGeyserMined 의 0x5b 등록 표(길이 6·버전 1·정책 0·observer 없음)와 실제 RawPacketDispatch 단일 분배(시각/기록/깊이)부터 같은 풀의 SquidFactory 생성·RawSquidDestroy 공통 삭제·SquidPop 공간 등록까지
//    고갈 흐름을 잇고, 소리 뒤 현재 boss/nugget 번호 재조회와 범위 실패 전 효과 보존을 확인한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E108: 간헐천 채집 통지 이식과 함께 추가, 2026-11-12 WB04-E323: 분배기 등록·실제 연결 시험 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { GEYSER_MINED_MESSAGE, RawGeyserMined, registerGeyserMined } from '../www/core/raw-geyser-mined.js';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RAW_POP_RESULT, SquidPop } from '../www/core/squid-pop.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E108)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'geysermined-x86.tsv'));
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = floatToBits;

// 예외가 나는지 본다. (2026-11-08 WB04-E108)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const freshTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));

test('GeyserMined: 원본 10.78 1156입력의 소리/수량/난수/고갈 삭제/emptyGeyser 생성의 효과 순서·난수 상태·간헐천과 생성물 슬롯이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 각 입력을 새 같은 풀/hash/난수 위에서 실행한다. 기대 수량이나 분기는 계산하지 않는다.
    for (const row of rows) {
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 6);
        const input = row[1].split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));
        assert.equal(input.length, 7);
        const [quantity, cost, seed, free, boss, surface, mutation] = input;
        const pool = new SidPool(32768);
        const hash = new SquidHash();
        const types = freshTypes();
        // 원본 격리 입력 번호 5..8 을 실제 client free list 로 확보한다.
        for (let sid = 5; sid <= 8; sid += 1) assert.equal(pool.allocate(2), sid);
        pool.allocated(7).fill(0);
        const raw = pool.allocated(6);
        raw.fill(0);
        raw[10] = 122;
        raw[11] = free;
        put(raw, 12, quantity, 2);
        put(raw, 14, bits(40.75));
        put(raw, 18, bits(41.25));
        types[154].cost = bitsToFloat(cost);
        pool.allocated(8)[10] = surface;
        hash.setCell(0, 40, 41, surface ? 8 : 0);
        const random = new GameRandom(seed);
        const state = { boss: boss !== 0, checking: false, nuggetType: 154, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
        const events = [];
        const hooks = {
            sound: (x, y, name, flags) => {
                assert.ok(/^geyserMined\d\.wav$/.test(name) && flags === 0);
                events.push(`S:${bits(x)},${bits(y)},${name[11]},${flags}`);
                if (mutation === 1) { put(raw, 12, 400, 2); state.nuggetType = 155; types[155].cost = 199.5; state.boss = true; }
                else if (mutation === 2) { put(raw, 14, bits(42.25)); put(raw, 18, bits(43.75)); put(raw, 12, 200, 2); }
                else if (mutation >= 4 && mutation <= 9) {
                    const cellX = mutation === 4 ? -0.25 : mutation === 5 ? 256 : mutation === 6 ? bitsToFloat(0x7fc00000) : mutation === 7 ? 4294967296 : mutation === 8 ? -4294967296 : Infinity;
                    const cellY = mutation === 4 ? 255.75 : mutation <= 6 ? 41.25 : 0;
                    put(raw, 14, bits(cellX));
                    put(raw, 18, bits(cellY));
                    put(raw, 12, 200, 2);
                } else if (mutation >= 10) {
                    raw[10] = 82;
                    raw[40] = mutation >= 12 ? 8 : 0;
                    state.checking = mutation === 11 || mutation === 12;
                }
            },
            destroy: (sid, flags) => {
                assert.ok(sid === 6 && flags === 0x1000);
                events.push(`D:${flags}`);
                raw[11] |= 2;
                if (mutation === 3) {
                    const x = Math.trunc(bitsToFloat(get(raw, 14)));
                    const y = Math.trunc(bitsToFloat(get(raw, 18)));
                    hash.setCell(0, x, y, surface === 97 ? 0 : 8);
                    pool.allocated(8)[10] = 97;
                }
            },
            create: (type, flags) => {
                assert.ok(type === 84 && flags === 0);
                events.push(`C:${type},${flags}`);
                const born = pool.allocated(7);
                born.fill(0);
                born[10] = type;
                born[11] = 4;
                return 7;
            },
            pop: (sid, x, y, flags) => {
                assert.ok(sid === 7 && flags === 0);
                events.push(`P:${bits(x)},${bits(y)},${flags}`);
                const born = pool.allocated(sid);
                put(born, 14, bits(x));
                put(born, 18, bits(y));
                born[11] = 0;
            }
        };
        if (mutation >= 4 && mutation <= 9) {
            const x = mutation === 5 ? 40 : 0;
            const y = mutation === 4 ? 255 : mutation <= 6 ? 41 : 0;
            hash.setCell(0, x, y, surface ? 8 : 0);
        }
        const handler = new RawGeyserMined(pool, hash, types, random, state, hooks);
        const packet = Uint8Array.of(0xab, 0xcd, 0xef, 0xff, 6, 0);
        if (row[3].endsWith(';A')) {
            assert.ok(throws(() => handler.handlePacket(packet)));
            events.push('A');
        } else {
            handler.handlePacket(packet);
        }
        const label = `행 ${checked}: ${row[1]}`;
        assert.equal(String(random.state), row[2], label);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[3], label);
        // 원본에서 가상 표만 격리 주소로 주입했으므로 그것을 제외한 모든 raw byte 를 비교한다.
        assert.equal(hex(pool.slot(6)).slice(8), row[4].slice(8), label);
        assert.equal(hex(pool.slot(7)).slice(8), row[5].slice(8), label);
        checked += 1;
    }
    assert.equal(checked, 1156);
});

test('GeyserMined: 필수 효과/타입 표 누락·짧은 패킷·CD 판본은 거부하고 소리 뒤 현재 boss 로 고갈 삭제 여부를 정한다', () => {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const types = freshTypes();
    types[122].footX = types[122].footY = types[84].footX = types[84].footY = 1;
    types[154].cost = 200;
    const full = { sound: () => {}, destroy: () => {}, create: () => 7, pop: () => {} };
    const state = { boss: false, checking: false, nuggetType: 154, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
    assert.ok(throws(() => new RawGeyserMined(pool, hash, types, new GameRandom(), state, {})));
    assert.ok(throws(() => new RawGeyserMined(pool, hash, types, new GameRandom(), state, { ...full, pop: null })));
    assert.ok(throws(() => new RawGeyserMined({ edition: 'cd1072' }, hash, types, new GameRandom(), state, full)));
    assert.ok(throws(() => new RawGeyserMined(pool, hash, types.slice(0, 171), new GameRandom(), state, full)));
    const handler = new RawGeyserMined(pool, hash, types, new GameRandom(), state, full);
    assert.equal(handler.pool, pool);
    assert.ok(throws(() => handler.handlePacket(new Uint8Array(0))));
    // 소리 뒤 현재 boss 를 다시 읽는다: 소리 콜백이 boss 를 켜면 같은 호출에서 고갈 삭제와 생성/배치가 일어난다.
    const geyser = pool.allocate(2);
    const raw = pool.allocated(geyser);
    raw[10] = 122;
    put(raw, 12, 200, 2);
    put(raw, 14, bits(40.75));
    put(raw, 18, bits(41.25));
    const effects = [];
    const live = new RawGeyserMined(pool, hash, types, new GameRandom(7), state, {
        sound: () => { effects.push('S'); state.boss = true; },
        destroy: (sid, flags) => { effects.push('D' + flags); },
        create: (type) => { effects.push('C' + type); return 9; },
        pop: (sid, x, y) => { effects.push('P' + sid + ':' + x + ',' + y); }
    });
    live.handle(geyser);
    assert.deepEqual(effects, ['S', 'D4096', 'C84', 'P9:40.75,41.25']);
    assert.equal(get(pool.slot(geyser), 12) & 0xffff, 0);
});

// cpppj `rawGeyserMined_real_packet_factory_destroy_and_space` 이식: 원본 등록/단일 분배부터 같은 풀의 실제 factory/공통 삭제/Pop 과 현재 표면까지 고갈 흐름을 잇는다.
// 기대값은 모듈이 낸 결과의 관계(등록 표 값·효과 횟수·SID 영역·공간 머리·좌표 비트)이다. (2026-11-12 WB04-E323)
test('GeyserMined: 실제 분배기 등록·단일 분배부터 같은 풀의 factory/공통 삭제/Pop 과 현재 표면까지 고갈 흐름이 이어진다', () => {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = freshTypes();
    types[122].footX = types[122].footY = types[84].footX = types[84].footY = 1;
    types[154].cost = 200;
    const factory = new SquidFactory(pool, types);
    const pop = new SquidPop(pool, hash, spots);
    const unpop = new SquidUnpop(pool, hash, spots);
    const destroy = new RawSquidDestroy(pool, unpop, types);
    // 서버 객체 Pop 의 실제 전송 요청을 기록한다. cpppj 원 시험은 이 장치를 묶지 않아 2026-11-04 `bd36f6e64`(Pop 전송 미연결 검사 추가) 뒤로 실패했고 cpppj 1304 에서 같은 방식으로 교정됐다(단위 문서 3절).
    const transmissions = [];
    pop.bindTransmission((sid, flags) => { transmissions.push([sid, flags]); });
    const geyser = factory.create(122, 2);
    put(pool.allocated(geyser), 12, 200, 2);
    assert.equal(pop.pop(geyser, types[122], 1, 1, 40.75, 41.25, 0), RAW_POP_RESULT.registered);
    // client 영역 간헐천의 Pop 은 전송하지 않는다.
    assert.deepEqual(transmissions, []);
    const random = new GameRandom();
    const state = { boss: true, checking: false, nuggetType: 154, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
    const counts = { sounds: 0, created: 0, recorded: 0 };
    let born = 0;
    const connection = new PacketConnection({ sender: 17 });
    const hooks = {
        // 소리는 분배기의 즉시 콜백 안(깊이 1)이며 기록 효과 뒤이다.
        sound: (x, y, name, flags) => {
            assert.ok(flags === 0 && connection.depth === 1 && counts.recorded === 1);
            counts.sounds += 1;
        },
        // 고갈한 간헐천은 공통 삭제 몸체로 지운다. 파생 효과 중 pre/post 만 완료하고 다른 효과는 이 장면의 미실행 경계이다.
        destroy: (sid, flags) => {
            assert.ok(sid === geyser && flags === 0x1000);
            destroy.destroy(sid, flags, {
                selected: () => 0,
                emit: (event) => {
                    if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) destroy.completePreDestroy();
                    else if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) destroy.completePostDestroy();
                    else throw new Error(`공통 채집 삭제 검사에서 다른 파생 효과(${event.effect})는 미실행 경계`);
                },
            });
        },
        // emptyGeyser 는 같은 풀의 실제 Factory 로 만든다(서버 풀의 flags 0 → 서버 영역).
        create: (type, flags) => {
            assert.ok(type === 84 && flags === 0);
            born = factory.create(type, flags);
            counts.created += 1;
            return born;
        },
        // 생성물은 실제 Pop 으로 공간에 등록한다.
        pop: (sid, x, y, flags) => {
            assert.ok(sid === born && flags === 0);
            assert.equal(pop.pop(sid, types[84], 1, 1, x, y, flags), RAW_POP_RESULT.registered);
        },
    };
    const handler = new RawGeyserMined(pool, hash, types, random, state, hooks);
    // 분배기 backend 장치: 고정 시각 723, 기록은 수신기 전(깊이 0)에 한 번, 종료/소멸/해제는 이 장면에서 일어나면 안 된다.
    const device = {
        now: () => 723,
        record: (current, packet, length) => {
            assert.ok(current === connection && current.depth === 0 && current.received === 723 && length === 6 && packet[1] === GEYSER_MINED_MESSAGE);
            counts.recorded += 1;
        },
        terminate: () => { throw new Error('간헐천 채집의 mismatch 0 은 버전 종료를 요청하지 않는다'); },
        close: () => { throw new Error('간헐천 채집은 이 연결을 닫지 않는다'); },
        release: () => { throw new Error('간헐천 채집은 이 연결을 해제하지 않는다'); },
    };
    const dispatch = new RawPacketDispatch(device);
    // 원본 타입 번호는 모듈 상수와 독립으로 0x5b 이다. 등록 전 표에 다른 observer/직접 수신기를 미리 두어 등록이 그것을 지우는지 본다.
    assert.equal(GEYSER_MINED_MESSAGE, 0x5b);
    dispatch.observePacket(0x5b, () => {});
    dispatch.registerDirect(0x5b, () => { throw new Error('등록 전 직접 수신기가 남았다'); });
    registerGeyserMined(dispatch, handler);
    assert.equal(handler.pool, pool);
    const registration = dispatch.registration(0x5b);
    assert.deepEqual([registration.length, registration.version, registration.mismatch, registration.observer, registration.packetObserver, registration.directReceive, typeof registration.receive],
        [6, 1, 0, null, null, null, 'function']);
    const packet = Uint8Array.of(0xff, 0x5b, 6, 0, geyser & 0xff, geyser >> 8);
    assert.ok(dispatch.validate(packet) && dispatch.dispatch(connection, packet));
    assert.deepEqual([connection.depth, connection.received, counts.recorded], [0, 723, 1]);
    assert.ok(counts.sounds === 1 && counts.created === 1 && born >= pool.layout.serverFirst);
    // 서버 영역 emptyGeyser 의 Pop 만 한 번 전송을 요청한다(flags 9 는 Pop flags 0 의 첫 활성화가 SquidPop 에서 낸 송신 flags 이다).
    assert.deepEqual(transmissions, [[born, 9]]);
    assert.ok(pool.slot(geyser)[11] & 1);
    assert.equal(hash.bucket(1, 40.75, 41.25), born);
    assert.equal(get(pool.slot(born), 14), bits(40.75));
    assert.equal(get(pool.slot(born), 18), bits(41.25));
    assert.ok(throws(() => handler.handlePacket(new Uint8Array(0))));
    assert.ok(throws(() => new RawGeyserMined(pool, hash, types, random, state, {})));
    // 처리기가 아닌 값은 등록 표를 바꾸기 전에 거부한다(웹 전용 계약: C++ 는 참조 타입으로 막는다).
    const other = new RawPacketDispatch(device);
    assert.ok(throws(() => registerGeyserMined(other, { handlePacket: () => {} })));
    assert.equal(other.registration(GEYSER_MINED_MESSAGE).receive, null);
});

// cpppj `rawGeyserMined_shared_boss_rechecked_after_sound` 이식: 채집 소리 효과 뒤의 현재 boss 로 고갈 삭제와 Empty 생성 여부를 정한다. (2026-11-12 WB04-E323)
test('GeyserMined: 소리 뒤 현재 boss 가 꺼지면 수량만 줄이고 고갈 삭제/생성/배치를 하지 않는다', () => {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const types = freshTypes();
    types[154].cost = 1;
    // 원본 분석 장면의 client 번호를 실제 풀에서 확보한다.
    for (let sid = 5; sid <= 8; sid += 1) assert.equal(pool.allocate(2), sid);
    const raw = pool.allocated(6);
    raw.fill(0);
    raw[10] = 122;
    put(raw, 12, 1, 2);
    put(raw, 14, bits(40.75));
    put(raw, 18, bits(41.25));
    let nextBoss = false;
    const counts = { destroyed: 0, created: 0, popped: 0 };
    // boss 는 공유 전역처럼 매번 현재 값을 읽는 getter 이다.
    let boss = true;
    const state = { get boss() { return boss; }, checking: false, nuggetType: 154, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
    const mined = new RawGeyserMined(pool, hash, types, new GameRandom(123), state, {
        sound: () => { boss = nextBoss; },
        destroy: (sid, flags) => { assert.ok(sid === 6 && flags === 0x1000); counts.destroyed += 1; raw[11] |= 2; },
        create: (type, flags) => { assert.ok(type === 84 && flags === 0); counts.created += 1; return 7; },
        pop: (sid, x, y, flags) => { assert.ok(sid === 7 && x === 40.75 && y === 41.25 && flags === 0); counts.popped += 1; },
    });
    mined.handle(6);
    assert.deepEqual([get(raw, 12, 2), counts.destroyed, counts.created, counts.popped], [0, 0, 0, 0]);
    put(raw, 12, 1, 2);
    nextBoss = true;
    boss = false;
    mined.handle(6);
    assert.deepEqual([get(raw, 12, 2), counts.destroyed, counts.created, counts.popped], [0, 1, 1, 1]);
});

// cpppj `NuggetTypeNumber_MiningReadsOwnerChangedBySound` 이식: 음향 뒤의 현재 nugget 번호로 단가를 읽으며, 범위 실패 전의 음향/난수 효과는 남고 수량은 그대로이다. (2026-11-12 WB04-E323)
test('GeyserMined: 소리 뒤 현재 nugget 번호의 단가로 수량을 줄이고 번호가 표 밖이면 소리 뒤 수량 기록 전에 실패한다', () => {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const types = freshTypes();
    types[154].cost = 1;
    types[70].cost = 20.75;
    types[71].cost = 30.25;
    // 같은 실제 슬롯의 단가/수량 조회만 준비한다.
    for (let sid = 5; sid <= 6; sid += 1) assert.equal(pool.allocate(2), sid);
    const raw = pool.allocated(6);
    raw.fill(0);
    raw[10] = 122;
    put(raw, 12, 100, 2);
    put(raw, 14, bits(40.75));
    put(raw, 18, bits(41.25));
    let current = 154;
    let next = 70;
    let sounds = 0;
    const state = { boss: false, checking: false, get nuggetType() { return current; }, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
    const random = new GameRandom(123);
    const mined = new RawGeyserMined(pool, hash, types, random, state, {
        sound: (x, y, name, flags) => { assert.ok(x === 40.75 && y === 41.25 && flags === 0); sounds += 1; current = next; },
        destroy: () => { throw new Error('현재 단가 검사에서 선택하지 않은 고갈 삭제'); },
        create: () => { throw new Error('현재 단가 검사에서 선택하지 않은 생성'); },
        pop: () => { throw new Error('현재 단가 검사에서 선택하지 않은 Pop'); },
    });
    mined.handle(6);
    assert.deepEqual([get(raw, 12, 2), sounds], [79, 1]);
    put(raw, 12, 100, 2);
    next = 71;
    mined.handle(6);
    assert.deepEqual([get(raw, 12, 2), sounds], [69, 2]);
    put(raw, 12, 100, 2);
    next = types.length;
    const before = random.state;
    assert.ok(throws(() => mined.handle(6)));
    // 실제 선택 범위 실패 전의 음향/난수 효과는 남는다.
    assert.deepEqual([get(raw, 12, 2), sounds], [100, 3]);
    assert.notEqual(random.state, before);
});
