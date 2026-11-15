// 트리즌 주문 대상 본문 raw-bomb-treason.js(RawBombTreason 자격·소유권/받침/이웃 전파)를 원본 10.78 관측 157행과 cpppj 시험 계약으로 대조한다(WB04-E552).
//  - bomb-treason-x86.tsv 157행(자격 E 144·적용 A 13): 소유자/genus/종류/방향/QA/바닥 발견/재진입 변경/후보 순회의 자격 결과·소유자 결과·효과 순서(O 소유권·R 갱신·F 펜스·L 문구·Q 검색·P 받침·D 진단·V 플레어).
//  - 계약: 판본/표 크기 거부, 갱신 효과가 누락되면 이미 한 소유권 쓰기를 되돌리지 않고 명시 오류.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E552)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawBombTreason, BOMB_TREASON_ROLE } from '../www/core/raw-bomb-treason.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E552)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-treason-x86.tsv'));
// 바이트 배열을 소문자 16진 문자열로 만든다(원문 Hex). (2026-11-15 WB04-E552)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 독립 트리즌 PE 입력과 같은 raw 슬롯/현재 행/장치 관찰을 이 검사에 한정한 장면이다(cpppj TreasonOwnerScene). (2026-11-15 WB04-E552)
class TreasonScene {
    constructor(row) {
        // 풀 번호 5~127 을 확보하고 네 단계 해시·TYPE/프레임 표를 둔다.
        this.pool = new SidPool(24000, { server: false });
        for (let id = 5; id <= 127; id += 1) assert.equal(this.pool.allocate(2), id);
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ name: '', footX: 1, footY: 1, flags1: 0, flags2: 0 }));
        this.frames = Array.from({ length: 188 }, () => ({ codes: [] }));
        this.events = [];
        // 현재 역할 배열이며 효과마다 다시 읽는다(NoIsland·EmptyGeyser·Fence·Bridge·Connector·Flare).
        this.roles = [157, 84, 136, 82, 155, 106];
        this.first = true;
        const kind = Number(row[2]);
        const side = Number(row[4]);
        this.mutate = Number(row[7]) !== 0;
        // 현재 TYPE 이름/발자국/방향의 명시 행이다.
        for (let type = 0; type < 188; type += 1) {
            this.types[type].name = `T${type}`;
            this.frames[type] = { codes: [{ side, variant: 65, number: 0, flags: 0 }] };
        }
        this.types[kind].flags2 = Number(row[1]);
        this.types[120].flags2 = 0x10040000;
        // 모든 읽기/쓰기는 확보한 50바이트 입력 범위에 한정한다.
        for (const sid of [50, 60, 61, 70, 71]) {
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            put(raw, 0, 0x11010000);
            raw[34] = sid === 50 || Number(row[3]) ? 2 : 1;
        }
        const parent = this.pool.allocated(50);
        const target = this.pool.allocated(60);
        const beneath = this.pool.allocated(61);
        parent[10] = 182;
        target[10] = kind;
        beneath[10] = 157;
        put(parent, 12, Number(row[5]) ? 60 : 0, 2);
        put(target, 14, floatToBits(100.25));
        put(target, 18, floatToBits(200.75));
        put(beneath, 14, floatToBits(101.5));
        put(beneath, 18, floatToBits(201.5));
        if (Number(row[8])) {
            // 고정 결과 대신 실제 해시/선저장 next 로 70→71 을 순회한다.
            for (const sid of [71, 70]) {
                const raw = this.pool.allocated(sid);
                raw[10] = sid === 70 ? (kind === 82 ? 155 : 120) : 121;
                const y = kind === 82 ? 199 : 200;
                put(raw, 14, floatToBits(100));
                put(raw, 18, floatToBits(y));
                put(raw, 4, this.hash.bucket(1, 100, y), 2);
                this.hash.setBucket(1, 100, y, sid);
            }
        }
    }

    // 원문 장치 경계의 실제 인자/순서만 기록하는 효과 묶음이다.
    hooks(found) {
        const push = (text) => this.events.push(text);
        return {
            role: (role) => this.roles[role],
            owner: (sid, table, owner) => {
                push(`O:${sid}:${table}:${owner}`);
                this.pool.allocated(sid)[34] = owner;
                // 첫 소유권 효과에서 현재 시전자/대상 가상 표를 바꾸는 명시 입력이다.
                if (this.mutate && this.first) {
                    this.pool.allocated(50)[34] = 7;
                    this.pool.allocated(60)[10] = 136;
                    put(this.pool.allocated(60), 0, 0x11010200);
                }
                this.first = false;
            },
            refresh: (sid, table) => push(`R:${sid}:${table}`),
            fence: (sid, rebuild) => push(`F:${sid}:${rebuild ? 1 : 0}`),
            trace: (level, text) => { assert.equal(level, 4); push(`L:${hex(Buffer.from(text, 'latin1'))}`); },
            find: (x, y, type) => { push(`Q:${floatToBits(x)}:${floatToBits(y)}:${type}`); return found ? 61 : 0; },
            support: (x, y, owner) => push(`P:${floatToBits(x)}:${floatToBits(y)}:${owner}`),
            assertion: (expression, file, line) => { assert.equal(file, '\\Ns\\O\\Bomb.cpp'); push(`D:${line}`); },
            flare: (sid, count, event, type, direction) => push(`V:${sid}:${count}:${event}:${type}:${direction}`)
        };
    }

    // 현재 장면의 실제 본문이다.
    body(hooks) { return new RawBombTreason(this.pool, this.hash, this.types, this.frames, hooks); }
}

test('RawBombTreason: 원본 10.78 자격·소유권/받침/이웃 전파 157행의 결과·소유자·효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 행마다 이전 효과가 남지 않는 새 명시 입력을 사용한다.
    for (const row of rows.filter((fields) => fields[0] !== 'mode')) {
        assert.equal(row.length, 12);
        const f = new TreasonScene(row);
        const body = f.body(f.hooks(Number(row[6]) !== 0));
        if (row[0] === 'E') {
            f.pool.allocated(60)[11] = 3;
            assert.equal(body.mayTarget(50, 60), Number(row[9]) !== 0, `행 ${count} 자격`);
        } else body.apply(50, 60);
        assert.equal(f.pool.slot(60)[34], Number(row[10]), `행 ${count} 소유자`);
        assert.equal(f.events.length === 0 ? '-' : f.events.join(';'), row[11], `행 ${count} 효과`);
        count += 1;
    }
    assert.equal(count, 157);
});

test('RawBombTreason: 판본/표 크기 거부와 갱신 효과 누락 시 앞선 소유권 쓰기 보존', { skip: !rows && '원본 fixture 없음' }, () => {
    const row = ['A', '0', '94', '0', '72', '0', '0', '0', '0'];
    const f = new TreasonScene(row);
    assert.throws(() => new RawBombTreason({ edition: 'cd1072', capacity: 24000 }, f.hash, f.types, f.frames, f.hooks(false)), TypeError);
    assert.throws(() => new RawBombTreason(f.pool, f.hash, f.types.slice(0, 187), f.frames, f.hooks(false)), TypeError);
    assert.throws(() => new RawBombTreason(f.pool, f.hash, f.types, f.frames.slice(0, 187), f.hooks(false)), TypeError);
    // 누락한 후단은 실제 owner 쓰기를 되돌리거나 플레어 성공으로 숨기지 않는다.
    const hooks = f.hooks(false);
    hooks.refresh = undefined;
    const body = f.body(hooks);
    assert.throws(() => body.apply(50, 60), /미연결/);
    assert.equal(f.pool.slot(60)[34], 2);
    assert.equal(f.events.length, 2);
    assert.equal(f.first, false);
    assert.deepEqual(Object.keys(BOMB_TREASON_ROLE), ['NoIsland', 'EmptyGeyser', 'Fence', 'Bridge', 'Connector', 'Flare']);
});

// 지정 SID 슬롯을 후보 객체로 만들고 같은 단계 해시 칸에 이어 붙인다(원문 시험의 선저장 next 연결과 같은 방식). (2026-11-15 WB04-E552)
function placeCandidate(f, sid, type, x, y, level = 1) {
    const raw = f.pool.allocated(sid);
    raw.fill(0);
    put(raw, 0, 0x11010000);
    raw[10] = type;
    raw[34] = 1;
    put(raw, 14, floatToBits(x));
    put(raw, 18, floatToBits(y));
    put(raw, 4, f.hash.bucket(level, x, y), 2);
    f.hash.setBucket(level, x, y, sid);
}

test('RawBombTreason: 자격 genus 의 단독 비트와 직접 값(절삭·TYPE/방향 범위·189칸 표)', { skip: !rows && '원본 fixture 없음' }, () => {
    // genus 0x100000 만 있어도 자격이 있다(자격 마스크 0x10170004 의 한 비트).
    const f = new TreasonScene(['E', String(0x100000), '94', '0', '65', '0', '0', '0', '0']);
    f.pool.allocated(60)[11] = 3;
    assert.equal(f.body(f.hooks(false)).mayTarget(50, 60), true);
    // 원문 _ftol 의 하위 DWORD: 음수는 0 쪽으로 절삭하고 2^32 배수는 0, 비유한/범위 밖은 0.
    assert.deepEqual([-1.5, 2.9, 4294967301.5, -4294967297.5, Number.NaN, 2 ** 63, 2 ** 70].map((value) => RawBombTreason.cell(value)), [-1, 2, 5, -1, 0, 0, 0]);
    // 현재 TYPE/방향 TYPE 이 표 크기이면 범위 오류이다.
    const body = f.body(f.hooks(false));
    f.pool.physical(60)[10] = 188;
    assert.throws(() => body.type(60), RangeError);
    assert.throws(() => body.side(60), RangeError);
    // 189칸 TYPE/프레임 표는 거부한다.
    assert.throws(() => new RawBombTreason(f.pool, f.hash, [...f.types, f.types[0]], f.frames, f.hooks(false)), TypeError);
    assert.throws(() => new RawBombTreason(f.pool, f.hash, f.types, [...f.frames, f.frames[0]], f.hooks(false)), TypeError);
});

test('RawBombTreason: 다리의 북·동·남·서 이웃(대각 제외)·표면 단계 후보·위치 재조회·진단 문구·현재 시전자 owner', { skip: !rows && '원본 fixture 없음' }, () => {
    const oSids = (f) => f.events.filter((event) => event.startsWith('O:')).map((event) => Number(event.split(':')[1]));
    // 다리(82): 대상 (100.25, 200.75)의 이웃 칸 N(100,199)·E(101,200)·S(100,201)·W(99,200)에 연결 객체(155)를 두면 순서대로 전파한다. 대각(101,199)은 건드리지 않는다.
    let f = new TreasonScene(['A', '0', '82', '0', '72', '0', '0', '0', '0']);
    placeCandidate(f, 70, 155, 100, 199);
    placeCandidate(f, 71, 155, 101, 200);
    placeCandidate(f, 72, 155, 100, 201);
    placeCandidate(f, 73, 155, 99, 200);
    placeCandidate(f, 74, 155, 101, 199);
    placeCandidate(f, 75, 155, 101, 200, 0);
    f.body(f.hooks(false)).apply(50, 60);
    // 표면(0단계) 후보 75 도 포함하고(표면 건너뛰기 없이 탐색), 순서는 북→동→남→서이다.
    const order = oSids(f);
    assert.deepEqual(order.filter((sid) => sid !== 75), [60, 70, 71, 72, 73]);
    assert.ok(order.includes(75));
    assert.ok(!order.includes(74));
    // 소유권 진단 문구와 첫 소유권 효과에서 시전자 owner 가 바뀌면 이후 받침 전파는 현재 시전자 owner 를 쓴다.
    const assertions = [];
    f = new TreasonScene(['A', String(0x10040000), '94', '0', '65', '0', '1', '0', '0']);
    let hooks = f.hooks(true);
    hooks.assertion = (expression, file, line) => assertions.push([expression, line]);
    const ownerHook = hooks.owner;
    hooks.owner = (sid, table, owner) => { ownerHook(sid, table, owner); f.pool.allocated(50)[34] = 7; };
    f.body(hooks).apply(50, 60);
    assert.deepEqual(assertions.map(([expression]) => expression), ["anIsle.getOrientation() == 'H'"]);
    assert.deepEqual(assertions.map(([, line]) => line), [0x5e9]);
    const supports = f.events.filter((event) => event.startsWith('P:'));
    assert.equal(supports.length, 2);
    assert.ok(supports.every((event) => event.endsWith(':7')));
    // 빈 섬 바닥(157)이 'H' 방향이 아니면 둘째 진단이 나오고, 받침 전파는 지금의 시전자 owner 를 쓴다.
    const second = [];
    f = new TreasonScene(['A', '0', '157', '0', '65', '0', '0', '0', '0']);
    hooks = f.hooks(false);
    hooks.assertion = (expression, file, line) => second.push([expression, line]);
    const ownerHook2 = hooks.owner;
    hooks.owner = (sid, table, owner) => { ownerHook2(sid, table, owner); f.pool.allocated(50)[34] = 9; };
    f.body(hooks).apply(50, 60);
    assert.deepEqual(second, [["s.getOrientation() == 'H'", 0x5f0]]);
    assert.ok(f.events.some((event) => event.startsWith('P:') && event.endsWith(':9')));
    // 받침 전파가 대상 위치를 바꾸면 이어지는 칸 위 후보 탐색은 바뀐 현재 위치를 쓴다.
    f = new TreasonScene(['A', String(0x10000000), '94', '0', '72', '0', '0', '0', '0']);
    placeCandidate(f, 70, 120, 150, 200);
    hooks = f.hooks(false);
    hooks.support = (x, y, owner) => { f.events.push(`P:${floatToBits(x)}:${floatToBits(y)}:${owner}`); put(f.pool.allocated(60), 14, floatToBits(150.5)); };
    f.body(hooks).apply(50, 60);
    assert.ok(oSids(f).includes(70));
});
