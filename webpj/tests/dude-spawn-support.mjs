// Dude 생성/드롭 검사(WB04-E206 dude-spawn·E208 dude-drop)가 공유하는 실제 10.78 Dude 장면 장치이다. cpppj `tests/DudeSpawnSupport.h` 의 DudeAssets·DudeSink·SpawnScene 에 대응한다.
//  - dudeAssets(): 실제 보호 TYPE(정의의 maxHitPoints 포함)·FrameCodeTable Run 표·SHP 헤더. 원본 준비용 미등록 base 74 는 빈 합성 레코드다.
//  - spawnScene(edition, capacity, count, blocked): SidPool(32768, server)·SquidHash·spot·SquidPostPopState·population·DirtyRegions(1024×768) sink 의 SquidDisplay·SquidUnpop·SquidPostPop·
//    SquidPop·SquidFactory·SquidOwner·SquidFrame·RawDudePostPop(`setBody('dude')` 등록)을 조립하고 base 74 를 SID 5 로 만든 뒤 population stale 꼬리·Totalmade seed·로컬 소유자 1 을 채운다.
//  - spawnHooks(scene): cpppj 와 같은 실제 Pop 연결(SquidDisplay.frameSize + SquidPop.pop, 등록 실패는 오류)이다. sceneTail(scene, born): 공통 출력 꼬리(population·raw·장부·변경 사각형)이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E208: E206 dude-spawn 시험의 장면 조립을 드롭 시험과 공유하도록 분리)
import assert from 'node:assert/strict';
import { adler32 } from './raw-support.mjs';
import { realPlayerAssets, hex, wordAdler, concatWordAdler } from './player-scene-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash, SQUID_HASH_LEVELS } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopState, SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';
import { RawDudePostPop, createDudeSpawnState, makeDudePostPopHooks } from '../www/core/raw-dude-spawn.js';

// 188칸 빈 TYPE 레코드이다(cpppj RiftTypeRecord 기본값). (2026-11-09 WB04-E206, E208 분리)
export const emptyRecord = (number) => ({ number, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0, cost: 0, zOrder: 0, constructorAddress: 0 });

// 한 번만 만든 실제 Dude 장면 자료이다. (2026-11-09 WB04-E206, E208 분리)
let cachedAssets = null;

// 실제 보호 TYPE(최대 HP 포함)·Run 표·SHP 헤더이다. 반환 배열은 공유하므로 바꿀 때는 사본을 만든다(cpppj DudeAssets). (2026-11-09 WB04-E206, E208 분리)
export function dudeAssets() {
    if (cachedAssets) return cachedAssets;
    const player = realPlayerAssets();
    const capture = realCaptureAssets();
    const types = player.types.map((record, number) => ({ ...record, maxHitPoints: capture.types[number].maxHitPoints }));
    types[74] = emptyRecord(74);
    // 자산 번호 70 부터 실제 프레임 Run 을 채운다.
    const frames = types.map((_, number) => {
        const definition = number >= 70 ? capture.table.definitions[number - 70] : null;
        return definition ? FrameCodeTable.fromDefinition(definition) : new FrameCodeTable([]);
    });
    assert.deepEqual([types[70].footX, types[70].footY, types[70].maxHitPoints, frames[70].run('I').first, capture.shapes[70].frameCount], [0, 0, 20, 64, 65]);
    cachedAssets = { types, frames, shapes: capture.shapes };
    return cachedAssets;
}

// 원본 상태와 실제 모든 생성/공간/표시/후처리 처리기를 소유한 장면이다(cpppj SpawnScene). capacity/count/blocked 는 population 입력이다. (2026-11-09 WB04-E206, E208 분리)
export function spawnScene(capacity, count, blocked) {
    const assets = dudeAssets();
    const { types, frames, shapes } = assets;
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const book = new SquidPostPopState();
    const population = new SquidPostPopList();
    const state = createDudeSpawnState(population, { blocked });
    const dirty = new DirtyRegions(1024, 768);
    const sink = { suppressed: () => dirty.fullRedraw, invalidate: (rect, flags) => dirty.add({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }, flags) };
    const display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 1024, bottom: 768 } });
    const unpop = new SquidUnpop(pool, hash, spots, display);
    const post = new SquidPostPop(pool, types, book);
    const pop = new SquidPop(pool, hash, spots, { display, postPop: post });
    const factory = new SquidFactory(pool, types, { unpop });
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const frame = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, unpop, pop));
    const dudePost = new RawDudePostPop(pool, frames, state, makeDudePostPopHooks(pool, frame, post));
    assert.equal(factory.create(74, 2), 5);
    population.entries = Array.from({ length: capacity }, (_, index) => (0x60000000 + index) >>> 0);
    population.count = count;
    book.localOwner = 1;
    // 원본 seed 0 과 각 표의 서로 다른 17 오프셋을 유지한다.
    [book.localCounts(), book.localSecondaryCounts(), book.globalCounts()].forEach((table, which) => {
        // 표 전체 칸을 같은 식으로 채운다.
        for (let index = 0; index < table.length; index += 1) table[index] = (Math.imul(index, 2654435761) + which * 17) >>> 0;
    });
    post.setBody('dude', dudePost);
    return { assets, pool, hash, spots, book, population, state, dirty, display, unpop, post, pop, factory, owner, frame, dudePost, types };
}

// 실제 가상 +90 Pop 연결이다(현재 SHP 프레임 0 크기로 등록, 등록 실패는 오류). (2026-11-09 WB04-E206, E208 분리)
export function spawnHooks(scene) {
    const { pool, display, pop, types } = scene;
    return {
        pop: (sid, x, y, flags) => {
            const raw = pool.slot(sid);
            const size = display.frameSize(raw, 0);
            if (pop.pop(sid, types[raw[10]], size[0], size[1], x, y, flags) !== RAW_POP_RESULT.registered) throw new Error('Dude 실제 Pop 등록 실패');
        },
    };
}

// 공통 출력의 population·슬롯·장부·변경 사각형 부분을 원본 순서로 직렬화한다. heads 는 해시 머리 Adler 를 넣을지 여부다. (2026-11-09 WB04-E208)
export function sceneTail(scene, born, { heads = true } = {}) {
    const populationText = scene.population.entries.length ? scene.population.entries.join(',') : '-';
    const counts = ['0', '0', ...scene.book.ownerFactories.map((list) => String(list.count))].join(',');
    const rectangles = scene.dirty.entries.map((entry) => `${entry.rect.left},${entry.rect.top},${entry.rect.right},${entry.rect.bottom},${entry.flags}`).join(';') || '-';
    const raw = hex(scene.pool.slot(born || 6));
    const space = heads ? [String(concatWordAdler(Array.from({ length: SQUID_HASH_LEVELS }, (_, level) => scene.hash.entries(level)), 2)), String(adler32(scene.spots))] : [];
    return {
        populationText,
        raw,
        tail: [...space, String(scene.book.totalCost), String(scene.book.productionDirty), String(scene.book.depth), String(wordAdler(scene.book.localCounts(), 4)),
            String(wordAdler(scene.book.localSecondaryCounts(), 4)), String(wordAdler(scene.book.globalCounts(), 4)), counts, '1', raw, scene.dirty.fullRedraw ? '1' : '0', rectangles],
    };
}
