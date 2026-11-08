// 공유 빔(process 19)의 실제 경로(E117 `raw-lightning-path.js`)·수명/프레임(E186 `raw-lightning-lifetime.js`·`raw-lightning-frame.js`)을 같은 Host/Kernel 과 공통 표시 목록(`render-process-system.js`)에 연결하는 모듈이다
// (cpppj `o/LightningVisualSystem.cpp/.h`, 10.78 전용). 계산/게임 수명은 실제 모듈을 쓰고 메모리/출력과 현재 raw 중심만 외부 경계로 둔다.
//  - LightningVisualProcess: 부모에 부착되고 Kernel 이 소유하는 빔 프로세스이다. runFrame 은 같은 Kernel 시각/게임 난수의 최초/후속 프레임(소진 시 자기 삭제), onFormDestroy 는 입자 반납 전 삭제 영역 dirty,
//    draw 는 이전 점/굵기 선 출력, destroy(Kernel 제거·생성 실패 때)는 경로 → 입자 → 공통 등록/base → 객체 반납과 조회 제거이다. fields 는 `createLightningFrameFields()` 이다. 시스템이 만든다.
//  - new LightningVisualSystem(rendering, random, lifetime, frame, hooks): rendering 은 같은 월드의 `RenderProcessSystem`, random 은 경로/프레임/게임이 공유하는 게임 난수, lifetime = { palette }(빔 공유 색 객체),
//    frame 은 현재 `{ now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY }`(배율은 rendering.state 와 같아야 하며 경로 생성 배율에도 반영),
//    hooks = { lifetime: { reserve, createPalette?, buildPalette? }, frame: { invalidate, line, sound }, center(sid)→[x, y](같은 월드의 004adea0 중심 조회), release(LightningRelease) } 이다. 소진 removeForm 은 그 프로세스의 Host 삭제(flags 0)로 연결한다.
//    시스템은 경로 상태 `{ scaleX, scaleY, vanderType 148, tallType 149, manOWarType 144 }` 와 실제 `RawLightningPath`·`RawLightningLifetime` 을 소유한다.
//  - add(parent, target, sound): 등록 → 실제 부착 → 120입자/공유 색/경로 초기화(sound 는 파일명 또는 null). addAtScreen(parent, from, to, life, sound): 정수 화면 양끝을 float 로 바꿔 직접 좌표 생성자를 실행한다.
//    부모 0·중첩 생성은 거부하고 실패하면 부착된 객체는 Host 삭제(flags 0x10), 부착 전 객체는 직접 소멸한다.
//  - size: 현재 소유한 빔 수. dispose(): Host 가 살아 있을 때 남은 빔을 Host 삭제(flags 0x10)로 정리한다(C++ 소멸자).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E196: 빔 표시 시스템 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawLightningPath } from './raw-lightning-path.js';
import { RawLightningLifetime } from './raw-lightning-lifetime.js';
import { RawLightningFrame, createLightningFrameFields } from './raw-lightning-frame.js';
import { renderProcessRemovalBounds } from './render-process-system.js';

// 시스템 정리/실패 정리에서 쓰는 실제 client Form 삭제 권한 flags 이다. (2026-11-09 WB04-E196)
const CLIENT_KILL = 0x10;

// 실제 Kernel 이 소유하는 빔 프로세스이다. bridge 는 시스템이 만든 내부 연결이다. (2026-11-09 WB04-E196)
export class LightningVisualProcess extends SquidProcess {
    #bridge;

    // 파생 준비 완료 표식은 시스템 add 가 마지막에 세운다. (2026-11-09 WB04-E196)
    constructor(bridge) {
        super();
        if (!bridge || typeof bridge.destroy !== 'function') throw new TypeError('빔 프로세스는 시스템이 만든다');
        this.#bridge = bridge;
        // 공통 등록·실제 경로/입자·시계의 단일 저장소이다.
        this.fields = createLightningFrameFields();
        // 파생 생성 완료 전 프레임/출력/삭제 통지 재진입을 막는다.
        this.ready = false;
    }

    // 별도 프레임 목록/시계를 만들지 않는다. (2026-11-09 WB04-E196)
    runFrame() { this.#bridge.run(this); }

    // 입자 반납 전에 현재 표시 영역을 통지한다. (2026-11-09 WB04-E196)
    onFormDestroy() { this.#bridge.removing(this); }

    // 공통 kind3 의 실제 출력 분배이다. (2026-11-09 WB04-E196)
    draw() { this.#bridge.draw(this); }

    // Kernel 소멸이 경로/입자/등록/객체를 원본 순서로 정리한다(C++ 소멸자). (2026-11-09 WB04-E196)
    destroy() { this.#bridge.destroy(this); }
}

// 같은 공통 목록·Host·Kernel 과 게임 난수/시간을 쓰는 빔 표시 관리자이다. (2026-11-09 WB04-E196)
export class LightningVisualSystem {
    #rendering;
    #random;
    #frame;
    #hooks;
    // 같은 배율과 원본 보정 타입 전역이다.
    #pathState;
    #path;
    #lifetime;
    // 토큰 → 프로세스(소유권은 pending 또는 Kernel)이다.
    #processes = new Map();
    // 실제 부착 전 임시 소유자이다.
    #pending = null;
    // 생성 콜백의 중첩 add 를 거부한다.
    #creating = false;
    // 프로세스가 시스템 내부 동작을 부르는 연결이다(C++ friend).
    #bridge;

    // 원본 생성/경로/프레임 참조를 같은 월드에 연결하고 출력 누락을 생성 전에 거부한다. (2026-11-09 WB04-E196)
    constructor(rendering, random, lifetime, frame, hooks) {
        this.#rendering = rendering;
        this.#random = random;
        this.#frame = frame;
        this.#hooks = hooks;
        this.#pathState = { scaleX: 16, scaleY: 11, vanderType: 148, tallType: 149, manOWarType: 144 };
        this.#path = new RawLightningPath(rendering.pool, random, this.#pathState, hooks?.center);
        this.#lifetime = new RawLightningLifetime(rendering.pool, rendering.registry, this.#path, lifetime, hooks.lifetime);
        if (typeof hooks.release !== 'function') throw new TypeError('빔 실제 반납 경계 누락');
        this.#bridge = Object.freeze({
            run: (process) => this.#run(process),
            draw: (process) => this.#draw(process),
            removing: (process) => this.#removing(process),
            destroy: (process) => this.#destroy(process),
        });
        this.#checkScale();
        // 프레임 출력 경계(invalidate/line/sound)도 실제 계산 몸체로 확인한다.
        new RawLightningFrame(rendering.pool, random, this.#path, frame, this.#frameHooks(null));
    }

    // 원본의 같은 배율을 다른 값으로 조용히 실행하지 않고 경로 생성 배율에 반영한다. (2026-11-09 WB04-E196)
    #checkScale() {
        const state = this.#rendering.state;
        if (this.#frame.pixelScaleX !== state.scaleX || this.#frame.pixelScaleY !== state.scaleY) throw new Error('빔 공통 표시 배율 불일치');
        this.#pathState.scaleX = this.#frame.pixelScaleX;
        this.#pathState.scaleY = this.#frame.pixelScaleY;
    }

    // 소진 순간의 Form 삭제는 실제 Host 권한 검사/종속 해제/Kernel 소멸로 이어진다. (2026-11-09 WB04-E196)
    #frameHooks(process) {
        const frame = this.#hooks.frame ?? {};
        return {
            invalidate: frame.invalidate,
            line: frame.line,
            sound: frame.sound,
            removeForm: () => {
                if (!process) throw new Error('빔 프레임 객체 미지정');
                this.#rendering.host.kill(process, 0);
            },
        };
    }

    // ready 전 재진입은 파생 필드를 읽지 않는다. 원본 공통 부착 순서를 바꾸지 않는다. 원본: 00472700/00472690 → 00471b20. (2026-11-09 WB04-E196)
    add(parent, target, sound = null) {
        return this.#addImpl(parent, (fields, token) => this.#lifetime.initialize(fields, token, parent, target, sound));
    }

    // float 변환은 객체 생성 뒤 공통 등록/Host 부착보다 먼저 한다. 원본: 00472770 → 00471e70. (2026-11-09 WB04-E196)
    addAtScreen(parent, from, to, life, sound = null) {
        let first = [0, 0];
        let last = [0, 0];
        return this.#addImpl(parent,
            (fields, token) => this.#lifetime.initializePoints(fields, token, parent, first, last, life, sound),
            () => {
                first = [Math.fround(from[0] | 0), Math.fround(from[1] | 0)];
                last = [Math.fround(to[0] | 0), Math.fround(to[1] | 0)];
            });
    }

    // 두 실제 생성자에 같은 Host/표시 목록 소유권과 실패 정리를 적용한다. (2026-11-09 WB04-E196)
    #addImpl(parent, initialize, prepare = null) {
        if (!parent || this.#creating) throw new Error('빔 Host 부모/중첩 생성 오류');
        this.#checkScale();
        const rendering = this.#rendering;
        let token = 0;
        this.#creating = true;
        try {
            const process = new LightningVisualProcess(this.#bridge);
            this.#pending = process;
            if (prepare) prepare();
            token = rendering.track(process.fields.registration, (fields, type, source, flags) => {
                if (this.#pending !== process || fields !== process.fields.registration) throw new Error('빔 부착 객체 불일치');
                // 소유권을 Host 로 넘긴다. 부착이 null 이나 예외로 실패하면 받는 쪽이 없으므로 C++ unique_ptr 매개변수 해제처럼 바로 소멸시킨다.
                this.#pending = null;
                let attached;
                try {
                    attached = rendering.host.attach(process, type, source, flags);
                } catch (error) {
                    process.destroy();
                    throw error;
                }
                if (!attached) {
                    process.destroy();
                    throw new Error('빔 실제 부모 부착 실패');
                }
                fields.form = attached.form;
                fields.parent = attached.parent;
            }, () => process.draw());
            this.#processes.set(token, process);
            initialize(process.fields, token);
            if (!this.#processes.has(token)) throw new Error('빔 생성 중 객체 삭제');
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

    // 프레임이 자기 Form 을 삭제할 수 있으므로 호출 뒤 process 를 다시 읽지 않는다. (2026-11-09 WB04-E196)
    #run(process) {
        if (!process.ready) return;
        this.#checkScale();
        new RawLightningFrame(this.#rendering.pool, this.#random, this.#path, this.#frame, this.#frameHooks(process)).step(process.fields);
    }

    // 별도 끝점 조회가 필요 없는 실제 이전 점 연결 출력이다. (2026-11-09 WB04-E196)
    #draw(process) {
        if (!process.ready) return;
        this.#checkScale();
        new RawLightningFrame(this.#rendering.pool, this.#random, this.#path, this.#frame, this.#frameHooks(process)).draw(process.fields);
    }

    // 같은 공통 표시 삭제 통지의 실제 정수화 순서를 유지한다. (2026-11-09 WB04-E196)
    #removing(process) {
        if (!process.ready) return;
        const rect = renderProcessRemovalBounds(process.fields.registration, this.#frame.cameraX, this.#frame.cameraY);
        this.#hooks.frame.invalidate({ left: rect[0], top: rect[1], right: rect[2], bottom: rect[3] });
    }

    // 생성 중 실패도 등록 여부에 따라 실제 소멸/조회 정리를 한다. (2026-11-09 WB04-E196)
    #destroy(process) {
        const token = process.fields.registration.token;
        if (process.fields.registration.slot >= 0) this.#lifetime.destroy(process.fields, 1, this.#hooks.release);
        this.#processes.delete(token);
        this.#rendering.forget(token);
    }

    // 다른 효과를 포함한 공통 목록 크기와 구별한 빔 수이다. (2026-11-09 WB04-E196)
    get size() { return this.#processes.size; }

    // 각 실제 삭제가 조회를 지우므로 매번 현재 첫 항목을 다시 읽는다(C++ 소멸자). 상위 월드는 Host 보다 먼저 이 시스템을 정리한다. (2026-11-09 WB04-E196)
    dispose() {
        while (this.#processes.size) this.#rendering.host.kill(this.#processes.values().next().value, CLIENT_KILL);
    }
}
