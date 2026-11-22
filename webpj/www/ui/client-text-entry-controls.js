// 실제 Client 공통 트리에 원본 편집기·버퍼와 두 Slider를 조립한다.
// 사용: new ClientTextEntryControls(owner, devices).allocate() → construct(token, width, height, text, font, background, color).
import { ClientScrollControls } from './client-scroll-controls.js';
import { TextEntryGump, TextEntryGumpNode, ConsoleGumpNode } from './text-entry-gump.js';
import { TextEntryStorage } from './text-entry-storage.js';
import { GumpText } from '../render/gump-text.js';
import { decodeGumpTextContext } from '../render/gump-text-draw.js';
import { displayTextCodec } from '../assets/display-text.js';
import { encodeUtf16Units } from '../assets/text-encoding.js';
import { hasExtendedDisplayText } from '../core/display-text-input.js';

// 원본 바이트와 네 바이트 확장 표시 문자가 섞인 글자의 [폭, 높이]를 해석된 글자 문맥으로 측정한다. 사용: createMixedGumpTextMeasure(fonts)(바이트, 해석된 문맥, 길이 -1).
// TextEntry 와 돌 버튼 라벨이 같은 측정을 쓴다. (2026-11-22 WB04-E1168: 돌 버튼 한국어 라벨 측정을 위해 입력창 측정에서 분리)
export function createMixedGumpTextMeasure(fonts) {
    if (typeof fonts?.measureFont !== 'function') throw new TypeError('혼합 글자 측정의 실제 글꼴 저장소 필요');
    const text = new GumpText(fonts);
    return (bytes, context, length = -1) => {
        if (!(bytes instanceof Uint8Array)) throw new TypeError('TextEntry 측정 바이트 필요');
        length |= 0; let count = length;
        if (length === -1) { count = bytes.indexOf(0); if (count < 0) throw new RangeError('Gump 글자 범위에 NUL 종료 필요'); }
        if (count > bytes.length) throw new RangeError('Gump 글자 측정 길이 범위 초과');
        const selected = bytes.subarray(0, Math.max(0, count));
        if (!hasExtendedDisplayText(selected)) {
            const result = text.measure(bytes, context, undefined, length); return [result.width, result.height];
        }
        // 1252 구간은 원본 글꼴로, 확장 글자만 유니코드 글꼴로 측정해 부분 선택 전후의 폭을 일치시킨다.
        const plain = { ...context, flags: context.flags & ~2 }; let width = 0; let height = 0; let start = 0;
        const add = (source) => { const result = text.measure(source, plain, undefined, source.length); width = (width + result.width) | 0; height = result.height; };
        // 유효한 네 바이트 확장 문자만 한 글자로 소비하고 나머지는 원본 구간에 남긴다.
        for (let at = 0; at < selected.length;) {
            const part = selected.subarray(at, at + 4);
            if (part.length === 4 && hasExtendedDisplayText(part)) {
                if (start < at) add(selected.subarray(start, at));
                add(encodeUtf16Units(displayTextCodec.decode(part))); at += 4; start = at;
            } else at += 1;
        }
        if (start < selected.length) add(selected.subarray(start));
        if (context.flags & 2) { width = (width + context.shadowX) | 0; height = (height + context.shadowY) | 0; }
        return [width, height];
    };
}

// 같은 실제 글꼴과 현재 Gump 문맥으로 원본 바이트 또는 확장 표시 문자를 측정한다.
export function createClientTextMeasure(owner, fonts) {
    if (!owner || typeof fonts?.measureFont !== 'function') throw new TypeError('Client TextEntry 실제 글꼴 저장소 필요');
    const measure = createMixedGumpTextMeasure(fonts);
    return (token, bytes, length = -1) => {
        const node = owner.node(token); if (!(node instanceof TextEntryGumpNode)) throw new Error('TextEntry 글꼴 측정 노드 없음');
        return measure(bytes, decodeGumpTextContext(node.textContext), length);
    };
}

// 같은 공통 Gump와 편집 버퍼·Slider의 수명을 소유한다.
export class ClientTextEntryControls {
    // 공유 소유자와 기존 실제 본체, 직접 할당한 주소들이다.
    #owner; #scroll; #storage; #entry; #owned = new Set(); #buffers = new Set(); #nextBuffer = 0x70000000; #deleteFlags = new Map();
    // 실제 전역·글꼴 측정·시간 및 선택된 출력 장치를 빌린다.
    constructor(owner, devices) {
        if (!owner || !devices?.entryState || typeof devices.now !== 'function' || (typeof devices.measure !== 'function' && typeof devices.fonts?.measureFont !== 'function')) throw new TypeError('Client TextEntry의 실제 소유자·상태·시계·글꼴 필요');
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
            now: devices.now, measure: devices.measure ?? createClientTextMeasure(owner, devices.fonts), position: (token) => owner.collection().position(token),
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
