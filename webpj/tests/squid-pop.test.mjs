// 공통 Pop(SquidPop·runSquidPopParent·runSquidActivate)을 원본 10.78 관측과 cpppj 계약 검사로 대조한다(WB04-E1).
//  - 원본 관측(cpppj/tests/fixtures, 10.78 블록만): pop-x86(Create→Pop/재등록→Unpop→반납 4블록 2456단계의 슬롯 전체와 풀/머리/spot 체크섬), pop-vtables(10.78 83행 가상 표 지원 여부),
//    pop-world-bounds-x86(Point.isValid 전체와 Pop 입구의 float 저장/NaN 단락 260행). 기대값은 fixture 에서 읽는다.
//  - 계약(cpppj SquidPopParentTests 의 World/Client 접합을 뺀 장면): 앞단 반환, 좌표 오류 로그, 미니맵 뒤 현재 TYPE, 시작점 진단 0x6b7, grid 교체 진단 0x6e8, 옆면 장치 부재의 부분 변경,
//    SHP 실패, 겹침, 자기 머리 진단 0x123, 표시 뒤 firstPop, 마지막 contained 진단 0x71a, 전송 직후 진단 0x35f.
// 실제 World(RawSurfaceWorld)·Client 접합 두 장면은 WB05 에서 옮긴다. CD 10.72 블록은 범위 밖이다. (2026-11-08 WB04-E1: 공통 Pop 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { isCurrentPopPointValid, runSquidPopParent } from '../www/core/raw-squid-pop-parent.js';
import { runSquidActivate } from '../www/core/raw-squid-activate.js';
import { WORLD_CELLS } from '../www/assets/terrain-mask.js';

// 원본 관측 파일 경로이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E1)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const popPath = path.join(FIXTURES, 'pop-x86.tsv');
const vtableRows = readRows(path.join(FIXTURES, 'pop-vtables.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const boundsRows = readRows(path.join(FIXTURES, 'pop-world-bounds-x86.tsv'));

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-08 WB04-E1)
function failed(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 전체 네 해시 배열을 원본 ushort 바이트 순서로 직렬화한다. (2026-11-08 WB04-E1)
function heads(hash) {
    const bytes = [];
    // 0단계부터 모든 머리를 포함한다.
    for (let level = 0; level < 4; level += 1) for (const head of hash.entries(level)) bytes.push(head & 0xff, head >>> 8);
    return Uint8Array.from(bytes);
}

// 합성 타입 입력이다(원본 파일 파서나 파생 생성자 전체를 대신한다고 해석하지 않는다). base fallback 생성에 유효한 최소 발자국만 지정한다. (2026-11-08 WB04-E1)
const syntheticTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 1, footY: 1, maxHitPoints: 91, zOrder: 7 }));

// pop-x86.tsv 의 10.78(originals) 블록을 재생한다: Begin(풀 크기)·Type(레코드/프레임 치수)·Claim(Take)·Fill(오염 payload)·Seed(spot)·Clear·Step(Create/Release/Unpop/Pop).
test('SquidPop: 원본10.78 Create→Pop/재등록→Unpop→반납 raw 재생', { skip: !fs.existsSync(popPath) && '원본 fixture 없음' }, () => {
    let pool = null;
    let hash = null;
    let factory = null;
    let pop = null;
    let unpop = null;
    let types = null;
    let spots = null;
    let fw = 1;
    let fh = 1;
    let active = false;
    let begins = 0;
    let steps = 0;
    const lines = fs.readFileSync(popPath, 'utf8').split(/\r?\n/);
    lines.forEach((line, lineIndex) => {
        if (line === '' || line[0] === '#') return;
        const row = line.split('\t');
        if (row[0] === 'Begin') {
            // CD 블록은 건너뛰고 다음 Begin 을 기다린다.
            active = row[1] === 'originals';
            if (!active) return;
            assert.equal(row.length, 4);
            pool = new SidPool(Number(row[2]), { server: false });
            hash = new SquidHash();
            spots = new Uint8Array(WORLD_CELLS * WORLD_CELLS);
            types = syntheticTypes();
            factory = new SquidFactory(pool, types);
            pop = new SquidPop(pool, hash, spots);
            unpop = new SquidUnpop(pool, hash, spots);
            begins += 1;
            return;
        }
        if (!active) return;
        const where = `줄 ${lineIndex + 1} ${row[0]}`;
        if (row[0] === 'Type') {
            assert.equal(row.length, 8);
            const type = types[Number(row[1])];
            type.flags1 = Number(row[2]);
            type.flags2 = Number(row[3]);
            type.footX = Number(row[4]) | 0;
            type.footY = Number(row[5]) | 0;
            fw = bitsToFloat(row[6]);
            fh = bitsToFloat(row[7]);
            factory = new SquidFactory(pool, types);
        } else if (row[0] === 'Claim') {
            // Take 로 free 슬롯을 확보한 뒤 Fill 이 모든 payload 를 덮는다. 목록/카운터는 보존한다.
            factory.take(74, Number(row[1]));
        } else if (row[0] === 'Fill') {
            assert.equal(row.length, 7);
            const bytes = pool.allocated(Number(row[1]));
            const seed = Number(row[2]);
            // x86 입력과 같은 오염 바이트를 슬롯 전체에 놓는다.
            for (let index = 0; index < bytes.length; index += 1) bytes[index] = (seed + index * 37) & 0xff;
            bytes[10] = Number(row[4]);
            bytes[11] = Number(row[3]);
            put(bytes, 0, Number(row[5]));
            put(bytes, 4, 0, 2);
            put(bytes, 8, 23, 2);
            put(bytes, 14, floatToBits(20));
            put(bytes, 18, floatToBits(20));
            bytes[40] = Number(row[6]);
            bytes[33] = 3;
            put(bytes, 36, 0, 4);
        } else if (row[0] === 'Seed') {
            spots[Number(row[2]) * WORLD_CELLS + Number(row[1])] = Number(row[3]);
        } else if (row[0] === 'Clear') {
            hash.reset();
            spots.fill(0);
        } else if (row[0] === 'Step') {
            assert.equal(row.length, 11);
            const sid = Number(row[2]);
            const arg = Number(row[3]);
            let result = 0;
            if (row[1] === 'Create') result = factory.create(arg, 2);
            else if (row[1] === 'Release') pool.release(sid);
            else if (row[1] === 'Unpop') unpop.unpop(sid, types[pool.slot(sid)[10]], arg);
            else if (row[1] === 'Pop') result = pop.pop(sid, types[pool.slot(sid)[10]], fw, fh, bitsToFloat(row[4]), bitsToFloat(row[5]), arg);
            else assert.fail(`${where}: 모르는 단계`);
            assert.equal(result, Number(row[6]), `${where} ${row[1]} SID ${row[2]} 반환`);
            assert.equal(adler32(pool.bytes()), Number(row[7]), `${where} 풀`);
            assert.equal(adler32(heads(hash)), Number(row[8]), `${where} 머리`);
            assert.equal(adler32(spots), Number(row[9]), `${where} spot`);
            const bytes = pool.slot(sid);
            assert.equal(Buffer.from(bytes).toString('hex'), row[10], `${where} 슬롯 ${sid}`);
            steps += 1;
        } else assert.fail(`${where}: 모르는 행`);
    });
    // 파일은 판본별 4블록씩 8블록(단계 4600)이고 10.78 블록은 4개, 2456단계이다.
    assert.equal(begins, 4);
    assert.equal(steps, 2456);
});

// 가상 표 지원 여부: PE 주소 표의 firstPop/postPop 과 두 표시 경로로 제한한다(마나·볼트·134 는 별도 확인 표시라 지원).
test('SquidPop.supports: 원본10.78 가상 표 지원 여부 83행', { skip: !vtableRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    for (const fields of vtableRows) {
        assert.equal(fields.length, 5);
        const vtable = Number(fields[1]);
        const supported = fields[4] === '1';
        // 마나 103/볼트 105 는 별도 표시 관측, 134 는 실제 7c/80/84 표와 Session 검사로 검증했다. 기존 metadata 는 불변이다.
        const mana = vtable === 0x50c3b0 || vtable === 0x509810 || vtable === 0x5070c0;
        assert.equal(SquidPop.supports(vtable, 0), supported || mana, `vtable ${vtable.toString(16)}`);
        assert.equal(SquidPop.supports(vtable, 0x2000), supported || mana, `vtable ${vtable.toString(16)} 0x2000`);
        rows += 1;
    }
    assert.equal(rows, vtableRows.length);
    assert.equal(rows, 83);
});

// 도달하지 않는 옆면 장치는 요구하지 않으며 invalid start 진단 앞의 좌표 쓰기는 유지한다.
test('SquidPop: 선택된 가드는 원문의 부분 쓰기를 보존한다', () => {
    const pool = new SidPool(32768, { server: false });
    const types = syntheticTypes();
    const factory = new SquidFactory(pool, types);
    const hash = new SquidHash();
    const spots = new Uint8Array(WORLD_CELLS * WORLD_CELLS);
    const pop = new SquidPop(pool, hash, spots);
    const sid = factory.create(74, 2);
    types[74].flags2 = 0x4000;
    assert.equal(pop.pop(sid, types[74], 1, 1, 30, 40), RAW_POP_RESULT.registered);
    assert.equal(hash.bucket(1, 30, 40), sid);
    const raw = pool.allocated(sid);
    raw[11] = 4;
    hash.reset();
    types[74].flags2 = 0x11;
    types[74].footX = 40;
    // 다음 명시 입력은 별도 void/빈 해시에서 시작한다.
    assert.ok(failed(() => pop.pop(sid, types[74], 1, 1, 35, 44)));
    assert.equal(bitsToFloat(get(raw, 14)), 35);
    assert.deepEqual([raw[22], raw[23], raw[24], raw[25], raw[11]], [0x30, 2, 0xe4, 1, 4]);
    assert.ok(spots.every((value) => value === 0));
    assert.equal(hash.bucket(1, 35, 44), 0);
});

// Point.isValid 전체와 Pop 입구의 현재 signed 크기/float 저장/NaN 단락: 원문이 같은 260입력의 비트와 메모리(getter) 읽기 횟수.
test('isCurrentPopPointValid: 원본 Pop 월드 경계 260행(비트와 getter 읽기 횟수)', { skip: !boundsRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of boundsRows) {
        assert.equal(row.length, 8);
        const limit = Number(BigInt.asIntN(32, BigInt(row[0])));
        const x = bitsToFloat(row[1]);
        const y = bitsToFloat(row[2]);
        let reads = 0;
        const current = () => { reads += 1; return limit; };
        const valid = isCurrentPopPointValid(x, y, current);
        assert.deepEqual([valid, reads], [Number(row[3]) !== 0, Number(row[4])], `행 ${count}`);
        reads = 0;
        let mappedX = x;
        let mappedY = y;
        if (!isCurrentPopPointValid(mappedX, mappedY, current)) mappedX = mappedY = 10;
        assert.deepEqual([floatToBits(mappedX), floatToBits(mappedY), reads], [Number(row[5]), Number(row[6]), Number(row[7])], `행 ${count} 부모 입구`);
        count += 1;
    }
    assert.equal(count, 260);
});

// 선택 함수 소유권과 x 단락: 자기 연결을 해제한 getter 도 한 번 조회를 끝낸다. NaN/음수 x 는 getter 를 읽지 않고, y 검사 전에 읽는 현재 크기의 실패는 숨기지 않는다.
test('isCurrentPopPointValid: 선택한 getter 는 자기 연결 해제에도 살아 있고 x 가 틀리면 읽지 않는다', () => {
    let getter = null;
    let calls = 0;
    getter = () => {
        getter = null;
        calls += 1;
        return 32;
    };
    const selected = getter;
    assert.equal(isCurrentPopPointValid(31, 20, selected), true);
    assert.deepEqual([calls, getter], [1, null]);
    calls = 0;
    const throwing = () => { calls += 1; throw new Error('선택 세계 크기 조회 중단'); };
    assert.equal(isCurrentPopPointValid(Number.NaN, 10, throwing), false);
    assert.equal(isCurrentPopPointValid(-1, 10, throwing), false);
    assert.equal(calls, 0);
    assert.ok(failed(() => isCurrentPopPointValid(1, Number.NaN, throwing)));
    assert.equal(calls, 1);
    // 기존 독립 미연결 256 입력만 유지한다.
    assert.deepEqual([isCurrentPopPointValid(255, 255), isCurrentPopPointValid(256, 10)], [true, false]);
});

// 현재 슬롯과 진단이 바꿀 원문 저장소를 한 수명 안에 유지하는 장면이다(cpppj PopParentScene). 명시 입력과 장치 관측만 공급하고 기대 분기/반환을 제품 대신 계산하지 않는다.
class PopParentScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = syntheticTypes();
        this.source = 50;
        this.checking = 1;
        this.metamap = true;
        this.size = [1, 1];
        this.firstFlags = 1;
        this.frameReads = 0;
        this.updates = 0;
        this.firsts = 0;
        this.activations = 0;
        this.activateFlags = 0;
        this.miniGenus = 0;
        this.lines = [];
        this.files = [];
        this.logs = [];
        this.events = [];
        // 각 외부 경계에서 요청할 명시 변경/실패이다.
        this.onAssertion = null;
        this.onMiniMap = null;
        this.onFrame = null;
        this.onUpdate = null;
        this.onActivate = null;
        this.onLog = null;
        // 필요한 합성 슬롯만 실제 풀에서 확보한다.
        for (let index = 5; index <= 60; index += 1) assert.equal(this.pool.allocate(2), index);
        this.types[74].name = 'base';
        this.types[75].name = 'NOW';
        this.types[76].name = 'old';
        this.types[77].name = 'THEN';
        this.types[74].flags2 = 1;
        const raw = this.raw(this.source);
        raw.fill(0);
        put(raw, 0, 0x501dd0);
        raw[10] = 74;
        raw[11] = 4;
        this.hooks = {
            type: (number) => this.types[number],
            metamap: () => this.metamap,
            frameSize: (sid) => {
                assert.equal(sid, this.source);
                this.frameReads += 1;
                this.events.push('frame');
                this.onFrame?.();
                return this.size;
            },
            trace: (level, text) => {
                assert.equal(level, 29);
                this.logs.push(text);
                this.events.push('log');
                this.onLog?.();
            },
            miniMap: (sid, genus) => {
                assert.equal(sid, this.source);
                this.miniGenus = genus;
                this.events.push('mini');
                this.onMiniMap?.();
            },
            update: (sid) => {
                assert.equal(sid, this.source);
                this.updates += 1;
                this.events.push('update');
                this.onUpdate?.();
            },
            // 가상 경계의 명시 반환이며 firstPop 계산 자체의 원문 관측은 아니다.
            firstPop: (sid) => {
                assert.equal(sid, this.source);
                this.firsts += 1;
                this.events.push('first');
                return this.firstFlags;
            },
            activate: (sid, flags) => {
                assert.equal(sid, this.source);
                this.activations += 1;
                this.activateFlags = flags;
                this.events.push('activate');
                this.raw(this.source)[11] &= 0xfb;
                this.onActivate?.();
            },
            assertion: (expression, file, line) => {
                this.lines.push(line);
                this.files.push(file);
                this.events.push('assert');
                this.onAssertion?.(line);
            },
        };
    }

    // 현재 free/dead 도 물리 슬롯에만 기록하며 게임 규칙의 성공 결과를 대신 쓰지 않는다.
    raw(sid) { return this.pool.physical(sid); }

    // 이웃 필드를 보존하는 명시 WORD/DWORD 입력이다.
    put(sid, offset, value, width = 4) { put(this.raw(sid), offset, value, width); }

    // 원문 폭의 현재 정수 필드만 관측한다.
    read(sid, offset, width = 4) { return get(this.raw(sid), offset, width); }

    // 저장된 위치의 float 비트를 추가 반올림 없이 관측한다.
    coordinate(offset) { return bitsToFloat(this.read(this.source, offset)); }

    // 실제 부모가 선택한 모든 공간 변경과 경계를 실행한다.
    run(x = 30, y = 40, flags = 0) {
        return runSquidPopParent(this.pool, this.hash, this.spots, this.source, x, y, flags, () => this.checking, this.hooks);
    }
}

// 범위/void/contained 앞단은 TYPE/SHP/장치가 없어도 반환한다.
test('runSquidPopParent: 앞단 반환은 TYPE 과 장치를 읽지 않는다', () => {
    const scene = new PopParentScene();
    scene.hooks = {};
    scene.raw(scene.source)[10] = 255;
    const before = scene.pool.bytes();
    assert.equal(runSquidPopParent(scene.pool, scene.hash, scene.spots, 0, 30, 40, 0, () => 0, scene.hooks), RAW_POP_RESULT.unchanged);
    assert.equal(runSquidPopParent(scene.pool, scene.hash, scene.spots, 24000, 30, 40, 0, () => 0, scene.hooks), RAW_POP_RESULT.unchanged);
    assert.deepEqual(scene.pool.bytes(), before);
    // 두 앞단 상태(void 아님, contained)를 독립 입력으로 확인한다.
    for (const state of [0, 12]) {
        scene.raw(scene.source)[11] = state;
        const raw = scene.raw(scene.source).slice();
        assert.equal(scene.run(), RAW_POP_RESULT.unchanged);
        assert.deepEqual(scene.raw(scene.source), raw);
    }
});

// 로그가 바꾼 현재 TYPE 을 좌표 저장 뒤에 조회한다.
test('runSquidPopParent: 잘못된 좌표는 저장 전에 로그를 쓰고 현재 TYPE 을 쓴다', () => {
    const scene = new PopParentScene();
    scene.metamap = false;
    scene.types[75].flags2 = 1;
    scene.size = [3, 1];
    scene.onLog = () => {
        assert.deepEqual([scene.coordinate(14), scene.coordinate(18)], [0, 0]);
        scene.raw(scene.source)[10] = 75;
        scene.put(scene.source, 14, floatToBits(99));
    };
    assert.equal(scene.run(-1, 7), RAW_POP_RESULT.registered);
    assert.deepEqual(scene.logs, ['Error base popped to(  -1.0,  7.0). Set to( 10,10)\n']);
    assert.deepEqual([scene.coordinate(14), scene.coordinate(18), scene.spots[10 * 256 + 10], scene.raw(scene.source)[33]], [10, 10, 1, 2]);
    assert.deepEqual([scene.read(scene.source, 22, 2), scene.read(scene.source, 24, 2), scene.hash.bucket(2, 10, 10)], [160, 110, scene.source]);
});

// 초기 TYPE 의 현재 발자국과 저장 입력 좌표를 쓰며 각 칸의 TYPE/해시는 다시 조회한다.
test('runSquidPopParent: 미니맵은 입력 범위를 보존하고 루프마다 현재 TYPE 을 읽는다', () => {
    const scene = new PopParentScene();
    scene.types[74].flags2 = 2;
    scene.types[75].flags2 = 1;
    scene.size = [3, 2];
    scene.onMiniMap = () => {
        assert.deepEqual([scene.coordinate(14), scene.coordinate(18)], [30, 40]);
        scene.types[74].footX = 2;
        scene.raw(scene.source)[10] = 75;
        scene.put(scene.source, 14, floatToBits(100));
        scene.put(scene.source, 18, floatToBits(110));
    };
    scene.onFrame = () => {
        assert.deepEqual([scene.spots[40 * 256 + 29], scene.spots[40 * 256 + 30]], [1, 1]);
        assert.equal(scene.raw(scene.source)[10], 75);
    };
    assert.equal(scene.run(), RAW_POP_RESULT.registered);
    assert.equal(scene.miniGenus, 2);
    assert.deepEqual([scene.hash.bucket(2, 100, 110), scene.hash.bucket(2, 30, 40), scene.spots[110 * 256 + 100]], [scene.source, 0, 0]);
    assert.deepEqual(scene.events, ['mini', 'frame', 'update', 'first', 'activate']);
});

// 원문 6b7 은 checking 과 무관하며 물리적으로 유효한 0번 칸은 계속 처리한다.
test('runSquidPopParent: 잘못된 시작점은 checking 이 꺼져도 진단하고 0번 칸을 이어간다', () => {
    const scene = new PopParentScene();
    scene.checking = 0;
    assert.equal(scene.run(0.25, 20.5), RAW_POP_RESULT.registered);
    assert.deepEqual(scene.lines, [0x6b7]);
    assert.equal(scene.files[0], '\\Ns\\O\\Squid.cpp');
    assert.equal(scene.spots[20 * 256], 1);
    assert.deepEqual([scene.coordinate(14), scene.coordinate(18), scene.frameReads], [0.25, 20.5, 1]);
});

// 진단 뒤 현재 TYPE/이름과 보관한 후보를 사용하고 다음 칸은 현재 genus 를 다시 읽는다.
test('runSquidPopParent: grid 교체 진단은 후보를 보관하고 현재 이름을 로그한다', () => {
    const scene = new PopParentScene();
    scene.types[74].flags2 = 0;
    scene.types[74].flags1 = 0x800;
    scene.types[74].footX = 2;
    scene.types[75].flags2 = 1;
    scene.raw(9)[10] = 76;
    scene.hash.setCell(0, 29, 40, 9);
    scene.onAssertion = (line) => {
        assert.equal(line, 0x6e8);
        scene.checking = 0;
        scene.raw(scene.source)[10] = 75;
        scene.raw(9)[10] = 77;
        scene.hash.setCell(0, 29, 40, 0);
        scene.put(scene.source, 14, floatToBits(60));
        scene.put(scene.source, 18, floatToBits(80));
    };
    assert.equal(scene.run(), RAW_POP_RESULT.registered);
    assert.deepEqual(scene.lines, [0x6e8]);
    assert.deepEqual(scene.logs, ['ERROR: gridSid 50( NOW) replacing 9( THEN)\n']);
    assert.deepEqual([scene.spots[40 * 256 + 29], scene.spots[40 * 256 + 30], scene.hash.bucket(1, 60, 80)], [0, 1, 50]);
});

// 도달한 옆면 장치 실패 앞의 좌표와 y/x 순서 OR 는 남긴다.
test('runSquidPopParent: 옆면 장치가 없으면 앞선 spot 을 남기고 오류이다', () => {
    const scene = new PopParentScene();
    scene.types[74].flags2 = 0x4001;
    scene.types[74].footX = scene.types[74].footY = 3;
    scene.raw(9)[10] = 75;
    scene.types[75].flags1 = 0x800;
    scene.hash.setCell(0, 28, 39, 9);
    assert.ok(failed(() => scene.run()));
    assert.deepEqual([scene.coordinate(14), scene.coordinate(18), scene.raw(scene.source)[11], scene.frameReads], [30, 40, 4, 0]);
    assert.deepEqual([scene.spots[38 * 256 + 28], scene.spots[38 * 256 + 29], scene.spots[38 * 256 + 30], scene.spots[39 * 256 + 28]], [1, 1, 1, 1]);
    assert.equal(scene.spots.filter((value) => value !== 0).length, 4);
    assert.equal(scene.hash.bucket(1, 30, 40), 0);
});

// SHP 조회가 실패해도 그 앞에서 실제로 실행한 부분 공간 변경은 보존한다.
test('runSquidPopParent: 프레임 조회 실패는 spot 뒤 해시 앞에서 일어난다', () => {
    const scene = new PopParentScene();
    scene.types[74].footX = 2;
    scene.onFrame = () => { throw new Error('명시 SHP 실패'); };
    assert.ok(failed(() => scene.run()));
    assert.deepEqual([scene.frameReads, scene.spots[40 * 256 + 29], scene.spots[40 * 256 + 30], scene.coordinate(14)], [1, 1, 1, 30]);
    assert.deepEqual([scene.hash.bucket(1, 30, 40), scene.raw(scene.source)[11], scene.raw(scene.source)[40], scene.updates], [0, 4, 0, 0]);
});

// 겹침은 미지원 가상 표/그림을 조회하기 전에 정상 반환한다.
test('SquidPop.popCurrent: spot 겹침은 미지원 가상 표와 SHP 조회 앞에서 반환한다', () => {
    const scene = new PopParentScene();
    scene.put(scene.source, 0, 0x12345678);
    scene.spots[40 * 256 + 30] = 1;
    const pop = new SquidPop(scene.pool, scene.hash, scene.spots);
    pop.bindDiagnostics({ types: scene.types, frameSize: () => { throw new Error('겹침 뒤 SHP 조회 금지'); } });
    assert.equal(pop.popCurrent(scene.source, 30, 40), RAW_POP_RESULT.overlap);
    assert.deepEqual([scene.coordinate(14), scene.coordinate(18), scene.raw(scene.source)[11], scene.hash.bucket(1, 30, 40)], [30, 40, 4, 0]);
});

// 진단이 head/좌표/단계를 바꿔도 선택 버킷과 next 는 보관하며 island 분기는 현재 단계이다.
test('runSquidPopParent: 자기 머리 진단은 선택 버킷과 next 를 보관한다', () => {
    const scene = new PopParentScene();
    scene.hash.setBucket(1, 30, 40, 50);
    scene.put(scene.source, 8, 41, 2);
    scene.types[75].flags2 = 2;
    scene.onAssertion = (line) => {
        assert.deepEqual([line, scene.files.at(-1)], [0x123, 'c:\\ns\\o\\squid.h']);
        scene.hash.setBucket(1, 30, 40, 9);
        scene.raw(scene.source)[10] = 75;
        scene.raw(scene.source)[33] = 0;
        scene.put(scene.source, 14, floatToBits(90));
        scene.put(scene.source, 18, floatToBits(80));
    };
    assert.equal(scene.run(), RAW_POP_RESULT.registered);
    assert.deepEqual(scene.lines, [0x123]);
    assert.deepEqual([scene.read(scene.source, 4, 2), scene.read(scene.source, 8, 2)], [50, 127]);
    assert.deepEqual([scene.hash.bucket(1, 30, 40), scene.hash.bucket(1, 90, 80), scene.raw(scene.source)[33]], [50, 0, 0]);
});

// 표시 뒤 현재 extra 를 읽어 firstPop 생략/실행을 판단한다.
test('runSquidPopParent: 표시 갱신 뒤의 현재 extra 가 firstPop 과 flags 를 정한다', () => {
    for (const skip of [false, true]) {
        const scene = new PopParentScene();
        scene.raw(scene.source)[40] = skip ? 0 : 0x80;
        scene.firstFlags = 2;
        scene.onUpdate = () => {
            assert.deepEqual([scene.hash.bucket(1, 30, 40), scene.raw(scene.source)[11] & 4], [50, 4]);
            scene.raw(scene.source)[40] = skip ? 0x80 : 1;
        };
        assert.equal(scene.run(30, 40, 0x2000), RAW_POP_RESULT.registered);
        assert.deepEqual([scene.firsts, scene.activateFlags, scene.raw(scene.source)[40]], skip ? [0, 0x2000, 0x80] : [1, 0x2002, 0x81], `skip=${skip}`);
    }
});

// Activate 뒤 71a 는 checking 0 에서도 진단하며 정상 복귀/중단 모두 앞선 변경을 유지한다.
test('runSquidPopParent: 마지막 contained 진단은 완료된 효과를 유지한다', () => {
    for (const abort of [false, true]) {
        const scene = new PopParentScene();
        scene.checking = 0;
        scene.onActivate = () => {
            scene.raw(scene.source)[11] |= 8;
            scene.raw(scene.source)[10] = 75;
        };
        scene.onAssertion = (line) => {
            assert.deepEqual([line, scene.activations, scene.raw(scene.source)[40]], [0x71a, 1, 0x80]);
            if (abort) throw new Error('명시 후단 진단 종료');
        };
        if (abort) assert.ok(failed(() => scene.run()));
        else assert.equal(scene.run(), RAW_POP_RESULT.registered);
        assert.deepEqual(scene.lines, [0x71a]);
        assert.deepEqual([scene.hash.bucket(1, 30, 40), scene.raw(scene.source)[11], scene.raw(scene.source)[10]], [50, 8, 75]);
    }
});

// 기존 front 거부 대신 실제 35f 진단이 부분 등록/void 해제 뒤에 실행된다.
test('SquidPop.popCurrent: 전송 직후 비트 진단은 void 해제 뒤에 실행된다', () => {
    const scene = new PopParentScene();
    scene.raw(scene.source)[11] = 0x14;
    const pop = new SquidPop(scene.pool, scene.hash, scene.spots);
    pop.bindDiagnostics({
        types: scene.types,
        frameSize: () => [1, 1],
        assertion: (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['!xmitJustCreatedFlag', '\\Ns\\O\\Squid.cpp', 0x35f]);
            assert.deepEqual([scene.raw(scene.source)[11], scene.hash.bucket(1, 30, 40)], [0x10, 50]);
            scene.lines.push(line);
            scene.raw(scene.source)[11] = 0;
        },
    });
    assert.equal(pop.popCurrent(scene.source, 30, 40), RAW_POP_RESULT.registered);
    assert.deepEqual(scene.lines, [0x35f]);
    assert.deepEqual([scene.raw(scene.source)[11], scene.raw(scene.source)[40]], [0, 0x80]);
});

// 공통 Activate: 서버 권한·client 번호·전송 비트·깊이 진단의 순서를 명시 장치로 확인한다. 이벤트: T=송신(flags:전송중 비트), P=가상 후처리(flags), D=진단(행 번호 16진).
test('runSquidActivate: 서버/client 번호/전송/깊이 진단의 순서', () => {
    const run = ({ sid = 50, state = 4, flags = 0, server = false, decrement = 1 }) => {
        const pool = new SidPool(24000, { server });
        pool.physical(sid)[11] = state;
        const events = [];
        const depth = { value: 0 };
        runSquidActivate(pool, sid, flags, depth, {
            server: () => server,
            assertion: (expression, file, line) => events.push(`D${line.toString(16)}`),
            transmit: (target, sendFlags) => events.push(`T${sendFlags}:${pool.physical(target)[11] & 0x10 ? 'x' : '-'}`),
            postPop: (target, postFlags) => {
                events.push(`P${postFlags}`);
                depth.value = (depth.value - decrement) >>> 0;
            },
        });
        return { events, state: pool.physical(sid)[11], depth: depth.value };
    };
    // 서버 0: 8 비트가 없으면 0x10 을 더하고 송신하지 않는다. void 비트는 지워진다.
    assert.deepEqual(run({ flags: 0 }), { events: ['P16'], state: 0, depth: 0 });
    assert.deepEqual(run({ flags: 8 }).events, ['P8']);
    // 서버 1, client 번호(5..14999): 0x10 을 더해 송신을 생략한다. 서버 Pop 비트(8)가 있으면 진단 뒤 이어간다.
    assert.deepEqual(run({ server: true, flags: 0 }).events, ['P16']);
    assert.deepEqual(run({ server: true, flags: 8 }).events, ['D364', 'P24']);
    // 서버 1, 서버 번호(15000 이상): 0x10 이 없으면 8 을 더해 송신하고, flags&7 이 있으면 송신 중에만 전송중 비트를 켠다.
    assert.deepEqual(run({ sid: 15000, server: true, flags: 0 }).events, ['T8:-', 'P8']);
    assert.deepEqual(run({ sid: 15000, server: true, flags: 1 }).events, ['T9:x', 'P9']);
    assert.deepEqual(run({ sid: 15000, server: true, flags: 0x10 }).events, ['P16']);
    // 깊이가 달라지면 curPostPop 진단이다. void 아님/전송 직후 비트도 진단 뒤 이어진다.
    assert.deepEqual(run({ decrement: 0 }).events, ['P16', 'D392']);
    assert.deepEqual(run({ state: 0 }).events, ['D35c', 'P16']);
    assert.deepEqual(run({ state: 0x14 }).events, ['D35f', 'P16']);
    // 선택된 장치가 없으면 그 위치에서 오류이다.
    assert.throws(() => runSquidActivate(new SidPool(24000, { server: false }), 50, 0, { value: 0 }, { server: () => false, assertion: () => {} }), /postPop/);
});
