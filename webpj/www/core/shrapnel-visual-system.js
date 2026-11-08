// 픽셀 파편(process 16)의 실제 몸체(E188 `raw-shrapnel-frame.js`·`raw-shrapnel-lifetime.js`)를 같은 Host/Kernel·더미 상호 감시와 공통 표시 목록(`render-process-system.js`)에 연결하는 모듈이다
// (cpppj `o/ShrapnelVisualSystem.cpp/.h`, 10.78 전용). 구조는 E193 분해 표시 시스템과 같고 게임 난수와 별도 CRT 난수의 두 스트림·세 가상 생성기(kind)를 쓴다.
//  - ShrapnelVisualProcess: 더미에 부착되고 Kernel 이 소유하는 파편 프로세스이다. runFrame 은 같은 Kernel 시각의 중력 프레임(진입 양수 입자가 없으면 자기 삭제), onFormDestroy 는 삭제 전 현재 영역 dirty,
//    draw 는 캡처 색 픽셀 출력, destroy(Kernel 제거·생성 실패 때 호출)는 캡처 그림 → 입자 → 공통 등록/base → 객체 반납·조회 제거와, 상호 감시 쌍을 만들기 전에 실패한 더미의 정리이다.
//    fields 는 `createShrapnelFrameFields()`, dummy 는 생성 직후 더미 SID 를 담는 `{ value }` 상자, paired/ready 는 상호 감시 완료·파생 초기화 완료 표시이다. 시스템이 만들며 외부에서 직접 생성하지 않는다.
//  - new ShrapnelVisualSystem(rendering, game, crt, frame, hooks): rendering 은 같은 월드의 `RenderProcessSystem`, game 은 게임 난수, crt 는 원본 TLS 와 같은 별도 CRT 스트림(두 스트림을 합치지 않음),
//    frame 은 현재 `{ now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY }`(배율은 rendering.state 와 같아야 함),
//    hooks = { lifetime: { reserve, create, pop, depend, capture }, frame: { reserve, release, invalidate, pixel }, release(ShrapnelRelease), destroyUnpaired(dummy) } 이다.
//    부착(attach)·준비(prepare)는 시스템이 실제 Host/`RawShrapnelFrame.prepare` 로, 소진(removeForm)은 그 프로세스의 Host 삭제(flags 0)로 연결한다.
//  - add(source, kind): 실제 104바이트 확보가 실패하면 null(등록/더미 생성 없음), 성공하면 더미에 붙은 같은 Kernel 의 process 16 이다. kind 는 `ShrapnelKind` 번호이다.
//    source 0·중첩 생성은 거부하고, 생성 실패는 이미 부착했으면 Host 삭제(flags 0x10), 아니면 직접 소멸로 정리한다.
//  - size: 현재 파편 수. dispose(): Host 가 살아 있을 때 남은 파편을 Host 삭제(flags 0x10)로 정리한다(C++ 소멸자).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E197: 파편 표시 시스템 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawShrapnelFrame, createShrapnelFrameFields } from './raw-shrapnel-frame.js';
import { RawShrapnelLifetime } from './raw-shrapnel-lifetime.js';
import { renderProcessRemovalBounds } from './render-process-system.js';

// client Form 의 실패/종료 정리 권한 flags 와 실제 객체 크기·슬롯 상태 위치/free·dead 비트이다. (2026-11-09 WB04-E197)
const CLIENT_KILL = 16;
const OBJECT_BYTES = 104;
const STATE = 11;
const FREE_OR_DEAD = 3;

// 더미에 부착되는 원본 파편 프로세스이다. bridge 는 시스템이 만든 내부 연결이다. (2026-11-09 WB04-E197)
export class ShrapnelVisualProcess extends SquidProcess {
    #bridge;

    // 파생 준비는 시스템 add 의 마지막에만 완료한다. (2026-11-09 WB04-E197)
    constructor(bridge) {
        super();
        if (!bridge || typeof bridge.destroy !== 'function') throw new TypeError('파편 프로세스는 시스템이 만든다');
        this.#bridge = bridge;
        // 표시 등록/캡처/입자/시각의 단일 저장소이다.
        this.fields = createShrapnelFrameFields();
        // 부착 전 예외에도 소유한 더미를 찾는 출력 상자이다.
        this.dummy = { value: 0 };
        // 실제 상호 감시 쌍/파생 초기화 완료 여부이다.
        this.paired = false;
        this.ready = false;
    }

    // 별도 프레임 목록/시계 없이 같은 Kernel 시각으로 실행한다. (2026-11-09 WB04-E197)
    runFrame() { this.#bridge.run(this); }

    // 현재 영역을 메모리 반납보다 먼저 통지한다. (2026-11-09 WB04-E197)
    onFormDestroy() { this.#bridge.removing(this); }

    // 공통 kind3 이 현재 픽셀을 출력한다. (2026-11-09 WB04-E197)
    draw() { this.#bridge.draw(this); }

    // 원본 메모리/등록 순서의 실제 소멸 몸체이다(C++ 소멸자). (2026-11-09 WB04-E197)
    destroy() { this.#bridge.destroy(this); }
}

// 기존 모든 효과와 같은 표시 registry/Host/게임 난수/CRT 난수를 쓰는 파편 시스템이다. (2026-11-09 WB04-E197)
export class ShrapnelVisualSystem {
    #rendering;
    #game;
    #crt;
    #frame;
    #hooks;
    // 토큰 → 프로세스(실제 소유권은 pending 또는 Kernel)이다.
    #processes = new Map();
    // 실제 부착 전 임시 소유자이다.
    #pending = null;
    // 중첩 add 가 같은 pending 을 바꾸지 못하게 한다.
    #creating = false;
    // 프로세스가 시스템 내부 동작을 부르는 연결이다(C++ friend).
    #bridge;

    // 세계 경계와 실제 프레임 출력의 누락을 첫 등록 전에 확인한다. (2026-11-09 WB04-E197)
    constructor(rendering, game, crt, frame, hooks) {
        const lifetime = hooks?.lifetime;
        if (!lifetime || ['reserve', 'create', 'pop', 'depend', 'capture'].some((name) => typeof lifetime[name] !== 'function')
            || typeof hooks.release !== 'function' || typeof hooks.destroyUnpaired !== 'function') throw new TypeError('파편 실제 세계/반납 경계 누락');
        this.#rendering = rendering;
        this.#game = game;
        this.#crt = crt;
        this.#frame = frame;
        this.#hooks = hooks;
        this.#bridge = Object.freeze({
            run: (process) => this.#run(process),
            draw: (process) => this.#draw(process),
            removing: (process) => this.#removing(process),
            destroy: (process) => this.#destroy(process),
        });
        this.#checkScale();
        // 프레임 출력 경계(reserve/release/invalidate/pixel)와 두 난수도 첫 등록 전에 실제 계산 몸체로 확인한다.
        new RawShrapnelFrame(rendering.pool, game, crt, frame, this.#frameHooks(null));
    }

    // 서로 다른 좌표 전역으로 조용히 출력하지 않는다. (2026-11-09 WB04-E197)
    #checkScale() {
        const state = this.#rendering.state;
        if (this.#frame.pixelScaleX !== state.scaleX || this.#frame.pixelScaleY !== state.scaleY) throw new Error('파편 공통 표시 배율 불일치');
    }

    // 현재 객체의 실제 프레임 몸체이다(두 난수·현재 시각/카메라 공유). (2026-11-09 WB04-E197)
    #body(process) {
        return new RawShrapnelFrame(this.#rendering.pool, this.#game, this.#crt, this.#frame, this.#frameHooks(process));
    }

    // 같은 pending 객체를 실제 Host 에 넘기고 원본 준비를 연결한 수명 경계이다. (2026-11-09 WB04-E197)
    #lifetimeHooks(process) {
        const base = this.#hooks.lifetime;
        const rendering = this.#rendering;
        return {
            reserve: base.reserve,
            create: base.create,
            pop: base.pop,
            capture: base.capture,
            attach: (fields, type, parent, flags) => {
                if (this.#pending !== process || fields !== process.fields.registration) throw new Error('파편 실제 부착 객체 불일치');
                // 소유권을 Host 로 넘긴다. 부착이 null 이나 예외로 실패하면 받는 쪽이 없으므로 C++ unique_ptr 매개변수 해제처럼 바로 소멸시킨다.
                this.#pending = null;
                let attached;
                try {
                    attached = rendering.host.attach(process, type, parent, flags);
                } catch (error) {
                    process.destroy();
                    throw error;
                }
                if (!attached) {
                    process.destroy();
                    throw new Error('파편 실제 dummy 부착 실패');
                }
                fields.form = attached.form;
                fields.parent = attached.parent;
            },
            depend: (form, dummy) => {
                base.depend(form, dummy);
                process.paired = true;
            },
            prepare: (fields, kind) => { this.#body(process).prepare(fields, kind); },
        };
    }

    // 실제 프레임의 소진은 같은 Form/Kernel/상호 감시 쌍 삭제로 이어진다. (2026-11-09 WB04-E197)
    #frameHooks(process) {
        const frame = this.#hooks.frame ?? {};
        return {
            reserve: frame.reserve,
            release: frame.release,
            invalidate: frame.invalidate,
            pixel: frame.pixel,
            removeForm: () => {
                if (!process) throw new Error('파편 소진 객체 미지정');
                this.#rendering.host.kill(process, 0);
            },
        };
    }

    // 공통 목록에 먼저 보인 뒤 실제 더미/부착/캡처/준비를 수행하고 ready 를 마지막에 세운다. 원본: 004604a0. (2026-11-09 WB04-E197)
    add(source, kind) {
        if (!source || this.#creating) throw new Error('파편 source/중첩 생성 오류');
        this.#checkScale();
        if (!this.#hooks.lifetime.reserve(OBJECT_BYTES)) return null;
        const rendering = this.#rendering;
        this.#creating = true;
        let token = 0;
        try {
            const process = new ShrapnelVisualProcess(this.#bridge);
            this.#pending = process;
            const lifetimeHooks = this.#lifetimeHooks(process);
            token = rendering.track(process.fields.registration, lifetimeHooks.attach, () => process.draw());
            this.#processes.set(token, process);
            new RawShrapnelLifetime(rendering.pool, rendering.registry, lifetimeHooks).initialize(process.fields, token, source, kind, process.dummy);
            if (!this.#processes.has(token)) throw new Error('파편 생성 중 현재 객체 삭제');
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

    // 자기 삭제 뒤 같은 객체를 다시 읽지 않는다. (2026-11-09 WB04-E197)
    #run(process) {
        if (!process.ready) return;
        this.#checkScale();
        this.#body(process).step(process.fields);
    }

    // 현재 같은 캡처 그림/실제 표면에 출력한다. (2026-11-09 WB04-E197)
    #draw(process) {
        if (!process.ready) return;
        this.#checkScale();
        this.#body(process).draw(process.fields);
    }

    // 프레임 dirty 와 다른 공통 삭제 영역 계산을 유지한다. (2026-11-09 WB04-E197)
    #removing(process) {
        if (!process.ready) return;
        const rect = renderProcessRemovalBounds(process.fields.registration, this.#frame.cameraX, this.#frame.cameraY);
        this.#hooks.frame.invalidate({ left: rect[0], top: rect[1], right: rect[2], bottom: rect[3] });
    }

    // 상호 감시 쌍 완료 뒤의 더미 삭제는 실제 Form 삭제 사슬에 맡기고, 쌍을 만들기 전 실패한 더미만 직접 정리한다. (2026-11-09 WB04-E197)
    #destroy(process) {
        const rendering = this.#rendering;
        const token = process.fields.registration.token;
        if (process.fields.registration.slot >= 0) new RawShrapnelLifetime(rendering.pool, rendering.registry, this.#lifetimeHooks(process)).destroy(process.fields, 1, this.#hooks.release);
        this.#processes.delete(token);
        rendering.forget(token);
        const dummy = process.dummy.value;
        if (dummy && !process.paired && !(rendering.pool.slot(dummy)[STATE] & FREE_OR_DEAD)) this.#hooks.destroyUnpaired(dummy);
    }

    // 다른 종류를 합산하지 않은 실제 파편 프로세스 수이다. (2026-11-09 WB04-E197)
    get size() { return this.#processes.size; }

    // 삭제가 현재 조회를 지우므로 매번 첫 객체를 다시 찾는다. 같은 실제 Host 가 이 시스템보다 오래 살아야 한다(C++ 소멸자). (2026-11-09 WB04-E197)
    dispose() {
        while (this.#processes.size) this.#rendering.host.kill(this.#processes.values().next().value, CLIENT_KILL);
    }
}
