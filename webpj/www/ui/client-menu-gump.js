// 원본 메뉴의 생성·레이아웃·입력·픽셀·수명을 같은 Client Gump에 조립한다.
import { GumpText } from '../render/gump-text.js';
import { GumpGlyphColorState, GumpTextDraw, makeGumpGlyphPixelHooks } from '../render/gump-text-draw.js';
import { GumpDecorationPixelTarget } from './gump-decoration.js';
import { styleTextField } from './style-text-fragments.js';
import { MenuGump, MenuGumpNode, MenuGumpState } from './menu-gump.js';
import { MenuGumpLayout, bindMenuLayoutText } from './menu-gump-layout.js';
import { MenuGumpControls, bindMenuGumpControlConfig } from './menu-gump-controls.js';
import { MenuGumpWidgets, MenuGumpWidgetState } from './menu-gump-widgets.js';
import { MenuGumpInput, MenuGumpInputState } from './menu-gump-input.js';
import { MenuGumpDraw, MenuGumpDrawState, bindMenuGumpDrawAssets } from './menu-gump-draw.js';

// 같은 Client의 실제 메뉴 노드·장식·제목과 controlled 스택을 소유한다.
export class ClientMenuGump {
    // 현재 공통 수명·자산·스타일과 원본 메뉴 처리기들이다.
    #root; #parts; #menu; #layout; #controls; #widgets; #input; #state = new MenuGumpState(); #drawState = new MenuGumpDrawState(); #colors = new GumpGlyphColorState();
    // 모든 사용자 효과는 실제 호출 장치에서 받는다.
    constructor(root, parts) {
        if (!root || !parts?.assets || !parts?.fonts || !parts?.text || !parts?.background || !parts?.decoration || !parts?.title || !parts?.session || !parts?.config || ['sound', 'command', 'invalidate'].some((name) => typeof parts[name] !== 'function') || ['backgroundType', 'foreground', 'disabled', 'shadow'].some((name) => !Number.isInteger(parts[name]))) throw new TypeError('Client 메뉴 Gump의 실제 장치 누락');
        this.#root = root; this.#parts = parts; const node = (token) => root.node(token), style = parts.text.styleEngine();
        this.#menu = new MenuGump(root.createState(), this.#state, root.lifetime(), { node, baseNode: node, push: (token) => root.tree().push(token),
            pop: (token) => { const current = node(token); root.attach(token, current.x, current.y, 0, 0); }, attach: (...args) => root.attach(...args),
            attachObject: (...args) => this.#widgets.attachObject(...args), redraw: (token, erase) => root.redraw(token, erase), baseRedraw: (token, erase) => root.redrawBase(token, erase) });
        const layoutHooks = { node, baseNode: node, push: (token) => root.tree().push(token), attach: (...args) => root.attach(...args),
            titleFit: (token, position) => parts.title.requestScroll(token, position), titleLayout: (token, width, height) => parts.title.fit(token, width, height),
            clearStyle: (token, index) => style.clear(styleTextField(node(token).items[index], 'styled')),
            parseStyle: (token, index, source, context) => style.parse(styleTextField(node(token).items[index], 'styled'), parts.fonts.encode(source), context),
            styled: (token, index) => { const size = style.measure(styleTextField(node(token).items[index], 'styled')); return [size.width, size.height]; } };
        bindMenuLayoutText(layoutHooks, new GumpText(parts.fonts)); this.#layout = new MenuGumpLayout(root.createState(), this.#menu, layoutHooks);
        const widgetState = new MenuGumpWidgetState(); widgetState.backgroundType = parts.backgroundType; widgetState.backgroundFrame = 1;
        widgetState.foreground = parts.foreground; widgetState.shadow = parts.shadow; widgetState.currentObjectFormType = () => 3;
        const host = parts.session.gumpFormHost();
        this.#widgets = new MenuGumpWidgets(root.createState(), this.#state, widgetState, { construct: (token, width, height, flags, mode, type, frame) => parts.background.construct(token, width, height, flags, type, frame, mode) },
            parts.decoration, parts.title, this.#layout, style, root.lifetime(), root.collection(), { attach: (...args) => root.attachBase(...args) }, {
                node, base: node, allocate: (size) => { if (size === 0xb90) return this.#allocate(); if (size === 0xc4) return parts.decoration.allocate(); if (size === 0xd8) return parts.title.allocate(); throw new RangeError('Client 메뉴 자식 할당 크기'); },
                attach: (...args) => root.attach(...args), objectForm: (type, mode) => { if (type !== 3) throw new RangeError('메뉴 Form TYPE 오류'); const token = host.create(mode); return { token, index: token }; },
                formOwner: (form, owner) => host.setOwner(form, owner), formObject: (form, object, flags) => host.attach(form, object, flags),
            });
        const controlHooks = { node, base: node }; bindMenuGumpControlConfig(controlHooks, parts.config);
        this.#controls = new MenuGumpControls(root.createState(), this.#state, this.#menu, this.#layout, { parse: (token, value) => parts.title.style(token, value) }, controlHooks);
        const inputState = new MenuGumpInputState(); Object.assign(inputState, parts.inputState ?? {});
        this.#input = new MenuGumpInput(root.createState(), inputState, this.#menu, root.collection(), root.events(), root.lifetime(), root.tree(), {
            node, base: node, push: (token) => root.tree().push(token), attach: (...args) => root.attach(...args), redraw: (token, erase) => root.redraw(token, erase),
            broadcast: (token, event, first, second) => this.#input.broadcast(token, event, first, second), handle: (...args) => root.handle(...args),
            sound: parts.sound, command: parts.command, invalidate: parts.invalidate,
        });
        Object.assign(this.#drawState, { foreground: parts.foreground, disabled: parts.disabled, backgroundType: parts.backgroundType, backgroundFrame: 1, markerType: parts.backgroundType });
    }
    // 실제 파생 메뉴 노드에 가상 입력·부착·출력·수명을 설치한다.
    #allocate() {
        const root = this.#root, parts = this.#parts, node = new MenuGumpNode(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => this.#input.handle(token, event, first, second),
            (current, flags) => this.#widgets.deleteScalar(current, flags), (current, x, y, parent, mode) => this.#widgets.attach(current, x, y, parent, mode),
            (current, event, first, second) => this.#input.broadcast(current, event, first, second), (current) => this.#menu.idle(current));
        root.bindRedraw(token, (current, erase) => this.#menu.redraw(current, erase));
        root.bindDraw(token, (current, target, clip) => parts.text.withTarget(target, () => {
            const hooks = { node: (value) => root.node(value), base: (value) => root.node(value) };
            bindMenuGumpDrawAssets(hooks, parts.assets, new GumpDecorationPixelTarget(target, parts.assets));
            const glyphs = new GumpTextDraw(parts.fonts, makeGumpGlyphPixelHooks(target, this.#colors));
            new MenuGumpDraw(root.createState(), clip, this.#drawState, root.collection(), parts.background.drawing(target, clip), glyphs, parts.text.styleEngine(), this.#layout, hooks).draw(current);
        }));
        return token;
    }
    // 현재 controlled 메뉴 생성·제목·본문·항목·부착을 원본 순서에 위임한다.
    beginFancy() { return this.#widgets.beginFancy(); }
    beginVertical(flags = 0) { return this.#widgets.beginVertical(flags); }
    title(value) { this.#controls.addTitle(value); }
    header(value) { this.#controls.addHeader(value); }
    entry(...args) { this.#controls.addText(...args); }
    idle(token, enabled) { this.#widgets.setIdle(token, enabled); }
    reference(token, object) { this.#widgets.reference(token, object); }
    root() { return this.#root.createState().collection.root; }
    attach(point, parent) { this.#menu.attachCurrent(point.x, point.y, parent); }
}
