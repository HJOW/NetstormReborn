// 실제 Client 공통 트리에 원본 편집기·버퍼와 두 Slider를 조립한다.
// 사용: new ClientTextEntryControls(owner, devices).allocate() → construct(token, width, height, text, font, background, color).
import { ClientScrollControls } from './client-scroll-controls.js';
import { TextEntryGump, TextEntryGumpNode, ConsoleGumpNode } from './text-entry-gump.js';
import { TextEntryStorage } from './text-entry-storage.js';

// 같은 공통 Gump와 편집 버퍼·Slider의 수명을 소유한다.
export class ClientTextEntryControls {
    // 공유 소유자와 기존 실제 본체, 직접 할당한 주소들이다.
    #owner; #scroll; #storage; #entry; #owned = new Set(); #buffers = new Set(); #nextBuffer = 0x70000000; #deleteFlags = new Map();
    // 실제 전역·글꼴 측정·시간 및 선택된 출력 장치를 빌린다.
    constructor(owner, devices) {
        if (!owner || !devices?.entryState || typeof devices.now !== 'function' || typeof devices.measure !== 'function') throw new TypeError('Client TextEntry의 실제 소유자·상태·시계·글꼴 필요');
        this.#owner = owner; this.#scroll = new ClientScrollControls(owner, devices);
        this.#storage = new TextEntryStorage(devices.clipboardRead, devices.clipboardWrite);
        const base = (token) => owner.node(token);
        const node = (token) => { const value = base(token); return value instanceof TextEntryGumpNode ? value : null; };
        const consoleNode = (token) => { const value = base(token); return value instanceof ConsoleGumpNode ? value : null; };
        const sliders = this.#scroll.backend();
        this.#entry = new TextEntryGump(devices.entryState, {
            node, console: consoleNode, base, buffer: (token) => this.#storage.find(token), slider: (token) => this.#scroll.node(token),
            allocate: (size) => {
                if (size === 0xd8) return this.#scroll.allocate();
                if (size !== 0x28 || this.#nextBuffer > 0xffffffff) throw new RangeError('TextEntry 실제 할당 크기 또는 주소 오류');
                const token = this.#nextBuffer++; this.#storage.create(token); this.#buffers.add(token); return token;
            },
            releaseBuffer: (token) => { this.#storage.releaseBuffer(token); this.#buffers.delete(token); },
            constructBackground: (token, flags) => this.#scroll.background().construct(token, 0, 0, flags, 1, 0, 0),
            constructSlider: (...args) => this.#scroll.construct(...args),
            baseAttach: (...args) => owner.attachBase(...args), attach: (...args) => owner.attach(...args), push: (token) => owner.tree().push(token),
            idle: (token, enabled) => owner.lifetime().setIdle(token, enabled), redraw: (token, erase) => owner.redraw(token, erase),
            bubble: (...args) => owner.events().bubble(...args), sliderRange: (...args) => sliders.setRange(...args),
            sliderPage: (...args) => sliders.setPage(...args), sliderPosition: (...args) => sliders.setPosition(...args),
            now: devices.now, measure: devices.measure, position: (token) => owner.collection().position(token),
            drawBackground: devices.drawBackground, drawText: devices.drawText, fill: devices.fill,
            recipient: devices.recipient, complete: devices.complete, submit: devices.submit,
            allocateBytes: (size) => this.#storage.allocate(size), releaseBytes: (address) => this.#storage.release(address), bytes: (address) => this.#storage.bytes(address),
            assertion: (line) => { throw new Error(`TextEntry 원본 진단: ${line}`); },
            destructBase: (token) => owner.lifetime().deleteScalar(token, this.#deleteFlags.get(token)),
        });
    }
    // 실제 편집기 또는 Console의 입력·부착·idle·삭제를 공통 저장소에 등록한다.
    allocate(console = false) {
        const node = console ? new ConsoleGumpNode() : new TextEntryGumpNode(); let token = 0;
        token = this.#owner.allocateDerived(node, (event, first, second) => console ? this.#entry.handleConsole(token, event, first, second) : this.#entry.handle(token, event, first, second),
            (current, flags) => this.#delete(current, flags, console), (current, ...args) => this.#entry.attach(current, ...args), null,
            (current) => this.#entry.idle(current));
        this.#owned.add(token); return token;
    }
    // 원본 파생 소멸 뒤 같은 기본 scalar 반납을 한 번만 실행한다.
    #delete(token, flags, console) {
        this.#deleteFlags.set(token, flags);
        try { this.#entry.destruct(token, console); if (flags & 1) this.#owned.delete(token); }
        finally { this.#deleteFlags.delete(token); }
        return token;
    }
    // 요청한 실제 파생 종류의 원본 생성자를 호출한다.
    construct(token, width, height, initial, font, background, color) {
        return this.#owner.node(token) instanceof ConsoleGumpNode
            ? this.#entry.constructConsole(token, width, height, initial, font, background, color)
            : this.#entry.construct(token, width, height, initial, font, background, color);
    }
    // 같은 편집 처리기와 버퍼 소유자를 빌린다.
    backend() { return this.#entry; }
    storage() { return this.#storage; }
    // 부분 생성과 미등록 할당을 포함해 실제 소유 객체를 반납한다.
    close() {
        this.#owner.lifetime().cleanup();
        // 부모 소멸이 자식 Slider를 먼저 제거하므로 현재 노드를 다시 조회한다.
        for (const token of [...this.#owned]) {
            const node = this.#owner.node(token); if (!node) { this.#owned.delete(token); continue; }
            if (this.#owner.createState().entries[node.id] === token) this.#owner.remove(token);
            else { this.#owner.discardUnregistered(token); this.#owned.delete(token); }
        }
        this.#scroll.close();
        // 버퍼 주소를 부모에 쓰기 전에 발생한 실패도 저장소에서 정리한다.
        for (const token of [...this.#buffers]) { this.#storage.find(token).destroy(); this.#storage.releaseBuffer(token); this.#buffers.delete(token); }
    }
}
