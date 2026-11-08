// 파편 표시 시스템 shrapnel-visual-system.js(ShrapnelVisualSystem)를 실제 Kernel/Host/Form 분배/상호 감시 위에서 검사한다.
// cpppj 에는 이 시스템 전용 시험이 없으므로(ShrapnelVisualSystem 은 RawCampaignParticles 를 통해서만 쓰임) 같은 구조인 cpppj `DissolveVisualSystemTests.cpp` 의 장면·관측을 파편에 맞게 옮긴 웹 조립 검사이다.
// 파편 계산 자체는 E188 의 실제 PE 대조 몸체이며 캡처는 5×6 명시 입력이다(실제 미션/SHP 대조가 아님).
//  - 세 가상 생성기(폭발 0·붕괴 1·셋째 2)마다: Form TYPE 16·부모=더미·상호 감시 쌍·중력 binary32, 대상 삭제 뒤에도 유지, 첫 프레임 dirty 2·출력 픽셀 수 = 사용 꼬리의 양수 수명 입자 수, 시간 경과 뒤 다음 프레임 자기 삭제와 Form·더미 반납·그림 → 입자 → 객체 반납·삭제 깊이 0.
//  - 104바이트 확보 실패(null, 더미 미생성)·캡처 예외·Host 부착 예외의 정리(Form/더미/목록 잔재 없음), 경계 누락·배율 불일치·source 0 거부와 dispose.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E197: 파편 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CrtRandom, GameRandom } from '../www/core/random.js';
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
import { ShrapnelKind } from '../www/core/raw-shrapnel-frame.js';
import { ShrapnelRelease } from '../www/core/raw-shrapnel-lifetime.js';
import { RenderProcessSystem } from '../www/core/render-process-system.js';
import { ShrapnelVisualSystem } from '../www/core/shrapnel-visual-system.js';

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-09 WB04-E197)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
// 세 가상 생성자가 저장하는 binary32 중력이다(E188 수명 몸체와 같은 PE 상수). (2026-11-09 WB04-E197)
const GRAVITY = [6, Math.fround(0.24), Math.fround(1.2)];

// 예외가 나는지 본다. (2026-11-09 WB04-E197)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 Form 분배와 의존 통지를 묶고 합성 대상을 배치한 파편 장면이다(cpppj DissolveScene 과 같은 조립). (2026-11-09 WB04-E197)
class ShrapnelScene {
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
        this.game = new GameRandom(0x12345678);
        this.crt = new CrtRandom(1);
        this.frame = { now: 1, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
        this.releases = [];
        this.pixels = [];
        this.dirty = [];
        this.dummy = 0;
        this.failObject = false;
        this.failCapture = false;
        this.dispatch.bind(this.host);
        this.dispatch.bind(this.depends);
        this.source = this.factory.create(133, 2);
        const raw = this.pool.allocated(this.source);
        put(raw, 14, floatToBits(2.125));
        put(raw, 18, floatToBits(3.25));
        raw[35] = 5;
        this.effect = new ShrapnelVisualSystem(this.rendering, this.game, this.crt, this.frame, this.hooks());
    }

    // 준비/프레임은 원본 모듈이며 캡처만 5×6 의 독립 입력이다. (2026-11-09 WB04-E197)
    hooks() {
        return {
            lifetime: {
                reserve: (bytes) => { assert.equal(bytes, 104); return !this.failObject; },
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
                    return { width: 5, height: 6, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(30).fill(71) };
                }
            },
            frame: { reserve: () => true, release: () => {}, invalidate: (area) => this.dirty.push(area), pixel: (pixel) => this.pixels.push(pixel) },
            release: (value) => this.releases.push(value),
            destroyUnpaired: (sid) => { this.dispatch.destroy(sid, 16); }
        };
    }
}

test('파편 표시 시스템: 세 생성기마다 대상 삭제 뒤에도 더미의 process 16 이 유지되고 자연 소진 때 Form·더미·감시 쌍이 모두 반납된다', () => {
    for (const kind of [ShrapnelKind.Explosion, ShrapnelKind.Collapse, ShrapnelKind.Variant2]) {
        const scene = new ShrapnelScene();
        const process = scene.effect.add(scene.source, kind);
        assert.ok(process);
        const form = process.form;
        const token = process.fields.registration.token;
        const dummy = scene.dummy;
        assert.ok(scene.pool.slot(form)[10] === 16 && process.parent === dummy && scene.depends.has(form, dummy));
        assert.ok(scene.effect.size === 1 && scene.kernel.size === 1 && process.fields.gravity === GRAVITY[kind]);
        // 최종 깊이는 캡처/준비 뒤 현재 source 의 깊이이다.
        assert.equal(process.fields.registration.zOrder, 5);
        assert.equal(scene.dispatch.destroy(scene.source, 16), true);
        assert.ok(scene.effect.size === 1 && scene.kernel.size === 1 && scene.rendering.lookup(token));
        scene.kernel.runFrame();
        scene.rendering.draw();
        const live = process.fields.particles.slice(0, process.fields.total).filter((particle) => particle.life > 0).length;
        assert.ok(scene.dirty.length === 2 && live > 0 && scene.pixels.length === live, `kind ${kind} 픽셀 ${scene.pixels.length} 생존 ${live}`);
        // 붕괴/셋째 생성기 수명은 10배 시간으로 최대 약 25 이므로 10초(delta 100)를 지나게 한다. 이번 조각에 죽는 입자도 이번 호출에서는 살아 있는 것으로 센다.
        scene.frame.now += 10;
        scene.kernel.runFrame();
        assert.equal(scene.effect.size, 1);
        scene.kernel.runFrame();
        assert.ok(scene.effect.size === 0 && scene.kernel.size === 0 && !scene.rendering.lookup(token));
        assert.ok((scene.pool.slot(form)[11] & 3) === 3 && (scene.pool.slot(dummy)[11] & 3) === 3);
        assert.deepEqual(scene.releases, [ShrapnelRelease.Source, ShrapnelRelease.Particles, ShrapnelRelease.Object]);
        assert.ok(scene.destroy.preDepth === 0 && scene.destroy.postDepth === 0);
    }
});

test('파편 표시 시스템: 104바이트 확보 실패·캡처 예외·Host 부착 예외도 실제 Form/더미/목록을 남기지 않는다', () => {
    const failure = new ShrapnelScene();
    failure.failObject = true;
    assert.equal(failure.effect.add(failure.source, ShrapnelKind.Explosion), null);
    assert.ok(failure.rendering.size === 0 && failure.kernel.size === 0 && !failure.dummy);
    failure.failObject = false;
    failure.failCapture = true;
    assert.ok(throwsOf(() => failure.effect.add(failure.source, ShrapnelKind.Explosion)));
    assert.ok(failure.rendering.size === 0 && failure.kernel.size === 0 && (failure.pool.slot(failure.dummy)[11] & 3) === 3);
    assert.ok(failure.destroy.preDepth === 0 && failure.destroy.postDepth === 0);
    const attach = new ShrapnelScene();
    const original = attach.host.attach;
    attach.host.attach = () => { throw new Error('명시 부착 실패'); };
    assert.ok(throwsOf(() => attach.effect.add(attach.source, ShrapnelKind.Collapse)));
    assert.ok(attach.effect.size === 0 && attach.rendering.size === 0 && attach.kernel.size === 0 && (attach.pool.slot(attach.dummy)[11] & 3) === 3);
    assert.deepEqual(attach.releases, [ShrapnelRelease.Object]);
    attach.host.attach = original;
    assert.ok(attach.effect.add(attach.source, ShrapnelKind.Collapse));
    assert.ok(attach.effect.size === 1 && attach.kernel.size === 1);
});

test('파편 표시 시스템 경계: 세계/반납·프레임 출력 경계 누락·배율 불일치·source 0 을 등록 전에 거부하고 dispose 가 남은 파편을 정리한다', () => {
    const scene = new ShrapnelScene();
    const hooks = scene.hooks();
    assert.ok(throwsOf(() => new ShrapnelVisualSystem(scene.rendering, scene.game, scene.crt, scene.frame, { ...hooks, destroyUnpaired: undefined })));
    assert.ok(throwsOf(() => new ShrapnelVisualSystem(scene.rendering, scene.game, scene.crt, scene.frame, { ...hooks, lifetime: { ...hooks.lifetime, capture: undefined } })));
    assert.ok(throwsOf(() => new ShrapnelVisualSystem(scene.rendering, scene.game, scene.crt, scene.frame, { ...hooks, frame: { ...hooks.frame, pixel: undefined } })));
    assert.ok(throwsOf(() => new ShrapnelVisualSystem(scene.rendering, scene.game, {}, scene.frame, hooks)), 'CRT 난수 누락');
    assert.ok(throwsOf(() => new ShrapnelVisualSystem(scene.rendering, scene.game, scene.crt, { ...scene.frame, pixelScaleX: 17 }, hooks)));
    assert.ok(throwsOf(() => scene.effect.add(0, ShrapnelKind.Explosion)));
    assert.ok(scene.effect.add(scene.source, ShrapnelKind.Variant2));
    assert.ok(scene.effect.size === 1 && scene.kernel.size === 1);
    scene.effect.dispose();
    assert.ok(scene.effect.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0);
    assert.ok(scene.destroy.preDepth === 0 && scene.destroy.postDepth === 0);
});
