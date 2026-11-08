// 불꽃 꼬리 표시 시스템 fiery-tail-visual-system.js(FieryTailVisualSystem)를 건설 표시 시스템과 같은 공통 표시 저장소·실제 Host/Kernel 위에서 검사한다(cpppj `tests/FieryTailVisualSystemTests.cpp`).
//  - 공유 목록/Kernel(토큰 1·2·3, 10입자, 공통 수집 3항목과 깊이 −25, 인접 색 DWORD 와 끝점 326/230 의 선 10개, 슬롯 재사용, 시간 조각 진행, 시스템 정리 순서와 반납),
//    부착 뒤 확보 실패·생성 중 재진입·dead 부모 실패의 잔재 없음, 부모 종속 삭제의 등록/Form 정리와 00487960 삭제 dirty, 수집 뒤 오래된 토큰과 배율 불일치 거부를 그대로 옮겼다.
//  - cpppj 첫 시험은 client `DrawVisibleScene`(DrawSubmission, PORT_MAP todo)으로 수집·정렬·출력을 한 번에 한다. 웹은 그 client 단위가 아직 없으므로 같은 관측을 기존 부품으로 조립했다:
//    공통 수집(`RenderProcessSystem.collect`) → 원본 정렬(`render/draw-sort.js` sortDrawEntries) → 토큰 출력(`drawToken`). DrawSubmission 자체의 이식/대조는 그 client 단위에서 한다.
//  - 00487960 삭제 영역 fixture 재생은 E192 `smoke-visual-system.test.mjs` 에 있다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E195: 불꽃 꼬리 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { createConstructionVisualState } from '../www/core/raw-construction-visual.js';
import { createConstructionFrameState } from '../www/core/raw-construction-frame.js';
import { RENDER_ENTRY_BYTES } from '../www/core/raw-render-collection.js';
import { FieryTailRelease } from '../www/core/raw-fiery-tail-lifetime.js';
import { RenderProcessSystem, renderProcessRemovalBounds } from '../www/core/render-process-system.js';
import { ConstructionVisualSystem } from '../www/core/construction-visual-system.js';
import { FieryTailVisualSystem } from '../www/core/fiery-tail-visual-system.js';
import { sortDrawEntries } from '../www/render/draw-sort.js';

// 인접 DWORD 는 실제 게임 heap 관찰값이 아닌 이 통합 검사의 명시 입력이다. (2026-11-09 WB04-E195)
const EXPLICIT_ADJACENT = 0xabcdef01;

// 내장 Form 생성자 주소와 합성 HP 보유 부모 타입 100 을 넣은 188칸 입력이다(cpppj VisualTypes). (2026-11-09 WB04-E195)
function visualTypes() {
    const result = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    result[100] = { ...result[100], flags1: 0x10, maxHitPoints: 100, footX: 1, footY: 1 };
    return result;
}

// 예외가 나는지 본다. (2026-11-09 WB04-E195)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 실제 raw 풀/공통 삭제/Kernel/Host 를 쓰며 장치 요청만 관찰하는 장면이다(cpppj VisualScene). (2026-11-09 WB04-E195)
class VisualScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = visualTypes();
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        // 공간/화면의 합성 부모 경계만 완료하고 Form 삭제는 실제 Host 가 처리한다.
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock, {
            asset: {
                selected: () => 0,
                emit: (event) => {
                    if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) this.destroy.completePreDestroy();
                    if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.destroy.completePostDestroy();
                }
            }
        });
        this.random = new GameRandom();
        this.visual = createConstructionVisualState();
        this.constructionClock = createConstructionFrameState();
        this.fieryClock = { now: 0, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
        // 색수 11 뒤 DWORD 까지 명시 제공한다.
        this.colors = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, EXPLICIT_ADJACENT];
        this.constructionPalette = { token: 71, count: 11, entries: this.colors };
        this.fieryLifetime = { palette: 77 };
        this.fieryPalette = { count: 11, readable: this.colors };
        this.dirty = [];
        this.lines = [];
        this.releases = [];
        this.allocations = [];
        this.draws = [];
        this.failParticles = false;
        this.probeCreation = false;
        this.probes = 0;
        this.parent = this.pool.allocate(0);
        const raw = this.pool.allocated(this.parent);
        put(raw, 0, 0x501dd0);
        raw[10] = 100;
        raw[11] = 0;
        raw[34] = 1;
        raw[35] = 255;
        raw[40] = 1;
        put(raw, 14, floatToBits(20.5));
        put(raw, 18, floatToBits(21.25));
        put(raw, 26, 50);
        this.visual.palette = 71;
        this.constructionClock.now = 2.0;
        this.fieryClock.now = 2.0;
        this.rendering = new RenderProcessSystem(this.pool, this.host, this.visual, { reserve: (bytes) => { this.allocations.push(bytes); return true; }, releaseArray: () => {} });
        this.construction = new ConstructionVisualSystem(this.rendering, this.constructionClock, this.constructionPalette, this.random, this.constructionHooks(), this.constructionFrameHooks(), () => {});
        this.fiery = new FieryTailVisualSystem(this.rendering, this.random, this.fieryLifetime, this.fieryClock, this.fieryPalette, this.fieryHooks());
    }

    // 실제 생성/Seed 몸체에 작은 명시 캡처를 공급한다. 공유 색 표 71 은 이미 있다. (2026-11-09 WB04-E195)
    constructionHooks() {
        return {
            reserve: () => true, createPalette: () => 71, gradient: () => {}, buildPalette: () => {}, releaseParticles: () => {},
            capture: () => ({ width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) })
        };
    }

    // HP/장치만 명시 입력/관찰이며 건설 프레임·삭제 몸체는 실제 모듈이다. (2026-11-09 WB04-E195)
    constructionFrameHooks() {
        return { maxHitPoints: () => 100, invalidate: () => {}, draw: () => this.draws.push('C') };
    }

    // 새 효과를 프레임/출력 없이 등록하는 빈 성공 경계는 쓰지 않는다. (2026-11-09 WB04-E195)
    fieryHooks() {
        return {
            lifetime: {
                reserve: (bytes) => {
                    assert.equal(bytes, 2880);
                    if (this.probeCreation) {
                        this.probes += 1;
                        const before = this.random.state;
                        this.kernel.runFrame();
                        this.rendering.draw();
                        const work = new Uint8Array(RENDER_ENTRY_BYTES * 2);
                        assert.ok(this.random.state === before && this.rendering.collect({ left: 0, top: 0, right: 256, bottom: 256 }, work, 2) === 0 && this.lines.length === 0);
                        assert.ok(throwsOf(() => this.fiery.add(this.parent)));
                    }
                    return !this.failParticles;
                }
            },
            frame: {
                invalidate: (rect) => this.dirty.push(rect),
                line: (line) => { this.lines.push(line); this.draws.push('F'); }
            },
            endpoint: (sid) => { assert.equal(sid, this.parent); return [326, 230]; },
            release: (released) => this.releases.push(released)
        };
    }

    // 공통 수집 → 원본 정렬 → 토큰 출력으로 client DrawVisibleScene 의 표시 부분을 조립한다(시험 쪽 조립, DrawSubmission 미이식). (2026-11-09 WB04-E195)
    drawVisible(view, capacity) {
        const work = new Uint8Array(RENDER_ENTRY_BYTES * capacity);
        const count = this.rendering.collect(view, work, capacity);
        const data = new DataView(work.buffer);
        const entries = Array.from({ length: count }, (_, index) => {
            const base = index * RENDER_ENTRY_BYTES;
            return { x: data.getFloat32(base, true), y: data.getFloat32(base + 4, true), token: data.getUint32(base + 8, true), depth: data.getInt16(base + 12, true), kind: data.getUint8(base + 14) };
        });
        sortDrawEntries(entries);
        for (const entry of entries) this.rendering.drawToken(entry.token);
        return entries;
    }
}

test('불꽃 꼬리 표시 시스템: 건설 표시와 하나의 번호/등록 배열·실제 Kernel·공통 수집/깊이 정렬·슬롯 재사용을 공유한다', () => {
    const scene = new VisualScene();
    const first = scene.construction.add(scene.parent, false);
    const tail = scene.fiery.add(scene.parent);
    const second = scene.construction.add(scene.parent, true);
    const firstToken = first.registration.token;
    const tailToken = tail.fields.registration.token;
    assert.ok(firstToken === 1 && tailToken === 2 && second.registration.token === 3);
    assert.ok(scene.rendering.size === 3 && scene.kernel.size === 3 && scene.construction.size === 2 && scene.fiery.size === 1);
    assert.ok(scene.host.fromForm(tail.form) === tail && scene.pool.slot(tail.form)[10] === 11 && tail.fields.registration.form === tail.form);
    assert.ok(tail.fields.particles.length === 120 && tail.fields.total === 0 && !tail.fields.first && tail.fields.registration.zOrder === -25);
    scene.kernel.runFrame();
    assert.ok(tail.fields.first && tail.fields.total === 10);
    const entries = scene.drawVisible({ left: 0, top: 0, right: 256, bottom: 256 }, 3);
    assert.ok(entries.length === 3 && entries[2].token === tailToken && entries[2].kind === 3 && entries[2].depth === -25);
    assert.ok(scene.lines.length === 10 && scene.draws.length === 12 && scene.draws[0] === 'C' && scene.draws[1] === 'C');
    // 최초 최대 수명이 읽는 인접 DWORD 도 명시 입력과 같아야 한다.
    for (const line of scene.lines) assert.ok(line.color === EXPLICIT_ADJACENT && line.x2 === 326 && line.y2 === 230);
    const oldForm = tail.form;
    scene.host.kill(first, 0x10);
    const replacement = scene.fiery.add(scene.parent);
    assert.ok(replacement.fields.registration.slot === 0 && replacement.fields.registration.token !== firstToken && replacement.fields.registration.token !== tailToken);
    scene.constructionClock.now = 2.1;
    scene.fieryClock.now = 2.1;
    scene.kernel.runFrame();
    assert.ok(tail.fields.total > 10 && replacement.fields.total === 10);
    scene.fiery.dispose();
    assert.ok(scene.kernel.size === 1 && scene.rendering.size === 1 && (scene.pool.slot(oldForm)[11] & 1));
    scene.construction.dispose();
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0 && get(scene.pool.slot(scene.parent), 6, 2) === 0);
    assert.ok(scene.allocations.length === 1 && scene.allocations[0] === 160 && scene.fieryLifetime.palette === 77);
    assert.deepEqual(scene.releases, [FieryTailRelease.Particles, FieryTailRelease.Object, FieryTailRelease.Particles, FieryTailRelease.Object]);
});

test('불꽃 꼬리 표시 시스템: 부착 뒤 확보 실패·생성 중 재진입·dead 부모 실패가 등록/Form/Kernel 잔재를 남기지 않는다', () => {
    const scene = new VisualScene();
    scene.failParticles = true;
    scene.probeCreation = true;
    assert.ok(throwsOf(() => scene.fiery.add(scene.parent)));
    assert.ok(scene.probes === 1 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.fiery.size === 0 && scene.rendering.slots[0] === 0);
    scene.failParticles = false;
    const alive = scene.fiery.add(scene.parent);
    assert.ok(scene.probes === 2 && alive.fields.registration.slot === 0 && alive.fields.total === 0);
    scene.probeCreation = false;
    scene.host.kill(alive, 0x10);
    scene.pool.allocated(scene.parent)[11] |= 2;
    assert.ok(throwsOf(() => scene.fiery.add(scene.parent)));
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0 && scene.fiery.size === 0);
});

test('불꽃 꼬리 표시 시스템: 부모의 실제 종속 삭제가 두 종류의 등록과 Form 을 모두 정리하고 불꽃 꼬리 삭제 dirty 를 보낸다', () => {
    const scene = new VisualScene();
    scene.construction.add(scene.parent, false);
    const tail = scene.fiery.add(scene.parent);
    const form = tail.form;
    scene.kernel.runFrame();
    scene.fieryClock.cameraX = 0.25;
    scene.fieryClock.cameraY = -0.5;
    const expected = renderProcessRemovalBounds(tail.fields.registration, 0.25, -0.5);
    scene.dirty.length = 0;
    scene.pool.allocated(scene.parent)[11] |= 4;
    scene.destroy.destroy(scene.parent, 0, scene.host.hooks());
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0 && scene.construction.size === 0 && scene.fiery.size === 0 && (scene.pool.slot(form)[11] & 1));
    assert.equal(scene.dirty.length, 1);
    assert.deepEqual([scene.dirty[0].left, scene.dirty[0].top, scene.dirty[0].right, scene.dirty[0].bottom], expected);
});

test('불꽃 꼬리 표시 시스템: 오래된 수집 토큰이 재사용 슬롯의 새 효과를 잘못 그리지 않고 두 배율을 섞지 않는다', () => {
    const scene = new VisualScene();
    const tail = scene.fiery.add(scene.parent);
    scene.kernel.runFrame();
    const token = tail.fields.registration.token;
    scene.host.kill(tail, 0x10);
    scene.fiery.add(scene.parent);
    scene.rendering.drawToken(token);
    assert.equal(scene.lines.length, 0);
    scene.fieryClock.pixelScaleX = 17;
    assert.ok(throwsOf(() => scene.kernel.runFrame()));
    scene.fieryClock.pixelScaleX = 16;
});
