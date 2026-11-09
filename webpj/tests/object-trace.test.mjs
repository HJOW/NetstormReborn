// 객체 trace 재귀 raw-object-trace.js(0044dab0)를 원본 10.78 관측과 cpppj `ObjectTraceTests`/`ObjectTraceMainTests` 계약으로 대조한다.
//  - 원본 관측 object-trace-x86.tsv 103행: 입력(시작 번호·논리 끝 번호·깊이·들여쓰기·checking·메타맵·boss·로그 직후 변경 번호·노드 목록 `번호,TYPE,상태,head,next`)에서 가상 호출(V)·들여쓰기(I)·로그(L)·dead 부모 assert(A)의
//    순서와 인자, 최종 깊이/들여쓰기가 같다. TYPE 0..69·끝 번호·free/dead·다단 링크·콜백이 바꾼 링크/전역을 모두 포함한다.
//  - 계약: 미연결 장치/손상 체인을 거부해도 앞선 가상 출력과 깊이를 되돌리지 않고, SID 0 은 장치 없이 반환, 실제 같은 풀에서 재귀 → 현재 가상 분배(E235 `traceRawObjectVirtual`) → 공통/파생 로그가 연결된다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E236: 객체 trace 재귀 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { traceRawObject } from '../www/core/raw-object-trace.js';
import { traceRawObjectVirtual } from '../www/core/raw-object-trace-main.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(10칸). (2026-11-10 WB04-E236)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-trace-x86.tsv'));

// 바이트를 문자 인코딩 변환 없이 TSV 의 단일 열(16진수)로 표현한다. (2026-11-10 WB04-E236)
const hex = (text) => Array.from(text, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('');
// dead 부모 assert 가 넘기는 원문 식 문자열이다. (2026-11-10 WB04-E236)
const DEAD_PARENT_EXPRESSION = 'inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy';

// 동일한 50 BYTE 물리 풀·현재 전역과 외부 변형 번호를 보유하는 독립 입력 장면이다. (2026-11-10 WB04-E236)
class TraceScene {
    constructor(columns) {
        this.pool = new SidPool(24000, { server: true }); // free/dead/void 도 원문이 읽는 물리 슬롯이다.
        this.calls = 0; // 가상 호출 횟수이다.
        this.events = []; // 호출된 실제 장치의 순서/인자를 보존하는 기록이다.
        this.sid = Number(columns[0]);
        this.count = Number(columns[1]);
        this.depth = Number(columns[2]) >>> 0;
        this.indent = Number(columns[3]) >>> 0;
        this.checking = Number(columns[4]) >>> 0;
        this.metamap = Number(columns[5]) >>> 0;
        this.boss = Number(columns[6]) !== 0;
        this.mutation = Number(columns[7]);
        for (const node of columns[8].split('/')) { // 현재 TYPE·상태와 head/next 를 같은 원문 물리 폭으로 설치한다.
            const fields = node.split(',').map(Number);
            const raw = this.pool.physical(fields[0]);
            raw[10] = fields[1];
            raw[11] = fields[2];
            this.word(fields[0], 6, fields[3]);
            this.word(fields[0], 4, fields[4]);
        }
    }

    // 정렬되지 않은 원문 링크 WORD 만 바꾸고 주변 바이트를 보존한다. (2026-11-10 WB04-E236)
    word(number, offset, value) { put(this.pool.physical(number), offset, value, 2); }

    // 명시된 원문 외부 가상 호출 반환 시점에만 링크/전역을 바꾼다. (2026-11-10 WB04-E236)
    mutate(number) {
        if (this.calls === 1) {
            switch (this.mutation) {
            case 1: this.word(number, 6, 7); break;
            case 2: this.word(3, 4, 7); break;
            case 3: this.word(4, 4, 7); break;
            case 4: this.depth = 41; break;
            case 5: this.pool.physical(3)[11] = 1; break;
            case 6: this.pool.physical(4)[10] = 70; break;
            case 7: this.count = 4; break;
            default: break;
            }
        }
        if (this.calls === 2 && this.mutation === 8) this.word(3, 4, 7);
        if (this.calls === 2 && this.mutation === 9) this.word(4, 4, 7);
    }

    // 현재 장면 전역을 빌리므로 외부 효과 이후의 읽기도 즉시 반영된다. (2026-11-10 WB04-E236)
    globals() {
        const scene = this;
        return {
            depth: { get value() { return scene.depth; }, set value(next) { scene.depth = next; } }, // 상자는 값을 감지 않는다: DWORD 감김은 모듈이 맡는다.
            count: () => scene.count, checkingDead: () => scene.checking, metamap: () => scene.metamap, boss: () => scene.boss,
        };
    }

    // 원문 함수 호출 위치에서 가상 출력/들여쓰기/로그/assert 인자를 기록한다. (2026-11-10 WB04-E236)
    hooks() {
        return {
            trace: (sid) => { this.events.push(`V:${sid}:${this.depth}:${this.indent}`); this.calls += 1; this.mutate(sid); },
            indent: (value) => { this.events.push(`I:${value}`); this.indent = value >>> 0; },
            log: (level, line) => { this.events.push(`L:${level}:${hex(line)}:${this.depth}:${this.indent}`); if (this.mutation === 10) this.depth = (this.depth + 2) >>> 0; },
            deadParent: () => { this.events.push(`A:${hex(DEAD_PARENT_EXPRESSION)}`); },
        };
    }
}

test('객체 trace 재귀: 원본 10.78 103행(TYPE 0..69·끝 번호·free/dead·다단 링크·현재 콜백 변경)의 호출 순서·인자·최종 전역이 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const columns of rows) {
        assert.equal(columns.length, 10);
        const scene = new TraceScene(columns);
        traceRawObject(scene.pool, scene.globals(), scene.hooks(), scene.sid);
        count += 1;
        assert.equal(`${scene.events.join(';')}|${scene.depth}|${scene.indent}`, columns[9], `행 ${count} 입력 ${columns.slice(0, 8).join(',')}`);
    }
    assert.equal(count, 103);
});

test('객체 trace 재귀 계약: 미연결 장치·손상 체인을 거부해도 앞선 효과를 되돌리지 않고 SID 0 은 장치 없이 반환한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const columns = '2\t20\t0\t17\t0\t0\t1\t0\t2,82,0,3,0/3,46,0,0,0'.split('\t');
    const scene = new TraceScene(columns);
    assert.throws(() => traceRawObject(scene.pool, scene.globals(), { ...scene.hooks(), indent: undefined }, 2), TypeError); // 가상 trace 뒤의 선택된 setter 만 누락시킨다.
    assert.ok(scene.depth === 3 && scene.events.join(';') === 'V:2:0:17');
    traceRawObject(scene.pool, scene.globals(), {}, 0);
    assert.equal(scene.depth, 3);
    const cycle = new TraceScene(columns);
    cycle.pool.physical(2)[10] = 69;
    cycle.word(3, 6, 2); // 자식이 부모를 자식으로 가리켜 현재 경로의 고리가 된다.
    assert.throws(() => traceRawObject(cycle.pool, cycle.globals(), cycle.hooks(), 2), Error);
    assert.ok(cycle.depth === 6 && cycle.calls === 2);
    const range = new TraceScene(columns);
    range.word(2, 6, 65535);
    assert.throws(() => traceRawObject(range.pool, range.globals(), range.hooks(), 2), RangeError);
    assert.ok(range.depth === 3 && range.calls === 1);
    // 형제 고리: 3 의 next 가 3 자신이면 같은 부모의 후보 고리로 거부한다.
    const sibling = new TraceScene(columns);
    sibling.word(3, 4, 3);
    assert.throws(() => traceRawObject(sibling.pool, sibling.globals(), sibling.hooks(), 2), Error);
    // 풀 판본 검사와 free 루트: free 루트는 한 줄만 기록하고 가상 호출·깊이 변경이 없다.
    const free = new TraceScene(columns);
    free.pool.physical(2)[11] = 1;
    traceRawObject(free.pool, free.globals(), free.hooks(), 2);
    assert.deepEqual(free.events, [`L:29:${hex('sid 2 is free\n')}:0:17`]);
    assert.equal(free.depth, 0);
    // 다른 판본의 풀 껍데기는 SID 0 이면 검사 없이 반환하고 0 이 아니면 슬롯을 읽기 전에 거부한다(실제 풀의 슬롯을 그대로 빌려도).
    const cdPool = { edition: 'cd1072', capacity: 24000, slot: (sid) => free.pool.slot(sid) };
    traceRawObject(cdPool, free.globals(), {}, 0);
    assert.throws(() => traceRawObject(cdPool, free.globals(), free.hooks(), 3), TypeError);
    // 끝 번호(count) 이상의 TYPE 0..69 후보를 만나면 순회를 끝낸다(뒤의 후보는 방문하지 않음).
    const ended = new TraceScene('2	10	0	0	0	0	1	0	2,82,0,3,0/3,46,0,0,15/15,46,0,0,4/4,46,0,0,0'.split('	'));
    traceRawObject(ended.pool, ended.globals(), ended.hooks(), 2);
    assert.deepEqual(ended.events.filter((event) => event.startsWith('V:')).map((event) => event.split(':')[1]), ['2', '3']);
    // 서로 다른 부모가 같은 정상 후보를 다시 참조해도 생략하지 않는다(현재 경로의 고리만 거부).
    const shared = new TraceScene('2	20	0	0	0	0	1	0	2,82,0,3,0/3,46,0,8,4/4,46,0,8,0/8,46,0,0,0'.split('	'));
    traceRawObject(shared.pool, shared.globals(), shared.hooks(), 2);
    assert.deepEqual(shared.events.filter((event) => event.startsWith('V:')).map((event) => event.split(':')[1]), ['2', '3', '8', '4', '8']);
});

test('객체 trace 재귀: 실제 같은 풀에서 재귀 → 현재 가상 분배 → 공통/파생 로그의 전체 경계가 연결된다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, () => ({ name: '', flags1: 0, flags2: 0 }));
    types[82].name = 'assetName';
    types[46].name = 'Regular';
    types[6].name = 'ContentForm';
    for (const sid of [10, 11, 12]) pool.physical(sid).fill(0); // free 슬롯의 초기 연결/상태를 지운 명시 live 입력을 설치한다.
    const slot = (sid, offset, value, width) => put(pool.physical(sid), offset, value, width);
    pool.physical(10)[10] = 82;
    slot(10, 0, 0x501dd0, 4);
    slot(10, 6, 11, 2);
    pool.physical(11)[10] = 46;
    slot(11, 0, 0x512098, 4);
    slot(11, 4, 12, 2);
    slot(11, 18, 999, 4);
    pool.physical(12)[10] = 6;
    slot(12, 0, 0x512208, 4);
    slot(12, 18, 82, 2);
    const state = { depth: 0, indent: 0, count: 24000, calls: 0, arguments: '' };
    const events = [];
    const log = (level, text, numbers) => { events.push(`${level}:${hex(text)}`); state.calls += 1; if (numbers) state.arguments = 'seen'; };
    traceRawObject(pool, {
        depth: { get value() { return state.depth; }, set value(next) { state.depth = next >>> 0; } },
        count: () => state.count, checkingDead: () => 0, metamap: () => 0, boss: () => true,
    }, {
        trace: (sid) => traceRawObjectVirtual(pool, types, 0, sid, log),
        indent: (value) => { state.indent = value; },
        log: (level, text) => log(level, text, null),
        deadParent: () => {},
    }, 10);
    assert.ok(state.depth === 0 && state.indent === 0 && state.calls === 7 && state.arguments !== '');
    assert.equal(events.at(-1), `29:${hex('~1~yEND 0010\n')}`);
});
