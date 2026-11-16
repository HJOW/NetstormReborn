// 생성자1651/ConnectFlare1884..1909의 독립 typed 플레어를 같은 World에 구성한다(225553df, WB04-E600).
// 사용: connectFactories 뒤 bind. 현재 SHP/프레임/난수·실제 Host를 빌리며 Runtime 공개는 뒤 생성 구간이다.
import { RawFlare } from './raw-flare.js';
import { RegularProcess } from './squid-process.js';

// 같은 현재 raw DWORD를 비정렬 little endian으로 읽는다.
function read(raw, offset) { return new DataView(raw.buffer, raw.byteOffset + offset, 4).getUint32(0, true); }

// 현재 플레어 상태와 원문의 한 번 Regular 예약을 잇는 생성 구간이다.
export class RawCampaignSessionFlare {
    // 기반 owner와 반복/부분 실패·정상 복귀/반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 실제 RawFlare가 값으로 빌리는 효과 표와 호출 때 현재 값을 읽는 상태다.
    hooks = null; state = null;

    // 생성은 플레어/프로세스를 만들거나 난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 기존 Factory 뒤 같은 SHP 헤더·현재 공간/표시·소리·Regular 수명에 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.factoryModules?.ready || this.#started) throw new Error('Session Flare의 초기화 전/반복 결선');
        this.#started = true;
        this.hooks = {
            processCount: () => { this.#live(); return c.kernel.size | 0; },
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); },
            sourceHotspot: (parent) => {
                this.#live(); const raw = c.pool.slot(parent); const frame = read(raw, 36) | 0; const shape = c.shapes[raw[10]];
                if (!shape || frame < 0 || frame >= shape.frames.length) throw new RangeError('플레어 원본 기준점 헤더 범위');
                const header = shape.frames[frame]; return [header.hotspotX, header.hotspotY];
            },
            owner: (sid, owner) => { this.#live(); c.owner.set(sid, owner); },
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
            reserveRegular: () => { this.#live(); if (o.pendingFlareRegular) throw new Error('플레어 Regular 예약 중첩'); o.pendingFlareRegular = new RegularProcess(0, 0); return true; },
            addRegular: (sid, event, payload) => {
                this.#live(); if (!o.pendingFlareRegular) throw new Error('플레어 Regular 실제 예약 누락'); const process = o.pendingFlareRegular; o.pendingFlareRegular = null; process.initializeDetached(event, payload);
                const attached = c.processes.attachRegular(process, sid); if (!attached) throw new Error('플레어 Regular 실제 부착 실패'); attached.setNextTime(c.processState.now);
            },
            frame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); },
            update: (sid) => { this.#live(); c.display.update(c.pool.physical(sid)); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            sound: (x, y, name, flags) => { this.#live(); if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('플레어 현재 위치 음향 미연결'); o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), flags, 0); },
        };
        this.state = { get suppressedTime() { return o.state.impactState.currentSuppressedTime; }, get loadingDepth() { return o.objectState.loadingDepth; }, get type() { return o.flareType; } };
        o.flare = new RawFlare(c.pool, c.types, c.frames, o.random, this.state, this.hooks); this.#ready = true;
    }

    // 반납 뒤 자기 closure가 현재 World에 다시 들어가지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Flare'); }
    // 부모가 World 프로세스를 먼저 정리하고, 미부착 실제 Regular 예약은 이 구간에서 반납한다.
    dispose() { this.#disposed = true; this.#owner.pendingFlareRegular = null; }
    // 생성자1651의 정상 복귀이며 실제 Runtime/Session 게시는 아니다.
    get ready() { return this.#ready; }
}
