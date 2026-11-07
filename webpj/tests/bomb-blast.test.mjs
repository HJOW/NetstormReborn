// 폭발 계열 raw-bomb-blast.js(RawBombBlast)의 자격/효과/캡처 위치/전체 상태를 독립 원본 관측 bomb-blast-x86.tsv 2082행과 bomb-blast-dais-x86.tsv 64행으로 대조한다.
// 입력 23개 DWORD 의 첫 칸이 연산(0 자격 · 1 직접 효과 · 그 밖 자격 뒤 효과)이며, 대상 TYPE genus/flags/프레임 보호 비트/좌표/발자국/동맹/권한, 부모 좌표·운반체, 그리고 효과 중 상태를 바꾸는 변형(1~11)을 포함한다.
// 반환·효과 문자열(피해/삭제/ground/edge, 교체 가상 표 구분)·부모/대상 포함 네 슬롯 전체·권한·현재 발자국을 원본(두 x87 정밀도)과 비교하며 기대값은 계산하지 않는다.
// (2026-11-08 WB04-E54: RawBombBlast 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawBombBlast } from '../www/core/raw-bomb-blast.js';
import { RawBombTargets } from '../www/core/raw-bomb-targets.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E54)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력 십진수를 원본 하위 32비트 수로 읽는다. (2026-11-08 WB04-E54)
const unsigned = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 원본 실행기의 부모/대상/교체 가상 표 토큰과 원본이 전체 바이트를 동결한 물리 슬롯이다. (2026-11-08 WB04-E54)
const BLAST_VTABLE = 0x502370;
const TARGET_VTABLE = 0x501dd0;
const ALTERNATIVE_VTABLE = 0x20000800;
const BLAST_SLOTS = [0, 50, 51, 60];

// 실제 판단과 좌표 계산은 제품이 실행하며 큰 효과의 명시 입력만 공급하는 장면이다. (2026-11-08 WB04-E54)
class BlastScene {
    pool = new SidPool(24000, { server: false });
    hash = new SquidHash();
    types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    frames = Array.from({ length: 188 }, () => []);
    ranges = new Array(188).fill(0);
    targetState = { boss: 1 };
    coordinates = new RawBombTargets(this.pool, this.hash, this.types, this.ranges, this.targetState, {});
    state = { boss: 0, growingRainBlockerType: 150, priestType: 158, alliances: new Uint32Array(81) };
    input = [];
    trace = '';

    // 슬롯의 쓰기 가능한 50바이트이다.
    raw(sid) {
        return this.pool.physical(sid).subarray(0, 50);
    }

    // 원문 형식으로 효과 인자의 전체 DWORD 와 순서를 기록한다.
    event(name, values) {
        if (this.trace) this.trace += ';';
        this.trace += name;
        for (const value of values) this.trace += `:${value >>> 0}`;
    }

    // 같은 명시 원문 입력을 설치하며 판단/피해 분기를 계산하지 않는다(23개 DWORD).
    prepare(text) {
        this.trace = '';
        this.state = { boss: 0, growingRainBlockerType: 150, priestType: 158, alliances: new Uint32Array(81) };
        this.targetState.boss = 1;
        this.input = text.split(',').map(unsigned);
        assert.equal(this.input.length, 23);
        const input = this.input;
        // 제자리 갱신하는 표들이다(좌표 계산기가 같은 배열을 빌리고 있다).
        for (let index = 0; index < this.types.length; index += 1) this.types[index] = { flags1: 0, flags2: 0, footX: 0, footY: 0 };
        this.ranges.fill(0);
        for (let index = 0; index < this.frames.length; index += 1) this.frames[index] = [];
        // 보존된 네 슬롯을 원문과 같이 매 입력 초기화한다.
        for (const sid of BLAST_SLOTS) this.raw(sid).fill(0);
        const parent = this.raw(50);
        put(parent, 0, BLAST_VTABLE, 4);
        parent[10] = 168;
        parent[34] = input[10] & 0xff;
        put(parent, 12, input[8], 2);
        put(parent, 14, input[14], 4);
        put(parent, 18, input[15], 4);
        const target = input[9] & 0xffff;
        const victim = this.raw(target);
        put(victim, 0, TARGET_VTABLE, 4);
        victim[10] = input[1] & 0xff;
        victim[11] = input[21] & 0xff;
        victim[34] = input[11] & 0xff;
        victim[40] = input[4] & 0xff;
        put(victim, 14, input[16], 4);
        put(victim, 18, input[17], 4);
        put(victim, 36, input[5], 4);
        put(victim, 26, input[22], 4);
        const type = this.types[input[1]];
        type.flags1 = input[3];
        type.flags2 = input[2];
        type.footX = input[18] | 0;
        type.footY = input[19] | 0;
        this.ranges[168] = bitsToFloat(input[13]);
        // 현재 프레임의 보호 비트 선택은 제품이 수행하며 네 코드만 입력한다.
        this.frames[input[1]] = Array.from({ length: 4 }, (_, index) => ({ side: 74, variant: 0, number: index + 1, flags: (input[6] ^ (index * 17)) & 0xff }));
        this.state.boss = input[7];
        this.state.alliances[input[10] * 9 + input[11]] = input[12];
    }

    // 독립 실행기의 큰 가상 효과/재진입만 공급한다.
    hooks() {
        const input = this.input;
        const target = () => input[9] & 0xffff;
        return {
            // 일반 경로의 실제 radius 는 기존 제품 helper 로 계산한다.
            radius: (parent, table) => {
                assert.ok(parent === 50 && table === BLAST_VTABLE);
                if (input[20] < 1 || input[20] > 4) return this.coordinates.defaultRadius(parent);
                this.event('radius-override', [50]);
                if (input[20] === 1) {
                    put(this.raw(target()), 14, floatToBits(200.0), 4);
                    put(this.raw(target()), 18, floatToBits(200.0), 4);
                }
                if (input[20] === 2) put(this.raw(target()), 0, ALTERNATIVE_VTABLE, 4);
                if (input[20] === 3) this.state.boss = 0;
                if (input[20] === 4) {
                    this.raw(target())[10] = 71;
                    this.types[70].footX = 8;
                }
                return 3;
            },
            // 현재 가상 표에 해당하는 피해 경계의 실제 인자를 기록한다.
            damage: (victim, table, amount, owner, parent) => {
                assert.ok(table === TARGET_VTABLE || table === ALTERNATIVE_VTABLE);
                this.event(table === TARGET_VTABLE ? 'damage' : 'damage-alt', [victim, amount, owner, parent]);
                if (input[20] === 5) {
                    put(this.raw(victim), 14, floatToBits(200.0), 4);
                    put(this.raw(victim), 18, floatToBits(200.0), 4);
                }
                if (input[20] === 6) this.raw(parent)[34] = 3;
                if (input[20] === 7) put(this.raw(victim), 0, ALTERNATIVE_VTABLE, 4);
                if (input[20] === 8) {
                    this.raw(victim)[11] = 2;
                    put(this.raw(victim), 26, 1, 4);
                }
            },
            // 현재 삭제 표와 두 종류의 정확한 flags 를 관측한다.
            destroy: (victim, table, flags) => {
                assert.ok(table === TARGET_VTABLE || table === ALTERNATIVE_VTABLE);
                this.event(table === TARGET_VTABLE ? 'destroy' : 'destroy-alt', [victim, flags]);
                if (input[20] === 9) {
                    put(this.raw(victim), 14, floatToBits(200.0), 4);
                    put(this.raw(victim), 18, floatToBits(200.0), 4);
                }
                if (input[20] === 10) put(this.raw(victim), 0, ALTERNATIVE_VTABLE, 4);
            },
            // 피해 전 캡처 중심을 받으며 이동 후 좌표로 기대값을 다시 계산하지 않는다.
            ground: (x, y) => {
                this.event('ground', [floatToBits(x), floatToBits(y)]);
                if (input[20] === 11) this.raw(target())[40] = 1;
            },
            // 실제 421f70 의 대상/부모 중심 요청만 관측한다.
            edge: (victim, x, y) => this.event('edge', [victim, floatToBits(x), floatToBits(y)]),
        };
    }

    // 실제 네 슬롯 전체를 원문 순서로 읽는다(예약 0번 슬롯도 제외하지 않는다).
    raws() {
        return BLAST_SLOTS.map((sid) => hex(this.raw(sid))).join(',');
    }
}

test('bomb-blast: 자격·직접 효과·합성 호출의 분기 순서/캡처 중심/현재 가상 표를 원본 2146행과 대조한다', () => {
    const scene = new BlastScene();
    let count = 0;
    // 기존/추가 기대값은 각각 독립 동결 파일에서 읽는다.
    for (const name of ['bomb-blast-x86.tsv', 'bomb-blast-dais-x86.tsv']) {
        const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
        // 자격/직접 효과/합성 호출을 원문 operation 대로 실행한다.
        for (const row of rows) {
            assert.equal(row.length, 11);
            scene.prepare(row[0]);
            const blast = new RawBombBlast(scene.pool, scene.types, scene.frames, scene.coordinates, scene.state, scene.hooks());
            const target = scene.input[9] & 0xffff;
            let result = 0;
            if (scene.input[0] === 0) result = blast.mayTarget(target) ? 1 : 0;
            else if (scene.input[0] === 1) blast.apply(50, target);
            else {
                result = blast.mayTarget(target) ? 1 : 0;
                if (result) blast.apply(50, target);
            }
            const actual = [String(result), scene.trace || '-', scene.raws(), String(scene.state.boss), String(scene.types[70].footX)];
            // 두 정밀도의 반환/효과/전체 raw/권한/현재 발자국과 각각 비교한다.
            for (let field = 0; field < actual.length; field += 1) {
                assert.equal(actual[field], row[1 + field], `Bomb 폭발 입력 ${row[0]} 필드 ${field}`);
                assert.equal(row[1 + field], row[6 + field]);
            }
            count += 1;
        }
    }
    assert.equal(count, 2146);
});

test('bomb-blast: 미도달 효과는 요구하지 않고 실제 하위 누락·잘못된 프레임/owner·다른 풀의 연결은 거부한다', () => {
    const scene = new BlastScene();
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-blast-x86.tsv'));
    scene.prepare(rows[0][0]);
    const blast = new RawBombBlast(scene.pool, scene.types, scene.frames, scene.coordinates, scene.state, {});
    assert.equal(blast.pool, scene.pool);
    assert.equal(blast.handles(50), true);
    assert.equal(blast.handles(60), false);
    // 운반체(QA)와 같으면 효과 경계를 요구하지 않는다.
    blast.apply(50, 51);
    assert.throws(() => blast.apply(50, 60), TypeError);
    scene.types[70].flags2 = 4;
    put(scene.raw(60), 36, 9, 4);
    assert.throws(() => blast.mayTarget(60), RangeError);
    scene.raw(60)[10] = 158;
    scene.raw(60)[34] = 255;
    assert.throws(() => blast.apply(50, 60), RangeError);
    const other = new SidPool(24000, { server: false });
    assert.throws(() => new RawBombBlast(other, scene.types, scene.frames, scene.coordinates, scene.state, {}), TypeError);
});
