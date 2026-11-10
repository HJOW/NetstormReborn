// 공통 후처리 squid-postpop.js(SquidPostPop)가 실제 SquidDisplay·DirtyRegions 와 함께 도는 원본 10.78 postpop-x86.tsv 의 표시 dirty 열을 대조한다(WB04-E258).
//  - postpop-x86.tsv `originals` 384 PostPop·72 Pop·36 Unpop 행: 비용/알림/깊이·Totalmade 세 표·목록·슬롯 전체(E41 이 이미 대조)에 더해 마지막 두 열 —
//    전체 갱신 여부(DirtyRegions.fullRedraw)와 변경 사각형 목록(왼쪽·위·오른쪽·아래·flags 다섯 값씩, 순서와 병합 포함)을 대조한다.
//    장면은 cpppj `SquidPostPop_X86_CostListsTotalsAndActiveLifecycle` 과 같다: 모든 TYPE 의 SHP 는 프레임 하나(loaded)이며 프레임 크기 {11,17,12,16}·{19,23,-3,5}, 카메라 0/0 배율 65536, 화면 640×480.
// E41 의 squid-postpop.test.mjs 는 표시 없이 장부만 재생했으므로(표시 열은 이 단위에서 대조) 같은 fixture 를 표시 장치까지 연결해 다시 재생한다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E258)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RAW_POP_RESULT } from '../www/core/raw-squid-pop-parent.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop } from '../www/core/squid-pop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, bitsToFloat, put, readRows } from './raw-support.mjs';

// 공통 객체(TYPE 74)의 가상 표 기록값(10.78 공통 postPop 몸체)과 관측 행 수이다. (2026-11-10 WB04-E258)
const COMMON_VTABLE = 0x501dd0;
const ROWS = { begins: 2, posts: 384, pops: 72, unpops: 36 };

// 10.78 postpop 관측 행이다. 파일이 없으면 null 이다. (2026-11-10 WB04-E258)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'postpop-x86.tsv'));

// 188칸 TYPE 입력이다(HP 91, z 순서 7, 발자국 1×1). 비용은 Type 행이 정한다. (2026-11-10 WB04-E258)
const makeTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1, cost: 0, maxHitPoints: 91, zOrder: 7 }));

// 바이트를 소문자 hex 로, DWORD 목록을 리틀엔디언 바이트로 바꾼다. (2026-11-10 WB04-E258)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
function wordBytes(words) {
    const bytes = new Uint8Array(words.length * 4);
    words.forEach((word, index) => put(bytes, index * 4, word >>> 0, 4));
    return bytes;
}

// Totalmade 세 표에 넣은 오래된 DWORD 입력(원문 fixture 의 고정 시드)이다. (2026-11-10 WB04-E258)
function seedCounts(state, seed) {
    // 세 표를 서로 다른 값으로 채운다.
    for (let index = 0; index < 256; index += 1) {
        const value = (seed + Math.imul(index, 0x9e3779b1)) >>> 0;
        state.localCounts()[index] = value;
        state.localSecondaryCounts()[index] = (value + 17) >>> 0;
        state.globalCounts()[index] = (value + 34) >>> 0;
    }
}

// 변경 사각형 목록 문자열("l,t,r,b,flags;…" 또는 "-")을 숫자 목록으로 읽는다. (2026-11-10 WB04-E258)
const numbers = (text) => (text === '-' ? [] : text.replace(/[,;]/g, ' ').split(/\s+/).filter(Boolean).map(Number));

// 한 결과 행(장부 + 표시 dirty 두 열)을 대조한다. (2026-11-10 WB04-E258)
function checkResult(row, start, state, pool, sid, dirty, label) {
    assert.equal(state.totalCost, Number(row[start]), `${label} 비용`);
    assert.equal(state.productionDirty, Number(row[start + 1]), `${label} 알림`);
    assert.equal(state.depth, Number(row[start + 2]), `${label} 깊이`);
    [state.localCounts(), state.localSecondaryCounts(), state.globalCounts()].forEach((table, index) => {
        assert.equal(adler32(wordBytes(table)), Number(row[start + 3 + index]), `${label} 표${index}`);
    });
    const counts = numbers(row[start + 6]);
    assert.equal(counts.length, 11);
    assert.equal(state.providers.count, counts[0], `${label} 공급원 수`);
    assert.equal(state.factories.count, counts[1], `${label} 공장 수`);
    const data = [...state.providers.entries, ...state.factories.entries];
    // 중립 owner 0 의 보존 메모리와 모든 소유자 목록을 포함한다.
    state.ownerFactories.forEach((list, index) => {
        assert.equal(list.count, counts[index + 2], `${label} owner${index} 공장 수`);
        data.push(...list.entries);
    });
    assert.equal(adler32(wordBytes(data)), Number(row[start + 7]), `${label} 목록`);
    assert.equal(hex(pool.slot(sid)), row[start + 8], `${label} 슬롯`);
    // 표시 열: 전체 갱신이면 후속 표시는 생략하고, 변경 사각형은 병합/순서/flags 까지 같아야 한다.
    assert.equal(dirty.fullRedraw, Number(row[start + 9]) !== 0, `${label} 전체 갱신`);
    const expected = numbers(row[start + 10]);
    assert.equal(expected.length, dirty.entries.length * 5, `${label} 사각형 수`);
    dirty.entries.forEach((entry, index) => {
        const n = index * 5;
        assert.deepEqual([entry.rect.left, entry.rect.top, entry.rect.right, entry.rect.bottom, entry.flags], expected.slice(n, n + 5), `${label} 사각형 ${index}`);
    });
}

test('postpop: 비용·목록·통계 장부와 표시 dirty(전체 갱신·변경 사각형)를 Pop/Unpop/PostPop 에서 원본 10.78 관측과 대조한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const counts = { begins: 0, posts: 0, pops: 0, unpops: 0 };
    let active = false;
    let pool;
    let hash;
    let spots;
    let types;
    let shapes;
    let post;
    let pop;
    let unpop;
    let state;
    let dirty;
    let sid = 0;
    let number = 74;
    // Begin/Type/State/Fill 은 명시적인 준비 입력이며 원본 월드 로딩을 실행한 수치가 아니다.
    for (const row of rows) {
        if (row[0] === 'Begin') {
            active = row[1] === 'originals';
            if (!active) continue;
            types = makeTypes();
            shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
            spots = new Uint8Array(65536);
            pool = new SidPool(32768, { server: false });
            hash = new SquidHash();
            sid = pool.allocate(2);
            assert.equal(sid, Number(row[3]));
            counts.begins += 1;
            continue;
        }
        if (!active) continue;
        if (row[0] === 'Type') {
            number = Number(row[1]);
            Object.assign(types[number], { flags1: Number(row[2]) >>> 0, flags2: Number(row[3]) >>> 0, cost: bitsToFloat(row[4]) });
            // 모든 타입의 SHP 는 프레임 하나이며 두 칸 크기 입력을 가진다(cpppj 와 같은 명시 입력).
            shapes[number] = {
                frameCount: 1,
                loaded: true,
                frames: [[11, 17, 12, 16], [19, 23, -3, 5]].map(([width, height, hotspotX, hotspotY]) => ({ width, height, hotspotX, hotspotY, cellWidth: 0, cellHeight: 0 })),
            };
        } else if (row[0] === 'State') {
            state = new SquidPostPopState();
            const [providerCapacity, factoryCapacity, ownerCapacity, initial] = [1, 2, 3, 4].map((index) => Number(row[index]));
            dirty = new DirtyRegions(640, 480);
            state.providers.entries = Array.from({ length: providerCapacity }, (_, index) => (index === 0 ? sid : 0x60000000 + index));
            state.factories.entries = Array.from({ length: factoryCapacity }, (_, index) => (index === 0 ? sid : 0x60000000 + index));
            state.providers.count = Math.min(initial, providerCapacity);
            state.factories.count = Math.min(initial, factoryCapacity);
            // 소유자 0도 원본 메모리 보존 검사에 포함한다.
            state.ownerFactories.forEach((list, owner) => {
                list.entries = Array.from({ length: ownerCapacity }, (_, index) => 0x70000000 + owner * 100 + index);
                list.count = Math.min(initial, ownerCapacity);
            });
            state.totalCost = Number(row[5]) | 0;
            state.productionDirty = Number(row[6]) >>> 0;
            seedCounts(state, Number(row[7]) >>> 0);
            state.localOwner = Number(row[8]);
            state.graphsEnabled = Number(row[9]) !== 0;
            state.suppressed = Number(row[10]) !== 0;
            state.depth = Number(row[11]) >>> 0;
            const sink = { suppressed: () => dirty.fullRedraw, invalidate: (rect, flags) => dirty.add({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }, flags) };
            const display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
            post = new SquidPostPop(pool, types, state);
            pop = new SquidPop(pool, hash, spots, { display, postPop: post });
            unpop = new SquidUnpop(pool, hash, spots, display);
        } else if (row[0] === 'Fill') {
            hash.reset();
            const bytes = pool.physical(sid);
            bytes.fill(0);
            put(bytes, 0, COMMON_VTABLE);
            bytes[10] = number;
            bytes[11] = 4;
            put(bytes, 14, 0x41a00000);
            put(bytes, 18, 0x41a00000);
            bytes[34] = Number(row[1]);
            bytes[40] = Number(row[2]);
            bytes[30] = 17;
        } else if (row[0] === 'PostPop') {
            post.postPop(sid, Number(row[1]) >>> 0);
            checkResult(row, 2, state, pool, sid, dirty, `PostPop ${row[1]}`);
            counts.posts += 1;
        } else if (row[0] === 'Pop') {
            assert.equal(pop.pop(sid, types[number], 1, 1, bitsToFloat(row[2]), bitsToFloat(row[3]), Number(row[1]) >>> 0), RAW_POP_RESULT.registered);
            checkResult(row, 4, state, pool, sid, dirty, `Pop ${row[1]}`);
            counts.pops += 1;
        } else if (row[0] === 'Unpop') {
            unpop.unpop(sid, types[number], Number(row[1]) >>> 0);
            checkResult(row, 2, state, pool, sid, dirty, `Unpop ${row[1]}`);
            counts.unpops += 1;
        } else assert.fail(`알 수 없는 행 ${row[0]}`);
    }
    assert.deepEqual(counts, ROWS);
});
