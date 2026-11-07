// 직접 피해 주문 네 종 raw-bomb-strike.js(RawBombStrike)의 자격/효과·전체 슬롯·공유 게임 난수를 독립 원본 관측 bomb-strike-x86.tsv 1420행으로 대조한다.
// 입력 20개 DWORD: [연산(0 자격 · 1 효과 · 그 밖 자격 뒤 효과), 가상 표 번호 0..3, 대상 TYPE, genus, flags1, extra, 상태, 운반체 SID, 대상 SID, 부모 owner, 대상 HP, 권한, 난수 시드, 변형 1~15, 부모 x·y, 대상 x·y, 발자국 x·y].
// 변형은 생성·pop·소리·피해·삭제·빔·투영·기둥 콜백이 대상 TYPE/좌표/상태/HP/owner/가상 표/운반체를 바꾸는 명시 입력이다. 반환·효과 문자열·다섯 슬롯 전체·공유 난수 상태·권한을 원본(두 x87 정밀도)과 비교하며 기대값은 계산하지 않는다.
// (2026-11-08 WB04-E54: RawBombStrike 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { GameRandom } from '../www/core/random.js';
import { RawBombStrike } from '../www/core/raw-bomb-strike.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, get, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E54)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력 십진수를 원본 하위 32비트 수로 읽는다. (2026-11-08 WB04-E54)
const unsigned = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 독립 실행기가 보호 PE 에서 확인한 실제 네 가상 표, 피해/삭제 가상 경계를 구별하는 원문 토큰, 전체 바이트를 동결한 물리 슬롯이다. (2026-11-08 WB04-E54)
const STRIKE_TABLES = [0x502908, 0x5029d0, 0x502aa8, 0x502b70];
const STRIKE_TARGET = 0x501dd0;
const STRIKE_ALT = 0x20000800;
const STRIKE_SLOTS = [0, 50, 51, 60, 70];

// 제품 계산과 구별되는 독립 raw/TYPE·큰 효과 입력 장면이다. 같은 types/state 객체를 제자리에서 갱신한다(몸체가 참조를 빌린다). (2026-11-08 WB04-E54)
class StrikeScene {
    pool = new SidPool(24000, { server: false });
    types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    random = new GameRandom();
    state = { boss: 0, specialType: 138, firstEffectType: 174, hailEffectType: 177 };
    input = [];
    trace = '';
    projects = 0;

    // 슬롯의 쓰기 가능한 50바이트이다.
    raw(sid) {
        return this.pool.physical(sid).subarray(0, 50);
    }

    // 부호 있는 화면 좌표도 원문 DWORD 로 기록한다.
    event(name, values) {
        if (this.trace) this.trace += ';';
        this.trace += name;
        // 실제 인자 순서와 전체 비트를 유지한다.
        for (const value of values) this.trace += `:${value >>> 0}`;
    }

    // 독립 원문 입력을 같은 원본 폭으로 설치한다(20개 DWORD).
    prepare(text) {
        this.trace = '';
        this.projects = 0;
        Object.assign(this.state, { boss: 0, specialType: 138, firstEffectType: 174, hailEffectType: 177 });
        this.input = text.split(',').map(unsigned);
        assert.equal(this.input.length, 20);
        const input = this.input;
        for (let index = 0; index < this.types.length; index += 1) this.types[index] = { flags1: 0, flags2: 0, footX: 0, footY: 0 };
        // 원문의 전체 관측 슬롯을 매 입력 초기화한다.
        for (const sid of STRIKE_SLOTS) this.raw(sid).fill(0);
        const parent = this.raw(50);
        put(parent, 0, STRIKE_TABLES[input[1]], 4);
        parent[10] = 168;
        parent[34] = input[9] & 0xff;
        put(parent, 12, input[7], 2);
        put(parent, 14, input[14], 4);
        put(parent, 18, input[15], 4);
        // 기본 두 후보는 QA 생존 조회도 같은 raw 를 읽게 한다.
        for (const sid of [51, 60]) {
            const raw = this.raw(sid);
            put(raw, 0, STRIKE_TARGET, 4);
            raw[10] = 70;
        }
        const target = input[8] & 0xffff;
        const raw = this.raw(target);
        raw.fill(0);
        put(raw, 0, STRIKE_TARGET, 4);
        raw[10] = input[2] & 0xff;
        raw[11] = input[6] & 0xff;
        raw[40] = input[5] & 0xff;
        put(raw, 14, input[16], 4);
        put(raw, 18, input[17], 4);
        put(raw, 26, input[10], 4);
        put(this.raw(70), 0, STRIKE_TARGET, 4);
        const type = this.types[input[2]];
        type.flags1 = input[4];
        type.flags2 = input[3];
        type.footX = input[18] | 0;
        type.footY = input[19] | 0;
        this.types[168].footX = 1;
        this.types[168].footY = 3;
        this.state.boss = input[11];
        this.random.setState(input[12]);
    }

    // 큰 실제 하위의 명시 입력/재진입만 공급하며 제품 분기를 계산하지 않는다.
    hooks() {
        const input = this.input;
        const target = () => input[8] & 0xffff;
        const bigPosition = (sid) => {
            put(this.raw(sid), 14, floatToBits(200.0), 4);
            put(this.raw(sid), 18, floatToBits(201.0), 4);
        };
        return {
            // 생성은 현재 대상/부모를 바꿀 수 있으며 중심 캡처 순서를 구별한다.
            create: (kind, flags) => {
                this.event('create', [kind, flags]);
                if (input[13] === 1) this.raw(target())[10] = 138;
                if (input[13] === 2) bigPosition(target());
                if (input[13] === 3) {
                    this.raw(50)[34] = 3;
                    put(this.raw(target()), 0, STRIKE_ALT, 4);
                }
                return 70;
            },
            // 원문은 생성 뒤 현재 가상 90 을 요청한다.
            pop: (created, table, x, y, flags) => {
                assert.ok(created === 70 && table === STRIKE_TARGET);
                this.event('pop', [created, floatToBits(x), floatToBits(y), flags]);
                if (input[13] === 4) bigPosition(target());
                if (input[13] === 5) {
                    this.raw(target())[10] = 138;
                    this.state.boss = 0;
                }
                if (input[13] === 6) {
                    this.raw(target())[10] = 70;
                    put(this.raw(target()), 0, STRIKE_ALT, 4);
                    this.raw(50)[34] = 4;
                }
            },
            // 음향 뒤 dead/QA/owner 를 바꾸는 명시 재진입이다.
            sound: (x, y) => {
                this.event('sound', [floatToBits(x), floatToBits(y), 0]);
                if (input[13] === 7) this.raw(target())[11] = 2;
                if (input[13] === 8) {
                    put(this.raw(50), 12, 0, 2);
                    this.raw(50)[34] = 6;
                }
            },
            // 실제 피해 몸체 대신 선언한 HP 감김/변경만 입력으로 준다.
            damage: (victim, table, amount, owner, parent) => {
                assert.ok(parent === 50 && victim === input[8] && (table === STRIKE_TARGET || table === STRIKE_ALT));
                this.event(table === STRIKE_TARGET ? 'damage' : 'damage-alt', [victim, amount, owner, parent]);
                put(this.raw(victim), 26, (get(this.raw(victim), 26, 4) - (amount >>> 0)) >>> 0, 4);
                if (input[13] === 9) put(this.raw(victim), 26, 1000000, 4);
                if (input[13] === 10) {
                    this.raw(parent)[34] = 7;
                    put(this.raw(victim), 0, STRIKE_ALT, 4);
                }
                if (input[13] === 11) {
                    this.raw(victim)[11] = 2;
                    put(this.raw(victim), 26, 0, 4);
                }
                if (input[13] === 12) put(this.raw(parent), 12, victim, 2);
            },
            // 특별 대상 삭제의 현재 표와 정확한 flags 를 기록한다.
            destroy: (victim, table, flags) => {
                assert.ok(table === STRIKE_TARGET || table === STRIKE_ALT);
                this.event(table === STRIKE_TARGET ? 'destroy' : 'destroy-alt', [victim, flags]);
            },
            // QA 조회와 이미 통과한 dead 관문을 빔 뒤 다시 검사하지 않는지 확인한다.
            beam: (source, victim) => {
                this.event('beam', [source, victim]);
                if (input[13] === 13) {
                    this.raw(victim)[11] = 2;
                    put(this.raw(50), 12, victim, 2);
                }
            },
            // 두 호출에 다른 정수점을 주고 대상 위치를 바꾸어 캡처/출력 선택을 구별한다.
            project: (x, y) => {
                this.event('project', [floatToBits(x), floatToBits(y)]);
                this.projects += 1;
                if (input[13] === 15) bigPosition(target());
                return [17 + 2 * this.projects, -23 - 2 * this.projects];
            },
            // 원문의 첫 점 y=0 과 둘째 투영·수명 5 를 전체 DWORD 로 관측한다.
            column: (victim, from, to, life) => {
                this.event('column', [victim, from[0], from[1], to[0], to[1], floatToBits(life)]);
                if (input[13] === 14) {
                    put(this.raw(50), 14, floatToBits(200.0), 4);
                    put(this.raw(50), 18, floatToBits(201.0), 4);
                }
            },
        };
    }

    // 원문과 같은 물리 슬롯 순서로 전체 바이트를 반환한다(선언 경계가 쓰지 않은 필드까지 대조).
    raws() {
        return STRIKE_SLOTS.map((sid) => hex(this.pool.slot(sid))).join(',');
    }
}

test('bomb-strike: 네 직접 피해 주문의 자격/효과·난수 반복·피해 전 HP 조기 종료를 원본 1420행과 대조한다', () => {
    const scene = new StrikeScene();
    const strike = new RawBombStrike(scene.pool, scene.types, scene.random, scene.state, scene.hooks());
    let rows = 0;
    const rowsData = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-strike-x86.tsv'));
    // 원문 기대값은 제품 계산으로 재생성하지 않고 보존한 두 정밀도와 대조한다.
    for (const fields of rowsData) {
        assert.equal(fields.length, 11);
        scene.prepare(fields[0]);
        // 훅이 입력(변형)을 읽으므로 입력 갱신 뒤 훅을 다시 만들어 연결한다.
        const current = new RawBombStrike(scene.pool, scene.types, scene.random, scene.state, scene.hooks());
        const target = scene.input[8] & 0xffff;
        let result = 0;
        if (scene.input[0] === 0) result = current.mayTarget(50, target);
        else if (scene.input[0] === 1) current.apply(50, target);
        else {
            result = current.mayTarget(50, target);
            if (result) current.apply(50, target);
        }
        const actual = [String(result), scene.trace || '-', scene.raws(), String(scene.random.state), String(scene.state.boss)];
        // 두 원본 정밀도 각각에 반환/효과/전체 슬롯/난수/권한이 같아야 한다.
        for (let index = 0; index < actual.length; index += 1) {
            assert.equal(actual[index], fields[index + 1], `직접 피해 주문 행 ${rows} 열 ${index} 입력 ${fields[0]}`);
            assert.equal(actual[index], fields[index + 6]);
        }
        rows += 1;
    }
    assert.equal(rows, 1420);
    assert.equal(strike.handles(50), true);
});

test('bomb-strike: 미도달 효과는 요구하지 않고 실제 하위 누락·초기 dead 관문의 계열 차이를 확인한다', () => {
    const scene = new StrikeScene();
    scene.prepare('1,0,70,2097152,16,0,0,60,60,1,1000,1,0,0,1112211456,1112670208,1112932352,1112801280,1,3');
    const missing = new RawBombStrike(scene.pool, scene.types, scene.random, scene.state, {});
    // QA 와 같은 대상은 큰 효과를 요구하지 않는다.
    missing.apply(50, 60);
    assert.equal(scene.random.state, 0);
    put(scene.raw(50), 12, 51, 2);
    assert.throws(() => missing.apply(50, 60), TypeError);
    put(scene.raw(50), 0, STRIKE_TABLES[1], 4);
    scene.raw(60)[11] = 2;
    missing.apply(50, 60);
    assert.equal(scene.random.state, 0);
    put(scene.raw(50), 0, STRIKE_TABLES[3], 4);
    scene.random.setState(1);
    // 마지막 계열은 dead 대상에도 난수/생성을 수행하므로 단순 조기 반환하면 안 된다.
    assert.throws(() => missing.apply(50, 60), TypeError);
    assert.notEqual(scene.random.state, 0);
    put(scene.raw(50), 0, 0x502370, 4);
    assert.equal(missing.handles(50), false);
    assert.throws(() => missing.mayTarget(50, 60), TypeError);
    // 구성 오류.
    assert.throws(() => new RawBombStrike({ edition: 'original1037' }, scene.types, scene.random, scene.state, {}), TypeError);
    assert.throws(() => new RawBombStrike(scene.pool, scene.types.slice(1), scene.random, scene.state, {}), TypeError);
    assert.equal(missing.pool, scene.pool);
});
