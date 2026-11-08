// 공통 표시 저장소 render-process-system.js(RenderProcessSystem)와 연기 표시 시스템 smoke-visual-system.js(SmokeVisualSystem)를 실제 Host/Kernel/공통 삭제 위에서 검사한다
// (cpppj `tests/SmokeVisualSystemTests.cpp` 4건을 그대로 옮김). 독립 fixture 가 없는 조립 시험이며 연기 계산 자체는 E191 의 실제 PE 대조 몸체를 그대로 쓴다.
//  - 실제 Host 프레임/출력/자기 삭제: 냉각 연기 생성(TYPE 21 Form·200입자·10회/간격 3·깊이 부모−1)·10프레임 방출·출력·수명 끝 다음 프레임 자기 삭제와 입자 → 객체 반납.
//  - 생성 중 차단/실패 정리/공유 색 재사용: 생성 중 프레임/출력/수집이 아무것도 하지 않고 중첩 생성 거부, 입자 확보 실패 시 Host 삭제로 객체만 반납, 두 번째 생성부터 공유 색 재사용.
//  - 일반 연기 기본값(8회/간격 5)과 부모 0 독립 소유(Host/Kernel 에 붙지 않고 시스템 정리 때 반납).
//  - 00487960 삭제 영역: render-removal-x86.tsv 기본 027f 48행(비교 037f 48행은 건너뜀)을 DWORD 단위로 대조한다.
//  - RenderProcessSystem 단독 계약(웹 추가): 00487960 삭제 영역의 정수화/저장 순서, 토큰·ready·forget·부착 토큰 불일치·Host 풀 불일치 거부. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E192: 공통 표시 저장소·연기 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { REPO_ROOT } from './helpers.mjs';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { createConstructionVisualState } from '../www/core/raw-construction-visual.js';
import { createRenderProcessFields } from '../www/core/raw-construction-lifetime.js';
import { RENDER_ENTRY_BYTES } from '../www/core/raw-render-collection.js';
import { SmokeRelease, buildSmokePalette, createSmokeRequest } from '../www/core/raw-smoke-effect.js';
import { RenderProcessSystem, renderProcessRemovalBounds } from '../www/core/render-process-system.js';
import { SmokeVisualSystem } from '../www/core/smoke-visual-system.js';

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력에 부모 129 의 원문 HP/group 을 더한다(cpppj SmokeVisualTypes). (2026-11-09 WB04-E192)
function smokeVisualTypes() {
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    types[129] = { ...types[129], flags1: 0x10, group: 1, maxHitPoints: 600, footX: 3, footY: 3 };
    return types;
}

// 예외가 나는지 본다. (2026-11-09 WB04-E192)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 단일 실제 풀/Host 와 같은 공통 표시 저장소를 가진 연기 조립 장면이다(cpppj SmokeVisualScene). (2026-11-09 WB04-E192)
class SmokeVisualScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = smokeVisualTypes();
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        // 합성 부모의 공통 전후처리만 완료하며 Form 의 실제 삭제/부착은 그대로 실행한다.
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
        this.frame = { now: 2, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
        this.lifetime = { palette: 0 };
        // 마지막 한 값은 이 검사의 명시 인접 메모리이다.
        this.colors = new Array(13).fill(0);
        this.palette = { count: 12, readable: this.colors };
        this.dirty = [];
        this.lines = [];
        this.released = [];
        this.palettes = 0;
        this.probes = 0;
        this.probe = false;
        this.fail = false;
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
        this.clock.now = 2;
        this.rendering = new RenderProcessSystem(this.pool, this.host, this.visual, { reserve: () => true, releaseArray: () => {} });
        this.smoke = new SmokeVisualSystem(this.rendering, this.random, this.lifetime, this.frame, this.palette, this.hooks());
    }

    // 생성/프레임은 실제 몸체이며 출력과 실제 배열 확보의 실패만 경계로 둔다. (2026-11-09 WB04-E192)
    hooks() {
        return {
            lifetime: {
                reserve: (bytes) => {
                    assert.ok(bytes === 4800 || bytes === 8);
                    if (this.probe && bytes === 4800) {
                        // 생성 중(부착 뒤·준비 전) 프레임/출력/수집은 아무것도 하지 않고 중첩 생성은 거부한다.
                        this.probes += 1;
                        const before = this.random.state;
                        this.kernel.runFrame();
                        this.rendering.draw();
                        const work = new Uint8Array(RENDER_ENTRY_BYTES * 2);
                        assert.ok(this.random.state === before && this.rendering.collect({ left: 0, top: 0, right: 256, bottom: 256 }, work, 2) === 0 && this.lines.length === 0);
                        assert.ok(throwsOf(() => this.smoke.addCooling(this.parent, [0, 0])));
                    }
                    return !(this.fail && bytes === 4800);
                },
                createPalette: () => { this.palettes += 1; return 7; },
                buildPalette: (token) => {
                    assert.equal(token, 7);
                    buildSmokePalette().forEach((value, index) => { this.colors[index] = value; });
                    this.colors[12] = 0xcafef00d;
                }
            },
            frame: { invalidate: (rect) => this.dirty.push(rect), line: (line) => this.lines.push(line) },
            release: (release) => this.released.push(release)
        };
    }

    // 프레임 시각과 Host 시계를 같이 옮긴다. (2026-11-09 WB04-E192)
    advance(seconds) {
        this.frame.now += seconds;
        this.clock.now = this.frame.now;
    }
}

test('연기 표시 시스템: 실제 Host 위 냉각 연기의 10회 방출·출력과 수명 끝 다음 프레임 자기 삭제를 확인한다', () => {
    const scene = new SmokeVisualScene();
    const process = scene.smoke.addCooling(scene.parent, [16, -11]);
    assert.ok(process);
    const form = process.form;
    const token = process.fields.registration.token;
    assert.ok(scene.kernel.size === 1 && scene.rendering.size === 1 && scene.host.fromForm(form) === process && scene.pool.slot(form)[10] === 21);
    const fields = process.fields;
    assert.ok(fields.capacity === 200 && fields.total === 0 && fields.remaining === 10 && fields.period === 3 && fields.registration.zOrder === 6);
    assert.equal(scene.palettes, 1);
    scene.kernel.runFrame();
    assert.ok(process.fields.first && process.fields.total === 0);
    // 원본은 한 호출마다 유한 방출 수를 하나씩 소비한다.
    for (let index = 0; index < 10; index += 1) {
        scene.advance(0.025);
        scene.kernel.runFrame();
    }
    assert.ok(process.fields.remaining === 0 && process.fields.total > 0);
    scene.rendering.draw();
    assert.ok(scene.lines.length > 0 && scene.dirty.length > 0);
    // 원문은 이번에 수명이 끝난 입자도 이동/dirty 후 다음 프레임에서 자기 삭제한다.
    scene.advance(100);
    scene.kernel.runFrame();
    assert.equal(scene.smoke.size, 1);
    scene.kernel.runFrame();
    assert.ok(scene.smoke.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.host.fromForm(form) === null && scene.rendering.lookup(token) === null);
    assert.deepEqual(scene.released, [SmokeRelease.Particles, SmokeRelease.Object]);
    assert.ok(scene.destroy.preDepth === 0 && scene.destroy.postDepth === 0);
});

test('연기 표시 시스템: 생성 중 출력/중첩을 차단하고 확보 실패 뒤 정리하며 두 번째 생성부터 공유 색을 재사용한다', () => {
    const scene = new SmokeVisualScene();
    scene.probe = true;
    scene.fail = true;
    assert.ok(throwsOf(() => scene.smoke.addCooling(scene.parent, [16, -11])));
    assert.ok(scene.probes === 1 && scene.smoke.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0 && scene.palettes === 0);
    assert.deepEqual(scene.released, [SmokeRelease.Object]);
    scene.probe = false;
    scene.fail = false;
    const first = scene.smoke.addCooling(scene.parent, [16, -11]);
    const second = scene.smoke.addCooling(scene.parent, [16, -11]);
    assert.ok(first && second);
    assert.ok(scene.palettes === 1 && scene.smoke.size === 2 && scene.rendering.size === 2 && scene.kernel.size === 2);
    scene.host.kill(first, 0x10);
    scene.host.kill(second, 0x10);
    assert.ok(scene.smoke.size === 0 && scene.kernel.size === 0 && scene.rendering.size === 0);
});

test('연기 표시 시스템: 일반 연기는 같은 Host/팔레트를 쓰되 냉각 wrapper 의 10회/간격 3 을 덮어쓰지 않는다', () => {
    const scene = new SmokeVisualScene();
    const request = createSmokeRequest();
    request.parent = scene.parent;
    const process = scene.smoke.add(request);
    assert.ok(process);
    const form = process.form;
    const fields = process.fields;
    assert.ok(fields.remaining === 8 && fields.period === 5 && fields.maximumLife === 100 && fields.velocity === -1 && fields.spread === 9 && fields.noise === 0.5);
    assert.ok(fields.hotspotX === 0 && fields.hotspotY === 0 && fields.windX === 0 && fields.windY === 0 && fields.capacity === 200 && fields.total === 0);
    assert.ok(scene.kernel.size === 1 && scene.rendering.size === 1 && scene.host.fromForm(form) === process && scene.pool.slot(form)[10] === 21);
    scene.kernel.runFrame();
    assert.ok(process.fields.first);
    scene.rendering.draw();
    scene.host.kill(process, 0x10);
    assert.ok(scene.kernel.size === 0 && scene.smoke.size === 0 && scene.rendering.size === 0);
    assert.deepEqual(scene.released, [SmokeRelease.Particles, SmokeRelease.Object]);
});

test('연기 표시 시스템: 원문 공통 생성자는 부모 0 의 객체를 등록만 하고 Host/Kernel 에 붙이지 않으며 시스템 정리 때 반납한다', () => {
    const scene = new SmokeVisualScene();
    const standalone = scene.smoke.add(createSmokeRequest());
    assert.ok(standalone);
    const token = standalone.fields.registration.token;
    assert.ok(scene.kernel.size === 0 && scene.smoke.size === 1 && scene.rendering.size === 1 && standalone.form === 0);
    const request = createSmokeRequest();
    request.parent = scene.parent;
    const attached = scene.smoke.add(request);
    assert.ok(attached);
    scene.kernel.runFrame();
    assert.ok(scene.kernel.size === 1 && scene.smoke.size === 2 && scene.rendering.size === 2 && !standalone.fields.first && standalone.fields.remaining === 8 && scene.rendering.lookup(token) !== null);
    scene.host.kill(attached, 0x10);
    scene.smoke.dispose();
    assert.ok(scene.kernel.size === 0 && scene.rendering.size === 0 && scene.rendering.lookup(token) === null);
    assert.deepEqual(scene.released, [SmokeRelease.Particles, SmokeRelease.Object, SmokeRelease.Particles, SmokeRelease.Object]);
});

test('공통 표시 저장소: 삭제 영역의 정수화/저장 순서와 토큰·ready·forget·부착 불일치·Host 풀 불일치를 원문 계약대로 처리한다', () => {
    // 각 변을 먼저 절삭한 뒤 카메라를 빼고 1픽셀 여유를 준다(top 만 저장 없이 넓은 값).
    // 예: top 20.9 → 20 → (20 − 0.25) − 1 = 18.75 → 18, 음수 left −3.7 → −3 → −3 + 1.5 = −1.5 → −2.5 → −2(0 방향 절삭).
    assert.deepEqual(renderProcessRemovalBounds({ left: 10.9, top: 20.9, right: 30.2, bottom: 40.7 }, 0.5, 0.25), [8, 18, 30, 40]);
    assert.deepEqual(renderProcessRemovalBounds({ left: -3.7, top: -2.2, right: 1.5, bottom: 2.5 }, -1.5, 1.75), [-2, -4, 3, 1]);
    assert.ok(throwsOf(() => renderProcessRemovalBounds({ left: NaN, top: 0, right: 0, bottom: 0 }, 0, 0)));
    const scene = new SmokeVisualScene();
    const other = new SidPool(32768, { server: true });
    assert.ok(throwsOf(() => new RenderProcessSystem(other, scene.host, scene.visual, { reserve: () => true, releaseArray: () => {} })), 'Host 와 다른 풀');
    const rendering = scene.rendering;
    const fields = createRenderProcessFields();
    let drawn = 0;
    const token = rendering.track(fields, () => {}, () => { drawn += 1; });
    assert.ok(token > 0 && fields.token === token && rendering.size === 1 && rendering.lookup(token) === null);
    // 아직 원본 등록(slot)이 없으면 ready 를 거부하고 출력하지 않는다.
    assert.ok(throwsOf(() => rendering.ready(token)));
    rendering.registry.construct(fields, token, 0, 21);
    rendering.ready(token);
    assert.equal(rendering.lookup(token), fields);
    rendering.draw();
    assert.equal(drawn, 1);
    // 이미 등록된 필드나 출력 경계 누락은 새 토큰을 만들지 않는다.
    assert.ok(throwsOf(() => rendering.track(fields, () => {}, () => {})));
    assert.ok(throwsOf(() => rendering.track(createRenderProcessFields(), null, () => {})));
    // 다른 필드 객체로 같은 토큰을 부착하면 거부한다.
    const stray = createRenderProcessFields();
    stray.token = token;
    assert.ok(throwsOf(() => rendering.registry.construct(stray, 999, scene.parent, 21)));
    rendering.registry.destroy(fields);
    rendering.forget(token);
    rendering.forget(token);
    assert.ok(rendering.size === 0 && rendering.lookup(token) === null);
    rendering.drawToken(token);
    assert.equal(drawn, 1);
});

// 실제 PE 관측 fixture: 00487960 공통 Form 삭제 영역(기본 027f 48행 + 비교 037f 48행, 비교 행은 중복 가산하지 않음). 입력은 영역 float 넷·카메라 둘·flags, 기대는 signed DWORD 넷과 flags 0 이다. (2026-11-09 WB04-E192)
test('공통 표시 저장소: 00487960 Form 삭제 영역 기본 027f 48입력을 실제 PE 와 DWORD 단위로 대조한다', () => {
    const removalRows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'render-removal-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
    const decode = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));
    const word = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
    const single = (bytes, offset) => {
        const view = new DataView(new ArrayBuffer(4));
        view.setUint32(0, word(bytes, offset), true);
        return view.getFloat32(0, true);
    };
    let count = 0;
    let extended = 0;
    // 기대값은 다시 계산하지 않고 원문 DWORD 를 그대로 비교한다.
    for (const row of removalRows) {
        assert.equal(row.length, 4);
        if (Number(row[1]) === 0x037f) { extended += 1; continue; }
        assert.equal(Number(row[1]), 0x027f);
        const payload = decode(row[2]);
        const expected = decode(row[3]);
        assert.ok(payload.length === 28 && expected.length === 20);
        const actual = renderProcessRemovalBounds({ left: single(payload, 0), top: single(payload, 4), right: single(payload, 8), bottom: single(payload, 12) }, single(payload, 16), single(payload, 20));
        actual.forEach((value, index) => assert.equal(value >>> 0, word(expected, index * 4), `표시 삭제 영역 ${row[0]} ${index}`));
        assert.equal(word(expected, 16), 0);
        count += 1;
    }
    assert.equal(count, 48);
    assert.equal(extended, 48);
});

// 웹 추가(2026-11-09 WB04-E194): Host 부착이 예외로 실패해도 C++ unique_ptr 매개변수 해제처럼 객체를 바로 소멸시켜 등록/조회를 남기지 않고, 다음 생성이 정상이다.
test('연기 표시 시스템: Host 부착 예외 실패는 객체를 즉시 소멸해 표시 등록을 남기지 않고 다음 생성을 막지 않는다', () => {
    const scene = new SmokeVisualScene();
    const attach = scene.host.attach;
    scene.host.attach = () => { throw new Error('명시 부착 실패'); };
    assert.ok(throwsOf(() => scene.smoke.addCooling(scene.parent, [16, -11])));
    assert.ok(scene.smoke.size === 0 && scene.rendering.size === 0 && scene.kernel.size === 0 && scene.rendering.slots[0] === 0);
    assert.deepEqual(scene.released, [SmokeRelease.Object]);
    scene.host.attach = attach;
    assert.ok(scene.smoke.addCooling(scene.parent, [16, -11]));
    assert.ok(scene.smoke.size === 1 && scene.kernel.size === 1);
});
