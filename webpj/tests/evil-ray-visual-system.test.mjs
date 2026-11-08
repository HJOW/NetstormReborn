// EvilRay 표시 시스템 evil-ray-visual-system.js(EvilRayVisualSystem)를 실제 Host/Kernel/공통 삭제 위에서 검사한다.
// cpppj 에는 이 시스템 전용 시험이 없으므로(EvilRayVisualSystem 은 cpppj `client/RawCampaignParticles` 를 통해서만 쓰임) 같은 구조인 cpppj `SmokeVisualSystemTests.cpp` 의 장면·관측을 EvilRay 에 맞게 옮긴 웹 조립 검사이다.
// EvilRay 계산 자체는 E190 의 실제 PE 대조 몸체이다.
//  - 실제 Host 부착(Form TYPE 10)·첫 프레임 방출/시작 음향·세 색 타원 출력, notify(3) 의 Stop 과 다른 mode 진단, 중단 뒤 수명 끝 다음 프레임 자기 삭제와 입자 → 객체 반납.
//  - 생성 중 프레임/출력/수집 무동작과 중첩 거부, 입자 확보 실패 정리, Host 부착 예외 정리, 부모 0 독립 소유(Host/Kernel 미부착, dispose 때 반납), 부모 종속 삭제의 dirty, 반납 관찰 누락·배율 불일치 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E198: EvilRay 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { createConstructionVisualState } from '../www/core/raw-construction-visual.js';
import { RENDER_ENTRY_BYTES } from '../www/core/raw-render-collection.js';
import { EvilRayRelease } from '../www/core/raw-evil-ray-lifetime.js';
import { RenderProcessSystem, renderProcessRemovalBounds } from '../www/core/render-process-system.js';
import { EvilRayVisualSystem } from '../www/core/evil-ray-visual-system.js';

// 내장 Form 생성자 주소와 HP 보유 부모 타입 129 를 넣은 188칸 입력이다(연기 시스템 시험과 같은 입력). (2026-11-09 WB04-E198)
function types() {
    const result = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    result[129] = { ...result[129], flags1: 0x10, group: 1, maxHitPoints: 600, footX: 3, footY: 3 };
    return result;
}

// 예외가 나는지 본다. (2026-11-09 WB04-E198)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 단일 실제 풀/Host 와 같은 공통 표시 저장소를 가진 EvilRay 조립 장면이다(cpppj SmokeVisualScene 과 같은 조립). (2026-11-09 WB04-E198)
class EvilRayScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = types();
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
        this.visual = createConstructionVisualState();
        this.frame = { now: 2, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
        this.colors = { count: 3, readable: [17, 0x12345678, 0xabcdef01] };
        this.dirty = [];
        this.circles = [];
        this.sounds = [];
        this.diagnostics = [];
        this.released = [];
        this.probe = false;
        this.fail = false;
        this.probes = 0;
        this.parent = this.pool.allocate(0);
        const raw = this.pool.allocated(this.parent);
        put(raw, 0, 0x501dd0);
        raw[10] = 129;
        raw[11] = 0;
        raw[34] = 1;
        raw[35] = 7;
        put(raw, 14, floatToBits(20.5));
        put(raw, 18, floatToBits(21.25));
        put(raw, 26, 600);
        // 이미 있는 공유 3색 객체와 source 전역(같은 부모)이다.
        this.lifetime = { palette: 1, processType: 10, source: this.parent };
        this.rendering = new RenderProcessSystem(this.pool, this.host, this.visual, { reserve: () => true, releaseArray: () => {} });
        this.ray = new EvilRayVisualSystem(this.rendering, this.lifetime, this.frame, this.colors, this.hooks());
    }

    // 생성/프레임은 실제 몸체이며 장치 출력과 확보 실패만 경계로 둔다. (2026-11-09 WB04-E198)
    hooks() {
        return {
            lifetime: {
                reserve: (bytes) => {
                    assert.equal(bytes, 1200);
                    if (this.probe) {
                        this.probes += 1;
                        this.kernel.runFrame();
                        this.rendering.draw();
                        const work = new Uint8Array(RENDER_ENTRY_BYTES * 2);
                        assert.ok(this.rendering.collect({ left: 0, top: 0, right: 256, bottom: 256 }, work, 2) === 0 && this.circles.length === 0);
                        assert.ok(throwsOf(() => this.ray.add(this.parent)));
                    }
                    return !this.fail;
                },
                assertion: (expression, file, line) => this.diagnostics.push([expression, file, line])
            },
            frame: {
                shapeWidth: () => 9,
                soundId: (sid, end) => (end ? 31 : 11),
                sound: (x, y, id) => this.sounds.push(id),
                invalidate: (rect) => this.dirty.push(rect),
                clip: () => {},
                circle: (circle) => this.circles.push(circle),
                assertion: (expression, file, line) => this.diagnostics.push([expression, file, line])
            },
            release: (release) => this.released.push(release)
        };
    }
}

test('EvilRay 표시 시스템: 실제 Host 부착·방출/음향·세 색 타원·notify(3) 중단 뒤 다음 프레임 자기 삭제와 반납', () => {
    const scene = new EvilRayScene();
    const process = scene.ray.add(scene.parent);
    const form = process.form;
    const token = process.fields.registration.token;
    assert.ok(scene.kernel.size === 1 && scene.rendering.size === 1 && scene.ray.size === 1 && scene.host.fromForm(form) === process && scene.pool.slot(form)[10] === 10);
    assert.ok(process.fields.particles.length === 50 && process.fields.source === scene.parent && process.fields.stopping === 0);
    scene.kernel.runFrame();
    assert.ok(process.fields.total === 1 && scene.sounds.length === 1 && scene.sounds[0] === 11 && scene.dirty.length === 2);
    scene.rendering.draw();
    assert.equal(scene.circles.length, 3);
    assert.deepEqual(scene.circles.map((circle) => circle.color), [0xabcdef01, 0x12345678, 17]);
    // 다른 mode 는 진단 뒤 중단하지 않고, mode 3 은 중단한다(ProcessForm notify1c 경로).
    process.notify(2, 0, 0);
    assert.ok(process.fields.stopping === 0 && scene.diagnostics.length === 1);
    process.notify(3, 0, 0);
    assert.equal(process.fields.stopping, 1);
    // 수명 13 을 넘기는 2초(delta 20)에는 이번에 죽는 입자가 아직 활성으로 세어지고 끝 음향을 낸다.
    scene.frame.now += 2;
    scene.kernel.runFrame();
    assert.ok(scene.ray.size === 1 && scene.sounds.length === 2 && scene.sounds[1] === 31);
    scene.kernel.runFrame();
    assert.ok(scene.ray.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.rendering.lookup(token) === null && scene.host.fromForm(form) === null);
    assert.deepEqual(scene.released, [EvilRayRelease.Particles, EvilRayRelease.Object]);
    assert.ok(scene.destroy.preDepth === 0 && scene.destroy.postDepth === 0);
});

test('EvilRay 표시 시스템: 생성 중 출력/중첩 차단·확보 실패·Host 부착 예외가 등록/Kernel 잔재를 남기지 않는다', () => {
    const scene = new EvilRayScene();
    scene.probe = true;
    scene.fail = true;
    assert.ok(throwsOf(() => scene.ray.add(scene.parent)));
    assert.ok(scene.probes === 1 && scene.ray.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0);
    assert.deepEqual(scene.released, [EvilRayRelease.Object]);
    scene.probe = false;
    scene.fail = false;
    const attach = scene.host.attach;
    scene.host.attach = () => { throw new Error('명시 부착 실패'); };
    assert.ok(throwsOf(() => scene.ray.add(scene.parent)));
    assert.ok(scene.ray.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0);
    scene.host.attach = attach;
    assert.ok(scene.ray.add(scene.parent));
    assert.ok(scene.ray.size === 1 && scene.kernel.size === 1);
});

test('EvilRay 표시 시스템: 부모 0 은 등록만 하고 Host/Kernel 에 붙이지 않으며 부모 종속 삭제는 삭제 dirty 를 내고 정리 때 모두 반납한다', () => {
    const scene = new EvilRayScene();
    const standalone = scene.ray.add(0);
    const token = standalone.fields.registration.token;
    assert.ok(scene.kernel.size === 0 && scene.ray.size === 1 && scene.rendering.size === 1 && standalone.form === 0 && scene.rendering.lookup(token) !== null);
    const attached = scene.ray.add(scene.parent);
    scene.kernel.runFrame();
    scene.frame.cameraX = 0.25;
    scene.frame.cameraY = -0.5;
    const expected = renderProcessRemovalBounds(attached.fields.registration, 0.25, -0.5);
    scene.dirty.length = 0;
    scene.pool.allocated(scene.parent)[11] |= 4;
    scene.destroy.destroy(scene.parent, 0, scene.host.hooks());
    assert.ok(scene.kernel.size === 0 && scene.ray.size === 1 && scene.rendering.size === 1);
    assert.equal(scene.dirty.length, 1);
    assert.deepEqual([scene.dirty[0].left, scene.dirty[0].top, scene.dirty[0].right, scene.dirty[0].bottom], expected);
    scene.ray.dispose();
    assert.ok(scene.ray.size === 0 && scene.rendering.size === 0 && scene.rendering.lookup(token) === null);
    assert.deepEqual(scene.released, [EvilRayRelease.Particles, EvilRayRelease.Object, EvilRayRelease.Particles, EvilRayRelease.Object]);
});

test('EvilRay 표시 시스템 경계: 반납 관찰 누락·배율 불일치를 생성 때 거부하고, 생성 뒤 배율이 어긋나면 프레임/출력에서 거부한다', () => {
    const scene = new EvilRayScene();
    const hooks = scene.hooks();
    assert.ok(throwsOf(() => new EvilRayVisualSystem(scene.rendering, scene.lifetime, scene.frame, scene.colors, { ...hooks, release: undefined })));
    assert.ok(throwsOf(() => new EvilRayVisualSystem(scene.rendering, scene.lifetime, { ...scene.frame, pixelScaleY: 12 }, scene.colors, hooks)));
    // cpppj Add 는 배율을 검사하지 않으므로 생성은 되고, 배율 검사는 프레임/출력 때 한다.
    scene.frame.pixelScaleX = 17;
    assert.ok(scene.ray.add(scene.parent));
    assert.ok(throwsOf(() => scene.kernel.runFrame()));
    assert.ok(throwsOf(() => scene.rendering.draw()));
    scene.frame.pixelScaleX = 16;
    scene.kernel.runFrame();
    assert.equal(scene.ray.size, 1);
});
