// 실제 Client Gump·돌 버튼·배경에 원본 Slider 전체 생성과 입력을 연결한다.
// 사용: new ClientScrollControls(owner, devices).allocate() → construct(token, width, height, type, frame).
import { ClientStoneControls } from './client-stone-controls.js';
import { ScrollGump, ScrollGumpNode, ScrollGumpThumbNode } from './scroll-gump.js';
import { ScrollGumpWidgets, ScrollGumpArrowNode } from './scroll-gump-widgets.js';
import { GumpGlyphColorState, GumpTextDraw, makeGumpGlyphPixelHooks } from '../render/gump-text-draw.js';

// 같은 Client 주소와 실제 Slider·돌 버튼 자식을 소유한다.
export class ClientScrollControls {
    // 공통 소유자와 기존 원본 본체, 자신이 할당한 Slider·손잡이다.
    #owner; #stones; #slider; #widgets; #owned = new Set(); #output; #colors = new GumpGlyphColorState();
    // 실제 장기 전역과 시간·입력·색·자산 장치를 빌린다.
    constructor(owner, devices) {
        if (!owner || !devices?.scrollState || !devices?.widgetState) throw new TypeError('Client Slider의 실제 소유자·상태 필요');
        this.#owner = owner; this.#output = devices.output ?? null;
        if (this.#output && (!this.#output.fonts || !this.#output.background)) throw new TypeError('Client Slider의 실제 출력 장치 누락');
        this.#stones = new ClientStoneControls(owner, devices);
        const base = (token) => owner.node(token);
        const node = (token) => { const value = base(token); return value instanceof ScrollGumpNode ? value : null; };
        const thumb = (token) => { const value = base(token); return value instanceof ScrollGumpThumbNode ? value : null; };
        const arrow = (token) => { const value = base(token); return value instanceof ScrollGumpArrowNode ? value : null; };
        this.#slider = new ScrollGump(owner.createState(), devices.scrollState, owner.tree(), owner.collection(), owner.events(), owner.input(), {
            node, thumbNode: thumb, baseNode: base, push: (token) => owner.tree().push(token), attach: (...args) => owner.attach(...args),
            redraw: (token, erase) => owner.redraw(token, erase), poll: devices.poll,
        });
        this.#widgets = new ScrollGumpWidgets(owner.createState(), devices.widgetState, owner.factory(), this.#stones.backend(), this.#stones.background(), this.#slider, owner.lifetime(), {
            node, thumb, arrow, base, attach: (...args) => owner.attach(...args), allocate: (size) => {
                if (size === 0x178) {
                    const child = this.#stones.allocate(new ScrollGumpArrowNode());
                    if (this.#output) owner.bindDraw(child, (current, target) => this.#widgets.drawArrow(current, owner.collection(), new GumpTextDraw(this.#output.fonts, makeGumpGlyphPixelHooks(target, this.#colors)), target));
                    return child;
                }
                if (size !== 0xd8) throw new RangeError('Slider 자식의 실제 할당 크기 오류');
                const child = new ScrollGumpThumbNode(); let token = 0;
                token = owner.allocateDerived(child, (event, first, second) => this.#slider.thumbHandle(token, event, first, second),
                    (current, flags) => this.#delete(current, flags), null,
                    (current, event, first, second) => this.#slider.thumbBroadcast(current, event, first, second));
                if (this.#output) owner.bindDraw(token, (current, target, clip) => this.#widgets.drawThumb(current, owner.collection(), {
                    draw: (value) => this.#output.background.draw(value, target, clip), background: (rect, flags, type, frame) => this.#output.background.paint(target, clip, rect, flags, type, frame),
                }));
                this.#owned.add(token); return token;
            },
        });
    }
    // 실제 Slider 부모의 가상 입력·부착·삭제를 공통 Client에 등록한다.
    allocate() {
        const node = new ScrollGumpNode(); let token = 0;
        token = this.#owner.allocateDerived(node, (event, first, second) => this.#slider.handle(token, event, first, second),
            (current, flags) => this.#delete(current, flags), (current, x, y, parent, mode) => this.#slider.attach(current, x, y, parent, mode));
        if (this.#output) this.#owner.bindDraw(token, () => {});
        this.#owned.add(token); return token;
    }
    // 현재 Slider 또는 손잡이의 실제 공통 scalar 삭제를 실행한다.
    #delete(token, flags) { const result = this.#widgets.deleteScalar(token, flags); if (flags & 1) this.#owned.delete(token); return result; }
    // 실제 화살표·배경·손잡이와 값 초기화를 기존 생성기에 위임한다.
    construct(token, width, height, type, frame) { return this.#widgets.construct(token, width, height, type, frame); }
    // 같은 실제 배경 생성기를 TextEntry가 빌린다.
    background() { return this.#stones.background(); }
    // 같은 실제 Slider 값 처리기를 TextEntry가 빌린다.
    backend() { return this.#slider; }
    // 현재 파생 노드를 같은 Client 저장소에서 조회한다.
    node(token) { const value = this.#owner.node(token); return value instanceof ScrollGumpNode ? value : null; }
    // 생성 실패 중 미부착 자식까지 각 실제 소유자가 반납한다.
    close() {
        this.#owner.lifetime().cleanup();
        // 부모 삭제가 손잡이를 반납할 수 있으므로 매번 현재 주소를 확인한다.
        for (const token of [...this.#owned]) {
            const node = this.#owner.node(token); if (!node) { this.#owned.delete(token); continue; }
            if (this.#owner.createState().entries[node.id] === token) this.#owner.remove(token);
            else { this.#owner.discardUnregistered(token); this.#owned.delete(token); }
        }
        this.#stones.close();
    }
}
