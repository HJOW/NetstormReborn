// SquidFactory(squid-factory.js)·생성자 표(squid-constructors.js)를 원본 10.78 관측과 대조한다(WB04-D4).
//  - 원본 관측(cpppj/tests/fixtures, 10.78 행만): constructors 188행·asset-constructor-current 567행·squid-creation-parent 294행·squid-take-parent-current 360행·
//    factory-initialization-current 288행·first-pop-flags 256행·creation 482단계·derived 3300단계·unpop 1809단계.
//    각 행의 합성 입력·명시 장치는 cpppj 검사(CreationTests·AssetConstructorCurrentTests·SquidCreationParentTests·SquidTakeParentCurrentTests·
//    FactoryInitializationCurrentTests·FactoryFirstPopCurrentTests·UnpopTests)와 같고 기대값은 fixture 에서 읽는다.
//  - 초기화 fixture 의 사제 1/4 HP·사제 TYPE 전역은 웹에서 같은 GameAuxState(bindGameAux)로 빌린다(cpppj 의 장기 주소 연결과 같은 역할).
// 계약 검사는 tests/squid-factory-contract.test.mjs 에 있다. 실제 게임·브라우저 실행이 아니다. (2026-11-06 WB04-D4: SquidFactory 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { PATCH_TYPE_CONSTRUCTORS, typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidFactory, createRawSquid, takeRawSquid } from '../www/core/squid-factory.js';

// 원본 관측 파일 폴더와 10.78 행 읽기이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-06 WB04-D4)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
// 첫 열이 originals 인 행만 고른다. (2026-11-06 WB04-D4)
const patchRows = (name) => readRows(path.join(FIXTURES, name))?.filter((row) => row[0] === 'originals') ?? null;
// Begin 행의 판본으로 블록을 나눠 10.78 블록의 행만 고른다(Begin 행 포함). (2026-11-06 WB04-D4)
function patchBlocks(name) {
    const rows = readRows(path.join(FIXTURES, name));
    if (!rows) return null;
    const result = [];
    let keep = false;
    // Begin 행이 다음 블록의 판본을 정한다.
    for (const row of rows) {
        if (row[0] === 'Begin') keep = row[1] === 'originals';
        if (keep) result.push(row);
    }
    return result;
}

// 원문 진단 파일 이름과 base 생성자 VT 이다. (2026-11-06 WB04-D4)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const BASE_VT = 0x501dd0;

// 바이트 배열의 소문자 16진 문자열과 그 역변환이다. (2026-11-06 WB04-D4)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 두 바이트 배열이 같은지 네이티브 비교로 확인한다(큰 풀 전체 비교용). (2026-11-06 WB04-D4)
function sameBytes(left, right) {
    return Buffer.from(left.buffer, left.byteOffset, left.length).equals(Buffer.from(right.buffer, right.byteOffset, right.length));
}

// 실제 생성자 주소 표를 넣은 188칸 TYPE 입력이다(cpppj Types). footOne 이면 발자국 1×1 도 둔다. (2026-11-06 WB04-D4)
function constructorTypes(footOne = false) {
    return PATCH_TYPE_CONSTRUCTORS.map((address) => (footOne ? { constructorAddress: address, footX: 1, footY: 1 } : { constructorAddress: address }));
}

// 삭제 기록 두 영역(client→server, SID/type DWORD)을 패딩 없이 직렬화한다(cpppj ExtraBytes 의 풀 부분). (2026-11-06 WB04-D4)
function deletionBytes(pool) {
    const values = [];
    // client 기록 다음 server 기록이다.
    for (const client of [true, false]) {
        // 레코드마다 SID 다음 TYPE 이다.
        for (const record of pool.deletions(client)) values.push(record.sid, record.type);
    }
    const bytes = new Uint8Array(values.length * 4);
    // 각 DWORD 를 낮은 바이트부터 둔다.
    values.forEach((value, index) => put(bytes, index * 4, value >>> 0, 4));
    return bytes;
}

// 공간 해시 네 단계의 WORD 머리를 0단계부터 직렬화한다(cpppj ExtraBytes 의 해시 부분). (2026-11-06 WB04-D4)
function hashBytes(hash) {
    const parts = [];
    // 가장 큰 0단계부터 3단계까지다.
    for (let level = 0; level < 4; level += 1) parts.push(hash.entries(level));
    const total = parts.reduce((sum, part) => sum + part.length, 0);
    const bytes = new Uint8Array(total * 2);
    let offset = 0;
    // 각 머리의 두 바이트를 배열 순서로 둔다.
    for (const part of parts) {
        for (const value of part) {
            bytes[offset] = value & 0xff;
            bytes[offset + 1] = (value >>> 8) & 0xff;
            offset += 2;
        }
    }
    return bytes;
}

// 풀 카운터·머리·꼬리·전체 Adler-32 묶음이다(두 재생 검사 공용). (2026-11-06 WB04-D4)
function poolSummary(pool) {
    return [pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.tail(true), pool.firstFree(false), pool.tail(false), adler32(pool.bytes())];
}

const constructorRows = patchRows('constructors-x86.tsv');
const assetCtorRows = patchRows('asset-constructor-current-x86.tsv');
const creationParentRows = patchRows('squid-creation-parent-x86.tsv');
const takeParentRows = patchRows('squid-take-parent-current-x86.tsv');
const initializationRows = patchRows('factory-initialization-current-x86.tsv');
const firstPopRows = patchRows('first-pop-flags-x86.tsv');
const creationBlocks = patchBlocks('creation-x86.tsv');
const derivedBlocks = patchBlocks('derived-x86.tsv');
const unpopBlocks = patchBlocks('unpop-x86.tsv');

// 원본 188행: 타입 번호별 생성자 주소 표(0 은 base)가 x86 대입 구간 결과와 같다.
test('squid-constructors: 원본10.78 생성자 주소 188행과 같다', { skip: !constructorRows && '원본 fixture 없음' }, () => {
    // 행마다 같은 번호의 주소를 비교한다.
    for (const row of constructorRows) {
        assert.equal(row.length, 3);
        assert.equal(typeConstructorAddress(Number(row[1])), Number(row[2]), `type ${row[1]}`);
    }
    assert.equal(constructorRows.length, 188);
    assert.equal(PATCH_TYPE_CONSTRUCTORS.filter(Boolean).length, 122);
    assert.throws(() => typeConstructorAddress(188));
});

// 원본 567행: 자산 bare 생성자(145 recipe 중 10.78 82개와 NULL·다이스 보조)가 물리 슬롯 전체에 남기는 바이트와 다이스 진단 정상 복귀 횟수.
test('SquidFactory.constructByAddress: 원본10.78 자산 생성자 567행과 같다', { skip: !assetCtorRows && '원본 fixture 없음' }, () => {
    // 행마다 새 client 풀(65535)에서 실행한다.
    for (const row of assetCtorRows) {
        assert.equal(row.length, 8);
        const pool = new SidPool(65535, { server: false });
        const factory = new SquidFactory(pool, Array.from({ length: 188 }, () => ({})));
        const sid = Number(row[2]);
        const before = unhex(row[4]);
        const raw = pool.physical(sid);
        raw.set(before);
        let diagnoses = 0;
        // 다이스 보조 생성자의 명시 정상 복귀 입력이다(mutation 이면 현재 payload 를 바꾼다).
        factory.bindDiagnostics({
            assertion: (expression, file, line) => {
                assert.deepEqual([expression, file, line], ['false', '.\\Dais.cpp', 0x484]);
                diagnoses += 1;
                if (Number(row[3])) {
                    raw[10] = 75;
                    raw[34] = 4;
                }
            },
        });
        const free = pool.freeCount;
        assert.equal(factory.constructByAddress(Number(row[1]), sid), true);
        assert.equal(hex(raw), row[5], `entry ${Number(row[1]).toString(16)} sid ${sid}`);
        assert.equal(diagnoses, Number(row[7]));
        assert.equal(pool.freeCount, free);
        assert.equal(raw[11], before[11]);
    }
    assert.equal(assetCtorRows.length, 567);
});

// 원본 294행: Create 부모의 fakeSurface/signed SID 진단·할당·생성자 조회·반환 객체 VT 저장·TYPE·postCreate 순서와 반환 객체 전체 바이트.
test('createRawSquid: 원본10.78 Create 부모 294행과 같다', { skip: !creationParentRows && '원본 fixture 없음' }, () => {
    const POOL = 0x13000000, VT = 0x14000000, CTOR = 0x10001000, STRIDE = 50;
    // 행마다 독립 토큰 버퍼로 실행한다.
    for (const row of creationParentRows) {
        assert.equal(row.length, 5);
        const [type, flags, sid, profile, base] = row[1].split(',').map(Number);
        let constructor = base ? 0 : CTOR;
        const bytes = new Uint8Array(128 * STRIDE);
        // 원문 실행 전의 독립 sentinel 입력이다.
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 37 + 19) & 0xff;
        // 객체 토큰을 검사 소유 버퍼에만 해석한다.
        const raw = (object) => {
            assert.ok(object >= POOL && object < POOL + bytes.length, `object ${object}`);
            return bytes.subarray(object - POOL, object - POOL + STRIDE);
        };
        const trace = [];
        const result = createRawSquid(type, flags, {
            fakeSurfaceType: () => 162,
            assertion: (expression, file, line) => trace.push(`D${expression}|${file}|${line}`),
            allocate: (value) => {
                trace.push(`A${value}`);
                if (profile === 1) constructor = CTOR + 0x10;
                return sid;
            },
            constructor: (number) => {
                trace.push(`T${number}:${constructor}`);
                return constructor;
            },
            // base 쓰기 또는 명시 cdecl 반환 객체를 공급한다(토큰 산술은 원문처럼 32비트로 감긴다).
            construct: (address, number) => {
                if (!address) {
                    const object = (POOL + Math.imul(number, STRIDE)) >>> 0;
                    put(raw(object), 0, BASE_VT);
                    trace.push(`B${object}:${BASE_VT}`);
                    return object;
                }
                trace.push(`C${address}:${number}`);
                const object = POOL + (profile === 1 ? 9 : 5) * STRIDE;
                put(raw(object), 0, VT);
                return object;
            },
            vtable: (object) => {
                const table = get(raw(object), 0);
                trace.push(`V${object}:${table}`);
                return table;
            },
            storeType: (object, value) => {
                trace.push(`W${object}:${value}`);
                raw(object)[10] = value;
                if (profile === 2) put(raw(object), 0, VT + 0x100);
            },
            postCreate: (object, table) => trace.push(`P${object}:${table}`),
        });
        assert.equal(result, Number(row[2]), row[1]);
        assert.equal(trace.join(';'), row[3], row[1]);
        assert.equal(hex(raw(result)), row[4], row[1]);
    }
    assert.equal(creationParentRows.length, 294);
});

// 원본 360행: Take 부모의 SID 진단·상태/TYPE/VT 읽기·상세·가상 Unpop·상태 저장·현재 생성자·반환 객체 VT 캐시·TYPE→void→postTake 순서와 두 객체 전체 바이트.
test('takeRawSquid: 원본10.78 Take 부모 360행과 같다', { skip: !takeParentRows && '원본 fixture 없음' }, () => {
    const POOL = 0x13000000, VT = 0x14000000, CTOR = 0x10001000, WIDTH = 50;
    // 행마다 독립 두 객체로 실행한다.
    for (const row of takeParentRows) {
        assert.equal(row.length, 5);
        const [type, sid, state, profile, base] = row[1].split(',').map(Number);
        const slots = [new Uint8Array(WIDTH), new Uint8Array(WIDTH)];
        const objects = [POOL + 5 * WIDTH, POOL + 9 * WIDTH];
        let constructor = base ? 0 : CTOR;
        const trace = [];
        // 객체 토큰을 인자의 SID 로 다시 해석하지 않는다.
        const raw = (object) => {
            assert.ok(object === objects[0] || object === objects[1]);
            return slots[object === objects[0] ? 0 : 1];
        };
        // 두 객체의 전체 payload 입력이다.
        for (let index = 0; index < 2; index += 1) {
            // 각 바이트에 관측기와 같은 패턴을 둔다.
            for (let offset = 0; offset < WIDTH; offset += 1) slots[index][offset] = (((index ? 9 : 5) * WIDTH + offset) * 37 + 19) & 0xff;
            put(slots[index], 0, VT);
            slots[index][10] = 74;
            slots[index][11] = state;
        }
        const returned = takeRawSquid(type, sid, {
            capacity: () => {
                trace.push('Q32768');
                return 32768;
            },
            slot: () => objects[0],
            state: (object) => {
                const value = raw(object)[11];
                trace.push(`R${object}:${value}`);
                return value;
            },
            type: (object) => {
                const value = raw(object)[10];
                trace.push(`T${object}:${value}`);
                return value;
            },
            vtable: (object) => {
                const value = get(raw(object), 0);
                trace.push(`V${object}:${value}`);
                return value;
            },
            detail: (server, client) => trace.push(`F${server}:${client}`, 'L'),
            assertion: (expression, file, line) => trace.push(`D${expression}|${file}|${line}`),
            // 실제 자식으로 주장하지 않는 정상 복귀 입력이다.
            unpop: (object, table) => {
                trace.push(`U${object}:${table}`);
                if (profile === 1) {
                    raw(object)[11] = 0x8f;
                    constructor = base ? 0 : CTOR + 0x10;
                }
            },
            storeState: (object, value) => {
                trace.push(`W${object}:${value}`);
                raw(object)[11] = value;
            },
            storeType: (object, value) => {
                trace.push(`Y${object}:${value}`);
                raw(object)[10] = value;
                if (profile === 2) {
                    put(raw(object), 0, VT + 0x100);
                    raw(object)[11] = 0x80;
                }
            },
            constructor: (number) => {
                trace.push(`C${number}:${constructor}`);
                return constructor;
            },
            // base 쓰기 또는 명시 cdecl 반환 객체를 공급한다.
            construct: (address, number) => {
                if (!address) {
                    trace.push(`B${objects[0]}:${BASE_VT}`);
                    put(raw(objects[0]), 0, BASE_VT);
                    return objects[0];
                }
                trace.push(`A${address}:${number}`);
                const object = objects[profile === 1 ? 1 : 0];
                put(raw(object), 0, VT);
                return object;
            },
            postTake: (object, table) => trace.push(`P${object}:${table}`),
        });
        assert.equal(returned, Number(row[2]), row[1]);
        assert.equal(trace.join(';'), row[3], row[1]);
        assert.equal(`${hex(slots[0])};${hex(slots[1])}`, row[4], row[1]);
    }
    assert.equal(takeParentRows.length, 360);
});

// 원본 288행: PostCreate/PostTake 의 진단 정상 복귀·현재 TYPE/서버 재조회·zOrder/owner/HP 쓰기와 현재 1/4 HP·사제 TYPE 전역(GameAuxState).
test('SquidFactory.postCreate/postTake: 원본10.78 초기화 288행과 같다', { skip: !initializationRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀·TYPE 표·전역 소유자로 실행한다.
    for (const row of initializationRows) {
        assert.equal(row.length, 7);
        const input = row[1].split(',');
        const [, type, state, server, mode, profile] = input.map(Number);
        const owner = { value: server !== 0 };
        const aux = new GameAuxState();
        const pool = new SidPool(32768, { serverOwner: owner });
        const sid = pool.allocate(2);
        const raw = pool.physical(sid);
        // native 초기 payload 와 같은 입력이다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 17 + 23) & 0xff;
        raw[10] = type;
        raw[11] = state;
        raw[40] = mode === 2 ? 1 : 0;
        const types = Array.from({ length: 188 }, () => ({}));
        // 입력 TYPE 은 원문 장치와 같은 값만 갖는다.
        for (const number of [3, 74, 75, 76]) {
            types[number] = { maxHitPoints: number === 74 ? 0x12345678 : -37, flags1: mode === 0 ? 16 : 0, flags2: mode ? 0x400000 : 0, zOrder: 0x123 + number };
        }
        const factory = new SquidFactory(pool, types);
        factory.bindGameAux(aux);
        const diagnostics = [];
        // 같은 원문 외부 진단의 명시 정상 복귀 변경이다.
        factory.bindDiagnostics({
            types,
            assertion: (expression, file, line) => {
                diagnostics.push(`${expression}|${file}|${line}|${hex(raw)}`);
                if (profile === 1) {
                    raw[10] = 75;
                    types[75].maxHitPoints = -37;
                    types[75].zOrder = 0x345;
                    aux.priestQuarterHp = true;
                    aux.priestType = 75;
                } else if (profile === 2) {
                    raw[10] = 76;
                    owner.value = false;
                    raw[11] &= ~8 & 0xff;
                }
            },
        });
        const free = pool.freeCount;
        if (input[0] === 'create') factory.postCreate(sid);
        else {
            assert.equal(input[0], 'take');
            factory.postTake(sid);
        }
        assert.equal(hex(raw), row[2], row[1]);
        assert.equal(diagnostics.length ? diagnostics.join(';') : '-', row[3], row[1]);
        assert.deepEqual([owner.value, aux.priestQuarterHp, aux.priestType], [Number(row[4]) !== 0, Number(row[5]) !== 0, Number(row[6])], row[1]);
        assert.equal(pool.freeCount, free);
    }
    assert.equal(initializationRows.length, 288);
});

// 원본 256행: 공통 firstPop 은 TYPE/상태/VT 와 무관하게 extra(+40) BYTE 의 1/8 비트만 읽는다(6 물리 입력, 풀 전체 무쓰기).
test('SquidFactory.firstPopFlags: 원본10.78 firstPop 256행과 같다', { skip: !firstPopRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    const factory = new SquidFactory(pool, Array.from({ length: 188 }, () => ({})));
    // [sid, TYPE, 상태, VT] 의 여섯 입력이다(예약 0/1/2·free·다른 VT 포함).
    const profiles = [[5, 70, 4, BASE_VT], [0, 255, 15, 0], [1, 6, 8, BASE_VT], [2, 70, 1, BASE_VT], [5, 255, 0, BASE_VT], [5, 70, 4, 0x12345678]];
    let calls = 0;
    // 행마다 같은 기대값을 여섯 입력에 적용한다.
    for (const row of firstPopRows) {
        assert.equal(row.length, 3);
        // 각 물리 입력에서 같은 결과와 무쓰기를 확인한다.
        for (const [sid, type, state, table] of profiles) {
            const raw = pool.physical(sid);
            raw.fill(0x65);
            raw[10] = type;
            raw[11] = state;
            raw[40] = Number(row[1]);
            put(raw, 0, table);
            const before = pool.bytes();
            assert.equal(factory.firstPopFlags(sid), Number(row[2]), `extra ${row[1]}`);
            assert.ok(sameBytes(pool.bytes(), before));
            calls += 1;
        }
    }
    assert.deepEqual([firstPopRows.length, calls], [256, 1536]);
    assert.throws(() => factory.firstPopFlags(24000), RangeError);
});

// creation/derived 재생: Begin·Type·Fill·Abstract 합성 입력 뒤 Create/Construct/Take/PostCreate/PostTake 의 반환 SID·풀 요약·결과 슬롯 전체를 대조한다(cpppj Replay). (2026-11-06 WB04-D4)
function replayCreation(rows) {
    let pool = null;
    let factory = null;
    let types = null;
    let unpop = null;
    let weak = false;
    let begins = 0;
    let steps = 0;
    // 각 행을 원문 순서로 적용한다.
    for (const row of rows) {
        if (row[0] === 'Begin') {
            assert.equal(row.length, 5);
            pool = new SidPool(Number(row[2]), { server: row[3] === '1' });
            types = constructorTypes();
            weak = row[4] === '1';
            unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536));
            factory = new SquidFactory(pool, types, { weakenedMana: weak, unpop });
            begins += 1;
        } else if (row[0] === 'Type') {
            assert.equal(row.length, 7);
            const type = types[Number(row[1])];
            Object.assign(type, { flags1: Number(row[2]) >>> 0, flags2: Number(row[3]) >>> 0, maxHitPoints: Number(row[4]) | 0, zOrder: Number(row[5]) | 0, constructorAddress: Number(row[6]) >>> 0 });
            factory = new SquidFactory(pool, types, { weakenedMana: weak, unpop });
        } else if (row[0] === 'Fill') {
            assert.equal(row.length, 6);
            const bytes = pool.allocated(Number(row[1]));
            const seed = Number(row[2]);
            // 기록된 입력 바이트만 놓는다.
            for (let index = 0; index < bytes.length; index += 1) bytes[index] = (seed + index * 37) & 0xff;
            bytes[11] = Number(row[3]);
            bytes[10] = Number(row[4]);
            put(bytes, 0, Number(row[5]) >>> 0);
        } else if (row[0] === 'Abstract') {
            assert.equal(row.length, 3);
            pool.allocated(Number(row[1]))[40] = Number(row[2]);
        } else {
            assert.equal(row.length, 13);
            const type = Number(row[2]);
            const arg = Number(row[3]) >>> 0;
            let result = arg & 0xffff;
            if (row[1] === 'Create') result = factory.create(type, arg);
            else if (row[1] === 'Construct') result = factory.construct(type, result);
            else if (row[1] === 'Take') result = factory.take(type, result);
            else if (row[1] === 'PostCreate') factory.postCreate(result);
            else if (row[1] === 'PostTake') factory.postTake(result);
            else assert.fail(`생성 fixture 명령 오류 ${row[1]}`);
            const label = `${begins}:${steps} ${row[1]} ${row[2]} ${row[3]}`;
            assert.deepEqual([result, ...poolSummary(pool)], row.slice(4, 12).map(Number), label);
            assert.equal(hex(pool.slot(result)), row[12], label);
            steps += 1;
        }
    }
    return [begins, steps];
}

// 원본 482단계: base 생성/초기화와 void Take.
test('SquidFactory: 원본10.78 base 생성 482단계와 같다', { skip: !creationBlocks && '원본 fixture 없음' }, () => {
    assert.deepEqual(replayCreation(creationBlocks), [2, 482]);
});

// 원본 3300단계: 모든 지원 파생 생성자와 TYPE 연결을 서버/클라이언트·약화 HP 양쪽에서.
test('SquidFactory: 원본10.78 파생 생성 3300단계와 같다', { skip: !derivedBlocks && '원본 fixture 없음' }, () => {
    assert.deepEqual(replayCreation(derivedBlocks), [4, 3300]);
});

// 원본 1809단계: 일반 non-void Unpop·Take·반납·공통 firstPop 의 반환값·풀 요약·삭제 기록/네 단계 해시/spot Adler 와 대상 슬롯 전체(cpppj UnpopTests).
test('SquidFactory/SquidUnpop: 원본10.78 Unpop/Take/반납/firstPop 1809단계와 같다', { skip: !unpopBlocks && '원본 fixture 없음' }, () => {
    let pool = null;
    let hash = null;
    let spots = null;
    let unpop = null;
    let factory = null;
    let types = null;
    let begins = 0;
    let steps = 0;
    // 각 행을 원문 순서로 적용한다(Type/Fill/Head/Space/Next 는 실제 Pop 을 대신하는 합성 입력이다).
    for (const row of unpopBlocks) {
        if (row[0] === 'Begin') {
            assert.equal(row.length, 5);
            pool = new SidPool(Number(row[2]), { server: row[3] === '1' });
            hash = new SquidHash();
            spots = new Uint8Array(65536);
            types = constructorTypes(true);
            unpop = new SquidUnpop(pool, hash, spots);
            factory = new SquidFactory(pool, types, { unpop });
            begins += 1;
        } else if (row[0] === 'Type') {
            assert.equal(row.length, 9);
            Object.assign(types[Number(row[1])], {
                flags1: Number(row[2]) >>> 0, flags2: Number(row[3]) >>> 0, maxHitPoints: Number(row[4]) | 0, zOrder: Number(row[5]) | 0,
                constructorAddress: Number(row[6]) >>> 0, footX: Number(row[7]), footY: Number(row[8]),
            });
            factory = new SquidFactory(pool, types, { unpop });
        } else if (row[0] === 'Space') {
            hash.reset();
            spots.fill(Number(row[1]));
        } else if (row[0] === 'Head') {
            assert.equal(row.length, 5);
            hash.setBucket(Number(row[1]), bitsToFloat(Number(row[2])), bitsToFloat(Number(row[3])), Number(row[4]));
        } else if (row[0] === 'Fill') {
            assert.equal(row.length, 11);
            const bytes = pool.allocated(Number(row[1]));
            const seed = Number(row[2]);
            // 오염 payload 위에 지정 필드만 덮는다.
            for (let index = 0; index < bytes.length; index += 1) bytes[index] = (seed + index * 37) & 0xff;
            bytes[11] = Number(row[3]);
            bytes[10] = Number(row[4]);
            put(bytes, 0, Number(row[5]) >>> 0);
            put(bytes, 4, Number(row[6]), 2);
            put(bytes, 14, Number(row[7]) >>> 0);
            put(bytes, 18, Number(row[8]) >>> 0);
            bytes[40] = Number(row[9]);
            bytes[33] = Number(row[10]);
            put(bytes, 36, 0, 4);
        } else if (row[0] === 'Next') {
            assert.equal(row.length, 3);
            put(pool.allocated(Number(row[1])), 4, Number(row[2]), 2);
        } else {
            assert.equal(row.length, 17);
            const type = Number(row[2]);
            const arg = Number(row[3]) >>> 0;
            const sid = Number(row[4]);
            let result = sid;
            if (row[1] === 'Create') result = factory.create(type, arg);
            else if (row[1] === 'Take') result = factory.take(type, arg & 0xffff);
            else if (row[1] === 'Unpop') unpop.unpop(sid, types[type], arg);
            else if (row[1] === 'FirstPop') result = factory.firstPopFlags(sid);
            else if (row[1] === 'Release') pool.release(sid);
            else assert.fail(`Unpop fixture 명령 오류 ${row[1]}`);
            const label = `${begins}:${steps} ${row[1]} ${row[2]} ${row[3]} ${row[4]}`;
            const actual = [result, ...poolSummary(pool), adler32(deletionBytes(pool)), adler32(hashBytes(hash)), adler32(spots)];
            assert.deepEqual(actual, row.slice(5, 16).map(Number), label);
            assert.equal(hex(pool.slot(sid)), row[16], label);
            steps += 1;
        }
    }
    assert.deepEqual([begins, steps], [9, 1809]);
});

// 교차 확인: base 와 82 recipe 의 최종 VT 집합이 D2 의 지원 자산 VT 83칸과 같고, 모든 자산 TYPE 의 현재 생성자 VT 가 지원된다(cpppj FortObjectSaveFactory_ConstructedVtableCoverage).
test('SquidFactory: 생성자 VT 83칸이 지원 자산 VT 표와 같다', async () => {
    const { SQUID_ASSET_VTABLES } = await import('../www/core/squid-vtables.js');
    const pool = new SidPool(32768);
    const types = constructorTypes();
    const factory = new SquidFactory(pool, types);
    const tables = new Set();
    // 자산 TYPE 마다 현재 생성자의 VT 를 조회한다(null/assert 생성자는 거부).
    for (let kind = 70; kind < types.length; kind += 1) {
        try {
            const table = factory.constructorVtable(types[kind]);
            assert.ok(SquidFactory.supportsConstructedVtable(table));
            tables.add(table);
        } catch (error) {
            assert.ok(error instanceof Error && types[kind].constructorAddress !== 0);
        }
    }
    assert.equal(tables.size, 83);
    assert.deepEqual([...tables].sort((a, b) => a - b), SQUID_ASSET_VTABLES.map((row) => row[0]).sort((a, b) => a - b));
    assert.equal(SquidFactory.supportsConstructedVtable(0), false);
    const before = adler32(pool.bytes());
    assert.throws(() => factory.constructorVtable({ constructorAddress: 0xffffffff }));
    assert.equal(adler32(pool.bytes()), before);
    assert.equal(SQUID_CPP, '\\Ns\\O\\Squid.cpp');
});
