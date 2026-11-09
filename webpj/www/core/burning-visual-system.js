// Burning(process 14)의 실제 몸체(E183 생성·연소점, E184 출력·소멸, E200 프레임)를 같은 Host/Kernel 과 공통 표시 목록(`render-process-system.js`)에 연결하는 모듈이다(cpppj `o/BurningVisualSystem.cpp/.h`, 10.78 전용).
// 원본 수치 몸체는 바꾸지 않고 실제 등록/부착/프레임 gate/출력/삭제 통지/소멸의 소유권만 연결한다.
//  - BurningVisualProcess: 피해 객체(부모)에 부착되고 Kernel 이 소유하는 Burning 프로세스이다. fields 는 `createBurningVisualFields()`, frame 은 `createBurningFrameFields()`(생성자가 남긴 두 float)이다.
//    runFrame 은 원본 +18 gate(487940, 표시 slot 과 현재 프레임 번호) 뒤 실제 +24 프레임, onFormDestroy 는 487960 의 현재 dirty 통지, notify 는 보호 VT 503730+1c 의 480260(빈 ret12),
//    draw 는 현재 dirty clip 의 5점 입자 출력, destroy(Kernel 제거·생성 실패 때)는 이미지 → 행 → 소리 → 입자 → 공통 등록 → 객체 반납과 조회 제거이다. 시스템이 만들며 외부에서 직접 생성하지 않는다.
//  - new BurningVisualSystem(rendering, random, types, visualState, sourcesState, frameState, palette, frameNumber, hooks):
//    rendering 은 같은 월드의 `RenderProcessSystem`, random 은 게임 난수, types 는 188칸 TYPE 표, visualState = { processType: 14, palette }(E183), sourcesState(E183 연소점 상태),
//    frameState(E200 `createBurningFrameState()`, 배율은 rendering.state 와 같아야 함), palette = { count, readable }(E184 출력 색), frameNumber() 는 원본 55b4a8 현재 DWORD 프레임 번호 getter,
//    hooks = { visual: { reserve, createPalette?, gradient?, buildPalette? }, frame: { capture, stopSound }(capture 는 `makeBurningFrameCapture`), invalidate(rect), clip() → 현재 dirty clip, pixel(x, y, color), release(BurningRelease) } 이다.
//    invalidate·clip·pixel·release·frame.stopSound 가 없거나 10.78 풀이 아니면 거부하고, 같은 풀/캡처 조건은 실제 RawBurningFrame 생성으로 미리 확인한다.
//  - add(victim, mode): 실제 모드 0/1 wrapper 의 같은 생성자이다. victim 0·중첩 생성은 거부하고, 공통 등록 → 실제 Host 부착(null/예외면 즉시 소멸) → 150입자/첫 색 표 → 준비 완료 순서이다.
//    실패하면 부착 전 객체는 직접 소멸, 부착된 객체는 Host 삭제(flags 0x10)로 정리한다.
//  - size: 현재 살아 있는 process 14 수. dispose(): Host 가 살아 있을 때 남은 객체를 Host 삭제(flags 0x10)로 정리한다(C++ 소멸자).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E201: Burning 표시 시스템 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawBurningVisual, createBurningVisualFields } from './raw-burning-visual.js';
import { RawBurningSources } from './raw-burning-sources.js';
import { RawBurningFrame, createBurningFrameFields } from './raw-burning-frame.js';
import { destroyBurningVisual, drawBurningVisual, renderProcessFrameDue } from './raw-burning-lifetime-draw.js';
import { renderProcessRemovalBounds } from './render-process-system.js';

// 월드 종료/생성 실패의 실제 client Form 반납 권한 flags 이다. (2026-11-09 WB04-E201)
const LOCAL_KILL = 0x10;

// 실제 client Form 에 부착되는 원본 Burning 프로세스이다. bridge 는 시스템이 만든 내부 연결이다. (2026-11-09 WB04-E201)
export class BurningVisualProcess extends SquidProcess {
    #bridge;

    // 파생 초기화는 add 가 실제 부착 뒤 완료한다. (2026-11-09 WB04-E201)
    constructor(bridge) {
        super();
        if (!bridge || typeof bridge.destroy !== 'function') throw new TypeError('Burning 프로세스는 시스템이 만든다');
        this.#bridge = bridge;
        // 실제 등록/150입자/캡처의 단일 소유 저장소이다.
        this.fields = createBurningVisualFields();
        // 생성자가 미정으로 남긴 두 float 이며 실제 timer 호출 뒤에만 소비한다.
        this.frame = createBurningFrameFields();
        // 생성 중 Kernel/출력/삭제 통지가 미완료 파생 필드를 읽지 않게 하는 준비 완료 표시이다.
        this.ready = false;
    }

    // 원본 +18 → 선택된 +24 의 실제 호출이다. (2026-11-09 WB04-E201)
    runFrame() { this.#bridge.run(this); }

    // 소멸 앞의 현재 영역 통지이다. (2026-11-09 WB04-E201)
    onFormDestroy() { this.#bridge.removing(this); }

    // 보호 VT 503730+1c 의 480260 은 빈 ret12 이다(세 인자를 읽지 않는다). (2026-11-09 WB04-E201)
    notify() {}

    // 공통 kind3 출력의 실제 파생 출력이다. (2026-11-09 WB04-E201)
    draw() { this.#bridge.draw(this); }

    // Kernel 이 소유한 객체의 실제 반납이다(C++ 소멸자). (2026-11-09 WB04-E201)
    destroy() { this.#bridge.destroy(this); }
}

// 같은 공통 목록·Host·난수와 Burning 생성/프레임/출력/소멸을 묶는 시스템이다. 참조한 상태/장치는 이 객체보다 오래 살아야 한다. (2026-11-09 WB04-E201)
export class BurningVisualSystem {
    #rendering;
    #random;
    #types;
    #frameState;
    #palette;
    #frameNumber;
    #hooks;
    #sink;
    #visual;
    #sources;
    // 토큰 → 프로세스(소유권은 pending 또는 Kernel)이다.
    #processes = new Map();
    // 실제 Host 부착 전 유일한 소유자이다.
    #pending = null;
    // 생성 중 같은 시스템 재진입을 거부한다.
    #creating = false;
    // 프로세스가 시스템 내부 동작을 부르는 연결이다(C++ friend).
    #bridge;

    // 같은 현재 월드/시각과 필수 장치를 확인한다. (2026-11-09 WB04-E201)
    constructor(rendering, random, types, visualState, sourcesState, frameState, palette, frameNumber, hooks) {
        if (typeof hooks?.invalidate !== 'function' || typeof hooks.clip !== 'function' || typeof hooks.pixel !== 'function' || typeof hooks.release !== 'function'
            || typeof hooks.frame?.stopSound !== 'function' || rendering?.pool?.edition !== 'patch1078' || typeof frameNumber !== 'function') throw new TypeError('Burning 실제 시스템/장치/판본 누락');
        this.#rendering = rendering;
        this.#random = random;
        this.#types = types;
        this.#frameState = frameState;
        this.#palette = palette;
        this.#frameNumber = frameNumber;
        this.#hooks = hooks;
        // Burning 자체의 두 SetRect 는 sink 의 억제 전역을 조회하지 않으며 flags 0 영역만 넘긴다.
        this.#sink = Object.freeze({ suppressed: () => false, invalidate: (rect) => hooks.invalidate(rect) });
        this.#visual = new RawBurningVisual(rendering.pool, rendering.registry, visualState, hooks.visual ?? {});
        this.#sources = new RawBurningSources(rendering.pool, types, this.#visual, random, sourcesState);
        this.#bridge = Object.freeze({
            run: (process) => this.#run(process),
            draw: (process) => this.#draw(process),
            removing: (process) => this.#removing(process),
            destroy: (process) => this.#destroy(process),
        });
        // 같은 풀/캡처 연결을 실제 프레임 생성으로 미리 확인한다.
        this.#body();
    }

    // 현재 상태로 실제 Burning 프레임 몸체를 만든다. (2026-11-09 WB04-E201)
    #body() {
        return new RawBurningFrame(this.#rendering.pool, this.#types, this.#visual, this.#sources, this.#random, this.#frameState, this.#sink, this.#hooks.frame);
    }

    // Raw 생성자의 목록 등록 → Host 부착 → 150입자/첫 색 표 순서를 바꾸지 않는다. (2026-11-09 WB04-E201)
    add(victim, mode) {
        if (!victim || this.#creating) throw new Error('Burning 실제 피해 부모/중첩 생성 오류');
        const rendering = this.#rendering;
        let token = 0;
        this.#creating = true;
        try {
            const process = new BurningVisualProcess(this.#bridge);
            this.#pending = process;
            token = rendering.track(process.fields.registration, (fields, type, target, flags) => {
                if (this.#pending !== process || fields !== process.fields.registration) throw new Error('Burning 실제 부착 필드 불일치');
                // 소유권을 Host 로 넘긴다. 부착이 null 이나 예외로 실패하면 받는 쪽이 없으므로 C++ unique_ptr 매개변수 해제처럼 바로 소멸시킨다.
                this.#pending = null;
                let attached;
                try {
                    attached = rendering.host.attach(process, type, target, flags);
                } catch (error) {
                    process.destroy();
                    throw error;
                }
                if (!attached) {
                    process.destroy();
                    throw new Error('Burning 실제 부모 부착 거부');
                }
                fields.form = attached.form;
                fields.parent = attached.parent;
            }, () => process.draw());
            this.#processes.set(token, process);
            this.#visual.initialize(process.fields, token, victim, mode);
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
                if (found) rendering.host.kill(found, LOCAL_KILL);
            }
            this.#creating = false;
            throw error;
        }
    }

    // 공통 Kernel 은 +18 을 부르며 뒤쪽 표시 slot 은 원본처럼 4프레임 중 한 번만 실행한다. (2026-11-09 WB04-E201)
    #run(process) {
        if (!process.ready || !renderProcessFrameDue(process.fields.registration.slot, this.#frameNumber())) return;
        const state = this.#rendering.state;
        if (this.#frameState.pixelScaleX !== state.scaleX || this.#frameState.pixelScaleY !== state.scaleY) throw new Error('Burning 현재 표시/프레임 배율 불일치');
        this.#body().step(process.fields, process.frame);
    }

    // 현재 renderer clip 이 설정된 뒤에만 실제 출력한다(프레임 상태의 사본에 현재 clip 을 넣는다). (2026-11-09 WB04-E201)
    #draw(process) {
        if (!process.ready) return;
        const state = { ...this.#frameState, viewport: this.#hooks.clip() };
        drawBurningVisual(process.fields, state, this.#palette, this.#hooks.pixel);
    }

    // 생성 미완료 객체에는 파생 사각형을 읽지 않는다. (2026-11-09 WB04-E201)
    #removing(process) {
        if (!process.ready) return;
        const rect = renderProcessRemovalBounds(process.fields.registration, this.#frameState.cameraX, this.#frameState.cameraY);
        this.#hooks.invalidate({ left: rect[0], top: rect[1], right: rect[2], bottom: rect[3] });
    }

    // 실제 소멸자는 공유 색 표를 건드리지 않고 단일 현재 조회만 제거한다. (2026-11-09 WB04-E201)
    #destroy(process) {
        const token = process.fields.registration.token;
        if (process.fields.registration.slot >= 0) destroyBurningVisual(process.fields, this.#rendering.registry, 1, this.#hooks.release, this.#hooks.frame.stopSound);
        this.#processes.delete(token);
        this.#rendering.forget(token);
    }

    // 다른 표시 종류와 구별한 실제 살아 있는 process 14 의 수이다. (2026-11-09 WB04-E201)
    get size() { return this.#processes.size; }

    // 각 삭제가 현재 조회를 바꾸므로 첫 항목을 매번 다시 읽는다(같은 실제 client Form 의 즉시 해제, C++ 소멸자). (2026-11-09 WB04-E201)
    dispose() {
        while (this.#processes.size) this.#rendering.host.kill(this.#processes.values().next().value, LOCAL_KILL);
    }
}
