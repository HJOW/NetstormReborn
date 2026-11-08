// 빔 표시 시스템 lightning-visual-system.js(LightningVisualSystem)를 불꽃 꼬리 표시와 같은 공통 표시 저장소·실제 Host/Kernel 위에서 검사한다(cpppj `tests/LightningVisualSystemTests.cpp` 3건을 그대로 옮김).
//  - 자연 수명과 파일명 소리: Form TYPE 19·120입자·경로 확보·공유 색 88·깊이 −25, 첫 프레임 Seed 1입자와 파일명 소리 1회(출력 선 없음), 다음 시간 조각의 중심/보조 색 195/85 선, 소진 뒤 자기 삭제와 경로 → 입자 → 객체 반납·오래된 토큰 출력 없음.
//  - 120입자/경로 확보 실패와 생성 중 재진입(프레임/출력/수집 무동작·중첩 거부)이 Form/등록/Kernel 잔재를 남기지 않음.
//  - 불꽃 꼬리(process 11)와 같은 목록/토큰/수집을 공유하고 부모 종속 삭제가 두 표시를 정리하며 삭제 dirty 2건을 냄.
//  - 웹 추가: 화면 정수 양끝 생성(addAtScreen)이 같은 Host/등록 수명을 쓰고 부모 0·배율 불일치를 거부한다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E196: 빔 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
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
import { LightningRelease } from '../www/core/raw-lightning-lifetime.js';
import { RenderProcessSystem } from '../www/core/render-process-system.js';
import { FieryTailVisualSystem } from '../www/core/fiery-tail-visual-system.js';
import { LightningVisualSystem } from '../www/core/lightning-visual-system.js';

// 내장 Form 생성자 주소와 이 검사의 명시 HP 보유 부모 타입 100 을 넣은 188칸 입력이다(cpppj Types). (2026-11-09 WB04-E196)
function types() {
    const result = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    result[100] = { ...result[100], flags1: 0x10, maxHitPoints: 100, footX: 1, footY: 1 };
    return result;
}

// 예외가 나는지 본다. (2026-11-09 WB04-E196)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 raw 풀/공통 삭제/Kernel/Host 에서 다른 표시와 빔을 함께 쓰는 장면이다(cpppj LightningScene). (2026-11-09 WB04-E196)
class LightningScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = types();
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        // 합성 부모의 자산 경계만 명시 완료하고 실제 Form 삭제는 Host 가 처리한다.
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock, {
            asset: {
                selected: () => 0,
                emit: (event) => {
                    if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) this.destroy.completePreDestroy();
                    if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.destroy.completePostDestroy();
                }
            }
        });
        this.random = new GameRandom(0x12345678);
        this.visual = createConstructionVisualState();
        this.frame = { now: 0, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
        this.beamLifetime = { palette: 88 };
        this.fieryLifetime = { palette: 77 };
        // 불꽃 꼬리 출력의 인접 DWORD 도 명시 입력이다.
        this.colors = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
        this.fieryPalette = { count: 11, readable: this.colors };
        this.dirty = [];
        this.lines = [];
        this.sounds = [];
        this.releases = [];
        this.probe = false;
        this.failBytes = 0;
        this.probes = 0;
        this.parent = this.asset(20.5, 21.25);
        this.target = this.asset(40.125, 35.375);
        this.frame.now = 2;
        this.rendering = new RenderProcessSystem(this.pool, this.host, this.visual, { reserve: () => true, releaseArray: () => {} });
        this.fiery = new FieryTailVisualSystem(this.rendering, this.random, this.fieryLifetime, this.frame, this.fieryPalette, this.fieryHooks());
        this.beam = new LightningVisualSystem(this.rendering, this.random, this.beamLifetime, this.frame, this.beamHooks());
    }

    // 실제 raw 부모/표적의 공간/피해 자료만 구성한다. (2026-11-09 WB04-E196)
    asset(x, y) {
        const sid = this.pool.allocate(0);
        const raw = this.pool.allocated(sid);
        put(raw, 0, 0x501dd0);
        raw[10] = 100;
        raw[11] = 0;
        raw[34] = 1;
        raw[35] = 255;
        raw[40] = 1;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 26, 50);
        return sid;
    }

    // 원본 불꽃 꼬리 몸체도 같은 저장소에 등록하며 장치 요청만 관찰한다. (2026-11-09 WB04-E196)
    fieryHooks() {
        return {
            lifetime: { reserve: (bytes) => { assert.equal(bytes, 2880); return true; } },
            frame: { invalidate: (rect) => this.dirty.push(rect), line: () => {} },
            endpoint: () => [326, 230],
            release: () => {}
        };
    }

    // 실제 프레임/소진 삭제와 원본 확보량만 쓰며 빈 빔으로 대체하지 않는다. (2026-11-09 WB04-E196)
    beamHooks() {
        return {
            lifetime: {
                reserve: (bytes) => {
                    assert.ok(bytes === 2880 || bytes === 64);
                    if (this.probe) {
                        this.probes += 1;
                        const before = this.random.state;
                        this.kernel.runFrame();
                        this.rendering.draw();
                        const work = new Uint8Array(RENDER_ENTRY_BYTES * 2);
                        assert.ok(before === this.random.state && this.rendering.collect({ left: 0, top: 0, right: 256, bottom: 256 }, work, 2) === 0 && this.lines.length === 0);
                        assert.ok(throwsOf(() => this.beam.add(this.parent, this.target, null)));
                    }
                    return bytes !== this.failBytes;
                }
            },
            frame: {
                invalidate: (rect) => this.dirty.push(rect),
                line: (line) => this.lines.push(line),
                sound: (x, y, name) => { assert.ok(x === 20.5 && y === 21.25); this.sounds.push(name); }
            },
            center: (sid) => {
                const raw = this.pool.slot(sid);
                return [Math.fround(bitsToFloat(get(raw, 14)) - 0.5), Math.fround(bitsToFloat(get(raw, 18)) - 0.5)];
            },
            release: (released) => this.releases.push(released)
        };
    }
}

test('빔 표시 시스템: Seed/파일명 소리·실제 선 출력과 자기 소진 삭제가 같은 Kernel/공통 등록에 연결된다', () => {
    const scene = new LightningScene();
    const beam = scene.beam.add(scene.parent, scene.target, 'shortZap.wav');
    const form = beam.form;
    const token = beam.fields.registration.token;
    assert.ok(scene.kernel.size === 1 && scene.rendering.size === 1 && scene.pool.slot(form)[10] === 19 && scene.host.fromForm(form) === beam);
    const fields = beam.fields;
    assert.ok(fields.particles.values.length === 120 && fields.particles.total === 0 && fields.pathAllocated && fields.palette === 88 && fields.registration.zOrder === -25);
    scene.kernel.runFrame();
    assert.ok(beam.fields.first && beam.fields.particles.total === 1);
    assert.deepEqual(scene.sounds, ['shortZap.wav']);
    scene.rendering.draw();
    assert.equal(scene.lines.length, 0);
    scene.frame.now = 2.025;
    scene.kernel.runFrame();
    scene.rendering.draw();
    assert.ok(scene.lines.length > 0);
    // 모든 빔 선은 원본 리터럴 중심/보조 색을 쓴다.
    for (const line of scene.lines) assert.ok(line.color === 195 || line.color === 85);
    scene.frame.now = 2.6;
    scene.kernel.runFrame();
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0 && scene.beam.size === 0 && (scene.pool.slot(form)[11] & 1));
    assert.ok(scene.sounds.length === 1 && scene.beamLifetime.palette === 88 && scene.dirty.length > 0);
    assert.deepEqual(scene.releases, [LightningRelease.Path, LightningRelease.Particles, LightningRelease.Object]);
    scene.lines.length = 0;
    scene.rendering.drawToken(token);
    assert.equal(scene.lines.length, 0);
});

test('빔 표시 시스템: 120입자/경로 확보 실패와 생성 중 재진입이 Form/등록/Kernel 잔재를 남기지 않는다', () => {
    const scene = new LightningScene();
    scene.probe = true;
    scene.failBytes = 2880;
    assert.ok(throwsOf(() => scene.beam.add(scene.parent, scene.target, null)));
    assert.ok(scene.probes === 1 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.beam.size === 0);
    scene.failBytes = 64;
    scene.releases.length = 0;
    assert.ok(throwsOf(() => scene.beam.add(scene.parent, scene.target, null)));
    assert.ok(scene.probes === 3 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.beam.size === 0);
    assert.deepEqual(scene.releases, [LightningRelease.Particles, LightningRelease.Object]);
    scene.failBytes = 0;
    const beam = scene.beam.add(scene.parent, scene.target, null);
    assert.ok(scene.probes === 5 && beam.fields.registration.slot === 0);
    scene.probe = false;
    scene.kernel.runFrame();
    assert.equal(scene.sounds.length, 0);
    scene.host.kill(beam, 0x10);
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0);
});

test('빔 표시 시스템: 불꽃 꼬리와 같은 목록/토큰/수집을 공유하고 부모 종속 삭제가 두 표시를 실제로 정리한다', () => {
    const scene = new LightningScene();
    const tail = scene.fiery.add(scene.parent);
    const beam = scene.beam.add(scene.parent, scene.target, null);
    assert.ok(tail.fields.registration.token === 1 && beam.fields.registration.token === 2 && scene.kernel.size === 2 && scene.rendering.size === 2);
    scene.kernel.runFrame();
    const entries = new Uint8Array(RENDER_ENTRY_BYTES * 2);
    assert.equal(scene.rendering.collect({ left: 0, top: 0, right: 256, bottom: 256 }, entries, 2), 2);
    const form = beam.form;
    scene.dirty.length = 0;
    scene.pool.allocated(scene.parent)[11] |= 4;
    scene.destroy.destroy(scene.parent, 0, scene.host.hooks());
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0 && scene.fiery.size === 0 && scene.beam.size === 0 && (scene.pool.slot(form)[11] & 1));
    assert.equal(scene.dirty.length, 2);
    assert.ok(scene.beamLifetime.palette === 88 && scene.fieryLifetime.palette === 77);
});

test('빔 표시 시스템(웹 추가): 화면 정수 양끝 생성도 같은 Host/등록 수명을 쓰고 부모 0·배율 불일치를 거부하며 dispose 가 정리한다', () => {
    const scene = new LightningScene();
    assert.ok(throwsOf(() => scene.beam.addAtScreen(0, [10, 20], [30, 40], 0, null)));
    const beam = scene.beam.addAtScreen(scene.parent, [10, 20], [330, 240], 0, null);
    assert.ok(beam && scene.kernel.size === 1 && scene.rendering.size === 1 && scene.pool.slot(beam.form)[10] === 19 && beam.fields.pathAllocated);
    scene.frame.pixelScaleX = 17;
    assert.ok(throwsOf(() => scene.beam.add(scene.parent, scene.target, null)));
    assert.ok(throwsOf(() => scene.kernel.runFrame()));
    scene.frame.pixelScaleX = 16;
    scene.beam.dispose();
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0 && scene.beam.size === 0);
    assert.deepEqual(scene.releases, [LightningRelease.Path, LightningRelease.Particles, LightningRelease.Object]);
});
