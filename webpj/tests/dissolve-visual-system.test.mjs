// 분해 표시 시스템 dissolve-visual-system.js(DissolveVisualSystem)를 실제 Kernel/Host/Form 분배/상호 감시 위에서 검사한다(cpppj `tests/DissolveVisualSystemTests.cpp` 2건을 그대로 옮김).
// 독립 fixture 가 없는 조립 시험이며 분해 계산은 E189 의 실제 PE 대조 몸체, 캡처는 3×4 명시 입력이다(실제 미션/SHP 대조가 아님).
//  - 대상(source) 삭제 뒤에도 저장한 캡처와 더미에 붙은 process 18 이 유지되고, 첫 프레임 delta 0·12픽셀 출력·dirty 2 뒤 자연 소진 때 Form·더미·감시 쌍이 모두 반납된다(가로/세로 두 방향).
//  - 완전 투명 캡처의 new 0 입자 반납·120바이트 확보 실패(null, 등록/더미 없음)·생성 중 캡처 예외(부착 뒤 Host 삭제로 Form·더미 정리)도 실제 Form/목록을 남기지 않는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E193: 분해 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidFormDispatch } from '../www/core/squid-form-dispatch.js';
import { DependFormHost } from '../www/core/depend-form-host.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { createConstructionVisualState } from '../www/core/raw-construction-visual.js';
import { ShrapnelRelease } from '../www/core/raw-shrapnel-lifetime.js';
import { RenderProcessSystem } from '../www/core/render-process-system.js';
import { DissolveVisualSystem } from '../www/core/dissolve-visual-system.js';

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-09 WB04-E193)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 예외가 나는지 본다. (2026-11-09 WB04-E193)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 Form 분배와 의존 통지를 묶고 세 합성 부모를 배치한 분해 장면이다(cpppj DissolveScene). (2026-11-09 WB04-E193)
class DissolveScene {
    constructor() {
        this.pool = new SidPool(32768);
        this.types = formTypeInputs();
        this.factory = new SquidFactory(this.pool, this.types);
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        // 파생 자산 효과의 완료 경계이며 Form 의 실제 수명은 분배기가 처리한다.
        const assets = {
            selected: () => 0,
            emit: (event) => {
                if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) this.destroy.completePreDestroy();
                if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.destroy.completePostDestroy();
            }
        };
        this.dispatch = new SquidFormDispatch(this.pool, this.destroy, assets, (sid, flags, hooks) => { this.destroy.destroy(sid, flags, hooks); return true; });
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock, { asset: this.dispatch.hooks() });
        // 더미(110) 감시자는 분배 삭제, 그 밖은 Host 의존 통지로 보낸다.
        this.depends = new DependFormHost(this.pool, this.destroy, {
            notice: (watcher, watched, form) => {
                if (this.pool.slot(watcher)[10] === 110) this.dispatch.destroy(watcher, 0);
                else this.host.notifyDependency(watcher, watched, form);
            },
            fallback: this.dispatch.hooks()
        });
        this.rendering = new RenderProcessSystem(this.pool, this.host, createConstructionVisualState(), { reserve: () => true, releaseArray: () => {} });
        this.random = new GameRandom(0x12345678);
        this.frame = { now: 0, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
        this.releases = [];
        this.pixels = [];
        this.dirty = [];
        this.dummy = 0;
        this.transparent = false;
        this.failObject = false;
        this.failCapture = false;
        this.dispatch.bind(this.host);
        this.dispatch.bind(this.depends);
        this.source = this.create(133, 2.125, 3.25);
        this.first = this.create(145, 0, 0);
        this.second = this.create(145, 10, 14);
        this.frame.now = 1;
        this.effect = new DissolveVisualSystem(this.rendering, this.random, this.frame, this.hooks());
    }

    // 실제 생성자가 쓴 가상 표/타입을 보존하고 명시 좌표만 공급한다. (2026-11-09 WB04-E193)
    create(type, x, y) {
        const sid = this.factory.create(type, 2);
        const raw = this.pool.allocated(sid);
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        return sid;
    }

    // 준비/프레임은 원본 모듈이며 캡처만 3×4 의 독립 입력이다. (2026-11-09 WB04-E193)
    hooks() {
        return {
            lifetime: {
                reserve: (bytes) => { assert.equal(bytes, 120); return !this.failObject; },
                create: (type, flags) => {
                    assert.ok(type === 110 && flags === 2);
                    this.dummy = this.factory.create(type, flags);
                    return this.dummy;
                },
                pop: (sid, x, y, flags) => {
                    assert.ok(sid === this.dummy && flags === 16);
                    const raw = this.pool.allocated(sid);
                    put(raw, 14, floatToBits(x));
                    put(raw, 18, floatToBits(y));
                },
                depend: (form, parent) => { this.depends.create(form, parent); },
                capture: (sid) => {
                    // 부착 뒤·준비 전에는 Kernel 프레임과 출력이 아무것도 하지 않는다.
                    assert.ok(sid === this.source && this.kernel.size === 1 && this.rendering.size === 1);
                    if (this.failCapture) throw new Error('명시 캡처 실패');
                    this.kernel.runFrame();
                    this.rendering.draw();
                    assert.equal(this.pixels.length, 0);
                    return { width: 3, height: 4, hotspotX: 1, hotspotY: 3, pixels: new Uint8Array(12).fill(this.transparent ? 255 : 71) };
                }
            },
            frame: { reserve: () => true, release: () => {}, invalidate: (area) => this.dirty.push(area), pixel: (pixel) => this.pixels.push(pixel) },
            release: (value) => this.releases.push(value),
            destroyUnpaired: (sid) => { this.dispatch.destroy(sid, 16); }
        };
    }
}

test('분해 표시 시스템: 대상 삭제 뒤에도 더미의 process 18 이 유지되고 자연 소진 때 Form·더미·감시 쌍이 모두 반납된다', () => {
    // 두 폴의 가로/세로 식을 같은 실제 Host 에서 실행한다.
    for (const orientation of [0, 1]) {
        const scene = new DissolveScene();
        const process = scene.effect.add(scene.source, scene.first, scene.second, orientation);
        assert.ok(process);
        const form = process.form;
        const token = process.fields.common.registration.token;
        const dummy = scene.dummy;
        assert.ok(scene.pool.slot(form)[10] === 18 && process.parent === dummy && scene.depends.has(form, dummy));
        assert.ok(scene.effect.size === 1 && scene.kernel.size === 1);
        assert.equal(scene.dispatch.destroy(scene.source, 16), true);
        assert.ok(scene.effect.size === 1 && scene.kernel.size === 1 && scene.rendering.lookup(token));
        scene.kernel.runFrame();
        scene.rendering.draw();
        assert.ok(process.fields.clockStarted && process.fields.common.delta === 0);
        assert.ok(scene.pixels.length === 12 && scene.dirty.length === 2, `방향 ${orientation} 픽셀 ${scene.pixels.length} dirty ${scene.dirty.length}`);
        scene.frame.now = 10;
        scene.kernel.runFrame();
        scene.frame.now = 11;
        scene.kernel.runFrame();
        assert.ok(scene.effect.size === 0 && scene.kernel.size === 0 && !scene.rendering.lookup(token));
        assert.ok((scene.pool.slot(form)[11] & 3) === 3 && (scene.pool.slot(dummy)[11] & 3) === 3);
        assert.deepEqual(scene.releases, [ShrapnelRelease.Source, ShrapnelRelease.Particles, ShrapnelRelease.Object]);
        assert.ok(scene.destroy.preDepth === 0 && scene.destroy.postDepth === 0);
    }
});

test('분해 표시 시스템: 투명 캡처의 new 0 반납·확보 실패·생성 중 캡처 예외도 실제 Form/목록을 남기지 않는다', () => {
    const transparent = new DissolveScene();
    transparent.transparent = true;
    transparent.effect.add(transparent.source, transparent.first, transparent.second, 1);
    transparent.kernel.runFrame();
    assert.ok(transparent.effect.size === 0 && transparent.kernel.size === 0 && transparent.pixels.length === 0);
    assert.equal(transparent.releases.length, 3);
    const failure = new DissolveScene();
    failure.failObject = true;
    assert.equal(failure.effect.add(failure.source, failure.first, failure.second, 0), null);
    assert.ok(failure.rendering.size === 0 && failure.kernel.size === 0 && !failure.dummy);
    failure.failObject = false;
    failure.failCapture = true;
    assert.ok(throwsOf(() => failure.effect.add(failure.source, failure.first, failure.second, 0)));
    assert.ok(failure.rendering.size === 0 && failure.kernel.size === 0 && (failure.pool.slot(failure.dummy)[11] & 3) === 3);
    assert.ok(failure.destroy.preDepth === 0 && failure.destroy.postDepth === 0);
});

test('분해 표시 시스템 경계(웹 추가): 세계/반납·프레임 출력 경계 누락·배율 불일치·source 0 을 등록 전에 거부하고 dispose 가 남은 분해를 정리한다', () => {
    const scene = new DissolveScene();
    const hooks = scene.hooks();
    assert.ok(throwsOf(() => new DissolveVisualSystem(scene.rendering, scene.random, scene.frame, { ...hooks, destroyUnpaired: undefined })));
    assert.ok(throwsOf(() => new DissolveVisualSystem(scene.rendering, scene.random, scene.frame, { ...hooks, lifetime: { ...hooks.lifetime, capture: undefined } })));
    assert.ok(throwsOf(() => new DissolveVisualSystem(scene.rendering, scene.random, scene.frame, { ...hooks, frame: { ...hooks.frame, pixel: undefined } })));
    assert.ok(throwsOf(() => new DissolveVisualSystem(scene.rendering, scene.random, { ...scene.frame, pixelScaleX: 17 }, hooks)));
    assert.ok(throwsOf(() => scene.effect.add(0, scene.first, scene.second, 0)));
    assert.ok(scene.effect.add(scene.source, scene.first, scene.second, 0));
    assert.ok(scene.effect.size === 1 && scene.kernel.size === 1);
    scene.effect.dispose();
    assert.ok(scene.effect.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0);
    assert.ok(scene.destroy.preDepth === 0 && scene.destroy.postDepth === 0);
});
