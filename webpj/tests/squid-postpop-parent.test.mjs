// 공통 postPop 부모(004b0d30)의 실제 순서·현재 조회·진단 복귀와 장부 앞부분 효과를 비멀티 명시 입력으로 대조한다(WB04-E24, cpppj PostPopCurrentTests 이식).
// 원본 관측 fixture 없이 PE 정적 확인 계약 열 개를 옮겼다(실제 마나 진단 연결 1건은 SquidPostPop 본체 단위에서). 외부 경계는 명시 변경/실패 입력이며 부모 분기를 대신 계산하지 않는다.
// (2026-11-08 WB04-E24: 공통 postPop 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { runSquidPostPopParent } from '../www/core/squid-postpop-parent.js';

// 도달한 하위 실패 뒤 부분 효과를 검사한다. (2026-11-08 WB04-E24)
const failed = (action) => { try { action(); } catch { return true; } return false; };

// 같은 슬롯과 현재 188 TYPE 를 callback 사이에도 유지하는 장면이다(cpppj PostParentScene). (2026-11-08 WB04-E24)
class PostParentScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.sid = this.pool.allocate(2);
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0 }));
        this.suppressed = false; this.graphs = false;
        this.state = { depth: 2 };
        this.owner = 1; this.graphReads = 0; this.typeReads = 0;
        this.events = [];
        this.change = null;
        this.raw()[10] = 74;
        this.raw()[34] = 1;
        this.hooks = {
            suppressed: () => this.suppressed,
            graphsEnabled: () => { this.graphReads += 1; return this.graphs; },
            type: (type) => { this.typeReads += 1; assert.ok(type < this.types.length, '명시 현재 TYPE 밖'); return this.types[type]; },
            region: (value) => { assert.equal(value, this.sid); this.event('region'); },
            add: (value) => { assert.equal(value, this.sid); this.event('add'); },
            resetGraph: (value) => { assert.equal(value, this.sid); this.event('reset'); },
            assertion: (expression, file, line) => { assert.deepEqual([expression, file, line], ['!(popFlags&pfDESTROY_GRAPH)', '\\Ns\\O\\Squid.cpp', 0x80b]); this.event('assert'); },
            cost: () => this.event('cost'), provider: () => this.event('provider'), factory: () => this.event('factory'),
            ownerFactory: (current) => this.event(`owner:${current}`),
            aiNotice: (current) => this.event(`ai:${current}`),
            localOwner: () => this.owner,
            localCount: (type) => this.event(`local:${type}`),
            globalCount: (type) => this.event(`global:${type}`),
            recheckSelection: () => this.event('selection'),
        };
    }

    // free 변경 뒤에도 동일한 물리를 읽는다.
    raw() { return this.pool.physical(this.sid); }

    // 자기 연결 교체도 현재 호출을 보존한다.
    event(value) { this.events.push(value); if (this.change) this.change(value); }

    // 호출자가 증가해 둔 깊이를 공통 몸체가 한 번 줄인다.
    run(flags) { runSquidPostPopParent(this.pool, this.sid, flags, this.state, this.hooks); }
}

// 메타맵은 범위 밖 SID/TYPE 나 미연결 장치를 조회하지 않는다.
test('공통 postPop: suppressed 는 슬롯·TYPE 보기 전에 끝나고 깊이는 32비트로 감긴다', () => {
    const scene = new PostParentScene();
    scene.suppressed = true;
    scene.sid = 65535;
    scene.state.depth = 0;
    delete scene.hooks.type;
    delete scene.hooks.graphsEnabled;
    scene.run(1);
    assert.deepEqual([scene.state.depth, scene.typeReads, scene.events.length], [0xffffffff, 0, 0]);
});

// 최초 genus 가 영역이고 활성이 0 이면 그 다음 그래프 재조회도 생략한다.
test('공통 postPop: 영역 군이고 그래프 비활성이면 그래프 단계를 통째로 건너뛴다', () => {
    const scene = new PostParentScene();
    scene.types[74].flags2 = 8;
    scene.hooks.graphsEnabled = () => { scene.graphReads += 1; return scene.graphReads > 1; };
    scene.run(0x200);
    assert.deepEqual([scene.graphReads, scene.events.length, scene.state.depth], [1, 0, 1]);
});

// 영역 하위가 바꾼 현재 활성을 다시 읽고 Add 를 생략한다.
test('공통 postPop: 영역 하위가 그래프를 끄면 Add 를 생략하고 장부로 간다', () => {
    const scene = new PostParentScene();
    scene.graphs = true;
    scene.types[74].flags2 = 8;
    scene.types[74].flags1 = 0x800;
    scene.change = (event) => { if (event === 'region') scene.graphs = false; };
    scene.run(3);
    assert.equal(scene.graphReads, 2);
    assert.deepEqual(scene.events, ['region', 'cost', 'ai:1', 'local:74', 'global:74', 'selection']);
    assert.equal(scene.state.depth, 1);
});

// checking 이 없어도 80b 정상 복귀 뒤 현재 TYPE 의 표면 분기를 선택한다.
test('공통 postPop: 파괴 진단 복귀 뒤 현재 TYPE 으로 표면 분기를 고른다', () => {
    const scene = new PostParentScene();
    scene.graphs = true;
    scene.change = (event) => { if (event === 'assert') { scene.raw()[10] = 75; scene.types[75].flags1 = 0x800; } };
    scene.run(0xa00);
    assert.deepEqual(scene.events, ['assert', 'add']);
    assert.equal(scene.state.depth, 1);
});

// 진단 중단은 먼저 실행한 영역 효과를 지우거나 깊이를 끝내지 않는다.
test('공통 postPop: 진단 중단은 영역 효과와 증가된 깊이를 남긴다', () => {
    const scene = new PostParentScene();
    scene.graphs = true;
    scene.types[74].flags2 = 8;
    scene.change = (event) => { if (event === 'assert') throw new Error('명시 80b 중단'); };
    assert.ok(failed(() => scene.run(0x801)));
    assert.equal(scene.state.depth, 2);
    assert.deepEqual(scene.events, ['region', 'assert']);
});

// noGraph 의 현재 표면 번호 리셋은 장부 앞이며 Add/영역은 호출하지 않는다.
test('공통 postPop: noGraph 표면은 장부 앞에서 리셋만 한다', () => {
    const scene = new PostParentScene();
    scene.graphs = true;
    scene.types[74].flags1 = 0x800;
    scene.run(0x2000001);
    assert.deepEqual(scene.events, ['reset', 'cost', 'ai:1', 'local:74', 'global:74', 'selection']);
    assert.equal(scene.state.depth, 1);
});

// 각 외부 효과가 TYPE/owner 를 바꾸며 공장 분기만 그래프 뒤 저장 genus 를 쓴다.
test('공통 postPop: 현재 TYPE 과 저장 genus 의 독립 역할', () => {
    const scene = new PostParentScene();
    scene.graphs = true;
    scene.types[74].flags2 = 8;
    scene.change = (event) => {
        if (event === 'region') { scene.raw()[10] = 75; scene.types[75].flags1 = 0x800; }
        else if (event === 'add') { scene.raw()[10] = 76; scene.types[76].flags2 = 0x4200; }
        else if (event === 'cost') { scene.raw()[10] = 77; scene.types[77].flags1 = 0x10000000; }
        else if (event === 'provider') { scene.raw()[10] = 78; scene.raw()[34] = 3; }
        else if (event === 'owner:3') scene.raw()[34] = 4;
        else if (event === 'factory') scene.raw()[34] = 5;
        else if (event === 'ai:5') { scene.raw()[34] = 6; scene.owner = 6; scene.raw()[10] = 255; }
        else if (event === 'local:255') scene.raw()[10] = 254;
    };
    scene.run(3);
    assert.deepEqual(scene.events, ['region', 'add', 'cost', 'provider', 'owner:3', 'factory', 'ai:5', 'local:255', 'global:254', 'selection']);
    assert.equal(scene.state.depth, 1);
});

// 현재 extra 1/8/9 는 그래프 실행 뒤 장부만 생략한다.
test('공통 postPop: 추상 extra 1/8/9 는 그래프만 마치고 장부를 생략한다', () => {
    for (const extra of [1, 8, 9]) {
        const scene = new PostParentScene();
        scene.graphs = true;
        scene.types[74].flags1 = 0x800;
        scene.raw()[40] = extra;
        scene.run(1);
        assert.deepEqual(scene.events, ['add']);
        assert.equal(scene.state.depth, 1);
    }
});

// 마지막 선택 하위 실패도 이미 기록한 통계와 현재 깊이를 되돌리지 않는다.
test('공통 postPop: 선택 재검사 실패는 앞선 통계와 깊이를 남긴다', () => {
    const scene = new PostParentScene();
    scene.change = (event) => { if (event === 'selection') throw new Error('명시 선택 하위 실패'); };
    assert.ok(failed(() => scene.run(1)));
    assert.equal(scene.state.depth, 2);
    assert.deepEqual(scene.events, ['cost', 'ai:1', 'local:74', 'global:74', 'selection']);
});

// 원문 두 mov 의 순서를 비교 피연산자 평가 순서에 맡기지 않는다.
test('공통 postPop: 저장 owner 읽기가 현재 로컬 owner getter 보다 먼저이다', () => {
    const scene = new PostParentScene();
    scene.hooks.localOwner = () => { scene.raw()[34] = 2; return 1; };
    scene.run(1);
    assert.equal(scene.raw()[34], 2);
    assert.deepEqual(scene.events, ['cost', 'ai:1', 'local:74', 'global:74', 'selection']);
    assert.equal(scene.state.depth, 1);
});

// 10.78 전용이며 도달한 장치만 요구한다.
test('공통 postPop: 판본과 누락 장치 계약', () => {
    const scene = new PostParentScene();
    assert.throws(() => runSquidPostPopParent({ edition: 'cd1072' }, 5, 0, { depth: 1 }, scene.hooks), TypeError);
    delete scene.hooks.recheckSelection;
    assert.throws(() => scene.run(1), /실제 장치 미연결/);
    assert.equal(scene.state.depth, 2);
});
