// 번개 팔레트 프로세스 thunder-process.js 를 cpppj `ThunderProcessTests.cpp` 의 장면과 웹 계약으로 검사한다.
//  - 실제 188 TYPE 자료·SquidFactory·SquidDestroy·SquidProcessHost·Kernel·삭제 수명(SquidDestroyLifecycle)을 한 풀에 묶고, 부모는 실제 다리(TYPE 82) 생성자가 만든 void 다리이다.
//    시계·전체화면 여부·팔레트 장치만 합성 입력이며 객체/프로세스 계산은 실제 모듈이다(cpppj ThunderScene 과 같은 구분).
//  - cpppj 대응: ThunderProcess_RealFormNaturalEndAndSummary(자연 종료)·_RealFormCancelRestoresCurrentScreenOnce(중단 복구 한 번)·_ParentDestroyReleasesSameProcess(부모 삭제)·
//    IssueCommands_ThunderBrightnessUsesActualFormPalette(signed 밝기가 실제 Form 생성자를 거쳐 같은 팔레트 계산에 도달).
//  - 옮기지 못한 것: 자연 종료 시험의 procSumm.txt 요약(WriteRawProcessSummary — 웹에는 개발 파일 출력이 없어 Kernel 진단 장치 몫)과
//    PaletteFrameChecking_ActualClientClock… (실제 Client 프레임 시계·일시정지·checking 소유자 필요 — 4-D Client 본체에서 옮긴다).
//  - 웹 계약: 부착 전에는 팔레트를 읽지 않음(읽기 0)·부착 직후 두 번 읽음·창 모드는 화면 팔레트를 전혀 바꾸지 않음(중단도)·runFrame 의 자기 삭제는 flags 0 으로 host.kill 을 부름·
//    destroyed 에 Kernel 번호를 알림(없어도 안전)·불완전한 장치 묶음 거부·밝기 0 은 기본 +64·자연 종료 뒤 중복 복구 없음.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E381)
import test from 'node:test';
import assert from 'node:assert/strict';
import { realPlayerAssets } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import { emptyScreenColor } from '../www/core/palette-flash.js';
import { THUNDER_PROCESS_TYPE, ThunderProcess } from '../www/core/thunder-process.js';

// 번개 프로세스 TYPE 은 원본 0054114c 의 55 이다(상수를 그대로 쓰면 변형이 가려지므로 리터럴로도 확인한다). (2026-11-13 WB04-E381)
const ORIGINAL_THUNDER_TYPE = 55;

// 실제 TYPE·공장·삭제 수명·Host·Kernel 을 한 풀에 묶은 장면이다. wall/full/palette 와 읽기·적용·복구·소멸 횟수가 외부 입력과 관측이다. (2026-11-13 WB04-E381)
function thunderScene() {
    const assets = realPlayerAssets();
    const types = assets.types.map((record) => ({ ...record }));
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const unpop = new SquidUnpop(pool, hash, spots);
    const book = new SquidPostPopState();
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const factory = new SquidFactory(pool, types, { unpop });
    const deletion = new SquidDeletionState();
    deletion.unitLostSuppressed1 = true;
    const kernel = new Kernel();
    const clock = new SquidProcessState();
    let host = null;
    const lifecycle = new SquidDestroyLifecycle(pool, types, book, deletion, destroy, {
        emit: () => {},
        forward: (event) => { if (host) host.hooks().emit(event); else throw new Error('번개 수명 검사 Host 미설정'); },
    });
    host = new SquidProcessHost(pool, types, kernel, destroy, clock, { asset: lifecycle.hooks() });
    const scene = {
        pool, types, factory, destroy, kernel, host, forms: new SquidForms(pool),
        palette: Array.from({ length: 256 }, emptyScreenColor),
        wall: 0, full: false, reads: 0, applies: 0, restores: 0, destructs: 0, removed: 0,
        // 같은 부분 팔레트 장치를 효과에 전달한다. 읽기는 현재 화면 팔레트의 사본을 돌려준다.
        hooks() {
            return {
                wallSeconds: () => scene.wall,
                fullScreen: () => scene.full,
                readPalette: (start, count, destination) => {
                    scene.reads += 1;
                    for (let index = 0; index < count; index += 1) destination[index] = { ...scene.palette[start + index] };
                },
                applyPalette: () => { scene.applies += 1; },
                restorePalette: () => { scene.restores += 1; },
            };
        },
        // 실제 다리 생성자가 초기화한 void 다리를 부모로 쓴다(별도 공간 Pop 은 필요하지 않다).
        parent() { return factory.create(82, 2); },
        // 실제 부모+6 WORD(종속 머리)를 읽는다.
        head(parent) {
            const raw = pool.slot(parent);
            return raw[6] | (raw[7] << 8);
        },
        // 실제 TYPE 55·flags 0x10 으로 등록한다. 초기화 직전의 원문 장치 수명(읽기 0)도 확인한다.
        attach(parent, { fullScreen = scene.full, brightness = 0, hooks = scene.hooks(), destroyed = (pid) => { scene.destructs += 1; scene.removed = pid; } } = {}) {
            const process = new ThunderProcess(fullScreen, hooks, destroyed, brightness);
            assert.equal(scene.reads, 0);
            return host.attach(process, THUNDER_PROCESS_TYPE, parent, 0x10) ? process : null;
        },
    };
    return scene;
}

test('번개 TYPE 번호는 원본 0054114c 의 55 이다', () => {
    assert.equal(THUNDER_PROCESS_TYPE, ORIGINAL_THUNDER_TYPE);
});

test('ThunderProcess 실제 Form: 창 모드는 부착 직후 팔레트를 두 번 읽고 시간이 지나면 자연 종료해 Form 이 삭제되며 팔레트는 한 번도 바꾸지 않는다(RealFormNaturalEndAndSummary)', () => {
    const scene = thunderScene();
    const { pool, kernel } = scene;
    const parent = scene.parent();
    const process = scene.attach(parent);
    assert.ok(process);
    const form = process.form;
    const id = kernel.find(process);
    assert.deepEqual([id !== 0, scene.reads, process.parent, pool.slot(form)[10]], [true, 2, parent, ORIGINAL_THUNDER_TYPE]);
    assert.equal(scene.forms.parent(form), parent);
    assert.equal(scene.head(parent), form);
    // 창 모드 지속(1/60초)을 넘기면 한 프레임에 끝난다: Kernel 에서 빠지고 소멸 통지는 같은 번호로 한 번, 화면 팔레트는 건드리지 않으며 Form 은 삭제 상태가 된다.
    scene.wall = 1.0;
    kernel.runFrame();
    assert.equal(kernel.get(id), null);
    assert.deepEqual([scene.destructs, scene.removed, scene.applies, scene.restores], [1, id, 0, 0]);
    assert.ok(pool.slot(form)[11] & 1);
    assert.equal(scene.head(parent), 0);
});

test('ThunderProcess 실제 Form: 전체화면 효과를 중간에 지우면 현재 화면 팔레트를 한 번만 복구하고 Form 이 반납된다(RealFormCancelRestoresCurrentScreenOnce)', () => {
    const scene = thunderScene();
    scene.full = true;
    const { pool, kernel } = scene;
    const parent = scene.parent();
    const process = scene.attach(parent);
    assert.ok(process);
    const form = process.form;
    kernel.runFrame();
    assert.deepEqual([scene.applies, scene.restores], [1, 0]);
    scene.host.kill(process, 0x10);
    assert.deepEqual([scene.destructs, scene.restores, kernel.size], [1, 1, 0]);
    assert.ok(pool.slot(form)[11] & 1);
    assert.equal(scene.head(parent), 0);
});

test('ThunderProcess 실제 Form: 부모를 지우면 같은 프로세스가 풀려 복구 한 번과 소멸 통지 한 번이 일어난다(ParentDestroyReleasesSameProcess)', () => {
    const scene = thunderScene();
    scene.full = true;
    const { pool, kernel, destroy, host } = scene;
    const parent = scene.parent();
    const process = scene.attach(parent);
    assert.ok(process);
    const form = process.form;
    const id = kernel.find(process);
    destroy.destroy(parent, 0x10, host.hooks());
    assert.equal(kernel.get(id), null);
    assert.deepEqual([scene.destructs, scene.restores], [1, 1]);
    assert.ok(pool.slot(parent)[11] & 1);
    assert.ok(pool.slot(form)[11] & 1);
});

test('ThunderProcess 밝기: 명령의 signed 밝기(-17)가 실제 Form 생성자를 거쳐 같은 팔레트 계산(색 228~245)에 도달한다(IssueCommands_ThunderBrightnessUsesActualFormPalette)', () => {
    const scene = thunderScene();
    scene.full = true;
    const parent = scene.parent();
    // 구별되는 원래 채널을 명시한다.
    for (const color of scene.palette) Object.assign(color, { red: 100, green: 120, blue: 140 });
    let observed = null;
    const hooks = scene.hooks();
    hooks.applyPalette = (start, count, colors) => {
        assert.deepEqual([start, count], [228, 18]);
        scene.applies += 1;
        observed = { ...colors[0] };
    };
    const process = scene.attach(parent, { hooks, brightness: -17 });
    assert.ok(process);
    scene.kernel.runFrame();
    assert.deepEqual([scene.applies, observed.red, observed.green, observed.blue], [1, 83, 103, 123]);
    scene.host.kill(process, 0x10);
    assert.deepEqual([scene.destructs, scene.restores], [1, 1]);
});

test('ThunderProcess 계약: 밝기 0 은 기본 +64 이고(채널 100→164) 255 에서 멈추며 창 모드는 중단해도 화면 팔레트를 바꾸지 않는다', () => {
    const scene = thunderScene();
    const parent = scene.parent();
    for (const color of scene.palette) Object.assign(color, { red: 100, green: 200, blue: 0 });
    // 전체화면 + 기본 밝기: 첫 프레임이 변경 색(100+64, 200+64→255 제한, 0+64)을 적용한다.
    scene.full = true;
    let first = null;
    const hooks = scene.hooks();
    hooks.applyPalette = (start, count, colors) => { scene.applies += 1; first ??= { ...colors[0] }; };
    const process = scene.attach(parent, { hooks });
    scene.kernel.runFrame();
    assert.deepEqual([first.red, first.green, first.blue], [164, 255, 64]);
    scene.host.kill(process, 0x10);
    // 창 모드: 같은 장면을 만들고 중단해도 적용/복구는 0 이고 소멸 통지만 한 번이다.
    const window = thunderScene();
    const windowParent = window.parent();
    const windowProcess = window.attach(windowParent);
    window.kernel.runFrame();
    window.host.kill(windowProcess, 0x10);
    assert.deepEqual([window.applies, window.restores, window.destructs], [0, 0, 1]);
});

test('ThunderProcess 계약: 자연 종료 프레임은 host.kill 을 flags 0 으로 한 번만 부르고 그 뒤 소멸에서 팔레트를 중복 복구하지 않는다', () => {
    const scene = thunderScene();
    scene.full = true;
    const parent = scene.parent();
    const process = scene.attach(parent);
    const kills = [];
    const original = scene.host.kill.bind(scene.host);
    scene.host.kill = (target, flags) => { kills.push([target === process, flags]); return original(target, flags); };
    // 전체화면 지속(0.5초)을 한참 넘기면 단계가 모두 끝나 PaletteFlash 가 저장 팔레트를 복구하고(restores 1) 종료한다.
    scene.wall = 10;
    scene.kernel.runFrame();
    assert.deepEqual(kills, [[true, 0]]);
    assert.deepEqual([scene.restores, scene.destructs, scene.kernel.size], [1, 1, 0]);
});

test('ThunderProcess 계약: 소멸 통지가 없어도 안전하고 불완전한 장치 묶음은 생성에서 거부하며 부착 전 번호는 0 이다', () => {
    const scene = thunderScene();
    const parent = scene.parent();
    const bare = new ThunderProcess(false, scene.hooks());
    assert.deepEqual([bare.form, bare.parent, bare.pid, scene.reads], [0, 0, 0, 0]);
    assert.ok(scene.host.attach(bare, THUNDER_PROCESS_TYPE, parent, 0x10));
    bare.host.kill(bare, 0x10);
    assert.equal(scene.kernel.size, 0);
    for (const name of ['wallSeconds', 'fullScreen', 'readPalette', 'applyPalette', 'restorePalette']) {
        const hooks = scene.hooks();
        delete hooks[name];
        assert.throws(() => new ThunderProcess(true, hooks), TypeError, name);
    }
    assert.throws(() => new ThunderProcess(true, null), TypeError);
});

test('ThunderProcess 계약: 생성 인자를 생략하면 밝기는 기본 +64, 소멸 통지는 없음이다(네 번째·세 번째 인자 기본값)', () => {
    const scene = thunderScene();
    scene.full = true;
    const parent = scene.parent();
    for (const color of scene.palette) Object.assign(color, { red: 10, green: 20, blue: 30 });
    let first = null;
    const hooks = scene.hooks();
    hooks.applyPalette = (start, count, colors) => { first ??= { ...colors[0] }; };
    const process = new ThunderProcess(true, hooks);
    assert.ok(scene.host.attach(process, THUNDER_PROCESS_TYPE, parent, 0x10));
    scene.kernel.runFrame();
    assert.deepEqual([first.red, first.green, first.blue], [74, 84, 94]);
    scene.host.kill(process, 0x10);
    assert.equal(scene.destructs, 0);
});

test('ThunderProcess 계약: 지속 시간은 모드로 정해진다 — 창 모드 1/60초는 0.1초에 끝나고 전체화면 0.5초는 0.1초에 계속되며 0.5초를 넘기면 끝난다', () => {
    // 창 모드: 0.1초면 64단계가 모두 지나 끝난다(진행만 하고 팔레트는 건드리지 않음).
    const window = thunderScene();
    const windowParent = window.parent();
    window.attach(windowParent);
    window.wall = 0.1;
    window.kernel.runFrame();
    assert.deepEqual([window.kernel.size, window.destructs, window.applies], [0, 1, 0]);
    // 전체화면: 0.1초에는 12단계만 지나 계속(적용 1)되고, 0.5초가 지나면(간격×64 초과) 끝나며 복구한다.
    const full = thunderScene();
    full.full = true;
    const fullParent = full.parent();
    const process = full.attach(fullParent);
    full.wall = 0.1;
    full.kernel.runFrame();
    assert.deepEqual([full.kernel.size, full.destructs, full.applies, full.restores], [1, 0, 1, 0]);
    full.wall = 0.51;
    full.kernel.runFrame();
    assert.deepEqual([full.kernel.size, full.destructs, full.restores], [0, 1, 1]);
    assert.equal(process.pid, full.removed);
});
