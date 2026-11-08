// EvilRay(process 10)의 실제 몸체(E190 `raw-evil-ray-frame.js`·`raw-evil-ray-lifetime.js`)를 같은 Host/Kernel 과 공통 표시 목록(`render-process-system.js`)에 연결하는 모듈이다(cpppj `o/EvilRayVisualSystem.cpp/.h`, 10.78 전용).
// 원본 수치 몸체는 바꾸지 않고 실제 등록/부착/통지/자기 삭제의 소유권만 연결한다.
//  - EvilRayVisualProcess: 부모에 부착되고 Kernel 이 소유하는 EvilRay 프로세스이다. runFrame 은 원문 전체 delta 프레임(중단 뒤 활성 입자가 없으면 자기 삭제), onFormDestroy 는 삭제 영역 dirty,
//    notify(event) 는 수명 몸체의 Stop(event = mode, 나머지 두 인자는 원문처럼 무시), draw 는 세 색 타원 출력, destroy(Kernel 제거·생성 실패·부모 0 정리 때)는 입자 → 공통 등록/base → 객체 반납과 조회 제거이다.
//    fields 는 `createEvilRayFields()` 이다. 시스템이 만들며 외부에서 직접 생성하지 않는다.
//  - new EvilRayVisualSystem(rendering, lifetime, frame, colors, hooks): rendering 은 같은 월드의 `RenderProcessSystem`, lifetime 은 E190 수명 상태({ palette, processType, source, sharedProcessType?, sharedSource? }),
//    frame 은 현재 `{ now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY }`(배율은 rendering.state 와 같아야 함), colors = { count, readable } 공유 3색 메모리(출력 때 현재 readable 을 읽음),
//    hooks = { lifetime: { reserve, createPalette?, buildPalette?, assertion? }, frame: { shapeWidth, soundId, sound, invalidate, clip, circle, assertion }(도달 시 요구), release(EvilRayRelease)(필수) } 이며 kill 은 그 프로세스의 Host 삭제로 연결한다.
//  - add(parent): 원문 공통 등록 → 부착 → 파생 초기화 순서이며 parent 0 은 등록만 하고 Host/Kernel 에 붙이지 않는다(그 객체는 시스템이 보관). 중첩 생성은 거부하고,
//    실패하면 부착된 객체는 Host 삭제(flags 0x10), 부착 전 객체는 직접 소멸한다.
//  - size: 현재 소유한 EvilRay 수. dispose(): 부모 0 객체는 직접 소멸, 나머지는 Host 삭제(flags 0x10)로 정리한다(C++ 소멸자).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E198: EvilRay 표시 시스템 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawEvilRayFrame } from './raw-evil-ray-frame.js';
import { RawEvilRayLifetime, createEvilRayFields } from './raw-evil-ray-lifetime.js';
import { renderProcessRemovalBounds } from './render-process-system.js';

// 월드 종료/생성 실패의 실제 client Form 반납 권한 flags 이다. (2026-11-09 WB04-E198)
const LOCAL_KILL = 0x10;

// 실제 client Form 에 부착되는 원본 EvilRay 프로세스이다. bridge 는 시스템이 만든 내부 연결이다. (2026-11-09 WB04-E198)
export class EvilRayVisualProcess extends SquidProcess {
    #bridge;

    // 원문 파생 초기화는 공통 부착 뒤 수행한다. (2026-11-09 WB04-E198)
    constructor(bridge) {
        super();
        if (!bridge || typeof bridge.destroy !== 'function') throw new TypeError('EvilRay 프로세스는 시스템이 만든다');
        this.#bridge = bridge;
        // 원본 필드를 복사 프레임 없이 같은 저장소에서 갱신한다.
        this.fields = createEvilRayFields();
        // 공통 부착 중에는 파생 필드가 아직 완성되지 않아 실행/출력/통지를 보류한다.
        this.ready = false;
    }

    // 실제 원문 전체 delta 프레임이다. (2026-11-09 WB04-E198)
    runFrame() { this.#bridge.run(this); }

    // 같은 삭제 영역을 실제 dirty 에 전달한다. (2026-11-09 WB04-E198)
    onFormDestroy() { this.#bridge.removing(this); }

    // ProcessForm 의 실제 notify1c 이다. event 를 Stop mode 로 넘기고 나머지 두 인자는 원문처럼 무시한다. (2026-11-09 WB04-E198)
    notify(event) { this.#bridge.notify(this, event >>> 0); }

    // 실제 타원 요청을 공통 kind3 에 제출한다. (2026-11-09 WB04-E198)
    draw() { this.#bridge.draw(this); }

    // Kernel 소멸도 같은 실제 반납 순서를 쓴다(C++ 소멸자). (2026-11-09 WB04-E198)
    destroy() { this.#bridge.destroy(this); }
}

// 같은 공통 목록·Host 와 EvilRay 생성/프레임/소멸을 묶는 시스템이다. 표시 시스템/전역/색 뷰/장치는 이 객체보다 오래 살아야 한다. (2026-11-09 WB04-E198)
export class EvilRayVisualSystem {
    #rendering;
    #frame;
    #colors;
    #hooks;
    #lifetime;
    // 토큰 → 프로세스(소유권은 pending·Kernel·부모 0 보관 목록 중 하나)이다.
    #processes = new Map();
    // 공통 부착 전 한 개의 유일한 소유자이다.
    #pending = null;
    // 원문 parent 0 은 Kernel 부착 없이 표시 등록을 유지하므로 시스템이 수명을 보관한다.
    #unattached = [];
    // 생성 중 중첩 add 를 거부한다.
    #creating = false;
    // 프로세스가 시스템 내부 동작을 부르는 연결이다(C++ friend).
    #bridge;

    // 같은 PATCH 풀/Host 와 단일 배율을 검증한다. 누락 장치는 실제 선택 위치까지 보류하되 소멸 관찰자는 미리 요구한다. (2026-11-09 WB04-E198)
    constructor(rendering, lifetime, frame, colors, hooks) {
        this.#rendering = rendering;
        this.#frame = frame;
        this.#colors = colors;
        this.#hooks = hooks;
        this.#lifetime = new RawEvilRayLifetime(rendering.pool, rendering.registry, lifetime, hooks?.lifetime ?? {});
        if (typeof hooks.release !== 'function') throw new TypeError('EvilRay 반납 관찰 장치 미연결');
        this.#bridge = Object.freeze({
            run: (process) => this.#run(process),
            draw: (process) => this.#draw(process),
            notify: (process, mode) => this.#notify(process, mode),
            removing: (process) => this.#removing(process),
            destroy: (process) => this.#destroy(process),
        });
        this.#checkScale();
    }

    // 같은 원본 전역을 두 값으로 복제하지 않는다. (2026-11-09 WB04-E198)
    #checkScale() {
        const state = this.#rendering.state;
        if (this.#frame.pixelScaleX !== state.scaleX || this.#frame.pixelScaleY !== state.scaleY) throw new Error('EvilRay 공통 표시 배율 불일치');
    }

    // 원문 parent 0 등록도 다른 성공 효과로 바꾸지 않는다. 원본: 004530b0 생성 경로. (2026-11-09 WB04-E198)
    add(parent) {
        if (this.#creating) throw new Error('EvilRay 중첩 생성 오류');
        const rendering = this.#rendering;
        this.#creating = true;
        let token = 0;
        try {
            const process = new EvilRayVisualProcess(this.#bridge);
            this.#pending = process;
            token = rendering.track(process.fields.registration, (fields, type, target, flags) => {
                if (this.#pending !== process || fields !== process.fields.registration) throw new Error('EvilRay 부착 객체 불일치');
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
                    throw new Error('EvilRay 실제 Host 부착 실패');
                }
                fields.form = attached.form;
                fields.parent = attached.parent;
            }, () => process.draw());
            this.#processes.set(token, process);
            this.#lifetime.initialize(process.fields, token, parent);
            if (!this.#processes.has(token)) throw new Error('EvilRay 생성 중 객체 삭제');
            process.ready = true;
            rendering.ready(token);
            // 부모 0 이면 부착 호출이 없어 대기 객체가 남으므로 시스템이 보관한다.
            if (this.#pending) {
                this.#unattached.push(this.#pending);
                this.#pending = null;
            }
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

    // 이번 실제 객체의 Host 자기 삭제를 고른 프레임 경계이다(나머지 장치는 도달 시 요구). (2026-11-09 WB04-E198)
    #frameHooks(process) {
        return { ...(this.#hooks.frame ?? {}), kill: (flags) => this.#rendering.host.kill(process, flags) };
    }

    // 현재 공유 3색 메모리 뷰이다(출력 때 현재 readable 을 읽는다). (2026-11-09 WB04-E198)
    #body(process) {
        return new RawEvilRayFrame(this.#rendering.pool, this.#frame, () => this.#colors.readable, this.#frameHooks(process));
    }

    // 삭제 뒤 process 를 다시 읽지 않는다. (2026-11-09 WB04-E198)
    #run(process) {
        if (!process.ready) return;
        this.#checkScale();
        this.#body(process).step(process.fields);
    }

    // 같은 현재 색/시계로 출력한다. (2026-11-09 WB04-E198)
    #draw(process) {
        if (!process.ready) return;
        this.#checkScale();
        this.#body(process).draw(process.fields);
    }

    // 공통 부착 중에는 아직 파생 상태가 아니므로 Stop 을 부르지 않는다. (2026-11-09 WB04-E198)
    #notify(process, mode) {
        if (process.ready) this.#lifetime.stop(process.fields, mode);
    }

    // 공통 삭제 통지는 파생 초기화 중이면 실행하지 않고, dirty 장치가 없으면 도달 시점에 거부한다. (2026-11-09 WB04-E198)
    #removing(process) {
        if (!process.ready) return;
        const bounds = renderProcessRemovalBounds(process.fields.registration, this.#frame.cameraX, this.#frame.cameraY);
        const invalidate = this.#hooks.frame?.invalidate;
        if (typeof invalidate !== 'function') throw new Error('EvilRay 실제 삭제 dirty 장치 미연결');
        invalidate({ left: bounds[0], top: bounds[1], right: bounds[2], bottom: bounds[3] });
    }

    // 실제 반납 뒤 토큰 조회를 없앤다. (2026-11-09 WB04-E198)
    #destroy(process) {
        const token = process.fields.registration.token;
        if (process.fields.registration.slot >= 0) this.#lifetime.destroy(process.fields, 1, this.#hooks.release);
        this.#processes.delete(token);
        this.#rendering.forget(token);
    }

    // 공통 전체 수와 다른 현재 EvilRay 소유 수이다. (2026-11-09 WB04-E198)
    get size() { return this.#processes.size; }

    // 조회 항목은 각 실제 소멸이 지우므로 현재 첫 항목을 다시 읽는다. 부모 0 의 등록도 입자/목록부터 반납한다(C++ 소멸자). (2026-11-09 WB04-E198)
    dispose() {
        while (this.#processes.size) {
            if (this.#unattached.length) this.#unattached.pop().destroy();
            else this.#rendering.host.kill(this.#processes.values().next().value, LOCAL_KILL);
        }
    }
}
