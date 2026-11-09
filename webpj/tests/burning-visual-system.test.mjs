// Burning 표시 시스템 burning-visual-system.js(BurningVisualSystem)를 실제 Host/Kernel/공통 삭제와 실제 보호 캡처 위에서 검사한다.
// cpppj 에는 이 시스템 전용 시험이 없으므로(BurningVisualSystem 은 cpppj `client/RawCampaignParticles` 를 통해서만 쓰이고, `BurningVisualTests.cpp` 는 E183 생성자 대조임)
// 같은 구조인 cpppj `SmokeVisualSystemTests.cpp` 의 장면·관측을 Burning 에 맞게 옮긴 웹 조립 검사이다. Burning 계산 자체는 E183·E184·E200 의 실제 PE 대조 몸체이다.
//  - 실제 Host 부착(Form TYPE 14·150입자·첫 색 표 1회)·단계 0 실제 ShapeToBuffer 캡처·단계 1 첫 좌표·visible 프레임의 연소점/방출/dirty·5점 출력·빈 notify, Host 삭제의 삭제 dirty 와 이미지 → 행 → 소리 → 입자 → 객체 반납.
//  - 생성 중 프레임/출력/수집 무동작과 중첩 거부, 입자 확보 실패·Host 부착 예외 정리, 두 번째 생성의 공유 색 재사용.
//  - 487940 프레임 gate(표시 slot 7 은 현재 프레임 번호가 맞을 때만 실행), 부모 종속 삭제의 dirty, dispose.
//  - 장치/판본/캡처 누락·피해 0 거부, 생성 뒤 배율 불일치의 프레임 거부(출력은 원문처럼 배율을 검사하지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E201: Burning 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { createConstructionVisualState } from '../www/core/raw-construction-visual.js';
import { RENDER_ENTRY_BYTES } from '../www/core/raw-render-collection.js';
import { BurningRelease } from '../www/core/raw-burning-lifetime-draw.js';
import { createBurningFrameState, makeBurningFrameCapture } from '../www/core/raw-burning-frame.js';
import { RenderProcessSystem, renderProcessRemovalBounds } from '../www/core/render-process-system.js';
import { BurningVisualSystem } from '../www/core/burning-visual-system.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';

// 실제 자료 TYPE 레코드에 내장 Form 네 생성자 주소를 더한 188칸 입력이다. (2026-11-09 WB04-E201)
function burningTypes() {
    return realCaptureAssets().types.map((record, number) => ({ ...record, cost: record.cost ?? 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
}

// 예외가 나는지 본다. (2026-11-09 WB04-E201)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 단일 실제 풀/Host 와 공통 표시 저장소·실제 보호 캡처를 가진 Burning 조립 장면이다(cpppj SmokeVisualScene 과 같은 조립). (2026-11-09 WB04-E201)
class BurningScene {
    constructor() {
        const assets = realCaptureAssets();
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = burningTypes();
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock, {
            asset: {
                selected: () => 0,
                emit: (event) => {
                    if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) this.destroy.completePreDestroy();
                    if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.destroy.completePostDestroy();
                }
            }
        });
        this.random = new GameRandom(0x2468ace0);
        this.visualState = { processType: 14, palette: 0 };
        this.sourcesState = { weakenedMana: false, manaType: 158, daisType: 165, victim: { checking: () => 0 } };
        this.frameState = createBurningFrameState();
        Object.assign(this.frameState, { now: 2, pixelScaleX: 16, pixelScaleY: 11, deltaMultiplier: Math.fround(0.3), viewport: { left: 0, top: 0, right: 1024, bottom: 768 } });
        // 첫 색 표 생성자가 채우는 8색과 원문이 읽을 수 있는 인접 DWORD 하나이다.
        this.colors = new Array(9).fill(0);
        this.palette = { count: 8, readable: this.colors };
        this.frameNumber = 0;
        this.dirty = [];
        this.pixels = [];
        this.released = [];
        this.stops = [];
        this.gradients = [];
        this.palettes = 0;
        this.probe = false;
        this.fail = false;
        this.probes = 0;
        this.parent = this.makeParent();
        this.rendering = new RenderProcessSystem(this.pool, this.host, createConstructionVisualState(), { reserve: () => true, releaseArray: () => {} });
        this.capture = new RawShapeToBuffer(this.pool, assets.types, assets.shapes, assets.database, createShapeToBufferState());
        this.burning = new BurningVisualSystem(this.rendering, this.random, this.types, this.visualState, this.sourcesState, this.frameState, this.palette, () => this.frameNumber, this.hooks());
    }

    // HP 10/20 의 실제 Dude(TYPE 70) 피해 부모이다(좌표 20.5/21.25·깊이 7·프레임 0). (2026-11-09 WB04-E201)
    makeParent() {
        const sid = this.pool.allocate(0);
        const raw = this.pool.allocated(sid);
        put(raw, 0, 0x508cc8);
        raw[10] = 70;
        raw[11] = 0;
        raw[34] = 0;
        raw[35] = 7;
        put(raw, 14, floatToBits(20.5));
        put(raw, 18, floatToBits(21.25));
        put(raw, 26, 10);
        put(raw, 36, 0);
        return sid;
    }

    // 생성/프레임은 실제 몸체이며 장치 출력과 확보 실패만 경계로 둔다. (2026-11-09 WB04-E201)
    hooks() {
        return {
            visual: {
                reserve: (bytes) => {
                    assert.ok(bytes === 3600 || bytes === 8);
                    if (this.probe && bytes === 3600) {
                        // 생성 중(부착 뒤·준비 전) 프레임/출력/수집은 아무것도 하지 않고 중첩 생성은 거부한다.
                        this.probes += 1;
                        const before = this.random.state;
                        this.kernel.runFrame();
                        this.rendering.draw();
                        const work = new Uint8Array(RENDER_ENTRY_BYTES * 2);
                        assert.ok(this.random.state === before && this.rendering.collect({ left: 0, top: 0, right: 256, bottom: 256 }, work, 2) === 0 && this.pixels.length === 0 && this.dirty.length === 0);
                        assert.ok(throwsOf(() => this.burning.add(this.parent, 1)));
                    }
                    return !(this.fail && bytes === 3600);
                },
                createPalette: () => { this.palettes += 1; return 9; },
                gradient: (first, last, count, noise) => this.gradients.push([first, last, count, noise]),
                buildPalette: (token) => {
                    assert.equal(token, 9);
                    // 검사용 8색(원본 색띠 몸체는 별도 단위)과 인접 DWORD 이다.
                    for (let index = 0; index < 8; index += 1) this.colors[index] = 0x40 + index;
                    this.colors[8] = 0xcafef00d;
                }
            },
            frame: makeBurningFrameCapture(this.capture ?? { pool: this.pool, capture: () => { throw new Error('캡처 전 호출'); } }, this.pool, { stopSound: (handle) => this.stops.push(handle) }),
            invalidate: (rect) => this.dirty.push(rect),
            clip: () => ({ left: 0, top: 0, right: 1024, bottom: 768 }),
            pixel: (x, y, color) => this.pixels.push([x, y, color]),
            release: (release) => this.released.push(release)
        };
    }

    // 프레임 시각과 Host 시계를 같이 옮긴다. (2026-11-09 WB04-E201)
    advance(seconds) {
        this.frameState.now += seconds;
        this.clock.now = this.frameState.now;
    }
}

test('Burning 표시 시스템: 실제 Host 부착·캡처/첫 좌표 단계·연소점 방출과 5점 출력·Host 삭제의 삭제 dirty 와 반납 순서', () => {
    const scene = new BurningScene();
    const process = scene.burning.add(scene.parent, 1);
    const form = process.form;
    const token = process.fields.registration.token;
    const fields = process.fields;
    assert.ok(scene.kernel.size === 1 && scene.rendering.size === 1 && scene.burning.size === 1 && scene.host.fromForm(form) === process && scene.pool.slot(form)[10] === 14);
    assert.ok(fields.capacity === 150 && fields.particles.length === 150 && fields.victim === scene.parent && fields.mode === 1 && fields.stage === 0);
    assert.ok(scene.palettes === 1 && scene.gradients.length === 1 && scene.gradients[0][2] === 8);
    // 빈 notify(480260 ret12)는 상태를 바꾸지 않는다.
    process.notify(3, 0, 0);
    assert.equal(fields.stage, 0);
    // 단계 0: 실제 ShapeToBuffer 로 피해 그림을 캡처한다.
    scene.kernel.runFrame();
    const expected = scene.capture.capture(scene.parent);
    assert.ok(fields.stage === 1 && fields.image.width === expected.width && fields.image.height === expected.height && expected.width > 0);
    assert.deepEqual(fields.image.pixels, expected.pixels);
    assert.equal(fields.rows.length, Math.trunc((expected.width + expected.height) / 5));
    // 단계 1: 피해 중심(월드 × 배율)을 첫 등록 범위로 두고 off 로 둔다.
    scene.kernel.runFrame();
    assert.ok(fields.stage === 2 && fields.off && fields.registration.left === 328 && fields.registration.top === Math.fround(21.25 * 11) && fields.registration.zOrder === 6);
    // visible 프레임: 다시 켜고 연소점(Dude 는 목표 1)을 고른 뒤 방출/이동과 프레임마다 dirty 두 건을 낸다.
    for (let index = 0; index < 12; index += 1) {
        scene.advance(0.1);
        scene.kernel.runFrame();
    }
    const sources = fields.particles.slice(0, fields.total).filter((particle) => particle.source).length;
    // 11×17 Dude 캡처는 300번 시도로 행을 고르지 못해 원본 폴백(불투명 점 7개, selected 증가 없음)을 매 visible 프레임 반복하므로 source 가 7개씩 쌓이고 행 표식은 비어 있다(E183 원본 대조 몸체).
    assert.ok(!fields.off && fields.selected === 0 && sources >= 7 && fields.total > sources && Array.from(fields.rows).every((value) => value === 0) && scene.dirty.length >= 2);
    scene.rendering.draw();
    assert.ok(scene.pixels.length > 0 && scene.pixels.length % 5 === 0 && scene.pixels.every(([, , color]) => color >= 0x40 && color <= 0x47));
    // Host 삭제: 삭제 dirty(현재 카메라) → 이미지 → 행 → 소리(핸들 0) → 입자 → 공통 등록 → 객체.
    scene.dirty.length = 0;
    const bounds = renderProcessRemovalBounds(fields.registration, 0, 0);
    scene.host.kill(process, 0x10);
    assert.deepEqual(scene.dirty.map((rect) => [rect.left, rect.top, rect.right, rect.bottom]), [bounds]);
    assert.deepEqual(scene.released, [BurningRelease.Image, BurningRelease.Rows, BurningRelease.Particles, BurningRelease.Object]);
    assert.deepEqual(scene.stops, [0]);
    assert.ok(scene.burning.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.rendering.lookup(token) === null && scene.host.fromForm(form) === null);
    assert.ok(scene.destroy.preDepth === 0 && scene.destroy.postDepth === 0);
});

test('Burning 표시 시스템: 생성 중 출력/중첩 차단·입자 확보 실패·Host 부착 예외가 잔재를 남기지 않고 두 번째 생성부터 공유 색을 재사용한다', () => {
    const scene = new BurningScene();
    scene.probe = true;
    scene.fail = true;
    assert.ok(throwsOf(() => scene.burning.add(scene.parent, 0)));
    assert.ok(scene.probes === 1 && scene.burning.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.palettes === 0);
    assert.deepEqual(scene.released, [BurningRelease.Object]);
    scene.probe = false;
    scene.fail = false;
    const attach = scene.host.attach;
    scene.host.attach = () => { throw new Error('명시 부착 실패'); };
    assert.ok(throwsOf(() => scene.burning.add(scene.parent, 0)));
    assert.ok(scene.burning.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0);
    scene.host.attach = attach;
    const first = scene.burning.add(scene.parent, 0);
    const second = scene.burning.add(scene.parent, 1);
    assert.ok(first && second && scene.palettes === 1 && scene.burning.size === 2 && scene.kernel.size === 2 && scene.rendering.size === 2);
    scene.burning.dispose();
    assert.ok(scene.burning.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0);
});

test('Burning 표시 시스템: 487940 프레임 gate 는 표시 slot 7 을 현재 프레임 번호가 맞을 때만 실행하고, 부모 종속 삭제는 준비된 객체마다 삭제 dirty 를 낸다', () => {
    const scene = new BurningScene();
    const processes = Array.from({ length: 8 }, () => scene.burning.add(scene.parent, 1));
    const late = processes[7];
    assert.equal(late.fields.registration.slot, 7);
    // 프레임 번호 0: slot 0..6 만 단계 0 → 1 로 간다.
    scene.kernel.runFrame();
    assert.deepEqual(processes.map((process) => process.fields.stage), [1, 1, 1, 1, 1, 1, 1, 0]);
    // (7 XOR 3) 의 하위 두 비트가 0 인 프레임 번호 3 에서 slot 7 도 실행한다.
    scene.frameNumber = 3;
    scene.kernel.runFrame();
    assert.deepEqual(processes.map((process) => process.fields.stage), [2, 2, 2, 2, 2, 2, 2, 1]);
    scene.frameState.cameraX = 0.25;
    scene.frameState.cameraY = -0.5;
    const expected = processes.map((process) => renderProcessRemovalBounds(process.fields.registration, 0.25, -0.5));
    scene.dirty.length = 0;
    scene.pool.allocated(scene.parent)[11] |= 4;
    scene.destroy.destroy(scene.parent, 0, scene.host.hooks());
    assert.ok(scene.kernel.size === 0 && scene.burning.size === 0 && scene.rendering.size === 0);
    assert.deepEqual(scene.dirty.map((rect) => [rect.left, rect.top, rect.right, rect.bottom]).sort(), expected.sort());
    assert.equal(scene.released.filter((release) => release === BurningRelease.Object).length, 8);
    scene.burning.dispose();
});

test('Burning 표시 시스템 경계: 장치/판본/캡처 누락과 피해 0 을 거부하고, 생성 뒤 배율 불일치는 프레임에서만 거부한다', () => {
    const scene = new BurningScene();
    const hooks = scene.hooks();
    const make = (changed, frameNumber = () => 0, rendering = scene.rendering) => () => new BurningVisualSystem(rendering, scene.random, scene.types, scene.visualState, scene.sourcesState, scene.frameState, scene.palette, frameNumber, changed);
    // 필수 장치 하나씩 빼면 거부한다.
    for (const key of ['invalidate', 'clip', 'pixel', 'release']) assert.ok(throwsOf(make({ ...hooks, [key]: undefined })), key);
    assert.ok(throwsOf(make({ ...hooks, frame: { capture: hooks.frame.capture } })));
    assert.ok(throwsOf(make({ ...hooks, frame: { stopSound: hooks.frame.stopSound } })));
    assert.ok(throwsOf(make(hooks, 0)));
    // 다른 풀의 캡처는 같은 월드가 아니다.
    const other = new SidPool(32768, { server: true });
    assert.ok(throwsOf(() => makeBurningFrameCapture(scene.capture, other)));
    assert.ok(throwsOf(() => scene.burning.add(0, 1)));
    // cpppj Add/Draw 는 배율을 검사하지 않으므로 생성·출력은 되고, 프레임 때 거부한다.
    scene.frameState.pixelScaleX = 17;
    const process = scene.burning.add(scene.parent, 1);
    assert.ok(process);
    assert.ok(throwsOf(() => scene.kernel.runFrame()));
    scene.rendering.draw();
    scene.frameState.pixelScaleX = 16;
    scene.kernel.runFrame();
    assert.equal(process.fields.stage, 1);
    scene.burning.dispose();
    assert.equal(scene.burning.size, 0);
});
