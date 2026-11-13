// 입자 본체 raw-campaign-particles.js(RawCampaignParticles)를 실제 번호 할당·공통 삭제·Host·Kernel 위에서 검사한다(cpppj `ParticleRenderingTests.cpp` 의 CampaignParticles_* 두 건에 대응하고,
// 웹에 Renderer 본체가 아직 없어 장면 합성은 시험 쪽의 단순 그리기(모든 직접 그리기 절차를 dirty 사각형 clip 으로 호출)로 대신한다. Renderer_ProceduresShareSortClipAndDirtyRemoval 은 Renderer 단위의 몫이다).
//  - 실제 GIFCLOUD.COL 팔레트로 만든 불꽃 색 표(11색 + 인접 DWORD 1)의 색 번호 196 선이 부모 화면 위치(328, 234)에 찍히고, 카메라가 바뀌면 같은 선이 이동하며, 부모 삭제로 효과가 사라진다.
//  - 번개 빔의 파일명 위치 음향이 첫 프레임에 한 번, 색 195/85 선, 소진 뒤 목록·Kernel 비움, 죽은 표적의 TargetUnavailable.
//  - 생성자/연결/바인딩/dirty 대기와 흐름/정렬 키/예외 뒤 출력 대상 해제/정리 순서 계약.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E390: 5-A RawCampaignParticles 이식으로 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { readData } from './helpers.mjs';
import { World } from './construction-visual-support.mjs';
import { GamePalette } from '../www/assets/palette.js';
import { RawCampaignParticles, particlePaletteInput } from '../www/render/raw-campaign-particles.js';
import { CameraView, projectCameraCoordinates } from '../www/render/camera-view.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { LightningCreateStatus } from '../www/core/raw-lightning-lifetime.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { formConstructorVtable } from '../www/core/squid-forms.js';
import { renderProcessRemovalBounds } from '../www/core/render-process-system.js';
import { buildLightningPalette } from '../www/core/particle-gradient.js';
import { buildEvilRayPalette } from '../www/core/raw-evil-ray-lifetime.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';
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
import { ShrapnelKind } from '../www/core/raw-shrapnel-frame.js';
import { buildSmokePalette, createSmokeRequest } from '../www/core/raw-smoke-effect.js';

// 실제 GIFCLOUD 팔레트이다. (2026-11-13 WB04-E390)
const PALETTE = GamePalette.parse(readData('d/GIFCLOUD.COL'));
// 화면 전체 사각형이다. (2026-11-13 WB04-E390)
const SCREEN = Object.freeze({ left: 0, top: 0, right: 1024, bottom: 768 });
const VIEW = Object.freeze({ cameraX: 0, cameraY: 0, left: 0, top: 0, right: 1024, bottom: 768 });

// 예외가 나는지 본다. (2026-11-13 WB04-E390)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 풀/Host/Kernel 위에 부모와 표적을 두고 입자 본체를 만드는 장면이다. 합성 TYPE 188칸·빈 SHP 표·실제 이웃 탐색(합성 발자국 0)을 쓴다. (2026-11-13 WB04-E390)
class Scene {
    constructor(options = {}) {
        // prepare 는 입자 본체를 만들기 전에 TYPE/SHP 표를 제자리에서 바꾸는 시험용 훅이다.
        const { prepare = null, ...particleOptions } = options;
        this.world = new World();
        const world = this.world;
        this.shapes = world.types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
        this.neighbors = new RawSquidNeighbors(world.pool, world.hash, world.spots, world.types, world.types.map(() => []));
        this.core = { pool: world.pool, types: world.types, shapes: this.shapes, processes: world.host, neighbors: this.neighbors, kernel: world.kernel };
        this.dirty = [];
        this.sounds = [];
        this.random = world.random;
        if (prepare) prepare(this);
        this.particles = new RawCampaignParticles(this.core, this.random, PALETTE, { sound: (x, y, name) => this.sounds.push([x, y, name]), ...particleOptions });
        this.display = { suppressed: () => false, invalidate: (rect, flags) => this.dirty.push([rect, flags]) };
    }

    // 모든 직접 그리기 절차를 같은 clip 으로 불러 8비트 표면에 합성한다(웹 Renderer 본체 전의 시험용 대역). (2026-11-13 WB04-E390)
    render(sprites, fill = 0, clip = SCREEN) {
        const pixels = new Uint8Array(clip.right * clip.bottom).fill(fill);
        for (const sprite of sprites) sprite.procedure.draw(pixels, clip.right, clip.bottom, sprite.clip);
        return pixels;
    }

    // 새 부모 하나를 같은 풀에 확보한다. (2026-11-13 WB04-E390)
    parent(type, x, y) {
        const sid = this.world.pool.allocate(0);
        const raw = this.world.pool.allocated(sid);
        put(raw, 0, 0x501dd0);
        raw[10] = type;
        raw[11] = 0;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        return sid;
    }

    // 부모의 Form 에 붙은 프로세스를 돌려준다. (2026-11-13 WB04-E390)
    processOf(parent) {
        return this.world.host.fromForm(get(this.world.pool.slot(parent), 6, 2));
    }

    // 한 프레임: 현재 시각을 공급하고 Kernel 을 한 번 돌린다. (2026-11-13 WB04-E390)
    frame(now, number = 0, weakened = false) {
        this.particles.prepareFrame(now, number, weakened);
        this.world.kernel.runFrame();
    }
}

// 픽셀 배열에서 특정 색의 개수이다. (2026-11-13 WB04-E390)
const countOf = (pixels, color) => pixels.reduce((sum, pixel) => sum + (pixel === color ? 1 : 0), 0);

test('CampaignParticles_ActualHostPaletteAndVfxPixels: 실제 팔레트의 불꽃 꼬리가 부모 화면 위치에 색 196 선을 찍고 카메라 이동·삭제를 따른다', () => {
    const scene = new Scene();
    const { world, particles } = scene;
    const parent = scene.parent(80, 20.5, 21.25);
    particles.setView(VIEW, scene.display);
    particles.add(11, parent);
    assert.ok(particles.rendering.size === 1 && world.kernel.size === 1);
    const process = scene.processOf(parent);
    assert.ok(process);
    scene.frame(2);
    assert.equal(process.fields.total, 10);
    const first = particles.sprites(SCREEN);
    assert.ok(first.length === 1 && typeof first[0].procedure.draw === 'function' && first[0].database === null);
    scene.render(first, 199);
    scene.frame(2.025);
    assert.equal(process.fields.total, 12);
    const pixels = scene.render(particles.sprites(SCREEN), 199);
    assert.equal(pixels[234 * 1024 + 328], 196);
    assert.ok(countOf(pixels, 196) > 3);
    // 회귀 고정(원본 대조가 아닌 이 이식의 결과): 둘째 프레임에 정확히 이 16픽셀이다. 프레임 상태의 scaledDelta 배율(rate 1)도 이 위치에 반영된다.
    const drawn = [];
    for (let index = 0; index < pixels.length; index += 1) if (pixels[index] !== 199) drawn.push(index);
    assert.deepEqual(drawn, [236871, 236873, 237895, 237896, 237897, 238917, 238918, 238920, 239943, 239944, 239945, 240966, 240967, 240968, 241989, 241993]);
    // 수명 3.75 의 원본 색 index10=196 과 배경만 남아야 한다. 초기 인접 값은 이 검사 기대값으로 만들지 않는다.
    assert.ok(pixels.every((pixel) => pixel === 199 || pixel === 196));
    // 카메라 원점 (300, 200): 같은 선이 (28, 34) 로 이동한다.
    particles.setView({ ...VIEW, cameraX: 300, cameraY: 200 }, scene.display);
    const shifted = particles.sprites(SCREEN);
    assert.equal(scene.render(shifted)[34 * 1024 + 28], 196);
    // 미복원 TYPE 12 는 거부하고 목록은 그대로이다.
    assert.throws(() => particles.add(12, parent), /미복원 TYPE12/);
    assert.equal(particles.rendering.size, 1);
    // 부모 삭제로 표시 목록과 Kernel 이 비고, 이미 만든 절차를 다시 그려도 아무 것도 찍히지 않는다.
    world.host.kill(process, 0x10);
    assert.ok(particles.rendering.size === 0 && world.kernel.size === 0);
    assert.equal(countOf(scene.render(shifted), 196), 0);
});

test('CampaignParticles_BeamRendererSoundAndRemoval: 빔이 공유 팔레트·파일명 음향·VFX 선으로 이어지고 소진 뒤 사라지며 죽은 표적은 건너뛴다', () => {
    const scene = new Scene();
    const { world, particles } = scene;
    // 실제 이웃 탐색의 중심 조회가 요구하는 합성 발자국(1×1)을 두 TYPE 에 준다.
    for (const type of [148, 128]) Object.assign(world.types[type], { footX: 1, footY: 1 });
    const parent = scene.parent(148, 20.5, 21.25);
    const victim = scene.parent(128, 40.125, 35.375);
    particles.setView(VIEW, scene.display);
    const created = particles.addBeam(parent, victim, 'shortZap.wav');
    assert.ok(created.status === LightningCreateStatus.Created && created.process !== null);
    assert.ok(world.kernel.size === 1 && particles.rendering.size === 1);
    scene.frame(2);
    assert.deepEqual(scene.sounds, [[20.5, 21.25, 'shortZap.wav']]);
    scene.frame(2.025);
    const image = scene.render(particles.sprites(SCREEN));
    assert.ok(countOf(image, 195) > 0 && countOf(image, 85) > 0);
    assert.ok(image.every((color) => color === 0 || color === 195 || color === 85));
    scene.frame(2.6);
    assert.ok(world.kernel.size === 0 && particles.rendering.size === 0 && scene.sounds.length === 1);
    assert.ok(scene.render(particles.sprites(SCREEN)).every((color) => color === 0));
    // 죽은(상태 비트 2) 표적은 생성 없이 건너뛴 SID 를 돌려준다.
    world.pool.allocated(victim)[11] |= 2;
    const skipped = particles.addBeam(parent, victim, null);
    assert.ok(skipped.status === LightningCreateStatus.TargetUnavailable && skipped.skippedTarget === victim && world.kernel.size === 0);
});

test('projectCameraCoordinates: CameraView.project 와 같은 값이고 원점은 DWORD 감김으로 뺀다', () => {
    const view = new CameraView();
    view.initialize();
    for (const [originX, originY] of [[0, 0], [17, 29], [-29, 13], [2147483647, -2147483648], [-2147483648, 2147483647]]) {
        view.setPixels(originX, originY);
        for (const x of [0, 20.5, -3.75, 1e10, -1e10, 2 ** 40, NaN, Infinity, 0.03125]) {
            for (const y of [0, 21.25, -7.5, 1e9, NaN, -Infinity]) {
                const expected = view.project(x, y);
                assert.deepEqual(projectCameraCoordinates(originX, originY, x, y), [expected.x, expected.y], `${originX},${originY} ${x},${y}`);
            }
        }
    }
    // 16배·11배와 +0.5 의 0 방향 절삭: 음수 좌표는 반올림이 아니다.
    assert.deepEqual(projectCameraCoordinates(0, 0, -0.03125, -0.0454), [0, 0]);
    assert.deepEqual(projectCameraCoordinates(0, 0, -0.04, -0.05), [0, 0]);
    assert.deepEqual(projectCameraCoordinates(10, 20, 1, 1), [16 - 10, 11 - 20]);
});

test('생성자·연결 계약: 옵션 검사, 미연결 효과 거부, 건설/연소 연결의 중복·필수 경계 누락', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    // 위치 음향은 함수만 받고 파편 장치는 공유 CRT 상태가 필수이다.
    assert.throws(() => new RawCampaignParticles(scene.core, scene.random, PALETTE, { sound: 'zap' }), TypeError);
    assert.throws(() => new RawCampaignParticles(scene.core, scene.random, PALETTE, { shrapnel: { lifetime: {}, destroyUnpaired: () => {} } }), /공유 CRT/);
    assert.ok(new RawCampaignParticles(scene.core, scene.random, PALETTE) instanceof RawCampaignParticles);
    // 파편/분해/건설/연소는 연결 전 생성이 거부된다(빈 효과로 통과하지 않음).
    const parent = scene.parent(80, 20.5, 21.25);
    assert.throws(() => particles.addShrapnel(parent, 0), /파편 세계\/캡처 미연결/);
    assert.throws(() => particles.addDissolve(parent, parent, parent, 0), /분해 세계\/캡처 미연결/);
    assert.throws(() => particles.addConstruction(parent, false), /실제 건설 표시 미연결/);
    assert.throws(() => particles.addBurning(parent, 0), /Burning14 캡처\/수명 미연결/);
    // 음향 소비자가 없으면 파일명이 있는 빔과 화면 빔은 거부하지만 파일명 없는 빔은 만들 수 있다.
    const mute = new Scene({ sound: null });
    for (const type of [148, 128]) Object.assign(mute.world.types[type], { footX: 1, footY: 1 });
    const beamParent = mute.parent(148, 20.5, 21.25);
    const beamVictim = mute.parent(128, 40.125, 35.375);
    assert.throws(() => mute.particles.addBeam(beamParent, beamVictim, 'shortZap.wav'), /파일명 음향 소비자 미연결/);
    assert.throws(() => mute.particles.addScreenBolt(beamParent, [10, 20], [30, 40], 0.5), /화면빔 실제파일명 음향미연결/);
    assert.equal(mute.particles.addBeam(beamParent, beamVictim, null).status, LightningCreateStatus.Created);
    // 건설 연결: 함수가 아닌 경계는 거부하고 한 번 연결하면 다시 연결할 수 없다.
    assert.throws(() => particles.connectConstruction(null, () => 100), TypeError);
    assert.throws(() => particles.connectConstruction(() => ({}), null), TypeError);
    particles.connectConstruction(() => ({ width: 1, height: 1, hotspotX: 0, hotspotY: 0, pixels: new Uint8Array(1) }), () => 100);
    assert.throws(() => particles.connectConstruction(() => ({}), () => 100), TypeError);
    // 연소 연결도 같다.
    assert.throws(() => particles.connectBurning(null, () => {}), TypeError);
    assert.throws(() => particles.connectBurning(() => ({}), null), TypeError);
    particles.connectBurning(() => ({ width: 1, height: 1, hotspotX: 0, hotspotY: 0, pixels: new Uint8Array(1) }), () => {});
    assert.throws(() => particles.connectBurning(() => ({}), () => {}), TypeError);
    // 카메라는 CameraView 만 받는다.
    assert.throws(() => particles.bindCameraView({}), TypeError);
    // 게임 시각은 유한 수만 받고 이전 값은 보존된다.
    assert.throws(() => particles.prepareFrame(NaN), /게임 시각 오류/);
    assert.throws(() => particles.prepareBridgeClock(Infinity, 1), RangeError);
    world.kernel.runFrame();
});

test('createTailByConstructor: 내장 Form 네 생성자는 VT 만 쓰고 Kaboom/Particle 은 진단 뒤 NULL, 줄기/불꽃/악마 광선은 시스템에 붙이며 미복원 주소는 거부한다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    // 내장 Form 네 생성자: 새 객체를 만들지 않고 지정한 물리 슬롯의 VT DWORD 하나만 쓴다.
    for (const [address, type] of [[0x004ae760, 1], [0x004ae3d0, 2], [0x004ae9f0, 3], [0x004aea40, 6]]) {
        const target = world.pool.allocate(0);
        const before = Uint8Array.from(world.pool.slot(target));
        particles.createTailByConstructor(address, target);
        const after = world.pool.slot(target);
        assert.equal(get(after, 0, 4), formConstructorVtable(type), `생성자 ${address.toString(16)}`);
        assert.deepEqual(Array.from(after.subarray(4)), Array.from(before.subarray(4)));
    }
    assert.ok(world.kernel.size === 0 && particles.rendering.size === 0);
    // Kaboom/Particle 은 진단 장치가 있어야 하고, 진단 뒤 아무 것도 만들지 않고 복귀한다.
    const parent = scene.parent(80, 20.5, 21.25);
    assert.throws(() => particles.createTailByConstructor(0x00470b00, parent), /진단 장치 미연결/);
    assert.throws(() => particles.createTailByConstructor(0x00487ef0, parent, 'x'), /진단 장치 미연결/);
    const calls = [];
    particles.createTailByConstructor(0x00470b00, parent, (...args) => calls.push(args));
    particles.createTailByConstructor(0x00487ef0, parent, (...args) => calls.push(args));
    assert.deepEqual(calls, [['0', '.\\Kaboom.cpp', 0xbc], ['0', '.\\Particle.cpp', 0x1b0]]);
    assert.ok(world.kernel.size === 0 && particles.rendering.size === 0);
    // 줄기(12)·불꽃(11)은 같은 공통 목록/Kernel 에 붙는다.
    particles.createTailByConstructor(0x00458760, parent);
    assert.ok(world.kernel.size === 1 && particles.rendering.size === 1);
    assert.equal(world.pool.slot(get(world.pool.slot(parent), 6, 2))[10], 12);
    const other = scene.parent(80, 30.5, 31.25);
    particles.createTailByConstructor(0x00458700, other);
    assert.ok(world.kernel.size === 2 && particles.rendering.size === 2);
    assert.equal(world.pool.slot(get(world.pool.slot(other), 6, 2))[10], 11);
    // 현재 TYPE 이 고른 다른 생성자는 복원되지 않았으므로 거부한다.
    assert.throws(() => particles.createTailByConstructor(0x00123456, parent), /미복원/);
    assert.throws(() => particles.createTailByConstructor(0x004ae761, parent), /미복원/);
});

test('장기 전역 바인딩: 같은 소유자 재호출은 허용하고 다른 소유자·getter 재연결은 거부하며 현재 값을 매번 다시 읽는다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    const aux = new GameAuxState();
    particles.bindTailProcessTypes(aux);
    particles.bindTailProcessTypes(aux);
    assert.throws(() => particles.bindTailProcessTypes(new GameAuxState()), /TYPE 소유자 재연결 오류/);
    assert.throws(() => particles.bindTailProcessTypes({}), TypeError);
    assert.throws(() => particles.bindTailProcessTypes(null), TypeError);
    // 붙이는 불꽃 프로세스의 TYPE 은 소유자의 현재 값을 읽는다(기본 11 → 바뀐 값 31).
    const first = scene.parent(80, 20.5, 21.25);
    particles.add(11, first);
    assert.equal(world.pool.slot(get(world.pool.slot(first), 6, 2))[10], 11);
    aux.fieryTailProcessType = 31;
    const second = scene.parent(80, 30.5, 31.25);
    particles.add(11, second);
    assert.equal(world.pool.slot(get(world.pool.slot(second), 6, 2))[10], 31);
    // 바람/약화 마나: getter 함수만 받고 같은 getter 는 다시 연결할 수 있으나 다른 getter 는 거부한다.
    const wind = () => 0.5;
    particles.bindWind(wind);
    particles.bindWind(wind);
    assert.throws(() => particles.bindWind(() => 0.5), /바람의 다른 owner/);
    assert.throws(() => particles.bindWind(0.5), TypeError);
    const weakened = () => false;
    particles.bindWeakenedMana(weakened);
    particles.bindWeakenedMana(weakened);
    assert.throws(() => particles.bindWeakenedMana(() => false), /약화 전역 중복/);
    assert.throws(() => particles.bindWeakenedMana(true), TypeError);
    // 연소점 HP 의 사제/다이스 TYPE 도 한 소유자만 받는다.
    particles.bindHitPointTypes(aux);
    particles.bindHitPointTypes(aux);
    assert.throws(() => particles.bindHitPointTypes(new GameAuxState()), /TYPE owner 재연결 오류/);
    assert.throws(() => particles.bindHitPointTypes({ priestType: 1 }), TypeError);
});

test('dirty 큐: 장치 연결 전 요청은 대기표에 두었다가 연결 때 순서대로 전달하고 카메라·영역 변화는 전체 갱신을 요청한다', () => {
    const scene = new Scene();
    const { particles } = scene;
    const parent = scene.parent(80, 20.5, 21.25);
    // 장치 연결 전에 만든 효과의 첫 프레임 요청은 전달되지 않고 대기한다.
    particles.add(11, parent);
    scene.frame(2);
    scene.frame(2.025);
    assert.equal(scene.dirty.length, 0);
    // 같은 영역으로 연결하면 영역이 바뀌지 않았으므로 전체 갱신 없이 대기 요청만 전달되고 큐는 비워진다.
    particles.setView(VIEW, scene.display);
    const delivered = scene.dirty.length;
    assert.ok(delivered >= 1);
    assert.ok(scene.dirty.every(([rect, flags]) => flags === 0 && rect.right > rect.left && rect.bottom > rect.top));
    // 같은 view 를 다시 주면 새 전달이 없다. 이후 프레임 요청은 곧바로 장치로 간다.
    particles.setView(VIEW, scene.display);
    assert.equal(scene.dirty.length, delivered);
    scene.frame(2.05);
    assert.ok(scene.dirty.length > delivered);
    // 카메라 원점이 바뀌면 영역 전체 한 건, 표시 영역이 바뀌어도 새 영역 전체 한 건이다.
    scene.dirty.length = 0;
    particles.setView({ ...VIEW, cameraX: 40 }, scene.display);
    assert.deepEqual(scene.dirty, [[{ left: 0, top: 0, right: 1024, bottom: 768 }, 0]]);
    scene.dirty.length = 0;
    particles.setView({ ...VIEW, cameraX: 40, right: 800 }, scene.display);
    assert.deepEqual(scene.dirty, [[{ left: 0, top: 0, right: 800, bottom: 768 }, 0]]);
    scene.dirty.length = 0;
    particles.setView({ ...VIEW, cameraX: 40, right: 800 }, scene.display);
    assert.equal(scene.dirty.length, 0);
    // CameraView 를 빌리면 카메라 원점은 그 snapshot 의 정수 픽셀이 소유하며 fallback 원점은 쓰지 않는다.
    const camera = new CameraView();
    camera.initialize();
    camera.setPixels(7, 9);
    particles.bindCameraView(camera);
    particles.setView({ ...VIEW, cameraX: 555, cameraY: 666, right: 800 }, scene.display);
    assert.equal(scene.dirty.length, 1);
    scene.dirty.length = 0;
    particles.setView({ ...VIEW, cameraX: 1, cameraY: 2, right: 800 }, scene.display);
    assert.equal(scene.dirty.length, 0);
    camera.setPixels(8, 9);
    particles.setView({ ...VIEW, right: 800 }, scene.display);
    assert.equal(scene.dirty.length, 1);
    // 장치가 없는 view 호출도 거부하지 않고 대기표를 유지한다.
    particles.setView({ ...VIEW, right: 800 }, null);
});

test('sprites: 공통 목록을 수집해 부모 위치/깊이 키와 삭제 영역 bounds 를 가진 직접 그리기 절차로 만들고 영역 밖은 제외한다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    particles.setView(VIEW, scene.display);
    const near = scene.parent(80, 20.5, 21.25);
    const far = scene.parent(80, 60.5, 60.25);
    particles.add(11, near);
    particles.add(11, far);
    scene.frame(2);
    const clip = { left: 0, top: 0, right: 1024, bottom: 768 };
    const sprites = particles.sprites(clip);
    assert.equal(sprites.length, 2);
    // 키는 부모 raw 좌표(+0.001)와 깊이 −25 이며 종류별 정렬은 하지 않고 등록 순서이다.
    assert.deepEqual(sprites.map((sprite) => sprite.order.depth), [-25, -25]);
    assert.deepEqual(sprites.map((sprite) => Math.round(sprite.order.x * 10) / 10), [20.5, 60.5]);
    assert.ok(sprites.every((sprite) => sprite.database === null && typeof sprite.procedure.draw === 'function'));
    // bounds 는 공통 삭제 영역(범위 ±1 픽셀)이고 clip 은 입력의 사본이다.
    const process = scene.processOf(near);
    const expected = renderProcessRemovalBounds(process.fields.registration, 0, 0);
    const bounds = sprites[0].procedure.bounds;
    assert.deepEqual([bounds.left, bounds.top, bounds.right, bounds.bottom], expected);
    clip.left = 999;
    assert.equal(sprites[0].clip.left, 0);
    // 화면 밖 영역에서는 하나도 모으지 않고, 한 부모의 효과만 걸치는 영역은 하나만 모은다.
    assert.equal(particles.sprites({ left: 0, top: 740, right: 100, bottom: 768 }).length, 0);
    assert.equal(particles.sprites({ left: 300, top: 200, right: 360, bottom: 260 }).length, 1);
    // collectDrawing 은 같은 수집이며 sprites 가 쓰는 공통 목록을 그대로 쓴다.
    const output = new Uint8Array(16 * 4);
    assert.equal(particles.collectDrawing({ left: 0, top: 0, right: 64, bottom: 64 }, output, 4), 2);
    // 효과 하나를 지우면 이미 만든 절차는 그리지 않고 다시 모으면 하나만 남는다.
    world.host.kill(process, 0x10);
    assert.equal(particles.sprites(SCREEN).length, 1);
    const pixels = new Uint8Array(1024 * 768);
    sprites[0].procedure.draw(pixels, 1024, 768, sprites[0].clip);
    assert.equal(countOf(pixels, 196), 0);
});

test('draw: 표면 용량 오류 같은 예외 뒤에도 출력 대상을 해제해 다음 그리기가 정상이고 제거된 토큰은 조용히 건너뛴다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    particles.setView(VIEW, scene.display);
    const parent = scene.parent(80, 20.5, 21.25);
    particles.add(11, parent);
    scene.frame(2);
    scene.frame(2.025);
    const [sprite] = particles.sprites(SCREEN);
    // 너무 작은 표면: VFX 선 출력이 용량 오류를 내고 그 뒤에도 정상 그리기가 가능하다(대상 해제 확인).
    assert.throws(() => sprite.procedure.draw(new Uint8Array(10), 1024, 768, sprite.clip), RangeError);
    const pixels = new Uint8Array(1024 * 768).fill(199);
    sprite.procedure.draw(pixels, 1024, 768, sprite.clip);
    assert.ok(countOf(pixels, 196) > 3);
    // drawEntry 는 같은 토큰 경로이며 제거된 토큰은 아무 일도 하지 않는다.
    const token = scene.processOf(parent).fields.registration.token;
    const again = new Uint8Array(1024 * 768).fill(199);
    particles.drawEntry(token, again, 1024, 768, SCREEN);
    assert.deepEqual(again, pixels);
    world.host.kill(scene.processOf(parent), 0x10);
    const blank = new Uint8Array(1024 * 768).fill(199);
    particles.drawEntry(token, blank, 1024, 768, SCREEN);
    assert.equal(countOf(blank, 196), 0);
});

test('줄기(12): 빔보다 먼저 만들어도 공유 빔 팔레트를 만들고 줄기 색은 빔 팔레트 번호(와 인접 DWORD)에서 나오며 이후 빔이 같은 색 객체를 재사용한다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    particles.setView(VIEW, scene.display);
    const parent = scene.parent(80, 20.5, 21.25);
    const streak = particles.addStreak(parent);
    assert.ok(streak);
    assert.equal(world.pool.slot(get(world.pool.slot(parent), 6, 2))[10], 12);
    scene.frame(2);
    scene.frame(2.025);
    const pixels = scene.render(particles.sprites(SCREEN));
    const allowed = new Set([0, 1, ...buildLightningPalette(Array.from(PALETTE.logical))]);
    let drawn = 0;
    for (const pixel of pixels) {
        if (pixel !== 0) drawn += 1;
        assert.ok(allowed.has(pixel), `줄기 픽셀 ${pixel}`);
    }
    assert.ok(drawn > 3);
    // 이어서 만든 빔도 같은 색 객체를 재사용해 정상 동작하고 줄기는 영향을 받지 않는다.
    for (const type of [148, 128]) Object.assign(world.types[type], { footX: 1, footY: 1 });
    const source = scene.parent(148, 40.5, 41.25);
    const victim = scene.parent(128, 50.125, 45.375);
    assert.equal(particles.addBeam(source, victim, 'a.wav').status, LightningCreateStatus.Created);
    scene.frame(2.05);
    assert.equal(world.kernel.size, 2);
    assert.ok(scene.sounds.length === 1 && scene.sounds[0][2] === 'a.wav');
});

test('건설 표시: 연결한 캡처/최대 HP 로 17 프로세스가 붙고 캡처 픽셀이 부모 화면 위치의 직사각형에만 복사된다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    const captures = [];
    const image = { width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) };
    particles.connectConstruction((sid) => { captures.push(sid); return image; }, (sid) => 100);
    particles.setView(VIEW, scene.display);
    const process = particles.addConstruction(world.parent, false);
    assert.ok(process);
    assert.deepEqual(captures, [world.parent]);
    assert.ok(particles.rendering.size === 1 && world.kernel.size === 1);
    scene.frame(2);
    scene.frame(2.1);
    const pixels = scene.render(particles.sprites(SCREEN), 199);
    // 부모 화면 위치 (328, 234) 에서 핫스팟을 뺀 5×7 구간만 바뀐다.
    let changed = 0;
    for (let y = 0; y < 768; y += 1) {
        for (let x = 0; x < 1024; x += 1) {
            if (pixels[y * 1024 + x] === 199) continue;
            changed += 1;
            assert.ok(x >= 326 && x < 331 && y >= 230 && y < 237, `바깥 픽셀 ${x},${y}`);
        }
    }
    assert.ok(changed > 0);
    assert.ok(scene.dirty.length > 0);
    // 그리기 clip 이 구간을 가리면 아무 것도 바뀌지 않는다.
    const clipped = new Uint8Array(1024 * 768).fill(199);
    for (const sprite of particles.sprites(SCREEN)) sprite.procedure.draw(clipped, 1024, 768, { left: 0, top: 0, right: 326, bottom: 768 });
    assert.equal(countOf(clipped, 199), 1024 * 768);
});

test('연소 표시: 실제 Dude 캡처와 정지 음향을 연결해 14 프로세스가 단계를 진행하고 삭제 때 dirty 와 소리 정지를 낸다', () => {
    const assets = realCaptureAssets();
    const scene = new Scene({
        prepare: (self) => {
            // 같은 188칸 TYPE 표를 제자리에서 실제 자료로 채운다(내장 Form 생성자 주소는 보존).
            assets.types.forEach((record, number) => Object.assign(self.world.types[number], record, { cost: record.cost ?? 0 }));
        },
    });
    const { particles, world } = scene;
    const stops = [];
    const capturer = new RawShapeToBuffer(world.pool, assets.types, assets.shapes, assets.database, createShapeToBufferState());
    particles.connectBurning((sid) => capturer.capture(sid), (handle) => stops.push(handle));
    particles.bindWind(() => 0.25);
    particles.bindWeakenedMana(() => false);
    particles.bindHitPointTypes(new GameAuxState());
    particles.setView(VIEW, scene.display);
    const victim = world.pool.allocate(0);
    const raw = world.pool.allocated(victim);
    put(raw, 0, 0x508cc8);
    raw[10] = 70;
    raw[11] = 0;
    raw[34] = 0;
    raw[35] = 7;
    put(raw, 14, floatToBits(20.5));
    put(raw, 18, floatToBits(21.25));
    put(raw, 26, 10);
    put(raw, 36, 0);
    const process = particles.addBurning(victim, 1);
    assert.ok(process);
    assert.ok(world.kernel.size === 1 && particles.rendering.size === 1);
    const fields = process.fields;
    assert.equal(fields.stage, 0);
    // 단계 0: 실제 ShapeToBuffer 로 피해 그림을 캡처한다.
    scene.frame(2, 1);
    const expected = capturer.capture(victim);
    assert.ok(fields.stage === 1 && fields.image.width === expected.width && expected.width > 0);
    assert.deepEqual(fields.image.pixels, expected.pixels);
    scene.frame(2.1, 2);
    assert.equal(fields.stage, 2);
    // 삭제: 현재 카메라 기준 삭제 영역 dirty 한 건과 소리 핸들 0 정지, 목록·Kernel 비움.
    scene.dirty.length = 0;
    const bounds = renderProcessRemovalBounds(fields.registration, 0, 0);
    world.host.kill(process, 0x10);
    assert.deepEqual(scene.dirty.map(([rect]) => [rect.left, rect.top, rect.right, rect.bottom]), [bounds]);
    assert.deepEqual(stops, [0]);
    assert.ok(particles.rendering.size === 0 && world.kernel.size === 0);
});

test('악마 광선: 현재 클라이언트 장치로 SHP 폭·음향·dirty 가 이어지고 TYPE/원천은 GameAuxState 에서 읽으며 타원은 0,0,639,479 pane 에 갇힌다', () => {
    const prepare = (self) => {
        Object.assign(self.world.types[129], { flags1: 0x10, maxHitPoints: 600, footX: 1, footY: 1 });
        self.shapes[129] = { frameCount: 4, loaded: true, frames: Array.from({ length: 4 }, () => ({ width: 9, height: 3 })) };
    };
    const run = (x) => {
        const scene = new Scene({ prepare });
        const { particles, world } = scene;
        const aux = new GameAuxState();
        const parent = world.pool.allocate(0);
        const raw = world.pool.allocated(parent);
        put(raw, 0, 0x501dd0);
        raw[10] = 129;
        raw[11] = 0;
        raw[34] = 1;
        raw[35] = 7;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(21.25));
        put(raw, 26, 600);
        aux.particleSource = parent;
        particles.bindTailProcessTypes(aux);
        const sounds = [];
        const diagnostics = [];
        particles.connectEvilRayDevices({
            soundId: (sid, end) => (end ? 31 : 11),
            sound: (px, py, id) => sounds.push([px, py, id]),
            frameError: () => { throw new Error('범위 안 프레임에서는 부르지 않는다'); },
            assertion: (...args) => diagnostics.push(args),
        });
        particles.setView(VIEW, scene.display);
        const ray = particles.addEvilRay(parent);
        assert.ok(ray);
        scene.frame(2);
        scene.frame(2.05);
        scene.frame(2.1);
        const pixels = scene.render(particles.sprites(SCREEN));
        return { scene, sounds, diagnostics, pixels };
    };
    const inside = run(20.5);
    assert.deepEqual(inside.diagnostics, []);
    assert.ok(inside.sounds.length >= 1 && inside.sounds[0][2] === 11 && inside.sounds[0][0] === 20.5 && inside.sounds[0][1] === 21.25);
    const allowed = new Set([0, 1, ...buildEvilRayPalette(Array.from(PALETTE.logical))]);
    assert.ok(inside.pixels.every((pixel) => allowed.has(pixel)));
    assert.ok(countOf(inside.pixels, 0) < inside.pixels.length);
    assert.ok(inside.scene.dirty.length > 0);
    // 원천 x=45 는 화면 x=720 이라 640 폭 pane 밖이므로 타원이 한 픽셀도 찍히지 않는다.
    const outside = run(45);
    assert.equal(countOf(outside.pixels, 0), outside.pixels.length);
});

test('악마 광선: 필요한 장치가 없으면 도달한 위치에서 거부하고 범위 밖 프레임은 상세 장치를 거친 뒤 물리 범위 오류가 된다', () => {
    const prepare = (self) => {
        Object.assign(self.world.types[129], { flags1: 0x10, maxHitPoints: 600, footX: 1, footY: 1 });
        self.shapes[129] = { frameCount: 4, loaded: true, frames: Array.from({ length: 4 }, () => ({ width: 9, height: 3 })) };
    };
    const scene = new Scene({ prepare });
    const { particles, world } = scene;
    const aux = new GameAuxState();
    const parent = world.pool.allocate(0);
    const raw = world.pool.allocated(parent);
    put(raw, 0, 0x501dd0);
    raw[10] = 129;
    raw[11] = 0;
    raw[34] = 1;
    raw[35] = 7;
    put(raw, 14, floatToBits(20.5));
    put(raw, 18, floatToBits(21.25));
    put(raw, 26, 600);
    aux.particleSource = parent;
    particles.bindTailProcessTypes(aux);
    particles.setView(VIEW, scene.display);
    // 장치 없이 만들고 첫 프레임이 SHP 폭을 읽으려면 진단·음향 장치가 필요하다.
    assert.ok(particles.addEvilRay(parent));
    assert.throws(() => scene.frame(2), /장치 미연결/);
    // 범위 밖 저장 프레임(100): 상세 장치 → 원문 진단 → 물리 범위 오류 순서이다.
    const order = [];
    particles.connectEvilRayDevices({
        soundId: () => 11,
        sound: () => {},
        frameError: (number, frame, abstract) => order.push(['error', number, frame, abstract]),
        assertion: (expression) => order.push(['assert', expression]),
    });
    put(world.pool.allocated(parent), 36, 100);
    world.pool.allocated(parent)[40] = 1;
    assert.throws(() => scene.frame(2.05), RangeError);
    assert.deepEqual(order.map((entry) => entry[0]), ['error', 'assert']);
    assert.deepEqual(order[0].slice(1), [129, 100, 1]);
    assert.match(order[1][1], /frameCheck/);
    // 장치를 비운 뒤에도 정리는 가능하다.
    particles.dispose();
    assert.ok(particles.rendering.size === 0 && world.kernel.size === 0);
});

// 파편/분해는 실제 Form 분배와 의존 통지(더미 상호 감시)가 필요하므로 별도 장면으로 입자 본체를 조립한다(shrapnel-visual-system.test.mjs 의 조립과 같다). (2026-11-13 WB04-E390)
class FormScene {
    constructor() {
        this.pool = new SidPool(32768);
        this.types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
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
        this.dispatch.bind(this.host);
        this.dispatch.bind(this.depends);
        this.shapes = this.types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
        this.core = { pool: this.pool, types: this.types, shapes: this.shapes, processes: this.host, neighbors: { center: () => [0, 0] }, kernel: this.kernel };
        this.dummy = 0;
        this.failCapture = false;
        this.dirty = [];
        this.source = this.create(133, 2.125, 3.25);
        this.first = this.create(145, 0, 0);
        this.second = this.create(145, 10, 14);
        this.particles = new RawCampaignParticles(this.core, new GameRandom(0x12345678), PALETTE, { shrapnel: this.shrapnelDevices() });
        this.particles.setView(VIEW, { suppressed: () => false, invalidate: (rect, flags) => this.dirty.push([rect, flags]) });
    }

    // 실제 생성자가 쓴 가상 표/타입을 보존하고 명시 좌표만 공급한다. (2026-11-13 WB04-E390)
    create(type, x, y) {
        const sid = this.factory.create(type, 2);
        const raw = this.pool.allocated(sid);
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        return sid;
    }

    // 더미 생성/Pop/의존/캡처와 부착 전 실패 삭제를 실제 분배기에 연결한 파편 장치 묶음이다. 캡처는 5×6 색 71 명시 입력이다. (2026-11-13 WB04-E390)
    shrapnelDevices() {
        return {
            lifetime: {
                create: (type, flags) => {
                    this.dummy = this.factory.create(type, flags);
                    return this.dummy;
                },
                pop: (sid, x, y) => {
                    const raw = this.pool.allocated(sid);
                    put(raw, 14, floatToBits(x));
                    put(raw, 18, floatToBits(y));
                },
                depend: (form, parent) => { this.depends.create(form, parent); },
                capture: () => {
                    if (this.failCapture) throw new Error('명시 캡처 실패');
                    return { width: 5, height: 6, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(30).fill(71) };
                },
            },
            destroyUnpaired: (sid) => { this.dispatch.destroy(sid, 16); },
            crt: new CrtRandom(1),
        };
    }
}

test('파편(16): 세 종류 모두 더미에 붙어 픽셀이 현재 clip 안에만 찍히고 표면보다 큰 clip 은 경계 불일치로 거부하며 캡처 실패는 잔재를 남기지 않는다', () => {
    for (const kind of [ShrapnelKind.Explosion, ShrapnelKind.Collapse, ShrapnelKind.Variant2]) {
        const scene = new FormScene();
        const { particles } = scene;
        // 생성 시각과 첫 프레임 시각을 맞춰 입자 수명이 소모되지 않게 한다.
        particles.prepareFrame(1);
        const process = particles.addShrapnel(scene.source, kind);
        assert.ok(process);
        assert.ok(scene.pool.slot(process.form)[10] === 16 && process.parent === scene.dummy);
        assert.ok(scene.kernel.size === 1 && particles.rendering.size === 1);
        particles.prepareFrame(1);
        scene.kernel.runFrame();
        assert.ok(scene.dirty.length >= 1);
        const sprites = particles.sprites(SCREEN);
        assert.equal(sprites.length, 1);
        const pixels = new Uint8Array(1024 * 768);
        sprites[0].procedure.draw(pixels, 1024, 768, SCREEN);
        const drawn = countOf(pixels, 71);
        assert.ok(drawn > 0 && pixels.every((pixel) => pixel === 0 || pixel === 71), `종류 ${kind}`);
        // 현재 clip 밖은 찍지 않는다: 왼쪽 위 10×10 구간만 허용하면 그 밖에는 하나도 없다.
        const narrow = new Uint8Array(1024 * 768);
        sprites[0].procedure.draw(narrow, 1024, 768, { left: 0, top: 0, right: 10, bottom: 10 });
        for (let y = 0; y < 768; y += 1) for (let x = 0; x < 1024; x += 1) if (x >= 10 || y >= 10) assert.equal(narrow[y * 1024 + x], 0);
        // clip 은 안인데 표면 폭 밖인 픽셀은 경계 불일치 오류이다(작은 8×8 표면).
        assert.throws(() => sprites[0].procedure.draw(new Uint8Array(64), 8, 8, SCREEN), RangeError);
        // 오류 뒤에도 출력 대상이 해제되어 다음 그리기가 정상이다.
        const again = new Uint8Array(1024 * 768);
        sprites[0].procedure.draw(again, 1024, 768, SCREEN);
        assert.equal(countOf(again, 71), drawn);
    }
    // 캡처가 실패하면 효과/더미/표시 목록 잔재가 없다.
    const failing = new FormScene();
    failing.failCapture = true;
    assert.throws(() => failing.particles.addShrapnel(failing.source, ShrapnelKind.Explosion), /명시 캡처 실패/);
    assert.ok(failing.kernel.size === 0 && failing.particles.rendering.size === 0);
});

test('분해(18): 원본 캡처 픽셀이 대상 삭제 뒤에도 유지되고 정리하면 파편과 분해가 함께 사라진다', () => {
    const scene = new FormScene();
    const { particles } = scene;
    particles.prepareFrame(1);
    const dissolve = particles.addDissolve(scene.source, scene.first, scene.second, 0);
    assert.ok(dissolve);
    assert.ok(scene.pool.slot(dissolve.form)[10] === 18 && dissolve.parent === scene.dummy);
    const shrapnel = particles.addShrapnel(scene.source, ShrapnelKind.Explosion);
    assert.ok(shrapnel);
    assert.ok(scene.kernel.size === 2 && particles.rendering.size === 2);
    // 대상(source)이 삭제되어도 더미에 붙은 두 효과는 유지된다.
    assert.equal(scene.dispatch.destroy(scene.source, 16), true);
    assert.ok(scene.kernel.size === 2 && particles.rendering.size === 2);
    particles.prepareFrame(1);
    scene.kernel.runFrame();
    const pixels = new Uint8Array(1024 * 768);
    for (const sprite of particles.sprites(SCREEN)) sprite.procedure.draw(pixels, 1024, 768, sprite.clip);
    assert.ok(countOf(pixels, 71) > 0 && pixels.every((pixel) => pixel === 0 || pixel === 71));
    // dispose 는 남은 효과를 Host 삭제로 모두 정리하고 한 번 더 불러도 안전하다.
    particles.dispose();
    assert.ok(scene.kernel.size === 0 && particles.rendering.size === 0);
    particles.dispose();
});

test('dispose: 여러 종류 효과가 섞여 있어도 선언 역순으로 모두 정리하고 표시 목록과 Kernel 이 비며 예외가 없다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    for (const type of [148, 128]) Object.assign(world.types[type], { footX: 1, footY: 1 });
    particles.connectConstruction(() => ({ width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) }), () => 100);
    particles.setView(VIEW, scene.display);
    particles.add(11, scene.parent(80, 10.5, 11.25));
    particles.addStreak(scene.parent(80, 12.5, 13.25));
    particles.addBeam(scene.parent(148, 20.5, 21.25), scene.parent(128, 40.125, 35.375), 'z.wav');
    particles.addConstruction(world.parent, false);
    particles.addCooling(scene.parent(80, 14.5, 15.25), [3, 4]);
    particles.addSmoke({ ...createSmokeRequest(), parent: scene.parent(80, 16.5, 17.25) });
    assert.equal(particles.rendering.size, 6);
    assert.equal(world.kernel.size, 6);
    particles.dispose();
    assert.ok(particles.rendering.size === 0 && world.kernel.size === 0);
    particles.dispose();
});

// ---- 변형 확인에서 드러난 공백을 독립 입력으로 보강한 시험들 (2026-11-13 WB04-E390) ----

test('particlePaletteInput: 팔레트 256색의 RGBX DWORD 를 번호 순서 그대로 정확히 256개 만든다', () => {
    const logical = particlePaletteInput(PALETTE);
    assert.equal(logical.length, 256);
    for (let index = 0; index < 256; index += 1) assert.equal(logical[index], PALETTE.logical[index], `색 ${index}`);
    // 예약 바이트(상위 8비트)는 색 계산 입력에 들어가지 않는다.
    assert.ok(logical.every((value) => value === (value & 0xffffff)));
    // 입력 팔레트를 바꾼 새 팔레트의 첫/끝 번호도 그대로 옮긴다.
    const rgb = new Uint8Array(768);
    rgb.set([1, 2, 3], 0);
    rgb.set([250, 251, 252], 255 * 3);
    const custom = particlePaletteInput(new GamePalette(rgb));
    assert.equal(custom[0], 1 | (2 << 8) | (3 << 16));
    assert.equal(custom[255], 250 | (251 << 8) | (252 << 16));
});

test('불꽃: 부모 좌표 float 의 모든 바이트를 읽어 끝점이 정확히 투영되고, 빌린 CameraView 의 정수 원점만큼 화면이 평행 이동한다', () => {
    const x = Math.fround(20.123456);
    const y = Math.fround(21.654321);
    const render = (camera) => {
        const scene = new Scene();
        const parent = scene.parent(80, x, y);
        if (camera) scene.particles.bindCameraView(camera);
        scene.particles.setView(VIEW, scene.display);
        scene.particles.add(11, parent);
        scene.frame(2);
        scene.frame(2.025);
        return scene.render(scene.particles.sprites(SCREEN), 199);
    };
    const base = render(null);
    // 끝점은 raw float 를 ×16/×11 +0.5 절삭한 화면 좌표이다: 모든 바이트가 0 이 아닌 좌표로 확인한다.
    const [endX, endY] = projectCameraCoordinates(0, 0, x, y);
    assert.equal(base[endY * 1024 + endX], 196);
    assert.ok(countOf(base, 196) > 3 && base.every((pixel) => pixel === 199 || pixel === 196));
    // 카메라 원점 (40, 25): 모든 화면 픽셀이 정확히 (−40, −25) 이동한다(월드 float 원점 56/60 은 정수 픽셀 float 이다).
    const camera = new CameraView();
    camera.initialize();
    camera.setPixels(40, 25);
    const shifted = render(camera);
    for (let row = 0; row < 768 - 25; row += 1) {
        for (let column = 0; column < 1024 - 40; column += 1) assert.equal(shifted[row * 1024 + column], base[(row + 25) * 1024 + column + 40], `${column},${row}`);
    }
    assert.equal(shifted[(endY - 25) * 1024 + (endX - 40)], 196);
});

test('설정 없이 쓰는 기본 영역: setView 전에도 1024×768 의 원점 0 으로 동작하고 첫 setView 는 전체 갱신을 요청하지 않는다', () => {
    const scene = new Scene();
    const { particles } = scene;
    const parent = scene.parent(80, 20.5, 21.25);
    particles.add(11, parent);
    scene.frame(2);
    scene.frame(2.025);
    // 기본 영역/원점 0: 부모 화면 위치 (328, 234) 에 같은 선이 찍힌다.
    const pixels = scene.render(particles.sprites(SCREEN), 199);
    assert.equal(pixels[234 * 1024 + 328], 196);
    // 기본값과 같은 view 로 처음 연결하면 변경이 없으므로 대기 요청만 전달된다(전체 갱신 한 건이 추가되지 않는다).
    const quiet = new Scene();
    quiet.particles.setView(VIEW, quiet.display);
    assert.deepEqual(quiet.dirty, []);
    // 영역의 각 변이 달라지면 변경으로 보고 새 영역 전체를 한 건 요청한다.
    for (const change of [{ left: 3 }, { top: 5 }, { right: 1000 }, { bottom: 700 }, { cameraX: 9 }, { cameraY: 11 }]) {
        const other = new Scene();
        other.particles.setView({ ...VIEW, ...change }, other.display);
        const next = { ...VIEW, ...change };
        assert.deepEqual(other.dirty, [[{ left: next.left, top: next.top, right: next.right, bottom: next.bottom }, 0]], JSON.stringify(change));
    }
    // 프레임이 장치로 곧바로 보내는 요청의 플래그는 0 이다.
    const live = new Scene();
    live.particles.setView(VIEW, live.display);
    live.particles.add(11, live.parent(80, 20.5, 21.25));
    live.frame(2);
    live.frame(2.025);
    assert.ok(live.dirty.length > 0 && live.dirty.every(([, flags]) => flags === 0));
});

test('sprites: 수집 영역은 (clip + 카메라 원점) ÷ 16/11 의 월드 사각형이고 항목의 y 키와 빌린 카메라의 삭제 영역을 따른다', () => {
    const scene = new Scene();
    const { particles } = scene;
    const camera = new CameraView();
    camera.initialize();
    camera.setPixels(48, 22);
    particles.bindCameraView(camera);
    particles.setView(VIEW, scene.display);
    const parent = scene.parent(80, 20.5, 21.25);
    particles.add(11, parent);
    scene.frame(2);
    scene.frame(2.025);
    const [sprite] = particles.sprites(SCREEN);
    // 키 x/y 는 부모 raw 좌표(+0.001 편향), 삭제 영역은 빌린 카메라 float 원점을 뺀 값이다.
    assert.equal(sprite.order.x, 20.5);
    assert.equal(Math.round(sprite.order.y * 1000) / 1000, 21.251);
    const registration = scene.processOf(parent).fields.registration;
    const bounds = sprite.procedure.bounds;
    assert.deepEqual([bounds.left, bounds.top, bounds.right, bounds.bottom], renderProcessRemovalBounds(registration, 48, 22));
    // 수집 영역 경계: 화면 사각형 [left, right) 의 월드 환산이 효과의 등록 범위와 겹칠 때만 모은다. 카메라 원점 (48, 22) 이므로 화면 x = 280 이 월드 20.5 이다.
    const include = (clip) => particles.sprites(clip).length;
    const edgeRight = Math.floor(20.5 * 16 - 48);
    assert.equal(include({ left: 0, top: 0, right: edgeRight + 24, bottom: 768 }), 1);
    assert.equal(include({ left: 0, top: 0, right: edgeRight - 24, bottom: 768 }), 0);
    const edgeBottom = Math.floor(21.25 * 11 - 22);
    assert.equal(include({ left: 0, top: 0, right: 1024, bottom: edgeBottom + 24 }), 1);
    assert.equal(include({ left: 0, top: 0, right: 1024, bottom: edgeBottom - 24 }), 0);
    assert.equal(include({ left: edgeRight + 40, top: 0, right: 1024, bottom: 768 }), 0);
    assert.equal(include({ left: 0, top: edgeBottom + 40, right: 1024, bottom: 768 }), 0);
});

test('냉각 연기(21)·일반 연기: 12색 표(인접 DWORD 포함)에서 나온 색으로 선을 찍고 부모 TYPE 21 로 붙는다', () => {
    const scene = new Scene();
    const { particles, world } = scene;
    particles.setView(VIEW, scene.display);
    const parent = scene.parent(80, 20.5, 21.25);
    const cooling = particles.addCooling(parent, [16, -11]);
    assert.ok(cooling);
    assert.equal(world.pool.slot(get(world.pool.slot(parent), 6, 2))[10], 21);
    const other = scene.parent(80, 40.5, 31.25);
    const smoke = particles.addSmoke({ ...createSmokeRequest(), parent: other });
    assert.ok(smoke && particles.rendering.size === 2);
    for (let step = 0; step < 6; step += 1) scene.frame(2 + step * 0.05);
    const allowed = new Set([0, 1, ...buildSmokePalette()]);
    const pixels = scene.render(particles.sprites(SCREEN));
    assert.ok(pixels.every((pixel) => allowed.has(pixel)));
    assert.ok(countOf(pixels, 0) < pixels.length);
    assert.ok(scene.dirty.length > 0);
});

test('악마 광선: 주소 0x004536a0 생성자는 악마 광선을 붙이고, TYPE 전역을 묶지 않으면 독립 기본 TYPE 10 을 쓴다', () => {
    const prepare = (self) => {
        Object.assign(self.world.types[129], { flags1: 0x10, maxHitPoints: 600, footX: 1, footY: 1 });
        self.shapes[129] = { frameCount: 4, loaded: true, frames: Array.from({ length: 4 }, () => ({ width: 9, height: 3 })) };
    };
    const scene = new Scene({ prepare });
    const { particles, world } = scene;
    particles.connectEvilRayDevices({ soundId: () => 11, sound: () => {}, frameError: () => {}, assertion: () => {} });
    const parent = world.pool.allocate(0);
    const raw = world.pool.allocated(parent);
    put(raw, 0, 0x501dd0);
    raw[10] = 129;
    raw[11] = 0;
    raw[34] = 1;
    raw[35] = 7;
    put(raw, 14, floatToBits(20.5));
    put(raw, 18, floatToBits(21.25));
    put(raw, 26, 600);
    particles.createTailByConstructor(0x004536a0, parent);
    assert.ok(world.kernel.size === 1 && particles.rendering.size === 1);
    // 전역을 묶지 않았으므로 기본 TYPE 10 으로 붙는다.
    assert.equal(world.pool.slot(get(world.pool.slot(parent), 6, 2))[10], 10);
});

test('악마 광선: SHP 폭 조회의 TYPE/프레임 범위 경계와 저장 프레임의 모든 바이트가 장치와 오류에 그대로 전달된다', () => {
    const make = (mutate) => {
        const scene = new Scene({
            prepare: (self) => {
                Object.assign(self.world.types[129], { flags1: 0x10, maxHitPoints: 600, footX: 1, footY: 1 });
                self.shapes[129] = { frameCount: 4, loaded: true, frames: Array.from({ length: 4 }, () => ({ width: 9, height: 3 })) };
                mutate?.(self);
            },
        });
        const { particles, world } = scene;
        const parent = world.pool.allocate(0);
        const raw = world.pool.allocated(parent);
        put(raw, 0, 0x501dd0);
        raw[10] = 129;
        raw[11] = 0;
        raw[34] = 1;
        raw[35] = 7;
        put(raw, 14, floatToBits(20.5));
        put(raw, 18, floatToBits(21.25));
        put(raw, 26, 600);
        const aux = new GameAuxState();
        aux.particleSource = parent;
        particles.bindTailProcessTypes(aux);
        const log = [];
        particles.connectEvilRayDevices({
            soundId: () => 11,
            sound: () => {},
            frameError: (...args) => log.push(['error', ...args]),
            assertion: () => log.push(['assert']),
        });
        particles.setView(VIEW, scene.display);
        assert.ok(particles.addEvilRay(parent));
        return { scene, parent, raw, log };
    };
    // 범위 안 프레임은 상세 장치를 거치지 않고 폭을 읽는다.
    const ok = make();
    ok.scene.frame(2);
    assert.deepEqual(ok.log, []);
    // 저장 프레임의 네 바이트가 모두 상세 장치로 전달된다(0x12345678 은 물리 범위 밖이라 오류).
    const wide = make();
    put(wide.raw, 36, 0x12345678);
    assert.throws(() => wide.scene.frame(2), RangeError);
    assert.deepEqual(wide.log[0], ['error', 129, 0x12345678, 0]);
    // 프레임 4 는 물리 프레임 수와 같아 범위 밖이므로 RangeError 이다(TypeError 가 아니다).
    const edge = make();
    put(edge.raw, 36, 4);
    assert.throws(() => edge.scene.frame(2), (error) => error instanceof RangeError && /물리 범위/.test(error.message));
    // 음수(0xffffffff = −1)도 범위 밖이다.
    const negative = make();
    put(negative.raw, 36, 0xffffffff);
    assert.throws(() => negative.scene.frame(2), (error) => error instanceof RangeError && /물리 범위/.test(error.message));
    assert.deepEqual(negative.log[0], ['error', 129, -1, 0]);
    // SHP 가 적재되지 않은 TYPE 은 진단 뒤 같은 물리 범위 오류이다.
    const unloaded = make((self) => { self.shapes[129].loaded = false; });
    assert.throws(() => unloaded.scene.frame(2), (error) => error instanceof RangeError && /물리 범위/.test(error.message));
    // 원천 TYPE 이 TYPE 표 길이(188) 이상이면 표를 읽기 전에 거부한다.
    const beyond = make();
    beyond.raw[10] = 188;
    assert.throws(() => beyond.scene.frame(2), (error) => error instanceof RangeError && /TYPE 물리 범위/.test(error.message));
    // SHP 표가 TYPE 번호보다 짧아도 같은 오류이다(표 길이 129 와 TYPE 129).
    const short = make((self) => { self.shapes.length = 129; });
    assert.throws(() => short.scene.frame(2), (error) => error instanceof RangeError && /TYPE 물리 범위/.test(error.message));
});

test('악마 광선: 타원은 0,0,639,479 pane 안에서만 찍히고 오른쪽·아래 경계 한 칸 밖은 비어 있으며 안쪽은 찍힌다', () => {
    const prepare = (self) => {
        Object.assign(self.world.types[129], { flags1: 0x10, maxHitPoints: 600, footX: 1, footY: 1 });
        self.shapes[129] = { frameCount: 4, loaded: true, frames: Array.from({ length: 4 }, () => ({ width: 9, height: 3 })) };
    };
    const run = (x, y) => {
        const scene = new Scene({ prepare });
        const { particles, world } = scene;
        const parent = world.pool.allocate(0);
        const raw = world.pool.allocated(parent);
        put(raw, 0, 0x501dd0);
        raw[10] = 129;
        raw[11] = 0;
        raw[34] = 1;
        raw[35] = 7;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 26, 600);
        const aux = new GameAuxState();
        aux.particleSource = parent;
        particles.bindTailProcessTypes(aux);
        particles.connectEvilRayDevices({ soundId: () => 11, sound: () => {}, frameError: () => {}, assertion: () => {} });
        particles.setView(VIEW, scene.display);
        particles.addEvilRay(parent);
        scene.frame(2);
        scene.frame(2.05);
        scene.frame(2.1);
        return scene.render(particles.sprites(SCREEN));
    };
    const bounds = (pixels) => {
        let maxX = -1;
        let maxY = -1;
        let minX = 1 << 30;
        let minY = 1 << 30;
        for (let index = 0; index < pixels.length; index += 1) {
            if (pixels[index] === 0) continue;
            const px = index % 1024;
            const py = Math.floor(index / 1024);
            maxX = Math.max(maxX, px);
            maxY = Math.max(maxY, py);
            minX = Math.min(minX, px);
            minY = Math.min(minY, py);
        }
        return { minX, minY, maxX, maxY };
    };
    // 원천을 화면 x≈640 근처에서 1/16 월드 칸씩 옮겨 가며 확인한다: 열 640 이상은 한 번도 찍히지 않고 열 639(pane 의 마지막 열)는 찍힌다.
    let farthestX = -1;
    for (let step = -24; step <= 24; step += 1) {
        const found = bounds(run(40 + step / 16, 21.25));
        assert.ok(found.maxX < 640, `x 단계 ${step}: ${JSON.stringify(found)}`);
        farthestX = Math.max(farthestX, found.maxX);
    }
    assert.equal(farthestX, 639);
    // 원천을 화면 y≈480 근처에서 옮겨 가며: 행 480 이상은 찍히지 않고 행 479 는 찍힌다.
    let farthestY = -1;
    for (let step = -24; step <= 24; step += 1) {
        const found = bounds(run(20.5, 480 / 11 + step / 11));
        assert.ok(found.maxY < 480, `y 단계 ${step}: ${JSON.stringify(found)}`);
        farthestY = Math.max(farthestY, found.maxY);
    }
    assert.equal(farthestY, 479);
    // 왼쪽/위쪽은 pane 이 0 이라 화면 안쪽만 찍힌다(원천이 화면 구석이어도 음수 좌표는 없다).
    const corner = bounds(run(0.2, 0.2));
    assert.ok(corner.minX >= 0 && corner.minY >= 0 && corner.maxX > 0);
});

test('건설 표시: 그리기 clip·표면 가장자리에서 복사 구간만 잘리고 부모가 화면 구석이어도 안전하다', () => {
    const run = (x, y, cut) => {
        const scene = new Scene();
        const { particles, world } = scene;
        const image = { width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) };
        particles.connectConstruction(() => image, () => 100);
        particles.setView(VIEW, scene.display);
        put(world.pool.allocated(world.parent), 14, floatToBits(x));
        put(world.pool.allocated(world.parent), 18, floatToBits(y));
        particles.addConstruction(world.parent, false);
        scene.frame(2);
        scene.frame(2.1);
        const [sprite] = particles.sprites(SCREEN);
        const pixels = new Uint8Array(1024 * 768).fill(199);
        if (sprite) sprite.procedure.draw(pixels, 1024, 768, cut);
        return pixels;
    };
    const full = run(20.5, 21.25, SCREEN);
    let minX = 1 << 30;
    let maxX = -1;
    let minY = 1 << 30;
    let maxY = -1;
    for (let index = 0; index < full.length; index += 1) {
        if (full[index] === 199) continue;
        const px = index % 1024;
        const py = Math.floor(index / 1024);
        minX = Math.min(minX, px);
        maxX = Math.max(maxX, px);
        minY = Math.min(minY, py);
        maxY = Math.max(maxY, py);
    }
    assert.ok(maxX > minX && maxY > minY);
    const midX = Math.floor((minX + maxX) / 2) + 1;
    const midY = Math.floor((minY + maxY) / 2) + 1;
    // 네 방향 clip 창은 전체 결과를 창으로 자른 것과 같다.
    const windows = [
        { left: midX, top: 0, right: 1024, bottom: 768 },
        { left: 0, top: 0, right: midX, bottom: 768 },
        { left: 0, top: midY, right: 1024, bottom: 768 },
        { left: 0, top: 0, right: 1024, bottom: midY },
    ];
    for (const window of windows) {
        const cut = run(20.5, 21.25, window);
        for (let row = 0; row < 768; row += 1) {
            for (let column = 0; column < 1024; column += 1) {
                const inside = column >= window.left && column < window.right && row >= window.top && row < window.bottom;
                assert.equal(cut[row * 1024 + column], inside ? full[row * 1024 + column] : 199, `${JSON.stringify(window)} ${column},${row}`);
            }
        }
    }
    // 표면이 clip 보다 작을 때(pitch·height 가 화면 안): 표면 밖 좌표는 다음 행으로 감기거나 표면 밖 메모리에 쓰이지 않고 건너뛴다.
    const changed = [];
    for (let index = 0; index < full.length; index += 1) if (full[index] !== 199) changed.push([index % 1024, Math.floor(index / 1024), full[index]]);
    // 표면 메모리는 pitch*height 보다 크게(1024×768) 잡아 표면 밖 쓰기를 드러낸다. 폭만 줄이는 경우와 높이만 줄이는 경우를 따로 확인한다.
    const renderSmall = (pitch, height) => {
        const scene = new Scene();
        const { particles, world } = scene;
        const image = { width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) };
        particles.connectConstruction(() => image, () => 100);
        particles.setView(VIEW, scene.display);
        particles.addConstruction(world.parent, false);
        scene.frame(2);
        scene.frame(2.1);
        const pixels = new Uint8Array(1024 * 768).fill(199);
        for (const sprite of particles.sprites(SCREEN)) sprite.procedure.draw(pixels, pitch, height, SCREEN);
        return pixels;
    };
    const minimumY = Math.min(...changed.map(([, py]) => py));
    for (const [pitch, height] of [[329, 768], [1024, minimumY + 1]]) {
        const expected = new Uint8Array(1024 * 768).fill(199);
        let kept = 0;
        for (const [px, py, color] of changed) {
            if (px >= pitch || py >= height) continue;
            expected[py * pitch + px] = color;
            kept += 1;
        }
        assert.ok(kept > 0 && kept < changed.length, `표면 ${pitch}×${height}`);
        assert.deepEqual(renderSmall(pitch, height), expected, `표면 ${pitch}×${height}`);
    }
    // 화면 왼쪽 위 구석의 부모: 이미지 사각형이 음수 좌표로 걸쳐도 안쪽 구간(열 0..3, 행 0..4)에만 찍히고 앞 행의 끝으로 감기지 않는다.
    const topLeft = run(0.05, 0.05, SCREEN);
    let drawn = 0;
    for (let index = 0; index < topLeft.length; index += 1) {
        if (topLeft[index] === 199) continue;
        drawn += 1;
        assert.ok(index % 1024 < 4 && Math.floor(index / 1024) < 5, `감김 ${index % 1024},${Math.floor(index / 1024)}`);
    }
    assert.ok(drawn > 0);
});

test('인자 검증 메시지: 연결·바인딩의 잘못된 경계는 어떤 경계가 틀렸는지 구별해 거부한다', () => {
    const scene = new Scene();
    const { particles } = scene;
    const image = () => ({ width: 1, height: 1, hotspotX: 0, hotspotY: 0, pixels: new Uint8Array(1) });
    assert.throws(() => particles.connectConstruction(null, () => 1), /건설 표시 연결 오류/);
    assert.throws(() => particles.connectConstruction(image, null), /건설 표시 연결 오류/);
    assert.throws(() => particles.connectBurning(null, () => {}), /Burning 연결 중복\/필수 경계 누락/);
    assert.throws(() => particles.connectBurning(image, null), /Burning 연결 중복\/필수 경계 누락/);
    // 첫 연결이 끝난 뒤의 중복도 같은 오류이다.
    particles.connectConstruction(image, () => 1);
    particles.connectBurning(image, () => {});
    assert.throws(() => particles.connectConstruction(image, () => 1), /건설 표시 연결 오류/);
    assert.throws(() => particles.connectBurning(image, () => {}), /Burning 연결 중복\/필수 경계 누락/);
    // 카메라: data 가 있어도 pixels 가 없으면, pixels 가 있어도 data 가 없으면 거부한다.
    assert.throws(() => particles.bindCameraView({ data: new DataView(new ArrayBuffer(64)) }), /CameraView/);
    assert.throws(() => particles.bindCameraView({ pixels: { x: 0, y: 0 } }), /CameraView/);
});

// 연소(14)와 파편(16) 출력 장치의 가장자리 규칙을 한 방향씩 독립으로 확인하는 보조 장치이다: 전체 그리기 결과(전체 clip, 충분한 표면)를 기준으로 삼는다. (2026-11-13 WB04-E390)
// 한 축만 자르는 네 창은 기준 결과를 창으로 자른 것과 같아야 하고, 표면 폭/높이는 가장 큰 좌표와 같으면 거부, 하나 크면 허용이다.
function edgeRules(label, makeDraw, expectation, strictWindows = true) {
    const draw = makeDraw();
    const full = new Uint8Array(1024 * 768);
    draw(full, 1024, 768, SCREEN);
    const changed = [];
    for (let index = 0; index < full.length; index += 1) if (full[index] !== 0) changed.push([index % 1024, Math.floor(index / 1024), full[index]]);
    assert.ok(changed.length >= 8, `${label}: 기준 픽셀 ${changed.length}`);
    const xs = changed.map(([px]) => px).sort((a, b) => a - b);
    const ys = changed.map(([, py]) => py).sort((a, b) => a - b);
    const midX = xs[Math.floor(xs.length / 2)];
    const midY = ys[Math.floor(ys.length / 2)];
    assert.ok(xs[0] < midX && xs[xs.length - 1] > midX && ys[0] < midY && ys[ys.length - 1] > midY, `${label}: 두 축 모두 걸쳐야 한다`);
    // 한 축만 자르는 네 창.
    const windows = [
        { left: midX, top: 0, right: 1024, bottom: 768 },
        { left: 0, top: 0, right: midX, bottom: 768 },
        { left: 0, top: midY, right: 1024, bottom: 768 },
        { left: 0, top: 0, right: 1024, bottom: midY },
    ];
    for (const window of windows) {
        const cut = new Uint8Array(1024 * 768);
        makeDraw()(cut, 1024, 768, window);
        for (let index = 0; index < cut.length; index += 1) {
            const column = index % 1024;
            const row = Math.floor(index / 1024);
            const inside = column >= window.left && column < window.right && row >= window.top && row < window.bottom;
            // 입자 하나의 여러 점이 모두 창 안일 때만 찍는 출력(연소)은 창 경계의 입자 전체가 빠지므로, 창 밖 누출 없음과 기준 결과의 부분집합만 확인한다.
            if (strictWindows ? cut[index] !== (inside ? full[index] : 0) : (!inside && cut[index] !== 0)) assert.fail(`${label} 창 ${JSON.stringify(window)} ${column},${row}`);
            if (!strictWindows && cut[index] !== 0 && full[index] === 0) assert.fail(`${label} 창 부분집합 위반 ${column},${row}`);
        }
    }
    // 표면이 clip 보다 작을 때: 가장 큰 열/행과 같은 폭/높이는 거부하고 하나 큰 폭/높이는 허용한다(표면 메모리는 크게 잡아 감김을 드러냄).
    const maxX = xs[xs.length - 1];
    const maxY = ys[ys.length - 1];
    const expectedRule = expectation === 'throw';
    if (expectedRule) {
        assert.throws(() => makeDraw()(new Uint8Array(1024 * 768), maxX, 768, SCREEN), RangeError, `${label}: 폭 ${maxX}`);
        assert.throws(() => makeDraw()(new Uint8Array(1024 * 768), 1024, maxY, SCREEN), RangeError, `${label}: 높이 ${maxY}`);
    }
    for (const [pitch, height] of [[maxX + 1, 768], [1024, maxY + 1]]) {
        const surface = new Uint8Array(1024 * 768);
        makeDraw()(surface, pitch, height, SCREEN);
        const expected = new Uint8Array(1024 * 768);
        for (const [px, py, color] of changed) expected[py * pitch + px] = color;
        assert.deepEqual(surface, expected, `${label}: ${pitch}×${height}`);
    }
}

// 실제 Dude 를 연소시키는 장면을 만들고 지정 시각까지 진행한 뒤 그리기 함수를 돌려준다. (2026-11-13 WB04-E390)
function burningDrawer(victimX, victimY, frames = 11) {
    return () => {
        const assets = realCaptureAssets();
        const scene = new Scene({ prepare: (self) => { assets.types.forEach((record, number) => Object.assign(self.world.types[number], record, { cost: record.cost ?? 0 })); } });
        const { particles, world } = scene;
        const capturer = new RawShapeToBuffer(world.pool, assets.types, assets.shapes, assets.database, createShapeToBufferState());
        particles.connectBurning((sid) => capturer.capture(sid), () => {});
        particles.setView(VIEW, scene.display);
        const victim = world.pool.allocate(0);
        const raw = world.pool.allocated(victim);
        put(raw, 0, 0x508cc8);
        raw[10] = 70;
        raw[11] = 0;
        raw[34] = 0;
        raw[35] = 7;
        put(raw, 14, floatToBits(victimX));
        put(raw, 18, floatToBits(victimY));
        put(raw, 26, 10);
        put(raw, 36, 0);
        particles.addBurning(victim, 1);
        assert.equal(world.pool.slot(get(world.pool.slot(victim), 6, 2))[10], 14);
        for (let index = 0; index < frames; index += 1) scene.frame(2 + index * 0.1, index + 1);
        // 화면 가장자리에 걸친 피해자는 보이지 않는 상태로 남아 입자가 없을 수 있다(그림 없음).
        const [sprite] = particles.sprites(SCREEN);
        return (pixels, pitch, height, clip) => { if (sprite) sprite.procedure.draw(pixels, pitch, height, clip); };
    };
}

test('연소 표시 출력: 팔레트 색의 5점 입자가 부모 위치에 찍히고 창·표면 가장자리 규칙을 한 축씩 따른다', () => {
    const draw = burningDrawer(20.5, 21.25)();
    const pixels = new Uint8Array(1024 * 768);
    draw(pixels, 1024, 768, SCREEN);
    const colors = new Set(pixels);
    colors.delete(0);
    // 첫 색 표(8색 띠)와 인접 DWORD 1 에서 나온 색만 쓴다.
    assert.ok(colors.size > 0 && colors.size <= 9);
    edgeRules('연소', burningDrawer(20.5, 21.25), 'throw', false);
    // 화면 왼쪽 위 구석: clip 이 음수까지 허용되면 표면 밖(음수) 좌표는 경계 불일치 오류이고, clip 을 화면으로 제한하면 건너뛴다.
    const corner = burningDrawer(0.1, 1.0)();
    assert.throws(() => corner(new Uint8Array(1024 * 768), 1024, 768, { left: -50, top: -50, right: 1024, bottom: 768 }), RangeError);
    const clipped = new Uint8Array(1024 * 768);
    // 화면으로 제한한 clip 에서는 음수 점이 든 입자는 통째로 건너뛰므로 오류 없이 음수 좌표 감김이 생기지 않는다(마지막 열에 찍힌 픽셀이 없음).
    corner(clipped, 1024, 768, SCREEN);
    for (let row = 0; row < 768; row += 1) assert.equal(clipped[row * 1024 + 1023], 0);
});

// 파편을 만들어 첫 프레임까지 진행한 뒤 그리기 함수를 돌려준다. (2026-11-13 WB04-E390)
function shrapnelDrawer(x, y) {
    return () => {
        const scene = new FormScene();
        put(scene.pool.allocated(scene.source), 14, floatToBits(x));
        put(scene.pool.allocated(scene.source), 18, floatToBits(y));
        scene.particles.prepareFrame(1);
        assert.ok(scene.particles.addShrapnel(scene.source, ShrapnelKind.Explosion));
        scene.kernel.runFrame();
        const [sprite] = scene.particles.sprites(SCREEN);
        return (pixels, pitch, height, clip) => sprite.procedure.draw(pixels, pitch, height, clip);
    };
}

test('파편 출력: 창·표면 가장자리 규칙을 한 축씩 따르고 clip 이 음수까지 허용되면 표면 밖 좌표를 거부한다', () => {
    edgeRules('파편', shrapnelDrawer(2.125, 3.25), 'throw');
    // 구석의 파편: clip 을 화면으로 제한하면 음수 좌표를 건너뛰고, 음수까지 허용하면 경계 불일치 오류이다.
    const corner = shrapnelDrawer(0.05, 0.05)();
    assert.throws(() => corner(new Uint8Array(1024 * 768), 1024, 768, { left: -100, top: -100, right: 1024, bottom: 768 }), RangeError);
    const clipped = new Uint8Array(1024 * 768);
    corner(clipped, 1024, 768, SCREEN);
    assert.ok(countOf(clipped, 71) > 0 && clipped.every((pixel) => pixel === 0 || pixel === 71));
});

// ---- 2차 변형 확인에서 드러난 공백을 보강한 시험들 (2026-11-13 WB04-E390) ----

test('불꽃: 화면 끝점이 모든 선의 공통 끝이라 그림이 끝점 둘레 몇 픽셀 안에 모이고, 설정 없는 기본 영역은 명시한 같은 영역과 같은 그림이다', () => {
    const x = Math.fround(20.123456);
    const y = Math.fround(21.654321);
    const make = (explicit) => {
        const scene = new Scene();
        const parent = scene.parent(80, x, y);
        if (explicit) scene.particles.setView(VIEW, scene.display);
        scene.particles.add(11, parent);
        scene.frame(2);
        scene.frame(2.025);
        return scene.render(scene.particles.sprites(SCREEN), 199);
    };
    const pixels = make(true);
    const [endX, endY] = projectCameraCoordinates(0, 0, x, y);
    let farthest = 0;
    for (let index = 0; index < pixels.length; index += 1) {
        if (pixels[index] === 199) continue;
        farthest = Math.max(farthest, Math.abs((index % 1024) - endX), Math.abs(Math.floor(index / 1024) - endY));
    }
    // 끝점을 읽는 float 바이트가 하나라도 어긋나면 선이 멀리 뻗는다.
    assert.ok(farthest <= 4, `끝점에서 가장 먼 픽셀 ${farthest}`);
    assert.deepEqual(make(false), pixels);
});

test('악마 광선: TYPE 번호가 TYPE 표 길이 이상이면(SHP 표가 더 길어도) 표를 읽기 전에 거부하고, SHP 폭 값이 광선 모양을 결정한다', () => {
    const build = (mutate, width = 9) => {
        const scene = new Scene({
            prepare: (self) => {
                Object.assign(self.world.types[129], { flags1: 0x10, maxHitPoints: 600, footX: 1, footY: 1 });
                self.shapes[129] = { frameCount: 4, loaded: true, frames: Array.from({ length: 4 }, () => ({ width, height: 3 })) };
                mutate?.(self);
            },
        });
        const { particles, world } = scene;
        const parent = world.pool.allocate(0);
        const raw = world.pool.allocated(parent);
        put(raw, 0, 0x501dd0);
        raw[10] = 129;
        raw[11] = 0;
        raw[34] = 1;
        raw[35] = 7;
        put(raw, 14, floatToBits(20.5));
        put(raw, 18, floatToBits(21.25));
        put(raw, 26, 600);
        const aux = new GameAuxState();
        aux.particleSource = parent;
        particles.bindTailProcessTypes(aux);
        particles.connectEvilRayDevices({ soundId: () => 11, sound: () => {}, frameError: () => {}, assertion: () => {} });
        particles.setView(VIEW, scene.display);
        particles.addEvilRay(parent);
        return { scene, raw, particles };
    };
    // SHP 표를 TYPE 표보다 길게 만들면 TYPE 번호 188 은 TYPE 표 검사에서만 걸린다.
    const long = build((self) => { while (self.shapes.length < 200) self.shapes.push({ frameCount: 0, loaded: false, frames: [] }); });
    long.raw[10] = 188;
    assert.throws(() => long.scene.frame(2), (error) => error instanceof RangeError && /TYPE 물리 범위/.test(error.message));
    // 폭은 광선의 길이를 정한다: 서로 다른 폭은 서로 다른 그림이고, 16비트 부호 감김으로 같은 폭(+65536)은 같은 그림이다.
    const image = (width) => {
        const { scene, particles } = build(null, width);
        scene.frame(2);
        scene.frame(2.05);
        scene.frame(2.1);
        return scene.render(particles.sprites(SCREEN));
    };
    const wide = image(9);
    assert.notDeepEqual(image(0), wide);
    assert.notDeepEqual(image(40), wide);
    assert.deepEqual(image(9 + 65536), wide);
    assert.deepEqual(image(9 - 65536), wide);
});

test('건설 표시: 모든 열·행이 복사되고 투명 255 는 복사하지 않으며 clip 경계 한 칸 안팎과 화면 0 열/0 행 픽셀을 정확히 다룬다', () => {
    const run = (x, y, cut = SCREEN) => {
        const scene = new Scene();
        const { particles, world } = scene;
        const image = { width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) };
        particles.connectConstruction(() => image, () => 100);
        particles.setView(VIEW, scene.display);
        put(world.pool.allocated(world.parent), 14, floatToBits(x));
        put(world.pool.allocated(world.parent), 18, floatToBits(y));
        particles.addConstruction(world.parent, false);
        scene.frame(2);
        scene.frame(2.1);
        const pixels = new Uint8Array(1024 * 768).fill(199);
        for (const sprite of particles.sprites(SCREEN)) sprite.procedure.draw(pixels, 1024, 768, cut);
        return pixels;
    };
    const list = (pixels) => {
        const result = [];
        for (let index = 0; index < pixels.length; index += 1) if (pixels[index] !== 199) result.push([index % 1024, Math.floor(index / 1024)]);
        return result;
    };
    const full = run(20.5, 21.25);
    const found = list(full);
    // 투명(255)을 복사하지 않으므로 255 가 없고, 복사 구간의 다섯 열과 두 행에 모두 걸친다(열/행을 건너뛰지 않는다).
    assert.ok(!full.includes(255));
    assert.deepEqual([...new Set(found.map(([px]) => px))].sort((a, b) => a - b), [326, 327, 328, 329, 330]);
    assert.deepEqual([...new Set(found.map(([, py]) => py))].sort((a, b) => a - b), [235, 236]);
    // 경계 한 칸 안쪽/바깥쪽 clip: 가장 바깥 열·행을 포함하는 창은 전체와 같고 하나 줄이면 그 열/행이 사라진다.
    const minX = 326;
    const maxX = 330;
    const minY = 235;
    const maxY = 236;
    const sameAsFull = [
        { left: minX, top: 0, right: 1024, bottom: 768 }, { left: 0, top: 0, right: maxX + 1, bottom: 768 },
        { left: 0, top: minY, right: 1024, bottom: 768 }, { left: 0, top: 0, right: 1024, bottom: maxY + 1 },
    ];
    for (const window of sameAsFull) assert.deepEqual(run(20.5, 21.25, window), full, JSON.stringify(window));
    const narrowed = [
        [{ left: minX + 1, top: 0, right: 1024, bottom: 768 }, ([px]) => px >= minX + 1],
        [{ left: 0, top: 0, right: maxX, bottom: 768 }, ([px]) => px < maxX],
        [{ left: 0, top: minY + 1, right: 1024, bottom: 768 }, ([, py]) => py >= minY + 1],
        [{ left: 0, top: 0, right: 1024, bottom: maxY }, ([, py]) => py < maxY],
    ];
    for (const [window, keep] of narrowed) {
        const expected = found.filter(keep);
        assert.ok(expected.length > 0 && expected.length < found.length);
        assert.deepEqual(list(run(20.5, 21.25, window)), expected, JSON.stringify(window));
    }
    // 화면 0 열의 픽셀은 복사한다: 부모를 왼쪽 가장자리에서 1/16 칸씩 옮겨 가며 첫 열에 닿는 위치를 찾는다.
    let zeroColumn = null;
    for (let step = 0; step <= 8 && zeroColumn === null; step += 1) {
        const pixels = list(run(step / 16, 21.25));
        if (pixels.some(([px]) => px === 0)) zeroColumn = pixels;
    }
    assert.ok(zeroColumn !== null, '0 열에 닿는 위치');
    assert.ok(zeroColumn.every(([px, py]) => px < 6 && py >= 230));
    // 화면 0 행의 픽셀도 같다(부모 y 가 음수여도 보이는 부분만 찍는다).
    let zeroRow = null;
    for (let step = -4; step <= 4 && zeroRow === null; step += 1) {
        const pixels = list(run(20.5, step / 11));
        if (pixels.some(([, py]) => py === 0)) zeroRow = pixels;
    }
    assert.ok(zeroRow !== null, '0 행에 닿는 위치');
    assert.ok(zeroRow.every(([px, py]) => py < 8 && px >= 320));
});

test('연소 표시: 화면 0 열·0 행의 픽셀도 오류 없이 찍고, 프레임 번호의 낮은 두 비트가 슬롯 7 이상의 연소 프로세스 실행 주기를 정한다', () => {
    // 위치를 옮겨 가며 첫 열/첫 행에 닿는 그림을 찾는다(음수 점이 없는 clip 이라 오류 없이 그려져야 한다).
    const drawAt = (x, y) => {
        const pixels = new Uint8Array(1024 * 768);
        burningDrawer(x, y)()(pixels, 1024, 768, SCREEN);
        return pixels;
    };
    const touches = (pixels, axis, value) => {
        for (let index = 0; index < pixels.length; index += 1) if (pixels[index] !== 0 && (axis === 'x' ? index % 1024 : Math.floor(index / 1024)) === value) return true;
        return false;
    };
    let zeroColumn = false;
    for (let step = 0; step <= 40 && !zeroColumn; step += 1) zeroColumn = touches(drawAt(step / 16, 5), 'x', 0);
    assert.ok(zeroColumn, '0 열에 닿는 위치');
    let zeroRow = false;
    for (let step = 0; step <= 40 && !zeroRow; step += 1) zeroRow = touches(drawAt(5, step / 11), 'y', 0);
    assert.ok(zeroRow, '0 행에 닿는 위치');
    // 슬롯 0..6 은 항상 실행하고 슬롯 7 이상은 (슬롯 ^ 프레임 번호) & 3 == 0 일 때만 실행한다. 슬롯 8 에서 단계 진행으로 확인한다.
    const assets = realCaptureAssets();
    const scene = new Scene({ prepare: (self) => { assets.types.forEach((record, number) => Object.assign(self.world.types[number], record, { cost: record.cost ?? 0 })); } });
    const { particles, world } = scene;
    const capturer = new RawShapeToBuffer(world.pool, assets.types, assets.shapes, assets.database, createShapeToBufferState());
    particles.connectBurning((sid) => capturer.capture(sid), () => {});
    particles.setView(VIEW, scene.display);
    for (let index = 0; index < 8; index += 1) particles.add(11, scene.parent(80, 5 + index, 5));
    const victim = world.pool.allocate(0);
    const raw = world.pool.allocated(victim);
    put(raw, 0, 0x508cc8);
    raw[10] = 70;
    raw[11] = 0;
    raw[34] = 0;
    raw[35] = 7;
    put(raw, 14, floatToBits(20.5));
    put(raw, 18, floatToBits(21.25));
    put(raw, 26, 10);
    put(raw, 36, 0);
    const process = particles.addBurning(victim, 1);
    assert.equal(process.fields.registration.slot, 8);
    // 프레임 번호 생략 = 0: 슬롯 8 은 (8 ^ 0) & 3 == 0 이라 실행한다.
    particles.prepareFrame(2);
    world.kernel.runFrame();
    assert.equal(process.fields.stage, 1);
    // 프레임 번호 1: (8 ^ 1) & 3 = 1 이라 건너뛴다.
    particles.prepareFrame(2.1, 1);
    world.kernel.runFrame();
    assert.equal(process.fields.stage, 1);
    // 프레임 번호 4 는 낮은 두 비트가 0 이라 실행한다(32비트 값을 그대로 쓴다: 반으로 줄이면 2 가 되어 건너뛴다).
    particles.prepareFrame(2.2, 4);
    world.kernel.runFrame();
    assert.equal(process.fields.stage, 2);
});

test('sprites: 수집 경계는 카메라 원점을 더한 화면 사각형을 16/11 로 나눈 월드 사각형이 효과 등록 범위와 만나는 첫 픽셀이다', () => {
    const scene = new Scene();
    const { particles } = scene;
    const camera = new CameraView();
    camera.initialize();
    camera.setPixels(48, 22);
    particles.bindCameraView(camera);
    particles.setView(VIEW, scene.display);
    const parent = scene.parent(80, 20.5, 21.25);
    particles.add(11, parent);
    scene.frame(2);
    scene.frame(2.025);
    const reg = scene.processOf(parent).fields.registration;
    const include = (clip) => particles.sprites(clip).length;
    // 등록 범위는 화면 배율 좌표(×16/×11)이고 영역은 화면 사각형 + 카메라 원점이다.
    const right = Math.ceil(reg.left - 48);
    const left = Math.floor(reg.right - 48);
    const bottom = Math.ceil(reg.top - 22);
    const top = Math.floor(reg.bottom - 22);
    assert.equal(include({ left: 0, top: 0, right, bottom: 768 }), 1);
    assert.equal(include({ left: 0, top: 0, right: right - 1, bottom: 768 }), 0);
    assert.equal(include({ left, top: 0, right: 1024, bottom: 768 }), 1);
    assert.equal(include({ left: left + 1, top: 0, right: 1024, bottom: 768 }), 0);
    assert.equal(include({ left: 0, top: 0, right: 1024, bottom }), 1);
    assert.equal(include({ left: 0, top: 0, right: 1024, bottom: bottom - 1 }), 0);
    assert.equal(include({ left: 0, top, right: 1024, bottom: 768 }), 1);
    assert.equal(include({ left: 0, top: top + 1, right: 1024, bottom: 768 }), 0);
});

test('불꽃: 시간이 더 지나면 색 196 선은 모두 사라지고 남은 잔여 픽셀 수가 고정된다(회귀 고정, 원본 대조가 아님)', () => {
    const scene = new Scene();
    const { particles } = scene;
    const parent = scene.parent(80, 20.5, 21.25);
    particles.setView(VIEW, scene.display);
    particles.add(11, parent);
    scene.frame(2);
    scene.frame(2.025);
    scene.frame(2.5);
    scene.frame(3.5);
    const later = scene.render(particles.sprites(SCREEN), 199);
    assert.equal(countOf(later, 196), 0);
    assert.equal(1024 * 768 - countOf(later, 199), 19);
});

test('파편·냉각 연기: 프레임 상태의 scaledDelta 배율이 입자 위치에 반영된다(회귀 고정, 원본 대조가 아님)', () => {
    // 파편 세 종류: 두 번째 프레임 뒤 그려진 픽셀 수와 처음 몇 개의 위치.
    const expectedShrapnel = [[19, [22560, 22561, 22562, 22563, 23584, 23585]], [20, [33825, 33826, 33827, 33828, 34849, 34850]], [20, [33825, 33826, 33827, 33828, 34849, 34850]]];
    for (const [kind, [count, first]] of [ShrapnelKind.Explosion, ShrapnelKind.Collapse, ShrapnelKind.Variant2].map((value, index) => [value, expectedShrapnel[index]])) {
        const scene = new FormScene();
        scene.particles.prepareFrame(1);
        scene.particles.addShrapnel(scene.source, kind);
        scene.kernel.runFrame();
        scene.particles.prepareFrame(1.05);
        scene.kernel.runFrame();
        const pixels = new Uint8Array(1024 * 768);
        for (const sprite of scene.particles.sprites(SCREEN)) sprite.procedure.draw(pixels, 1024, 768, sprite.clip);
        const drawn = [];
        for (let index = 0; index < pixels.length; index += 1) if (pixels[index]) drawn.push(index);
        assert.equal(drawn.length, count, `종류 ${kind}`);
        assert.deepEqual(drawn.slice(0, 6), first, `종류 ${kind}`);
    }
    const smoke = new Scene();
    smoke.particles.setView(VIEW, smoke.display);
    smoke.particles.addCooling(smoke.parent(80, 20.5, 21.25), [16, -11]);
    for (let step = 0; step < 6; step += 1) smoke.frame(2 + step * 0.05);
    const pixels = smoke.render(smoke.particles.sprites(SCREEN));
    const drawn = [];
    for (let index = 0; index < pixels.length; index += 1) if (pixels[index]) drawn.push(index);
    assert.equal(drawn.length, 10);
    assert.deepEqual(drawn.slice(0, 6), [222551, 222552, 225621, 225622, 225623, 225624]);
});
