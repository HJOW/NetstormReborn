// 불꽃 꼬리(process 11)·건설 마나(process 12)의 실제 몸체(E187 `raw-fiery-tail-frame.js`·`raw-fiery-tail-lifetime.js`)를 같은 Host/Kernel 과 공통 표시 목록(`render-process-system.js`)에 연결하는 모듈이다
// (cpppj `o/FieryTailVisualSystem.cpp/.h`, 10.78 전용). 원본 등록/입자/시간/난수/색 계산은 바꾸지 않고 실제 프로세스 소유권만 연결한다.
//  - FieryTailVisualProcess: 부모에 부착되고 Kernel 이 소유하는 효과 프로세스이다. runFrame 은 같은 Kernel 시각의 실제 프레임(최초 10입자·시간 조각), onFormDestroy 는 입자 반납보다 먼저 삭제 영역 dirty,
//    draw 는 부모의 현재 화면 끝점까지의 선 출력, destroy(Kernel 제거·생성 실패 때)는 입자 → 공통 등록/base → 객체 반납과 조회 제거이다. fields 는 `createFieryTailFrameFields()` 이다. 시스템이 만든다.
//  - new FieryTailVisualSystem(rendering, random, lifetime, frame, palette, hooks, variant = Fiery): rendering 은 같은 월드의 `RenderProcessSystem`, lifetime 은 E187 수명 상태({ palette, sharedPalette?, sharedProcessType? }),
//    frame 은 현재 `{ now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY, diagnostic }`(배율은 rendering.state 와 같아야 함), palette = { count, readable } 공유 색 표이다.
//    hooks = { lifetime: { reserve, createPalette?, buildPalette? }, frame: { invalidate, line }, endpoint(parent)→[x, y](원본 00497220 의 현재 부모 화면 좌표), release(FieryTailRelease) } 이며,
//    endpoint/line 은 호출 중 효과를 지우거나 게임 상태를 진행하지 않아야 한다.
//  - add(parent): 표시 등록 → 실제 client Form 부착 → 120입자/공유 색 초기화 순서이며 부모 0·중첩 생성은 거부한다. 실패하면 부착된 객체는 Host 삭제(flags 0x10), 부착 전 객체는 직접 소멸한다.
//  - size: 현재 소유한 효과 수. dispose(): Host 가 살아 있을 때 남은 효과를 Host 삭제(flags 0x10)로 정리한다(C++ 소멸자).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E195: 불꽃 꼬리 표시 시스템 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawFieryTailFrame, createFieryTailFrameFields } from './raw-fiery-tail-frame.js';
import { FieryTailVariant, RawFieryTailLifetime } from './raw-fiery-tail-lifetime.js';
import { renderProcessRemovalBounds } from './render-process-system.js';

// client Form 의 실제 로컬 삭제 권한 flags 이다. (2026-11-09 WB04-E195)
const CLIENT_KILL = 0x10;

// Kernel 이 소유하는 불꽃 꼬리 프로세스이다. bridge 는 시스템이 만든 내부 연결이다. (2026-11-09 WB04-E195)
export class FieryTailVisualProcess extends SquidProcess {
    #bridge;

    // 파생 필드 초기화 완료 표시는 시스템 add 가 마지막에 세운다. (2026-11-09 WB04-E195)
    constructor(bridge) {
        super();
        if (!bridge || typeof bridge.destroy !== 'function') throw new TypeError('불꽃 꼬리 프로세스는 시스템이 만든다');
        this.#bridge = bridge;
        // 원본 입자·시계·공통 등록의 단일 저장소이다.
        this.fields = createFieryTailFrameFields();
        // 파생 생성 완료 전 실행/출력/삭제 표시 통지를 막는다.
        this.ready = false;
    }

    // 같은 Kernel 등록 순서와 현재 시각을 그대로 쓴다. (2026-11-09 WB04-E195)
    runFrame() { this.#bridge.run(this); }

    // raw Form 의 가상 0xc 는 입자 반납보다 먼저 현재 표시 영역을 무효화한다. (2026-11-09 WB04-E195)
    onFormDestroy() { this.#bridge.removing(this); }

    // 같은 공통 목록의 kind3 출력 분배이다. (2026-11-09 WB04-E195)
    draw() { this.#bridge.draw(this); }

    // Kernel 소멸이 입자/표시 등록을 원본 순서대로 정리한다(C++ 소멸자). (2026-11-09 WB04-E195)
    destroy() { this.#bridge.destroy(this); }
}

// 같은 공통 registry/Host/Kernel·난수와 실제 불꽃 꼬리 수명을 묶는 시스템이다. (2026-11-09 WB04-E195)
export class FieryTailVisualSystem {
    #rendering;
    #random;
    #frame;
    #palette;
    #hooks;
    #lifetime;
    // 토큰 → 프로세스(실제 소유권은 pending 또는 Kernel)이다.
    #processes = new Map();
    // 공통 등록의 Host 부착 전 소유자이다.
    #pending = null;
    // 생성 콜백의 중첩 add 를 거부한다.
    #creating = false;
    // 프로세스가 시스템 내부 동작을 부르는 연결이다(C++ friend).
    #bridge;

    // 출력 경계와 같은 풀/판본은 객체를 만들기 전에 검사한다. (2026-11-09 WB04-E195)
    constructor(rendering, random, lifetime, frame, palette, hooks, variant = FieryTailVariant.Fiery) {
        this.#rendering = rendering;
        this.#random = random;
        this.#frame = frame;
        this.#palette = palette;
        this.#hooks = hooks;
        this.#lifetime = new RawFieryTailLifetime(rendering.pool, rendering.registry, lifetime, hooks?.lifetime, variant);
        if (typeof hooks.endpoint !== 'function' || typeof hooks.release !== 'function') throw new TypeError('Fiery Tail 실제 끝점/반납 경계 누락');
        this.#bridge = Object.freeze({
            run: (process) => this.#run(process),
            draw: (process) => this.#draw(process),
            removing: (process) => this.#removing(process),
            destroy: (process) => this.#destroy(process),
        });
        this.#checkScale();
        // 프레임 출력 경계(invalidate/line)도 실제 계산 몸체로 확인한다.
        new RawFieryTailFrame(rendering.pool, random, frame, palette, hooks.frame);
    }

    // 두 값은 원본의 같은 전역이므로 다른 배율로 조용히 실행하지 않는다. (2026-11-09 WB04-E195)
    #checkScale() {
        const state = this.#rendering.state;
        if (this.#frame.pixelScaleX !== state.scaleX || this.#frame.pixelScaleY !== state.scaleY) throw new Error('Fiery Tail 공통 표시 배율 불일치');
    }

    // 생성 중 프레임/출력은 ready 가 막는다. 실제 Host 부착 뒤에 파생 초기화를 마친다. (2026-11-09 WB04-E195)
    add(parent) {
        if (!parent || this.#creating) throw new Error('Fiery Tail Host 부모/중첩 생성 오류');
        const rendering = this.#rendering;
        let token = 0;
        this.#creating = true;
        try {
            const process = new FieryTailVisualProcess(this.#bridge);
            this.#pending = process;
            token = rendering.track(process.fields.registration, (fields, type, target, flags) => {
                if (this.#pending !== process || fields !== process.fields.registration) throw new Error('Fiery Tail 부착 객체 불일치');
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
                    throw new Error('Fiery Tail 실제 부모 부착 실패');
                }
                fields.form = attached.form;
                fields.parent = attached.parent;
            }, () => process.draw());
            this.#processes.set(token, process);
            this.#lifetime.initialize(process.fields, token, parent);
            if (!this.#processes.has(token)) throw new Error('Fiery Tail 생성 중 객체 삭제');
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
                if (found) rendering.host.kill(found, CLIENT_KILL);
            }
            this.#creating = false;
            throw error;
        }
    }

    // 최초 10입자와 후속 시간 조각을 같은 난수/시계로 실행한다. (2026-11-09 WB04-E195)
    #run(process) {
        if (!process.ready) return;
        this.#checkScale();
        new RawFieryTailFrame(this.#rendering.pool, this.#random, this.#frame, this.#palette, this.#hooks.frame).step(process.fields);
    }

    // 끝점은 호출 시점의 실제 부모로 읽으며 픽셀 배율/카메라를 여기서 다시 더하지 않는다. (2026-11-09 WB04-E195)
    #draw(process) {
        if (!process.ready) return;
        this.#checkScale();
        const endpoint = this.#hooks.endpoint(process.fields.registration.parent & 0xffff);
        new RawFieryTailFrame(this.#rendering.pool, this.#random, this.#frame, this.#palette, this.#hooks.frame).draw(process.fields, endpoint[0] | 0, endpoint[1] | 0);
    }

    // 일반 프레임의 영역 계산 대신 실제 삭제 통지의 두 단계 정수화를 쓴다. (2026-11-09 WB04-E195)
    #removing(process) {
        if (!process.ready) return;
        const rect = renderProcessRemovalBounds(process.fields.registration, this.#frame.cameraX, this.#frame.cameraY);
        this.#hooks.frame.invalidate({ left: rect[0], top: rect[1], right: rect[2], bottom: rect[3] });
    }

    // 소멸 뒤에는 부모/Form 대신 원본 −12345 표식만 남긴다. (2026-11-09 WB04-E195)
    #destroy(process) {
        const token = process.fields.registration.token;
        if (process.fields.registration.slot >= 0) this.#lifetime.destroy(process.fields, 1, this.#hooks.release);
        this.#processes.delete(token);
        this.#rendering.forget(token);
    }

    // 공통 목록 전체 수와 다른 이 시스템의 소유 항목 수이다. (2026-11-09 WB04-E195)
    get size() { return this.#processes.size; }

    // 각 삭제가 조회를 지우므로 매번 현재 첫 항목을 다시 읽는다(C++ 소멸자). (2026-11-09 WB04-E195)
    dispose() {
        while (this.#processes.size) this.#rendering.host.kill(this.#processes.values().next().value, CLIENT_KILL);
    }
}
