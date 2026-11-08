// 냉각 연기(process 21)의 실제 몸체(`raw-smoke-effect.js`)를 같은 Host/Kernel 과 공통 표시 목록(`render-process-system.js`)에 연결하는 모듈이다(cpppj `o/SmokeVisualSystem.cpp/.h`, 10.78 전용).
//  - SmokeVisualProcess: 실제 client Form 에 부착되고 Kernel 이 소유하는 연기 프로세스이다. runFrame 은 같은 Kernel 시각의 실제 프레임, onFormDestroy 는 삭제 전 현재 영역 dirty,
//    draw 는 공통 표시 출력, destroy(Kernel 제거·부착 실패·부모 0 정리 때 호출)는 입자 → 공통 등록/base → 객체 반납과 조회 토큰 제거이다. fields 는 단일 의미 필드(`createSmokeFields()`)이다.
//    시스템이 만들며 외부에서 직접 생성하지 않는다.
//  - new SmokeVisualSystem(rendering, random, lifetime, frame, palette, hooks): rendering 은 같은 월드의 `RenderProcessSystem`, random 은 게임 난수, lifetime = { palette } 공유 색 토큰,
//    frame 은 현재 `{ now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY }`(배율은 rendering.state 의 scaleX/scaleY 와 같아야 함), palette = { count, readable } 색 표이다.
//    hooks = { lifetime: { reserve, createPalette, buildPalette }, frame: { invalidate, line }, release(SmokeRelease) } 이며 kill 은 프로세스별 실제 Host 삭제로 연결된다.
//  - addCooling(parent, [hotspotX, hotspotY]): 004a8770 의 현재 SHP 포구로 실제 21·200입자·10회/간격 3 연기를 만든다(부모 0 거부). add(request): 004a8660 의 일반 기본 연기(8회/간격 5)이다.
//    생성 중 중첩 생성은 거부하고, 생성 실패는 이미 부착했으면 Host 삭제(flags 0x10), 아니면 직접 소멸로 정리한다. 원문 공통 생성자는 부모 0 이면 Host/Kernel 에 붙지 않으므로 그 객체는 시스템이 보관한다.
//  - size: 현재 소유한 process 21 수이다. dispose(): Host 가 살아 있을 때 남은 연기를 정리한다(부모 0 객체는 직접 소멸, 나머지는 Host 삭제 flags 0x10).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E192: 연기 표시 시스템 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawSmokeFrame, RawSmokeLifetime, createSmokeFields, createSmokeRequest } from './raw-smoke-effect.js';
import { renderProcessRemovalBounds } from './render-process-system.js';

// client Form 을 월드 종료/생성 실패 때 실제 반납하는 원본 권한 flags 이다. (2026-11-09 WB04-E192)
const LOCAL_KILL = 0x10;

// 실제 client Form 에 부착되는 원본 냉각 연기 프로세스이다. bridge 는 시스템이 만든 내부 연결이다. (2026-11-09 WB04-E192)
export class SmokeVisualProcess extends SquidProcess {
    #bridge;

    // 실제 생성/부착/초기화는 시스템의 생성 경로가 순서대로 수행한다. (2026-11-09 WB04-E192)
    constructor(bridge) {
        super();
        if (!bridge || typeof bridge.destroy !== 'function') throw new TypeError('연기 프로세스는 시스템이 만든다');
        this.#bridge = bridge;
        // 복사 없이 같은 저장소를 계속 갱신하는 단일 의미 필드이다.
        this.fields = createSmokeFields();
        // 생성 중 프레임/출력/삭제 통지가 미초기화 필드를 읽지 않게 하는 준비 완료 표시이다.
        this.ready = false;
    }

    // 같은 Kernel 현재 시각으로 실제 프레임을 실행한다. (2026-11-09 WB04-E192)
    runFrame() { this.#bridge.run(this); }

    // Form 소멸 직전 현재 표시 영역을 실제 dirty 에 통지한다. (2026-11-09 WB04-E192)
    onFormDestroy() { this.#bridge.removing(this); }

    // 공통 kind3 출력의 실제 파생 출력이다. (2026-11-09 WB04-E192)
    draw() { this.#bridge.draw(this); }

    // Kernel 제거(또는 부착 전 소유 해제)에서 입자/등록/조회 토큰을 같은 순서로 반납한다(C++ 소멸자). (2026-11-09 WB04-E192)
    destroy() { this.#bridge.destroy(this); }
}

// 같은 원본 registry/Host/Kernel·난수와 연기 생성/프레임/소멸을 묶는 시스템이다. (2026-11-09 WB04-E192)
export class SmokeVisualSystem {
    #rendering;
    #random;
    #frame;
    #palette;
    #hooks;
    #lifetime;
    // 토큰 → 프로세스(소유권은 pending·Kernel·부모 0 보관 목록 중 하나)이다.
    #processes = new Map();
    // Host 부착 전의 유일한 소유자이다.
    #pending = null;
    // 원문 부모 0 의 표시 등록은 Host/Kernel 에 붙지 않으므로 시스템이 수명을 보관한다.
    #unattached = [];
    // 생성 중 중첩 생성을 거부한다.
    #creating = false;
    // 프로세스가 시스템 내부 동작을 부르는 연결이다(C++ friend).
    #bridge;

    // 모든 참조와 출력/공유 색 메모리는 이 시스템보다 오래 살아야 한다. 출력/반납 경계 누락과 배율 불일치는 거부한다. (2026-11-09 WB04-E192)
    constructor(rendering, random, lifetime, frame, palette, hooks) {
        if (typeof hooks?.frame?.invalidate !== 'function' || typeof hooks.frame.line !== 'function' || typeof hooks.release !== 'function') throw new TypeError('냉각 연기 실제 장치/반납 미연결');
        this.#rendering = rendering;
        this.#random = random;
        this.#frame = frame;
        this.#palette = palette;
        this.#hooks = hooks;
        this.#lifetime = new RawSmokeLifetime(rendering.pool, rendering.registry, lifetime, hooks.lifetime);
        this.#bridge = Object.freeze({
            run: (process) => this.#run(process),
            draw: (process) => this.#draw(process),
            removing: (process) => this.#removing(process),
            destroy: (process) => this.#destroy(process),
        });
        this.#checkScale();
    }

    // 공통 수집과 입자가 같은 원본 픽셀 배율을 쓰는지 확인한다. (2026-11-09 WB04-E192)
    #checkScale() {
        const state = this.#rendering.state;
        if (this.#frame.pixelScaleX !== state.scaleX || this.#frame.pixelScaleY !== state.scaleY) throw new Error('냉각 연기 공통 표시 배율 불일치');
    }

    // 일반 생성자는 원문 기본 8회/간격 5 를 그대로 쓴다. 원본: 004a8660. (2026-11-09 WB04-E192)
    add(request) {
        return this.#create(request, false);
    }

    // 냉각 wrapper 는 같은 생성 뒤 남은 횟수 10/간격 3 으로 바꾼다. 원본: 004a8770. (2026-11-09 WB04-E192)
    addCooling(parent, hotspot) {
        const request = createSmokeRequest();
        request.parent = parent;
        request.hotspotX = hotspot[0] | 0;
        request.hotspotY = hotspot[1] | 0;
        return this.#create(request, true);
    }

    // 실제 공통 등록/부착/200입자/공유 표 생성과 실패 정리를 두 wrapper 가 공유한다. (2026-11-09 WB04-E192)
    #create(request, cooling) {
        if ((cooling && !request.parent) || this.#creating) throw new Error('연기 실제 부모/중첩 생성 오류');
        const rendering = this.#rendering;
        let token = 0;
        this.#creating = true;
        try {
            this.#pending = new SmokeVisualProcess(this.#bridge);
            const process = this.#pending;
            token = rendering.track(process.fields.registration, (fields, type, target, flags) => {
                if (!this.#pending || this.#pending.fields.registration !== fields) throw new Error('냉각 연기 부착 객체 불일치');
                // 소유권을 Host 로 넘긴다. 부착이 실패하면 넘겨받은 쪽이 없으므로 C++ unique_ptr 해제처럼 바로 소멸시킨다.
                const owned = this.#pending;
                this.#pending = null;
                const attached = rendering.host.attach(owned, type, target, flags);
                if (!attached) {
                    owned.destroy();
                    throw new Error('냉각 연기 실제 Host 부착 실패');
                }
                fields.form = attached.form;
                fields.parent = attached.parent;
            }, () => process.draw());
            this.#processes.set(token, process);
            if (cooling) this.#lifetime.initializeCooling(process.fields, token, request.parent, [request.hotspotX, request.hotspotY]);
            else this.#lifetime.initialize(process.fields, token, request);
            if (!this.#processes.has(token)) throw new Error('연기 생성 중 삭제');
            process.ready = true;
            rendering.ready(token);
            // 원문 공통 생성자는 부모 0 일 때만 부착 호출을 생략한다. 다음 생성이 이 객체를 덮어쓰지 않게 보관한다.
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

    // 실제 프레임이 종료 요청을 낼 때 현재 객체를 같은 Host 로 삭제하는 출력 경계이다. (2026-11-09 WB04-E192)
    #frameHooks(process) {
        const frame = this.#hooks.frame;
        return { invalidate: frame.invalidate, line: frame.line, kill: (flags) => this.#rendering.host.kill(process, flags) };
    }

    // 준비 완료된 실제 프레임만 실행한다. 자기 삭제 뒤 process 필드를 다시 읽지 않는다. (2026-11-09 WB04-E192)
    #run(process) {
        if (!process.ready) return;
        this.#checkScale();
        new RawSmokeFrame(this.#rendering.pool, this.#random, this.#frame, this.#palette, this.#frameHooks(process)).step(process.fields);
    }

    // 현재 카메라/색 메모리의 실제 출력 요청이다. (2026-11-09 WB04-E192)
    #draw(process) {
        if (!process.ready) return;
        this.#checkScale();
        new RawSmokeFrame(this.#rendering.pool, this.#random, this.#frame, this.#palette, this.#frameHooks(process)).draw(process.fields);
    }

    // 파생 생성 중에는 공통 삭제 통지가 아직 완성되지 않은 필드를 읽지 않는다. (2026-11-09 WB04-E192)
    #removing(process) {
        if (!process.ready) return;
        const rect = renderProcessRemovalBounds(process.fields.registration, this.#frame.cameraX, this.#frame.cameraY);
        this.#hooks.frame.invalidate({ left: rect[0], top: rect[1], right: rect[2], bottom: rect[3] });
    }

    // 실제 입자 반납 → 공통 목록/base → 조회 토큰 제거 순서이다. (2026-11-09 WB04-E192)
    #destroy(process) {
        const token = process.fields.registration.token;
        if (process.fields.registration.slot >= 0) this.#lifetime.destroy(process.fields, 1, this.#hooks.release);
        this.#processes.delete(token);
        this.#rendering.forget(token);
    }

    // 현재 소유한 연기 프로세스 수이다. (2026-11-09 WB04-E192)
    get size() { return this.#processes.size; }

    // 삭제가 조회 항목을 지우므로 매번 현재 첫 항목을 다시 읽는다. 부모 0 객체는 Host 가 소유하지 않으므로 직접 소멸한다(C++ 소멸자). (2026-11-09 WB04-E192)
    dispose() {
        // 상위는 같은 Host 가 살아 있을 때 이 시스템을 정리한다.
        while (this.#processes.size) {
            if (this.#unattached.length) this.#unattached.pop().destroy();
            else this.#rendering.host.kill(this.#processes.values().next().value, LOCAL_KILL);
        }
    }
}
