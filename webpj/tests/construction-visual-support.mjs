// 건설 표시 시스템 시험(construction-visual-system.test.mjs)과 건설 표시 렌더러 시험(construction-visual-renderer.test.mjs)이 함께 쓰는 장면 장치이다.
// 실제 번호 할당·공통 삭제·Host·Kernel 위에 건설 표시 시스템을 올리고 장치 경계(출력·반납·팔레트·색 표)는 관찰만 한다(cpppj `ConstructionVisualSystemTests.cpp` 의 World, 10.78 전용).
// 실제 게임·장치 실행이 아니다. (2026-11-09 WB04-E194: 건설 표시 시스템 시험과 함께 추가, 2026-11-13 WB04-E382: 렌더러 시험과 공유하도록 지원 모듈로 분리)
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
import { ConstructionRelease } from '../www/core/raw-construction-lifetime.js';
import { RENDER_ENTRY_BYTES } from '../www/core/raw-render-collection.js';
import { RawContainedLookup, makeContainedConstructionHooks } from '../www/core/raw-contained-lookup.js';
import { ConstructionProcess, ConstructionProcessState, RawConstructionProcess } from '../www/core/raw-construction-process.js';
import { ConstructionVisualProcess, ConstructionVisualSystem, makeConstructionVisualSystemHooks } from '../www/core/construction-visual-system.js';

// cpppj RawSceneSupport 의 장면 풀 용량이다. (2026-11-09 WB04-E194)
export const CAPACITY = 24000;

// 합성 타입 100 에 최대 HP 100 을 공급하고 내장 Form 생성자 주소를 넣은 188칸 입력이다. (2026-11-09 WB04-E194)
export function types() {
    const result = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    result[100] = { ...result[100], flags1: 0x10, maxHitPoints: 100, footX: 1, footY: 1 };
    return result;
}

// 예외가 나는지 본다. (2026-11-09 WB04-E194)
export function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 각 완성 객체는 소유 버퍼 셋과 객체 자체를 차례로 반납한다(cpppj Released). (2026-11-09 WB04-E194)
export function released(list, count) {
    if (list.length !== count * 4) return false;
    // 반복된 네 단계가 모두 같은 원본 순서인지 비교한다.
    for (let index = 0; index < list.length; index += 4) {
        if (list[index] !== ConstructionRelease.SOURCE || list[index + 1] !== ConstructionRelease.OUTPUT || list[index + 2] !== ConstructionRelease.PARTICLES || list[index + 3] !== ConstructionRelease.OBJECT) return false;
    }
    return true;
}

// 실제 번호 할당과 공통 삭제를 공유하며 장치 경계는 출력/반납을 관찰하는 장면이다(cpppj World, 10.78). (2026-11-09 WB04-E194)
export class World {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = types();
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        // 부모 삭제가 Form 삭제와 같은 카운터를 공유하는지 본다. 공간 자산 효과는 완료 통지만 처리한다.
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
        this.visualState = createConstructionVisualState();
        this.frameState = createConstructionFrameState();
        this.colors = Array.from({ length: 12 }, (_, index) => index);
        this.palette = { token: 71, count: 11, entries: this.colors };
        this.released = [];
        this.allocations = [];
        this.dirty = [];
        this.draws = [];
        this.captures = 0;
        this.maximumCalls = 0;
        this.system = null;
        // 부모와 Form 은 각각 서버/client 번호에서 실제로 할당한다.
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
        put(raw, 26, 0);
        this.visualState.palette = 71;
        this.frameState.now = 2.0;
    }

    // 원본 목록의 32비트 확보 크기를 기록한다. 부착은 시스템이 실제 Host 로 바꾼다. (2026-11-09 WB04-E194)
    registryHooks() {
        return { reserve: (bytes) => { this.allocations.push(bytes); return true; }, releaseArray: () => {} };
    }

    // 실제 부착이 캡처보다 먼저 끝났고 생성 중 객체가 목록과 Kernel 양쪽에 있는지 본다. (2026-11-09 WB04-E194)
    visualHooks() {
        return {
            reserve: () => true,
            createPalette: () => 71,
            gradient: () => {},
            buildPalette: () => {},
            releaseParticles: () => {},
            capture: (sid) => {
                assert.ok(sid === this.parent && this.system.size > 0 && this.kernel.size > 0);
                const form = get(this.pool.slot(this.parent), 6, 2);
                const process = this.host.fromForm(form);
                assert.ok(process instanceof ConstructionVisualProcess && this.pool.slot(form)[10] === 17);
                assert.equal(this.system.slots[process.registration.slot], process.registration.token);
                this.captures += 1;
                return { width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) };
            }
        };
    }

    // HP/dirty/Draw 는 호출 인자와 픽셀을 읽기만 한다. kill 은 시스템 안에서 Host 에 연결한다. (2026-11-09 WB04-E194)
    frameHooks() {
        return {
            maxHitPoints: (sid) => { assert.equal(sid, this.parent); this.maximumCalls += 1; return 100; },
            invalidate: (rect) => this.dirty.push(rect),
            draw: (request) => {
                assert.ok(request.pitch === 5 && request.clip.right === 5 && request.clip.bottom === 7 && request.pixels.length === 35);
                this.draws.push([request.x, request.y]);
            }
        };
    }

    // 시스템을 마지막에 만들어 Host/Kernel/색 표보다 먼저 정리한다. (2026-11-09 WB04-E194)
    createSystem(visual = this.visualHooks(), frame = this.frameHooks(), registry = this.registryHooks()) {
        this.system = ConstructionVisualSystem.standalone(this.pool, this.host, this.visualState, this.frameState, this.palette, this.random, registry, visual, frame, (kind) => this.released.push(kind));
    }

    // 부모 HP(10.78 DWORD)를 쓴다. (2026-11-09 WB04-E194)
    hp(value) { put(this.pool.allocated(this.parent), 26, value); }

    // 완료 소리/공간 효과는 명시 경계로 두고 진행/완료/Form 몸체만 연결한다. (2026-11-09 WB04-E194)
    buildHooks() {
        return makeConstructionVisualSystemHooks(this.pool, this.system, {
            reserve: () => true,
            loadSound: () => 0,
            reserveSound: () => false,
            attachSound: () => {},
            refresh: () => {},
            maxHitPoints: () => 100,
            updateDisplay: () => {},
            removeContained: () => {},
            playAt: () => {},
            readySound: () => 0,
            play: () => {},
            unpop: (sid) => { this.pool.allocated(sid)[11] |= 4; },
            repop: (sid, flags) => { assert.equal(flags, 9); this.pool.allocated(sid)[11] &= 0xfb; },
            push: () => {}
        });
    }
}

// 수집 항목 16바이트의 토큰(+8)·종류(+14)이다. (2026-11-09 WB04-E194)
export const entryToken = (work, index) => new DataView(work.buffer, work.byteOffset).getUint32(index * RENDER_ENTRY_BYTES + 8, true);
