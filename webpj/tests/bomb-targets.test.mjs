// 주문 대상 검색 raw-bomb-targets.js(RawBombTargets)를 실제 네 단계 해시를 포함한 독립 원본 관측 bomb-targets-x86.tsv 300행으로 대조한다.
// 입력 10개 DWORD: [장면(0 기본·1 발자국·2 상태/extra·3 extra·4 경계 좌표), 권한, 반경 float 비트, 부모 x 비트, 부모 y 비트, 출력 한도, 출력 개수, 자격 모드(0 거부·1 모두·2 짝수), 변형 1~8, 등록 순서 반전].
// 변형 1~8 은 첫 자격 질의에서 부모 가상 표 교체·후보 extra·후보 좌표·후보 체인 머리·부모 좌표·출력 한도/개수·후보 발자국을 바꾸는 명시 입력이다.
// 기하(중심·정수 반경·float 사각형 비트), 자격 호출 순서, 100칸 DWORD 배열 전체, 출력 헤더, 13개 raw 슬롯 전체, 현재 발자국을 원본(두 x87 정밀도)과 비교한다. 기대값은 계산하지 않는다.
// (2026-11-08 WB04-E54: RawBombTargets 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawBombTargets } from '../www/core/raw-bomb-targets.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SQUID_HASH_SCALES, SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E54)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 원본 실행기의 실제/교체 가상 표 토큰, 예약·클라이언트·서버 대역의 후보 SID, 독립 원문이 준 후보 좌표(선택 결과가 아님), 경계 장면의 네 좌표, 상태/extra 패턴이다. (2026-11-08 WB04-E54)
const BOMB_TABLE = 0x502370;
const ALTERNATE_TABLE = 0x20000800;
const TARGET_IDS = [1, 4, 5, 14999, 15000, 15001, 15002, 15003, 15004, 15005, 15006, 15007];
const POSITIONS = [[50.75, 52.5], [51, 53], [51.5, 53], [52, 53], [54.5, 53], [54.500003814697266, 53], [53.5, 55], [47.5, 53], [50.75, 48], [57, 53], [52, 55], [51, 52]];
const EDGE_POSITIONS = [[1.5, 1.5], [255, 255], [1, 255], [255, 1]];
const STATES = [0, 1, 2, 4, 8, 0, 0, 0, 0, 0, 0, 0];
const EXTRAS = [0, 0, 0, 0, 0, 1, 8, 16, 32, 0, 1, 0];

// 입력 십진수를 원본 하위 32비트 수로 읽는다. (2026-11-08 WB04-E54)
const unsigned = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 입력 장면의 실제 풀/해시/TYPE 와 명시 파생 효과만 준비한다. 검색/거리/출력 개수의 기대값은 계산하지 않는다. (2026-11-08 WB04-E54)
class TargetScene {
    pool = new SidPool(24000, { server: false });
    hash = new SquidHash();
    types = [];
    ranges = [];
    state = { boss: 0 };
    storage = new Uint32Array(100);
    output = { storage: this.storage, capacity: 0, count: 0 };
    active = null;
    input = [];
    trace = '';
    queries = 0;

    // 슬롯의 쓰기 가능한 50바이트이다.
    raw(sid) {
        return this.pool.physical(sid).subarray(0, 50);
    }

    // 관측된 큰 외부 경계의 DWORD 인자만 기록한다.
    event(name, value) {
        if (this.trace) this.trace += ';';
        this.trace += `${name}:${value}`;
    }

    // 같은 버킷의 머리 삽입 입력만 작성한다. Pop/탐색 결과를 계산하지 않는다.
    object(index) {
        const scene = this.input[0];
        const sid = TARGET_IDS[index];
        const raw = this.raw(sid);
        raw.fill(0);
        put(raw, 0, BOMB_TABLE, 4);
        raw[10] = 70 + index;
        raw[11] = scene === 2 ? STATES[index] : 0;
        raw[40] = scene === 2 || scene === 3 ? EXTRAS[index] : 0;
        const position = scene === 4 && index < 4 ? EDGE_POSITIONS[index] : POSITIONS[index];
        put(raw, 14, floatToBits(position[0]), 4);
        put(raw, 18, floatToBits(position[1]), 4);
        this.types[70 + index].footX = scene === 1 ? (index % 4) + 1 : 1;
        this.types[70 + index].footY = scene === 1 ? (index % 3) + 1 : 1;
        const level = index % 4;
        const scale = SQUID_HASH_SCALES[level];
        const cellX = Math.floor(Math.trunc(position[0]) / scale);
        const cellY = Math.floor(Math.trunc(position[1]) / scale);
        put(raw, 4, this.hash.cell(level, cellX, cellY), 2);
        this.hash.setCell(level, cellX, cellY, sid);
    }

    // 입력만 설치한다. 원문의 10개 DWORD 입력을 그대로 읽는다.
    prepare(text) {
        this.trace = '';
        this.queries = 0;
        this.hash.reset();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.ranges = new Array(188).fill(0);
        this.input = text.split(',').map(unsigned);
        assert.equal(this.input.length, 10);
        this.state.boss = this.input[1];
        this.output.capacity = this.input[5] | 0;
        this.output.count = this.input[6] | 0;
        // 실제 등록 순서만 바꿔 같은 버킷의 후보 순서를 재현한다.
        for (let index = 0; index < TARGET_IDS.length; index += 1) this.object(this.input[9] ? TARGET_IDS.length - 1 - index : index);
        const raw = this.raw(50);
        raw.fill(0);
        put(raw, 0, BOMB_TABLE, 4);
        raw[10] = 168;
        put(raw, 14, this.input[3], 4);
        put(raw, 18, this.input[4], 4);
        this.ranges[168] = bitsToFloat(this.input[2]);
        // 배열의 미사용 범위도 동일 입력 표식으로 비교한다.
        for (let index = 0; index < this.storage.length; index += 1) this.storage[index] = (0x11110000 + index) >>> 0;
    }

    // 독립 원문에 선언한 파생 자격과 재진입 변경만 공급한다.
    hooks() {
        const input = this.input;
        return {
            // 실제 41e3c0 helper 는 제품이 실행하며 가상 분배 토큰만 확인한다.
            radius: (parent, table) => {
                assert.ok(table === BOMB_TABLE || table === ALTERNATE_TABLE);
                if (!this.active) throw new Error('검사 대상 몸체 미설치');
                return this.active.defaultRadius(parent);
            },
            // 현재 가상 표의 파생 자격 경계이며 기하 판단을 대체하지 않는다.
            eligible: (parent, table, candidate) => {
                assert.equal(parent, 50);
                assert.ok(table === BOMB_TABLE || table === ALTERNATE_TABLE);
                this.event(table === BOMB_TABLE ? 'eligible' : 'eligible-alt', candidate);
                this.queries += 1;
                if (this.queries === 1) {
                    if (input[8] === 1) put(this.raw(parent), 0, ALTERNATE_TABLE, 4);
                    if (input[8] === 2) this.raw(15003)[40] = 1;
                    if (input[8] === 3) put(this.raw(15003), 14, floatToBits(200.0), 4);
                    if (input[8] === 4) put(this.raw(candidate), 4, 0, 2);
                    if (input[8] === 5) {
                        put(this.raw(parent), 14, floatToBits(200.0), 4);
                        put(this.raw(parent), 18, floatToBits(200.0), 4);
                    }
                    if (input[8] === 6) this.output.capacity = 0;
                    if (input[8] === 7) this.output.count = this.output.capacity;
                    if (input[8] === 8) this.types[77].footX = 8;
                }
                return input[7] === 1 || (input[7] === 2 && candidate % 2 === 0);
            },
            // 원본의 복귀 가능한 진단만 기록한다.
            assertion: (line) => this.event('assert', line),
        };
    }

    // 실제 13슬롯 전체를 원문과 같은 순서로 읽는다(예약 번호도 제외하지 않는다).
    raws() {
        let result = hex(this.raw(50));
        for (const sid of TARGET_IDS) result += `,${hex(this.raw(sid))}`;
        return result;
    }

    // 실제 DWORD 배열의 전체 400바이트를 원문 형식으로 직렬화한다.
    array() {
        const bytes = new Uint8Array(400);
        for (let index = 0; index < this.storage.length; index += 1) put(bytes, 4 * index, this.storage[index], 4);
        return hex(bytes);
    }
}

// 원본 관측 순서로 중심 2/정수 반경/float 사각형 4 를 기록한다. (2026-11-08 WB04-E54)
function geometryText(geometry) {
    let result = `${floatToBits(geometry.center[0])},${floatToBits(geometry.center[1])},${geometry.radius >>> 0}`;
    // 원본 41def0 에 전달된 네 float 비트를 비교한다.
    for (const value of geometry.area) result += `,${floatToBits(value)}`;
    return result;
}

test('bomb-targets: 네 단계 해시 순회·기하·자격 호출·출력 헤더·재진입을 원본 300행과 대조한다', () => {
    const scene = new TargetScene();
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-targets-x86.tsv'));
    let count = 0;
    // 각 독립 입력을 새 몸체로 실행하며 기대값은 동결 원문만 사용한다.
    for (const row of rows) {
        assert.equal(row.length, 15);
        scene.prepare(row[0]);
        const targets = new RawBombTargets(scene.pool, scene.hash, scene.types, scene.ranges, scene.state, scene.hooks());
        scene.active = targets;
        const geometry = targets.find(50, scene.output);
        const actual = [geometryText(geometry), scene.trace || '-', scene.array(), String(scene.output.capacity >>> 0), String(scene.output.count >>> 0), scene.raws(), String(scene.types[77].footX)];
        // 두 정밀도의 원문과 기하/효과/전체 배열/헤더/전체 raw/현재 발자국을 각각 대조한다.
        for (let field = 0; field < actual.length; field += 1) {
            assert.equal(actual[field], row[1 + field], `Bomb 대상 입력 ${row[0]} 필드 ${field}`);
            assert.equal(row[1 + field], row[8 + field]);
        }
        count += 1;
    }
    assert.equal(count, 300);
});

test('bomb-targets: 미도달 자격은 요구하지 않고 실제 선택 경계와 물리 버퍼 범위를 확인한다', () => {
    const scene = new TargetScene();
    scene.prepare('0,1,1084227584,1112211456,1112670208,100,0,1,0,0');
    const missing = new RawBombTargets(scene.pool, scene.hash, scene.types, scene.ranges, scene.state, {});
    assert.equal(missing.pool, scene.pool);
    assert.throws(() => missing.find(50, scene.output), TypeError);
    const hooks = { radius: () => 5 };
    // 실제 자격 경계 누락을 성공으로 처리하지 않는다.
    assert.throws(() => new RawBombTargets(scene.pool, scene.hash, scene.types, scene.ranges, scene.state, hooks).find(50, scene.output), TypeError);
    scene.hash.reset();
    // 후보가 없는 검색에서는 자격을 호출하지 않는다.
    assert.equal(new RawBombTargets(scene.pool, scene.hash, scene.types, scene.ranges, scene.state, hooks).find(50, scene.output).radius, 5);
    scene.prepare('0,1,1084227584,1112211456,1112670208,100,0,1,0,0');
    // 물리 배열보다 큰 헤더 용량도 실제 버퍼 밖에 쓰지는 않는다.
    const shortBuffer = { storage: new Uint32Array(0), capacity: 100, count: 0 };
    assert.throws(() => new RawBombTargets(scene.pool, scene.hash, scene.types, scene.ranges, scene.state, { ...hooks, eligible: () => true }).find(50, shortBuffer), RangeError);
    // 권한이 없으면 복귀 가능한 진단 0x130 을 먼저 부른다(없으면 거부).
    scene.state.boss = 0;
    assert.throws(() => new RawBombTargets(scene.pool, scene.hash, scene.types, scene.ranges, scene.state, hooks).find(50, scene.output), TypeError);
    // 구성 오류.
    assert.throws(() => new RawBombTargets({ edition: 'original1037' }, scene.hash, scene.types, scene.ranges, scene.state, hooks), TypeError);
    assert.throws(() => new RawBombTargets(scene.pool, scene.hash, scene.types, scene.ranges.slice(1), scene.state, hooks), TypeError);
});
