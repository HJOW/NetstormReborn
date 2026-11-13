// UI(Gump) 트리 수집기 render/gump-collection.js 를 원본 10.78 관측(gumpcollection-x86.tsv originals 389행)과 cpppj `GumpCollectionTests.cpp` 의 최상위별 한도·손상 링크 거부·공통 제출 연결로 검사한다.
//  - 관측 한 행 = 완성 입력 한 건(루트/override 토큰·대상·한도·화면 사각형, 128바이트 노드 원본, 월드 투영 응답)과 결과(반환 좌표/경계/개수, 출력 32칸 전체, 전 노드 캐시, 투영 호출 순서, 출력 앞뒤 0xa5 보호 항목)이다.
//    월드 투영 응답은 정수점과 호출 때 바꿀 next 링크를 가진 명시 경계이다. CD(originalCD)·추가판(original1037) 행은 웹이 10.78 만 지원하므로 사용하지 않는다.
//  - cpppj 시험 이식: PatchNative(위 389행)·PerRootLimitAndSafeBuffer·RejectsDamagedLinksAndMissingProjection·CommonSubmissionIntegration(실제 트리 수집→병합→정렬→kind 2 분배와 UI 전용 dirty).
//    `ObjectCollection_CommonSubmissionIntegration`(UI·지면과의 공통 제출)은 이 모듈과 `tile-group-collection.js` 가 모두 갖춰져 이 단위에서 닫는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E395)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, bitsToFloat } from './raw-support.mjs';
import { collectDrawEntries, dispatchDrawEntries, sortRenderEntries } from '../www/render/draw-submission.js';
import { GumpCollection, bindGumpCollection } from '../www/render/gump-collection.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E395)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'gumpcollection-x86.tsv'));
// 바이트 배열 보기를 관측 열과 같은 소문자 16진 문자열로 바꾼다. (2026-11-13 WB04-E395)
const hex = (bytes) => Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex');
// 관측 열의 16진 문자열을 새 바이트 배열로 되돌린다. (2026-11-13 WB04-E395)
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
// 동작이 예외를 던지면 true 를 돌려준다(관측 행의 거부 결과 비교용). (2026-11-13 WB04-E395)
const throwsOf = (action) => { try { action(); } catch { return true; } return false; };

// 입력 노드와 명시 월드 투영 응답을 가진 호스트 저장소이다. (2026-11-13 WB04-E395)
class GumpScene {
    constructor() {
        // 토큰 → 노드, 투영 응답(SID → [x, y, 바꿀 노드, 새 next]), 상태, 투영 호출 순서이다.
        this.nodes = new Map();
        this.anchors = new Map();
        this.state = { root: 0, overrideHead: 0 };
        this.projected = '';
    }

    // 빈 노드(모든 필드 0)를 만들어 등록한다.
    make(token, fields = {}) {
        const node = { flags: 0, next: 0, parent: 0, child: 0, object: 0, depth: 0, width: 0, height: 0, x: 0, y: 0, cached: { x: 0, y: 0 }, ...fields };
        this.nodes.set(token, node);
        return node;
    }

    // 원본 128바이트 입력을 노드 필드에 그대로 공급한다.
    load(input) {
        // 완성 입력 토큰별로 부모/자식/로컬 필드와 초기 캐시를 읽는다.
        for (const item of input.split(';')) {
            const [token, bytes] = item.split('=');
            const raw = unhex(bytes);
            this.make(Number(token), {
                flags: get(raw, 4), next: get(raw, 12), parent: get(raw, 20), child: get(raw, 24), object: get(raw, 36), depth: get(raw, 48),
                width: bitsToFloat(get(raw, 68)), height: bitsToFloat(get(raw, 72)), x: bitsToFloat(get(raw, 76)), y: bitsToFloat(get(raw, 80)),
                cached: { x: get(raw, 108) | 0, y: get(raw, 112) | 0 },
            });
        }
    }

    // 주소순 전체 노드 캐시이다. 월드 부착 자신을 일반 조회처럼 갱신하지 않았는지 대조한다.
    caches() {
        return [...this.nodes.keys()].sort((a, b) => a - b).map((token) => `${token},${this.nodes.get(token).cached.x},${this.nodes.get(token).cached.y}`).join(';');
    }

    // 위치 조회의 명시 응답은 현재 링크를 바꿀 수 있다. 기대 수집 순서는 fixture 에서 읽는다.
    hooks() {
        return {
            node: (token) => this.nodes.get(token) ?? null,
            project: (sid) => {
                if (this.projected) this.projected += ',';
                this.projected += String(sid);
                const response = this.anchors.get(sid);
                if (response[2]) this.nodes.get(response[2]).next = response[3];
                return { x: response[0], y: response[1] };
            },
        };
    }
}

test('GumpCollection_PatchNative: 원본 10.78 관측 389행의 위치/경계/수집 출력·노드 캐시·투영 호출 순서와 출력 보호 항목이 모두 일치한다', { skip: rows === null }, () => {
    let replayed = 0;
    // 입력 행마다 저장소를 초기화해 앞선 수집의 캐시/링크 변경이 남지 않게 한다.
    for (const row of rows) {
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 12);
        const scene = new GumpScene();
        const config = row[3].split(',').map(Number);
        scene.load(row[4]);
        scene.state = { root: config[0], overrideHead: config[1] };
        // 투영의 정수점/변경할 next 는 완성 응답 입력이다.
        if (row[5] !== '-') {
            for (const item of row[5].split(';')) {
                const values = item.split(',').map(Number);
                scene.anchors.set(values[0], values.slice(1));
            }
        }
        const collector = new GumpCollection(scene.state, scene.hooks());
        const target = config[2];
        const limit = config[3];
        const view = { left: config[4], top: config[5], right: config[6], bottom: config[7] };
        const guarded = new Uint8Array(34 * 16).fill(0xa5);
        const work = guarded.subarray(16, 16 + 32 * 16);
        let result;
        if (row[1] === 'Position') { const point = collector.position(target); result = `${point.x},${point.y}`; }
        else if (row[1] === 'Bounds') { const rect = collector.bounds(target); result = `${rect.left},${rect.top},${rect.right},${rect.bottom}`; }
        else result = String(row[1] === 'Collect' ? collector.collect(view, work, limit) : collector.collectNode(target, view, work, limit));
        const output = hex(work);
        const guard = hex(guarded.subarray(0, 16)) + hex(guarded.subarray(33 * 16, 34 * 16));
        const label = `Gump ${row[0]} ${row[1]} ${row[2]}`;
        assert.equal(result, row[6], label);
        assert.equal(output, row[7], label);
        assert.equal(scene.caches(), row[8], label);
        assert.equal(scene.projected === '' ? '-' : scene.projected, row[9], label);
        assert.equal(guard, row[11].slice(0, 64), label);
        replayed += 1;
    }
    assert.equal(replayed, 389);
});

test('GumpCollection_PerRootLimitAndSafeBuffer: limit 은 최상위별 제한이고 실제 버퍼를 넘는 쓰기는 거부한다', () => {
    const scene = new GumpScene();
    scene.state = { root: 1, overrideHead: 0 };
    scene.make(1, { child: 2 });
    scene.make(2, { flags: 1, next: 3, parent: 1, depth: 4, width: 10, height: 10, x: 1, y: 1 });
    scene.make(3, { flags: 1, parent: 1, depth: 5, width: 10, height: 10, x: 20, y: 20 });
    const collector = new GumpCollection(scene.state, scene.hooks());
    const work = new Uint8Array(2 * 16);
    assert.equal(collector.collect({ left: 0, top: 0, right: 100, bottom: 100 }, work, 1), 2);
    const view = new DataView(work.buffer);
    assert.equal(view.getUint32(8, true), 2);
    assert.equal(view.getUint32(16 + 8, true), 3);
    assert.ok(throwsOf(() => collector.collect({ left: 0, top: 0, right: 100, bottom: 100 }, work.subarray(0, 16), 1)));
    assert.equal(collector.collect({ left: 0, top: 0, right: 100, bottom: 100 }, new Uint8Array(0), 0), 0);
    assert.equal(collector.collectNode(999, { left: 0, top: 0, right: 0, bottom: 0 }, new Uint8Array(0), 0), 0);
});

test('GumpCollection_RejectsDamagedLinksAndMissingProjection: 순환·누락·잘못된 월드 부모는 유효 원본 포인터 계약 밖이므로 거부한다', () => {
    const scene = new GumpScene();
    scene.state = { root: 1, overrideHead: 0 };
    scene.make(1, { child: 2 });
    scene.make(2, { flags: 1, parent: 1, width: 10, height: 10, x: 1, y: 1 });
    const collector = new GumpCollection(scene.state, scene.hooks());
    const work = new Uint8Array(8 * 16);
    const view = { left: 0, top: 0, right: 100, bottom: 100 };
    assert.throws(() => new GumpCollection(scene.state, {}), TypeError);
    assert.ok(throwsOf(() => collector.position(999)));
    scene.nodes.get(2).parent = 2;
    assert.ok(throwsOf(() => collector.position(2)));
    scene.nodes.get(2).parent = 1;
    scene.nodes.get(2).next = 2;
    assert.ok(throwsOf(() => collector.collect(view, work, 8)));
    scene.nodes.get(2).next = 0;
    scene.nodes.get(2).child = 2;
    assert.ok(throwsOf(() => collector.collect(view, work, 8)));
    scene.nodes.get(2).child = 0;
    scene.nodes.get(2).flags = 0x2001;
    scene.nodes.get(2).parent = 999;
    assert.ok(throwsOf(() => collector.position(2)));
    scene.nodes.get(2).parent = 1;
    const missing = new GumpCollection(scene.state, { node: scene.hooks().node });
    assert.ok(throwsOf(() => missing.position(2)));
});

test('GumpCollection_CommonSubmissionIntegration: 공통 UI 슬롯을 실제 Gump 트리에 연결해 순차 병합·정렬·kind 2 제출과 UI 전용 dirty 를 확인한다', () => {
    const scene = new GumpScene();
    scene.state = { root: 1, overrideHead: 0 };
    scene.make(1, { child: 2 });
    scene.make(2, { flags: 1, next: 3, parent: 1, depth: 3, width: 10, height: 10, x: 1, y: 1 });
    scene.make(3, { flags: 1, parent: 1, depth: 7, width: 10, height: 10, x: 20, y: 20 });
    const collector = new GumpCollection(scene.state, scene.hooks());
    const hooks = {};
    bindGumpCollection(hooks, collector);
    let collected = false;
    // 건설 표시 수집기는 항목 하나(x 0·y 0·token 8·depth 5·kind 3·tail 19)를 쓴다.
    hooks.construction = (_visible, output) => {
        collected = true;
        const view = new DataView(output.buffer, output.byteOffset, 16);
        view.setFloat32(0, 0, true);
        view.setFloat32(4, 0, true);
        view.setUint32(8, 8, true);
        view.setInt16(12, 5, true);
        output[14] = 3;
        output[15] = 19;
        return 1;
    };
    const work = new Uint8Array(8 * 16);
    const request = { screen: { left: 0, top: 0, right: 100, bottom: 100 }, visible: { left: 0, top: 0, right: 0, bottom: 0 }, unclipped: { left: 0, top: 0, right: 0, bottom: 0 }, dirtyFlags: 0, groupRowLimit: 0 };
    let result = collectDrawEntries(request, work, hooks);
    assert.equal(result.count, 3);
    assert.ok(collected);
    const entries = work.subarray(0, 3 * 16);
    sortRenderEntries(entries);
    const drawn = [];
    const dispatch = { ui: (token) => drawn.push(token), construction: (token) => drawn.push(token) };
    dispatchDrawEntries(entries, {}, dispatch);
    assert.deepEqual(drawn, [3, 8, 2]);
    assert.equal(work[14], 2);
    assert.equal(work[15], 0);
    assert.equal(work[16 + 15], 19);
    collected = false;
    request.dirtyFlags = 4;
    result = collectDrawEntries(request, work, hooks);
    assert.equal(result.count, 2);
    assert.equal(collected, false);
});

// ---- 직접 계산 시험: 위치/경계/로컬 변환/수집 규칙 (fixture 가 다루지 않는 경로, 2026-11-13 WB04-E395) ----

// 루트 1 아래 2(10.5,20.25), 그 아래 3(1.25,2.5)인 작은 트리를 만든다. 투영은 호출 목록을 기록한다. (2026-11-13 WB04-E395)
function smallTree() {
    const scene = new GumpScene();
    scene.state = { root: 1, overrideHead: 0 };
    scene.make(1, { child: 2 });
    scene.make(2, { flags: 1, parent: 1, child: 3, x: 10.5, y: 20.25, width: 10.9, height: 5.5, depth: 4 });
    scene.make(3, { flags: 1, parent: 2, x: 1.25, y: 2.5, width: 2, height: 2, depth: 5 });
    return scene;
}

test('GumpCollection 위치/경계: 부모 체인을 float32 로 더해 절삭하고 일반 노드만 캐시를 갱신하며 부모 자신의 캐시는 건드리지 않는다', () => {
    const scene = smallTree();
    const collector = new GumpCollection(scene.state, scene.hooks());
    // 노드 3: (10.5+1.25, 20.25+2.5) = (11.75, 22.75) → 절삭 (11, 22). 노드 2 의 캐시는 그대로이다.
    assert.deepEqual(collector.position(3), { x: 11, y: 22 });
    assert.deepEqual(scene.nodes.get(3).cached, { x: 11, y: 22 });
    assert.deepEqual(scene.nodes.get(2).cached, { x: 0, y: 0 });
    // 반환값을 바꿔도 노드 캐시는 바뀌지 않는다.
    const returned = collector.position(3);
    returned.x = 999;
    assert.equal(scene.nodes.get(3).cached.x, 11);
    // 노드 2: (10.5, 20.25) → (10, 20), 경계는 정수 위치에 크기를 더해 절삭: (10+10.9, 20+5.5) → (20, 25).
    assert.deepEqual(collector.bounds(2), { left: 10, top: 20, right: 20, bottom: 25 });
    // 음수 위치는 0 쪽으로 절삭한다.
    scene.nodes.get(3).x = -1.5;
    scene.nodes.get(3).y = -0.25;
    assert.deepEqual(collector.position(3), { x: 9, y: 20 });
    // 비유한 위치는 CRT 불확정 정수의 하위 0 이다.
    scene.nodes.get(3).x = Infinity;
    scene.nodes.get(3).y = NaN;
    assert.deepEqual(collector.position(3), { x: 0, y: 0 });
    // 2^63 이상(또는 −2^63 미만)도 0 이고 −2^63 은 하위 DWORD 0 이다.
    scene.nodes.get(3).x = 2 ** 63;
    scene.nodes.get(3).y = -(2 ** 63);
    assert.deepEqual(collector.position(3), { x: 0, y: 0 });
    // 2^40+2^20 은 float32 로 정확하고 하위 DWORD 가 2^20 이다(해당 값에 부모 위치를 더한 float32 의 절삭).
    scene.nodes.get(2).x = 0;
    scene.nodes.get(2).y = 0;
    scene.nodes.get(3).x = 2 ** 40 + 2 ** 20;
    scene.nodes.get(3).y = 0;
    assert.deepEqual(collector.position(3), { x: 2 ** 20, y: 0 });
});

test('GumpCollection 월드 부착: 투영점에 절삭한 로컬 위치를 DWORD 덧셈하고 자신의 캐시는 보존하며 자식은 투영점을 부모로 더한다', () => {
    const scene = smallTree();
    scene.nodes.get(2).flags = 0x2001;
    scene.nodes.get(2).object = 77;
    scene.nodes.get(2).x = 1.5;
    scene.nodes.get(2).y = -1.5;
    scene.anchors.set(77, [100, 200, 0, 0]);
    const collector = new GumpCollection(scene.state, scene.hooks());
    // 월드 부착 자신: (100+1, 200+(-1)) = (101, 199). 캐시는 갱신하지 않는다.
    assert.deepEqual(collector.position(2), { x: 101, y: 199 });
    assert.deepEqual(scene.nodes.get(2).cached, { x: 0, y: 0 });
    assert.equal(scene.projected, '77');
    // 자식 3: 부모(월드 부착)의 투영점 (101,199) 에 자기 로컬 (1.25,2.5)를 더하고 루트 위치(0)를 더한다 → (102.25, 201.5) → (102, 201).
    assert.deepEqual(collector.position(3), { x: 102, y: 201 });
    // DWORD 덧셈 감김: 투영점 INT_MAX 에 로컬 1 을 더하면 INT_MIN 이다.
    scene.anchors.set(77, [2147483647, 0, 0, 0]);
    scene.nodes.get(2).x = 1;
    scene.nodes.get(2).y = 0;
    assert.equal(collector.position(2).x, -2147483648);
    // 월드 부착은 반드시 루트의 직계 자식이어야 한다.
    scene.nodes.get(2).parent = 3;
    assert.throws(() => collector.position(2), /루트\/투영/);
});

test('GumpCollection 화면→로컬 변환: 루트 자신의 위치는 빼지 않고 각 단계를 float32 로 저장하며 월드 부착 부모 이후 일반 부모의 위치는 쓰지 않는다', () => {
    const scene = smallTree();
    const collector = new GumpCollection(scene.state, scene.hooks());
    // 일반 체인: (100,200) − 노드 3 (1.25,2.5) − 노드 2 (10.5,20.25) = (88.25, 177.25). 루트(1)는 부모가 없어 위치를 빼지 않는다.
    assert.deepEqual(collector.screenToLocal(3, { x: 100, y: 200 }), [88.25, 177.25]);
    assert.deepEqual(collector.screenToLocal(2, { x: 100, y: 200 }), [89.5, 179.75]);
    assert.deepEqual(collector.screenToLocal(1, { x: 100, y: 200 }), [100, 200]);
    // 위치 캐시는 갱신하지 않는다.
    assert.deepEqual(scene.nodes.get(3).cached, { x: 0, y: 0 });
    // 정수 좌표는 float32 로 먼저 저장한다: 16777217 → 16777216.
    assert.deepEqual(collector.screenToLocal(1, { x: 16777217, y: -16777217 }), [16777216, -16777216]);
    // 월드 부착 부모: 투영점 (51,62) = (50,60)+(1,2) 를 빼고 그 위의 일반 부모 위치는 쓰지 않는다.
    scene.nodes.get(2).flags = 0x2001;
    scene.nodes.get(2).object = 9;
    scene.nodes.get(2).x = 1.5;
    scene.nodes.get(2).y = 2.5;
    scene.anchors.set(9, [50, 60, 0, 0]);
    // 노드 3: 부모 2 가 월드 부착 → (100−51, 200−62) = (49, 138) 이고 이후 노드 2 의 부모(루트)는 일반이라 위치를 빼지 않는다.
    assert.deepEqual(collector.screenToLocal(3, { x: 100, y: 200 }), [49, 138]);
    // 노드 2 자신(부모 루트는 일반)은 자기 로컬 위치를 뺀다.
    assert.deepEqual(collector.screenToLocal(2, { x: 100, y: 200 }), [98.5, 197.5]);
    // 손상된 링크는 거부한다.
    scene.nodes.get(2).parent = 3;
    scene.nodes.get(2).flags = 1;
    assert.throws(() => collector.screenToLocal(3, { x: 0, y: 0 }), /순환/);
    assert.throws(() => collector.screenToLocal(999, { x: 0, y: 0 }), /없음/);
    assert.equal(collector.flags(3), 1);
    assert.throws(() => collector.flags(999), /없음/);
});

test('GumpCollection 수집 규칙: override 머리·삭제 대기 최상위 제외·통과 플래그·경계 교차(맞닿음 제외, 내부 영크기 허용)·깊이 부호·최상위별 한도', () => {
    const scene = new GumpScene();
    scene.state = { root: 1, overrideHead: 0 };
    scene.make(1, { child: 2 });
    // 2: dead 최상위(제외), 3: 보이지 않는 최상위(제외), 4: 화면 밖이지만 통과 플래그, 5: 안쪽 영크기, 6: 맞닿음.
    scene.make(2, { flags: 0x10001, next: 3, parent: 1, x: 1, y: 1, width: 5, height: 5 });
    scene.make(3, { flags: 0, next: 4, parent: 1, x: 1, y: 1, width: 5, height: 5 });
    scene.make(4, { flags: 0x21, next: 5, parent: 1, child: 7, x: 500, y: 500, width: 5, height: 5, depth: 1 });
    scene.make(5, { flags: 1, next: 6, parent: 1, x: 50, y: 60, width: 0, height: 0, depth: 0xfffe });
    scene.make(6, { flags: 1, parent: 1, x: 100, y: 10, width: 5, height: 5, depth: 3 });
    scene.make(7, { flags: 0x10001, parent: 4, x: 1, y: 1, width: 3, height: 3, depth: 9 });
    const collector = new GumpCollection(scene.state, scene.hooks());
    const view = { left: 0, top: 0, right: 100, bottom: 100 };
    const out = new Uint8Array(8 * 16);
    // 출력 앞 count 개 항목의 토큰(+8 DWORD)을 순서대로 읽는다.
    const tokens = (count) => Array.from({ length: count }, (_, index) => new DataView(out.buffer, index * 16, 16).getUint32(8, true));
    // 4 는 화면 밖이라 자신은 쓰지 않지만 통과 플래그로 자식 7 을 본다(자식에는 dead 필터가 없고 자식 위치는 부모 위치(500,500)에 더해져 화면 밖이라 쓰이지 않는다).
    // 5 는 안쪽 영크기라 쓰고, 6 은 left=100 이 view.right=100 과 맞닿아 제외한다.
    assert.equal(collector.collect(view, out, 8), 1);
    assert.deepEqual(tokens(1), [5]);
    // 깊이는 WORD 로 저장한다(0xfffe → 읽으면 −2).
    assert.equal(new DataView(out.buffer, 0, 16).getInt16(12, true), -2);
    // 자식 7 은 화면 안으로 옮기면 dead 여도 쓴다.
    scene.nodes.get(7).x = -499;
    scene.nodes.get(7).y = -499;
    out.fill(0);
    assert.equal(collector.collect(view, out, 8), 2);
    assert.deepEqual(tokens(2), [7, 5]);
    // override 머리가 있으면 root.child 대신 그 머리부터 형제를 훑는다.
    scene.state.overrideHead = 6;
    out.fill(0);
    assert.equal(collector.collect({ left: 0, top: 0, right: 200, bottom: 100 }, out, 8), 1);
    assert.deepEqual(tokens(1), [6]);
    scene.state.overrideHead = 0;
    // collectNode 는 지정 노드의 visible/dead 를 직접 검사하지 않는다: dead 인 7 도 경계 안이면 쓴다.
    out.fill(0);
    assert.equal(collector.collectNode(7, view, out, 8), 1);
    assert.deepEqual(tokens(1), [7]);
    // 한도: 자신이 한도를 소모하면 자식은 수집하지 않는다.
    scene.nodes.get(2).flags = 1;
    scene.nodes.get(2).child = 3;
    scene.nodes.get(3).flags = 1;
    scene.nodes.get(3).parent = 2;
    scene.nodes.get(3).next = 0;
    out.fill(0);
    assert.equal(collector.collectNode(2, view, out, 1), 1);
    assert.deepEqual(tokens(1), [2]);
    assert.equal(collector.collectNode(2, view, out, 2), 2);
    assert.deepEqual(tokens(2), [2, 3]);
    assert.equal(collector.collectNode(2, view, out, 0), 0);
    // 출력 칸이 모자라면 거부한다.
    assert.throws(() => collector.collectNode(2, view, out.subarray(0, 16), 2), RangeError);
});

test('GumpCollection 경계 절삭 범위: 합이 ±2^63 밖이면 비영 하위 DWORD 가 있어도 0 이고 안이면 하위 DWORD 를 쓴다', () => {
    const scene = smallTree();
    const collector = new GumpCollection(scene.state, scene.hooks());
    scene.nodes.get(2).x = 2 ** 30;
    scene.nodes.get(2).y = 0;
    // 위치 1073741824 + 폭 2^63 = 2^63+2^30(double 로 정확) → 범위 밖이라 0.
    scene.nodes.get(2).width = 2 ** 63;
    assert.equal(collector.bounds(2).right, 0);
    // 폭 2^62: 합 2^62+2^30 은 범위 안이고 하위 DWORD 는 2^30 이다.
    scene.nodes.get(2).width = 2 ** 62;
    assert.equal(collector.bounds(2).right, 2 ** 30);
    // 폭 −2^64: 합 −2^64+2^30 은 범위 밖이라 0. 폭 −2^63: 합 −2^63+2^30 은 범위 안이고 하위 DWORD 2^30.
    scene.nodes.get(2).width = -(2 ** 64);
    assert.equal(collector.bounds(2).right, 0);
    scene.nodes.get(2).width = -(2 ** 63);
    assert.equal(collector.bounds(2).right, 2 ** 30);
});

test('GumpCollection 교차 판정: 영크기 view 가 노드 안에 있으면 교차하고 view 가 역전되면 교차하지 않으며 경로/형제 한도는 1000개이다', () => {
    const scene = smallTree();
    const collector = new GumpCollection(scene.state, scene.hooks());
    const out = new Uint8Array(4 * 16);
    // 노드 2 의 경계 (10,20)~(20,25) 안의 한 점 view 는 교차한다(좌표 영크기 view 허용).
    assert.equal(collector.collectNode(2, { left: 15, top: 22, right: 15, bottom: 22 }, out, 1), 1);
    assert.equal(collector.collectNode(2, { left: 15, top: 22, right: 15, bottom: 23 }, out, 1), 1);
    assert.equal(collector.collectNode(2, { left: 15, top: 22, right: 16, bottom: 22 }, out, 1), 1);
    // 역전된 view 는 교차하지 않는다.
    assert.equal(collector.collectNode(2, { left: 16, top: 22, right: 15, bottom: 23 }, out, 1), 0);
    assert.equal(collector.collectNode(2, { left: 15, top: 23, right: 16, bottom: 22 }, out, 1), 0);
    // 형제 체인 1000개는 허용하고 1001개는 순환/한도 오류이다(화면 밖이라 항목은 쓰지 않는다).
    const chain = new GumpScene();
    chain.state = { root: 1, overrideHead: 0 };
    chain.make(1, { child: 2 });
    // 토큰 2..1002 를 next 로 이은 화면 밖 형제 1001개를 만든다(마지막만 next 0).
    for (let index = 0; index < 1001; index += 1) chain.make(2 + index, { flags: 1, parent: 1, next: index < 1000 ? 3 + index : 0, x: 5000, y: 5000, width: 1, height: 1 });
    const chained = new GumpCollection(chain.state, chain.hooks());
    const view = { left: 0, top: 0, right: 100, bottom: 100 };
    chain.nodes.get(1001).next = 0;
    assert.equal(chained.collect(view, new Uint8Array(16), 8), 0);
    chain.nodes.get(1001).next = 1002;
    assert.throws(() => chained.collect(view, new Uint8Array(16), 8), /순환|한도/);
});
