// TextEntryGump 시험이 함께 쓰는 지원 틀이다. 실제 TextEntryStorage/TextEntryBuffer 위에서 노드·Slider·훅을 구성하고 모든 효과를 log 에 기록한다. (2026-11-14 WB04-E439)
// ScrollGump 의 setRange/setPage/setPosition 은 웹 ScrollGump(www/ui/scroll-gump.js)와 같은 계약(정렬·같으면 최대+1·page 제한·위치 제한)을 틀 안에서 재현한다.
import { TextEntryGump, TextEntryGumpNode, ConsoleGumpNode, TextEntryGumpState } from '../www/ui/text-entry-gump.js';
import { ScrollGumpNode } from '../www/ui/scroll-gump.js';
import { TextEntryStorage } from '../www/ui/text-entry-storage.js';

// 바이트 배열(NUL 포함 가능)을 첫 NUL 앞의 바이트 문자열로 바꾼다.
export function cstr(bytes) { let text = ''; for (const byte of bytes) { if (!byte) break; text += String.fromCharCode(byte); } return text; }
// 바이트 문자열을 NUL 로 끝나는 바이트 배열로 만든다.
export const bytesOf = (text) => Uint8Array.from([...Buffer.from(text, 'latin1'), 0]);

// 구성하지 않은 편집 Gump 틀이다. options: { console, scale, clipboard(문자열), complete(함수) }. 토큰: 편집 노드 1, 버퍼 300+, Slider 200+, 제외문자 저장소는 TextEntryStorage 주소다.
export function rig(options = {}) {
    const log = []; const nodes = new Map(); const clip = { text: options.clipboard ?? '', written: [] };
    const storage = new TextEntryStorage((bytes) => { bytes.fill(0); bytes.set(Buffer.from(`${clip.text}\0`, 'latin1')); }, (bytes) => clip.written.push(Buffer.from(bytes).toString('latin1')));
    const state = new TextEntryGumpState(); state.brown = 0x11; state.black = 0x22; state.yellow = 0x33;
    const clock = { now: 100 }; let nextBuffer = 300; let nextSlider = 200;
    const root = options.console ? new ConsoleGumpNode() : new TextEntryGumpNode(); root.scale = options.scale ?? 1; root.serial = 77; nodes.set(1, root);
    // 웹 ScrollGump 와 같은 값 처리(setRange/setPage/setPosition)를 노드에 적용한다.
    const position = (node, value) => { value |= 0; if (value < node.minimum) value = node.minimum; if (node.maximum < ((node.page + value) | 0)) value = (node.maximum - node.page) | 0; node.position = value; };
    const hooks = {
        node: (token) => nodes.get(token) ?? null, console: (token) => (nodes.get(token) instanceof ConsoleGumpNode ? nodes.get(token) : null), base: (token) => nodes.get(token) ?? null,
        buffer: (token) => storage.find(token), slider: (token) => (nodes.get(token) instanceof ScrollGumpNode ? nodes.get(token) : null),
        allocate: (size) => {
            if (options.failBuffer && size === 0x28) { log.push(['allocate', size, 0]); return 0; }
            if (size === 0x28) { const token = nextBuffer; nextBuffer += 1; storage.create(token); log.push(['allocate', size, token]); return token; }
            const token = nextSlider; nextSlider += 1; const node = new ScrollGumpNode(); node.serial = token; nodes.set(token, node); log.push(['allocate', size, token]); return token;
        },
        releaseBuffer: (token) => { log.push(['releaseBuffer', token]); storage.releaseBuffer(token); },
        constructBackground: (token, background) => { log.push(['constructBackground', token, background]); nodes.get(token).backgroundFlags = background; },
        constructSlider: (token, width, height, brown, black) => { log.push(['constructSlider', token, width, height, brown, black]); const node = nodes.get(token); node.baseWidth = width; node.baseHeight = height; node.scale = 1; node.flags = 0x401; },
        baseAttach: (token, x, y, parent, mode) => log.push(['baseAttach', token, x, y, parent, mode]), attach: (token, x, y, parent, mode) => log.push(['attach', token, x, y, parent, mode]),
        push: (token) => log.push(['push', token]), idle: (token, enable) => log.push(['idle', token, enable]), redraw: (token, erase) => log.push(['redraw', token, erase]),
        bubble: (token, event, first, second) => log.push(['bubble', token, event, first, second]),
        sliderRange: (token, first, last) => {
            const node = nodes.get(token); log.push(['sliderRange', token, first, last]);
            let a = first | 0; let b = last | 0; if (b < a) [a, b] = [b, a]; if (a === b) b += 1; node.minimum = a; node.maximum = b; if (node.maximum - node.minimum < node.page) node.page = node.maximum - node.minimum; position(node, node.position); position(node, node.position);
        },
        sliderPage: (token, page) => { const node = nodes.get(token); log.push(['sliderPage', token, page]); const length = node.maximum - node.minimum; node.page = length < page ? length : page; position(node, node.position); },
        sliderPosition: (token, value) => { log.push(['sliderPosition', token, value]); position(nodes.get(token), value); },
        now: () => { if (options.clock) return options.clock(); return clock.now; },
        // 글자 폭은 바이트당 8, 높이는 16이다. 명시 길이가 있으면 NUL 과 무관하게 그만큼, 없으면 첫 NUL 앞까지 잰다.
        measure: (token, text, length = -1) => { const end = text.indexOf(0); const count = length >= 0 ? Math.min(length, text.length) : (end < 0 ? text.length : end); return [8 * count, 16]; },
        position: () => ({ x: 10, y: 20 }),
        drawBackground: (token) => log.push(['drawBackground', token]),
        drawText: (token, bytes, point, context) => log.push(['drawText', cstr(bytes), point.x, point.y, context ? context[2] : null]),
        fill: (rect, color) => log.push(['fill', rect.left, rect.top, rect.right, rect.bottom, color]),
        recipient: (name) => { log.push(['recipient', name]); return name === 'Ann'; },
        allocateBytes: (size) => storage.allocate(size), releaseBytes: (address) => { log.push(['releaseBytes', address]); storage.release(address); }, bytes: (address) => storage.bytes(address),
        complete: (autocomplete, word, length, reverse) => { log.push(['complete', autocomplete, word, length, reverse]); return options.complete ? options.complete(word, length, reverse) : null; },
        submit: (token, text) => log.push(['submit', token, text]), assertion: (line) => log.push(['assertion', line]), destructBase: (token) => log.push(['destructBase', token]),
    };
    const gump = new TextEntryGump(state, hooks);
    return { gump, state, hooks, nodes, storage, log, clock, clip, root, buffer: () => storage.find(root.buffer), sliderOf: (token) => nodes.get(token) };
}

// 구성까지 마친 틀이다. initial 이 문자열이면 초기 본문(전체 선택), 생략하면 빈 본문이다. width/height 는 기본 200×60 이며 로그는 구성 뒤 비운다.
export function built(options = {}, initial = undefined, width = 200, height = 60) {
    const r = rig(options); const build = options.console ? 'constructConsole' : 'construct';
    r.gump[build](1, width, height, initial, 0x1234, 0x9988, 0x5566); r.log.length = 0; return r;
}
