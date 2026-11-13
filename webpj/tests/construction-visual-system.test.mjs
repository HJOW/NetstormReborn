// 건설 표시 시스템 construction-visual-system.js(ConstructionVisualSystem)를 실제 Kernel/Form/종속 삭제 위에서 검사한다(cpppj `tests/ConstructionVisualSystemTests.cpp`).
// cpppj 15건 중 `o/` 부품만 쓰는 10건을 10.78 판으로 그대로 옮겼다. client 렌더러(ConstructionVisualRenderer `DrawVisibleConstruction`·DrawSubmission `DrawVisibleScene`, PORT_MAP todo)를 쓰는
// RenderCollection_VisibleConstructionSubmission·RenderCollection_SmallTiesAndInvalidSortCoordinates·DrawSort_ConstructionUsesNativeTieOrder·DrawSubmission 2건은 그 client 단위에서 옮긴다. 10.72(CD) 반복은 범위 밖이다.
//  - 부착/실행/출력/자기 삭제/빈 칸 재사용, Start 의 표시+HP 진행 두 프로세스 완주, contained 조회의 최근 표시만 삭제, 부모 삭제의 전체 반납, 시스템 정리, 생성 실패 되돌림,
//    생성 중 재진입 차단, 출력 중 삭제와 목록 40→80 확장, 수집 뒤 삭제/재사용 토큰, 다른 풀/경계 누락 거부와 캡처 중 취소.
//  - 합성 TYPE 표는 cpppj `Types()` 에 웹 Host 가 ProcessForm 을 만들 때 읽는 내장 Form 생성자 주소 넷(cpppj MakeFormTypeInputs 와 같은 입력)을 더했다. 기대값 계산에는 쓰지 않는다.
// 실제 게임·장치 실행이 아니다. (2026-11-09 WB04-E194: 건설 표시 시스템 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { get } from './raw-support.mjs';
import { Kernel } from '../www/core/kernel.js';
import { RENDER_ENTRY_BYTES } from '../www/core/raw-render-collection.js';
import { RawContainedLookup, makeContainedConstructionHooks } from '../www/core/raw-contained-lookup.js';
import { ConstructionProcess, ConstructionProcessState, RawConstructionProcess } from '../www/core/raw-construction-process.js';
import { ConstructionVisualProcess, ConstructionVisualSystem, makeConstructionVisualSystemHooks } from '../www/core/construction-visual-system.js';
import { World, entryToken, released, throwsOf, types } from './construction-visual-support.mjs';


test('건설 표시 시스템: 실제 client Form 부착·Kernel 실행·현재 카메라 출력·자기 삭제와 빈 칸 재사용(일반/즉시)', () => {
    const world = new World();
    world.createSystem();
    // 즉시/일반 건설은 같은 Kernel/소멸 경로를 쓴다.
    for (const instant of [false, true]) {
        const process = world.system.add(world.parent, instant);
        const form = process.form;
        const token = process.registration.token;
        assert.ok(form >= 5 && form < world.pool.layout.serverFirst && process.parent === world.parent);
        assert.ok(world.host.fromForm(form) === process && world.kernel.size === 1 && process.registration.form === form);
        assert.ok(process.registration.slot === 0 && process.registration.zOrder === -1 && process.fields.visual.instant === instant);
        assert.ok((world.pool.slot(form)[11] & 12) === 8 && get(world.pool.slot(form), 16, 2) === world.parent);
        world.hp(50);
        world.kernel.runFrame();
        assert.ok(process.fields.visual.lastTime === world.frameState.now && world.dirty.length > 0);
        assert.ok(process.registration.left === process.fields.visual.left && process.registration.right === process.fields.visual.right);
        world.draws.length = 0;
        world.system.draw();
        assert.deepEqual(world.draws, [[326, 230]]);
        world.frameState.now += 0.1;
        world.frameState.cameraX = 4;
        world.frameState.cameraY = 5;
        world.kernel.runFrame();
        world.draws.length = 0;
        world.system.draw();
        assert.deepEqual(world.draws, [[322, 225]]);
        world.pool.allocated(world.parent)[40] = 0;
        world.kernel.runFrame();
        assert.ok(world.kernel.size === 0 && world.system.size === 0 && world.system.slots[0] === 0 && (world.pool.slot(form)[11] & 1));
        assert.ok(world.system.slots.length === 40 && world.visualState.palette === 71 && token !== 0);
        world.pool.allocated(world.parent)[40] = 1;
        world.frameState.cameraX = 0;
        world.frameState.cameraY = 0;
    }
    assert.ok(released(world.released, 2));
    assert.deepEqual(world.allocations, [160]);
});

test('건설 표시 시스템: Start 는 표시를 먼저, HP 진행을 뒤에 붙이고 완료의 abstract 해제 뒤 다음 Kernel 순회에서 표시가 끝난다', () => {
    const world = new World();
    world.createSystem();
    const contained = new RawContainedLookup(world.pool, (form, flags) => {
        const process = world.host.fromForm(form);
        assert.ok(process);
        world.host.kill(process, flags);
    });
    // 표시 타입 17 과 HP 진행 타입 30 의 생성 순서도 본다(기본 10초/지정 8초).
    for (const duration of [0, 8]) {
        const state = new ConstructionProcessState();
        state.now = 2.0;
        const hooks = makeContainedConstructionHooks(world.pool, contained, world.buildHooks());
        const build = new RawConstructionProcess(world.pool, world.types, state, hooks, world.host);
        build.start(world.parent, Math.fround(duration));
        const visual = world.kernel.get(1);
        const progress = world.kernel.get(2);
        assert.ok(world.kernel.size === 2 && visual instanceof ConstructionVisualProcess && progress instanceof ConstructionProcess);
        assert.ok(visual.fields.visual.instant === (duration === 0) && get(world.pool.slot(progress.form), 4, 2) === visual.form);
        const end = duration === 0 ? 12.0 : 10.0;
        world.frameState.now = 6.0;
        state.now = 6.0;
        world.kernel.runFrame();
        assert.equal(get(world.pool.slot(world.parent), 26, 4), duration === 0 ? 40 : 50);
        world.frameState.now = end;
        state.now = end;
        world.kernel.runFrame();
        assert.ok(world.kernel.size === 1 && world.system.size === 1);
        assert.equal(world.pool.slot(world.parent)[40] & 1, 0);
        world.kernel.runFrame();
        assert.ok(world.kernel.size === 0 && world.system.size === 0 && get(world.pool.slot(world.parent), 6, 2) === 0);
        world.hp(0);
        world.pool.allocated(world.parent)[40] = 1;
    }
    assert.ok(released(world.released, 2));
});

test('건설 표시 시스템: 실제 contained 조회가 가장 최근 표시만 지우며 다른 표시/Regular/부모 링크는 보존한다', () => {
    const world = new World();
    world.createSystem();
    const first = world.system.add(world.parent, false);
    const firstForm = first.form;
    const second = world.system.add(world.parent, true);
    const secondForm = second.form;
    const regular = world.host.addRegular(world.parent, 1, 1.0);
    const contained = new RawContainedLookup(world.pool, (sid, flags) => {
        assert.equal(flags, 0);
        world.host.kill(world.host.fromForm(sid), flags);
    });
    assert.equal(contained.find(world.parent, 17), secondForm);
    contained.find(world.parent, 17, 0, true);
    assert.ok(world.kernel.size === 2 && world.system.size === 1 && world.system.slots[0] !== 0 && world.system.slots[1] === 0);
    assert.ok((world.pool.slot(secondForm)[11] & 1) && get(world.pool.slot(regular.form), 4, 2) === firstForm);
    contained.find(world.parent, 17, 0, true);
    assert.ok(world.kernel.size === 1 && world.system.size === 0 && released(world.released, 2));
    world.host.kill(regular, 0);
    assert.equal(get(world.pool.slot(world.parent), 6, 2), 0);
});

test('건설 표시 시스템: 부모 삭제 때 공통 종속 삭제 → Kernel 소멸 → 버퍼/등록 제거 → Form 반납을 모두 거친다', () => {
    const world = new World();
    world.createSystem();
    const first = world.system.add(world.parent, false).form;
    const second = world.system.add(world.parent, true).form;
    // 부모의 공간 Unpop 은 별도 범위이므로 이미 void 인 부모의 공통 삭제를 실행한다.
    world.pool.allocated(world.parent)[11] |= 4;
    world.destroy.destroy(world.parent, 0, world.host.hooks());
    assert.ok(world.kernel.size === 0 && world.system.size === 0 && world.system.slots[0] === 0 && world.system.slots[1] === 0);
    assert.ok((world.pool.slot(first)[11] & 1) && (world.pool.slot(second)[11] & 1) && (world.pool.slot(world.parent)[11] & 1));
    assert.ok(world.destroy.preDepth === 0 && world.destroy.postDepth === 0 && released(world.released, 2));
});

test('건설 표시 시스템: 시스템이 먼저 정리돼도 살아 있는 Host/Kernel 에 시스템 참조나 Form 을 남기지 않는다', () => {
    const world = new World();
    world.createSystem();
    const first = world.system.add(world.parent, false).form;
    const second = world.system.add(world.parent, true).form;
    world.system.dispose();
    assert.ok(world.kernel.size === 0 && get(world.pool.slot(world.parent), 6, 2) === 0 && released(world.released, 2));
    assert.ok((world.pool.slot(first)[11] & 1) && (world.pool.slot(second)[11] & 1) && world.visualState.palette === 71);
});

test('건설 표시 시스템: 부착 전/후 실패는 기존 표시를 보존하고 새 Kernel/Form/표시 칸을 정리한다', () => {
    const world = new World();
    let failObject = false;
    let failCapture = false;
    const hooks = world.visualHooks();
    const capture = hooks.capture;
    hooks.reserve = (bytes) => !(failObject && bytes === 136);
    hooks.capture = (sid) => {
        if (failCapture) throw new Error('캡처 실패 입력');
        return capture(sid);
    };
    world.createSystem(hooks);
    const alive = world.system.add(world.parent, false);
    const form = alive.form;
    failObject = true;
    assert.ok(throwsOf(() => world.system.add(world.parent, false)));
    failObject = false;
    failCapture = true;
    assert.ok(throwsOf(() => world.system.add(world.parent, false)));
    failCapture = false;
    world.visualState.processType = 100;
    assert.ok(throwsOf(() => world.system.add(world.parent, false)));
    world.visualState.processType = 17;
    assert.ok(world.kernel.size === 1 && world.system.size === 1 && world.host.fromForm(form) === alive && world.system.slots[1] === 0);
    assert.equal(get(world.pool.slot(world.parent), 6, 2), form);
    const next = world.system.add(world.parent, true);
    assert.ok(next.registration.slot === 1 && world.system.size === 2);
});

test('건설 표시 시스템: 캡처 중 Kernel/출력/수집은 생성 중 객체를 실행하지 않고 중첩 생성 거부 뒤 다음 생성은 정상이다', () => {
    const world = new World();
    const hooks = world.visualHooks();
    const capture = hooks.capture;
    hooks.capture = (sid) => {
        const calls = world.maximumCalls;
        const draws = world.draws.length;
        world.kernel.runFrame();
        world.system.draw();
        assert.ok(world.maximumCalls === calls && world.draws.length === draws);
        const work = new Uint8Array(RENDER_ENTRY_BYTES);
        assert.equal(world.system.collect({ left: -1000, top: -1000, right: 1000, bottom: 1000 }, work, 1), 0);
        assert.ok(throwsOf(() => world.system.add(world.parent, false)));
        return capture(sid);
    };
    world.createSystem(hooks);
    world.system.add(world.parent, false);
    world.kernel.runFrame();
    assert.ok(world.maximumCalls === 1 && world.system.size === 1);
});

test('건설 표시 시스템: 출력 콜백이 현재 객체를 지우고 목록을 40→80칸으로 늘려도 해제된 객체를 다시 쓰지 않는다', () => {
    const world = new World();
    const hooks = world.frameHooks();
    let changed = false;
    let draws = 0;
    let first = null;
    hooks.draw = (request) => {
        assert.equal(request.pixels.length, 35);
        draws += 1;
        if (changed) return;
        changed = true;
        world.host.kill(first, 0);
        // 이미 지난 슬롯 0 은 다음 출력, 이후 슬롯은 같은 출력에서 제출된다.
        for (let index = 0; index < 41; index += 1) world.system.add(world.parent, false);
    };
    world.createSystem(world.visualHooks(), hooks);
    first = world.system.add(world.parent, false);
    world.system.draw();
    assert.ok(draws === 41 && world.system.size === 41 && world.system.slots.length === 80);
    assert.deepEqual(world.allocations, [160, 320]);
    world.system.draw();
    assert.ok(draws === 82 && released(world.released, 1));
    world.system.dispose();
    assert.ok(world.kernel.size === 0 && released(world.released, 42));
});

test('건설 표시 수집: 수집 뒤 삭제된 토큰은 건너뛰고 재사용된 슬롯의 새 객체를 이전 목록에 섞지 않으며 다른 종류 항목은 거부한다', () => {
    const world = new World();
    world.createSystem();
    const first = world.system.add(world.parent, false);
    const second = world.system.add(world.parent, false);
    const work = new Uint8Array(RENDER_ENTRY_BYTES * 3);
    assert.equal(world.system.collect({ left: 20, top: 20, right: 22, bottom: 23 }, work, 3), 2);
    const oldToken = entryToken(work, 0);
    world.host.kill(first, 0);
    const replacement = world.system.add(world.parent, false);
    assert.ok(replacement.registration.slot === 0 && replacement.registration.token !== oldToken);
    world.system.drawCollected(work.subarray(0, RENDER_ENTRY_BYTES * 2));
    assert.ok(world.draws.length === 1 && entryToken(work, 1) === second.registration.token);
    world.draws.length = 0;
    work[14] = 2;
    assert.ok(throwsOf(() => world.system.drawCollected(work.subarray(0, RENDER_ENTRY_BYTES))));
    assert.equal(world.draws.length, 0);
});

test('건설 표시 시스템: 다른 풀이나 누락된 장치 경계는 생성 전에 거부하고 캡처 중 취소된 객체도 다시 접근하지 않는다', () => {
    const world = new World();
    const other = new World();
    const frame = world.frameHooks();
    frame.draw = undefined;
    assert.ok(throwsOf(() => world.createSystem(world.visualHooks(), frame)));
    assert.ok(throwsOf(() => ConstructionVisualSystem.standalone(other.pool, world.host, world.visualState, world.frameState, world.palette, world.random, world.registryHooks(), world.visualHooks(), world.frameHooks(), () => {})));
    const visual = world.visualHooks();
    let cancel = true;
    visual.capture = () => {
        if (cancel) {
            const form = get(world.pool.slot(world.parent), 6, 2);
            world.host.kill(world.host.fromForm(form), 0);
        }
        return { width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) };
    };
    world.createSystem(visual);
    assert.ok(throwsOf(() => world.system.add(world.parent, false)));
    assert.ok(world.kernel.size === 0 && world.system.size === 0 && world.system.slots[0] === 0);
    cancel = false;
    world.system.add(world.parent, false);
    assert.ok(world.kernel.size === 1 && world.system.size === 1);
    assert.ok(throwsOf(() => makeConstructionVisualSystemHooks(other.pool, world.system, {})));
});
