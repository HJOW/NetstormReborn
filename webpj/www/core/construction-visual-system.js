// 복원한 건설 표시 몸체(E128 `raw-construction-visual.js`·E130 `raw-construction-frame.js`·E129 수명)를 실제 ProcessForm·Kernel·공통 표시 목록의 소유권에 연결하는 모듈이다
// (cpppj `o/ConstructionVisualSystem.cpp/.h`, 10.78 전용, process 17). 장치 출력·client 깊이 정렬(ConstructionVisualRenderer/DrawSubmission)은 별도 client 단계이다.
//  - ConstructionVisualProcess: Kernel 이 소유하는 건설 표시 프로세스이다. fields 는 `createConstructionFrameFields()`(visual·delta·scaledDelta), registration 은 공통 등록 필드, ready 는 생성 완료 표시이다.
//    runFrame 은 실제 프레임(자기 삭제 가능), draw 는 현재 출력 버퍼 제출, destroy(Kernel 제거·생성 실패 때)는 캡처/출력/입자 → 표시 등록 → 객체 반납과 조회 제거이다. 시스템이 만들며 외부에서 직접 생성하지 않는다.
//  - new ConstructionVisualSystem(render, frameState, palette, random, visualHooks, frameHooks, release): 다른 효과와 같은 `RenderProcessSystem`(render)의 목록/토큰/Host 를 쓴다.
//    ConstructionVisualSystem.standalone(pool, host, visualState, frameState, palette, random, registryHooks, visualHooks, frameHooks, release): 공통 저장소를 직접 만들어 소유하는 단독 형태이다.
//    visualHooks 는 표시 생성 경계(reserve·createPalette·gradient·buildPalette·capture·releaseParticles, attach 는 시스템이 공통 목록 등록으로 바꿈), frameHooks 는 프레임 경계(maxHitPoints·invalidate·draw, kill 은 프로세스별 Host 삭제),
//    release(ConstructionRelease) 는 반납 관찰(예외·수명 변경 금지)이다. 다른 풀/경계 누락은 생성 전에 거부한다.
//  - add(parent, instant): 생성 중 목록 등록 → 실제 client Form 부착 → 캡처/입자 초기화 순서이며 중첩 생성은 거부한다. 실패하면 부착된 객체는 Host 삭제(flags 0x10), 부착 전 객체는 직접 소멸한다.
//  - draw()/collect(view, output, limit)/drawCollected(entries)/drawToken(token): 공통 저장소의 슬롯 순서 출력·원본 수집·수집 항목(16바이트, kind 3 만) 분배·토큰 출력이다(수집 뒤 삭제된 토큰은 생략).
//  - slots/size/pool/rendering, dispose(): 남은 표시를 Host 삭제(flags 0x10)로 정리한다(C++ 소멸자).
//  - makeConstructionVisualSystemHooks(pool, system, hooks): 건설 Start 의 표시 효과(effect)를 이 시스템의 add 로 잇는다(다른 풀 거부).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E194: 건설 표시 시스템 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawConstructionVisual } from './raw-construction-visual.js';
import { RawConstructionFrame, createConstructionFrameFields } from './raw-construction-frame.js';
import { createRenderProcessFields, destroyConstructionVisual, makeConstructionRegistryHooks } from './raw-construction-lifetime.js';
import { RenderProcessSystem } from './render-process-system.js';
import { RENDER_ENTRY_BYTES } from './raw-render-collection.js';

// 실패 정리/시스템 종료에서 client Form 삭제 권한을 명시하는 원본 flags 와 표시 수집 항목 종류이다. (2026-11-09 WB04-E194)
const CLIENT_KILL = 0x10;
const RENDER_KIND = 3;

// 프레임이 바꾼 표시 영역/깊이를 같은 RenderProcess 등록 필드에 반영한다. (2026-11-09 WB04-E194)
function synchronizeRegistration(registration, visual) {
    registration.left = visual.left;
    registration.top = visual.top;
    registration.right = visual.right;
    registration.bottom = visual.bottom;
    registration.zOrder = visual.zOrder;
}

// Kernel 이 소유하는 건설 표시 프로세스이다. 생성 중에는 실행/출력을 건너뛴다. bridge 는 시스템이 만든 내부 연결이다. (2026-11-09 WB04-E194)
export class ConstructionVisualProcess extends SquidProcess {
    #bridge;

    // 부착 전 필드는 기본값이며 생성 완료 표시는 시스템 add 가 설정한다. (2026-11-09 WB04-E194)
    constructor(bridge) {
        super();
        if (!bridge || typeof bridge.destroy !== 'function') throw new TypeError('건설 표시 프로세스는 시스템이 만든다');
        this.#bridge = bridge;
        // 이 프로세스가 소유하는 픽셀/입자/시각이다.
        this.fields = createConstructionFrameFields();
        // 원본 RenderProcess 등록 필드이다.
        this.registration = createRenderProcessFields();
        // 생성 몸체가 끝난 뒤에만 실행/출력을 허용한다.
        this.ready = false;
    }

    // 실제 Kernel 호출을 복원한 프레임 몸체에 전달한다. 자기 삭제 뒤 멤버를 읽지 않는다. (2026-11-09 WB04-E194)
    runFrame() { this.#bridge.run(this); }

    // 클라이언트 출력 단계에서 현재 출력 버퍼를 제출한다. (2026-11-09 WB04-E194)
    draw() { this.#bridge.draw(this); }

    // 캡처/출력/입자와 표시 등록을 원본 순서로 반납한다(C++ 소멸자). Form 반납은 Host 가 맡는다. (2026-11-09 WB04-E194)
    destroy() { this.#bridge.destroy(this); }
}

// 원본 복원 몸체를 묶는 Host 어댑터이다. (2026-11-09 WB04-E194)
export class ConstructionVisualSystem {
    #pool;
    #host;
    #visualState;
    #frameState;
    #palette;
    #random;
    #visualHooks;
    #frameHooks;
    #release;
    #rendering;
    // 토큰 → 프로세스(소유권은 pending 또는 Kernel)이다.
    #processes = new Map();
    // 상위 부착 전 생성 중인 객체의 소유자이다.
    #pending = null;
    // 콜백 중 중첩 생성을 막는다.
    #creating = false;
    // 프로세스가 시스템 내부 동작을 부르는 연결이다(C++ friend).
    #bridge;

    // 다른 효과와 같은 표시 목록/토큰/Host 를 공유한다. render 는 이 시스템보다 오래 살아야 한다. (2026-11-09 WB04-E194)
    constructor(render, frameState, palette, random, visualHooks, frameHooks, release) {
        if (!(render instanceof RenderProcessSystem)) throw new TypeError('건설 표시 시스템 연결 오류');
        this.#rendering = render;
        this.#pool = render.pool;
        this.#host = render.host;
        this.#visualState = render.state;
        this.#frameState = frameState;
        this.#palette = palette;
        this.#random = random;
        this.#visualHooks = { ...visualHooks };
        this.#frameHooks = { ...frameHooks };
        this.#release = release;
        this.#bridge = Object.freeze({
            run: (process) => this.#run(process),
            draw: (process) => this.#drawProcess(process),
            destroy: (process) => this.#destroy(process),
        });
        this.#validate();
    }

    // 공통 저장소를 직접 만들어 소유하는 단독 형태이다(cpppj 첫 생성자). (2026-11-09 WB04-E194)
    static standalone(pool, host, visualState, frameState, palette, random, registryHooks, visualHooks, frameHooks, release) {
        return new ConstructionVisualSystem(new RenderProcessSystem(pool, host, visualState, registryHooks), frameState, palette, random, visualHooks, frameHooks, release);
    }

    // 생성 경로와 관계없이 같은 원본 몸체의 필수 경계를 검사한다. (2026-11-09 WB04-E194)
    #validate() {
        if (this.#pool !== this.#host.pool || typeof this.#release !== 'function') throw new TypeError('건설 표시 시스템 연결 오류');
        new RawConstructionVisual(this.#pool, this.#visualState, this.#random, { ...this.#visualHooks, attach: () => {} });
        new RawConstructionFrame(this.#pool, this.#visualState, this.#frameState, this.#palette, this.#random, { ...this.#frameHooks, kill: () => {} });
    }

    // 한 프레임만 쓰는 kill 콜백이다. 프로세스 자체에 자기 삭제 콜백을 저장하지 않는다. (2026-11-09 WB04-E194)
    #frameHooksFor(process) {
        const host = this.#host;
        return { ...this.#frameHooks, kill: (flags) => host.kill(process, flags) };
    }

    // 생성 중 목록 등록 → 실제 client Form 부착 → 캡처/입자 초기화 순서이다. 반환 객체는 자기 종료/부모 삭제 때 무효이다. (2026-11-09 WB04-E194)
    add(parent, instant) {
        if (this.#creating) throw new Error('건설 표시 중첩 생성은 지원하지 않습니다');
        const rendering = this.#rendering;
        let token = 0;
        this.#creating = true;
        try {
            const process = new ConstructionVisualProcess(this.#bridge);
            this.#pending = process;
            token = rendering.track(process.registration, (fields, type, target, flags) => {
                if (this.#pending !== process || fields !== process.registration) throw new Error('건설 표시 부착 객체 불일치');
                // 소유권을 Host 로 넘긴다. 부착이 null 이나 예외로 실패하면 받는 쪽이 없으므로 C++ unique_ptr 매개변수 해제처럼 바로 소멸시킨다.
                this.#pending = null;
                let attached;
                try {
                    attached = this.#host.attach(process, type, target, flags);
                } catch (error) {
                    process.destroy();
                    throw error;
                }
                if (!attached) {
                    process.destroy();
                    throw new Error('건설 표시 부모 부착 실패');
                }
                fields.form = attached.form;
                fields.parent = attached.parent;
            }, () => process.draw());
            this.#processes.set(token, process);
            const visual = new RawConstructionVisual(this.#pool, this.#visualState, this.#random, makeConstructionRegistryHooks(rendering.registry, process.registration, token, this.#visualHooks));
            const fields = visual.create(parent, instant);
            if (!this.#processes.has(token)) throw new Error('건설 표시 생성 중 객체 삭제');
            process.fields.visual = fields;
            synchronizeRegistration(process.registration, fields);
            process.ready = true;
            rendering.ready(token);
            this.#creating = false;
            return process;
        } catch (error) {
            if (this.#pending) {
                const pending = this.#pending;
                this.#pending = null;
                pending.destroy();
            } else {
                const found = this.#processes.get(token);
                if (found) this.#host.kill(found, CLIENT_KILL);
            }
            this.#creating = false;
            throw error;
        }
    }

    // 프레임 종료 뒤에는 토큰 조회로 생존을 확인해 삭제된 객체에 사각형을 쓰지 않는다. (2026-11-09 WB04-E194)
    #run(process) {
        if (!process.ready) return;
        const token = process.registration.token;
        new RawConstructionFrame(this.#pool, this.#visualState, this.#frameState, this.#palette, this.#random, this.#frameHooksFor(process)).run(process.fields);
        if (this.#processes.get(token) === process) synchronizeRegistration(process.registration, process.fields.visual);
    }

    // 출력의 kill 경계는 쓰이지 않지만 필수 계약을 채운다. 제출 뒤 객체를 다시 읽지 않는다. (2026-11-09 WB04-E194)
    #drawProcess(process) {
        if (!process.ready) return;
        new RawConstructionFrame(this.#pool, this.#visualState, this.#frameState, this.#palette, this.#random, { ...this.#frameHooks, kill: () => {} }).draw(process.fields);
    }

    // 소멸 경계는 예외를 던지지 않는 계약이다. 부착 전 확보 실패라면 아직 등록할 칸이 없다. (2026-11-09 WB04-E194)
    #destroy(process) {
        const token = process.registration.token;
        if (process.registration.slot >= 0) destroyConstructionVisual(process.fields.visual, process.registration, this.#rendering.registry, 1, this.#release);
        this.#processes.delete(token);
        this.#rendering.forget(token);
    }

    // 전체 표시 슬롯 순서로 출력을 제출하는 경로이다(가시 영역 출력은 collect/drawCollected). (2026-11-09 WB04-E194)
    draw() { this.#rendering.draw(); }

    // 원본 수집 몸체로 초기화 완료 객체의 가시 항목을 만든다. view 는 월드 좌표, output 은 16바이트 항목 Uint8Array 이다. (2026-11-09 WB04-E194)
    collect(view, output, limit) { return this.#rendering.collect(view, output, limit); }

    // 수집/정렬한 kind 3 항목(16바이트 Uint8Array)을 현재 토큰으로 조회해 제출한다. 수집 뒤 삭제된 항목은 건너뛰고 다른 종류는 거부한다. (2026-11-09 WB04-E194)
    drawCollected(entries) {
        const view = new DataView(entries.buffer, entries.byteOffset, entries.length);
        // 목록은 수집 시점의 항목이며 출력 중 새 객체를 만들어도 이번 목록에 넣지 않는다.
        for (let offset = 0; offset + RENDER_ENTRY_BYTES <= entries.length; offset += RENDER_ENTRY_BYTES) {
            if (view.getUint8(offset + 14) !== RENDER_KIND) throw new RangeError('건설 표시 수집 항목 종류 오류');
            this.drawToken(view.getUint32(offset + 8, true));
        }
    }

    // 공통 kind 3 분배가 현재 살아 있는 표시 토큰 하나를 그린다. (2026-11-09 WB04-E194)
    drawToken(token) { this.#rendering.drawToken(token); }

    // 목록의 구멍(사본)과 현재 소유 프로세스 수·같은 월드 확인 참조이다. (2026-11-09 WB04-E194)
    get slots() { return this.#rendering.slots; }
    get size() { return this.#processes.size; }
    get pool() { return this.#pool; }
    get rendering() { return this.#rendering; }

    // Host 가 system 보다 오래 살아 있으므로 남은 Kernel/Form 을 공통 삭제로 반납한다(C++ 소멸자). (2026-11-09 WB04-E194)
    dispose() {
        // 각 Kill 이 소멸에서 조회 항목을 지우므로 매번 현재 첫 항목을 다시 읽는다.
        while (this.#processes.size) this.#host.kill(this.#processes.values().next().value, CLIENT_KILL);
    }
}

// 건설 Start 의 표시 효과를 실제 표시 프로세스 생성에 연결한다. system 은 반환한 hooks 보다 오래 살아야 한다. (2026-11-09 WB04-E194)
export function makeConstructionVisualSystemHooks(pool, system, hooks) {
    if (pool !== system?.pool) throw new TypeError('건설 표시 시작의 풀 연결 오류');
    return { ...hooks, effect: (parent, instant) => { system.add(parent, instant); } };
}
