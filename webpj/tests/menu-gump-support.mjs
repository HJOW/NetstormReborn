// MenuGump*(메뉴 처리기 6종) 시험이 공유하는 실제 Gump 저장소 장면이다. (2026-11-14 WB04-E428)
// 토큰 1 루트·2 메뉴·3 장식(배경 파생)·4 제목(배경 파생)·5/6 자식 메뉴를 기본 생성 장치로 등록하고, 가상 경계는 호출 기록 대역으로 둔다.
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpBackgroundNode } from '../www/ui/gump-background-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { MenuGump, MenuGumpNode, MenuGumpState } from '../www/ui/menu-gump.js';
import { MenuGumpLayout } from '../www/ui/menu-gump-layout.js';

// 문자열은 바이트 하나가 문자 하나인 바이너리 문자열이다.
export const words = (...values) => Uint32Array.from([0, 0, 0, 0, 0, 0, 0, 0].map((zero, index) => values[index] ?? zero));

// 메뉴 크기/프레임/스택 처리기와 실제 기본 생성·수명 저장소를 한 장면으로 묶는다. on.<이름> 으로 호출 시점에 상태를 바꿀 수 있다.
export function menuScene(options = {}) {
    const create = new GumpCreateState(); create.screenWidth = 640; create.screenHeight = 480;
    const nodes = new Map([[1, new GumpCreateNode()], [2, new MenuGumpNode()], [3, new GumpBackgroundNode()], [4, new GumpBackgroundNode()], [5, new MenuGumpNode()], [6, new MenuGumpNode()]]);
    const lookup = (token) => nodes.get(token);
    const factory = new GumpCreate(create, { node: lookup, allocate: () => 1 }); factory.initializeRoot();
    // 토큰마다 기본 크기를 주고 등록한다(루트 아래 부모는 시험이 필요할 때 연결한다).
    for (const [token, width, height] of [[2, 50, 40], [3, 10, 10], [4, 60, 20], [5, 30, 30], [6, 30, 30]]) factory.construct(token, 0, width, height, 1);
    const lifeState = new GumpLifetimeState(); const lifetime = new GumpLifetime(create, lifeState, { node: lookup });
    const state = new MenuGumpState(); const log = []; const on = {};
    const record = (name) => (...args) => { log.push([name, ...args]); return on[name]?.(...args); };
    const hooks = { node: lookup, baseNode: lookup, push: record('push'), pop: record('pop'), attach: record('attach'), attachObject: record('attachObject'), redraw: record('redraw'), baseRedraw: record('baseRedraw') };
    for (const key of options.omit ?? []) delete hooks[key];
    const menu = new MenuGump(create, state, lifetime, hooks);
    return { create, nodes, factory, lifeState, lifetime, state, log, on, hooks, menu, menuNode: nodes.get(2), fancy: nodes.get(3), title: nodes.get(4), childA: nodes.get(5), childB: nodes.get(6) };
}

// 레이아웃 처리기를 같은 장면에 얹는다. plain/styled/marker 측정은 sizes/markerSize 로 정하고 모든 가상 경계를 기록한다.
export function layoutScene(options = {}) {
    const s = menuScene(options.menu ?? {}); const sizes = options.sizes ?? {}; const state = { markerSize: [6, 5], plainDefault: [10, 4], styledDefault: [12, 9] };
    const record = (name) => (...args) => { s.log.push([name, ...args.map((value) => (value instanceof Uint32Array ? Array.from(value) : value))]); return s.on[name]?.(...args); };
    const hooks = {
        node: s.hooks.node, baseNode: s.hooks.baseNode,
        plain: (token, index, context) => { s.log.push(['plain', token, index, Array.from(context)]); return s.on.plain?.(token, index, context) ?? sizes[index] ?? state.plainDefault; },
        styled: (token, index) => { s.log.push(['styled', token, index]); return s.on.styled?.(token, index) ?? sizes[index] ?? state.styledDefault; },
        marker: (token, context) => { s.log.push(['marker', token, Array.from(context)]); return s.on.marker?.(token, context) ?? state.markerSize; },
        clearStyle: record('clear'), parseStyle: record('parse'), push: record('lpush'), titleFit: record('titleFit'), titleLayout: record('titleLayout'), attach: record('lattach'),
    };
    for (const key of options.omit ?? []) delete hooks[key];
    const layout = new MenuGumpLayout(s.create, s.menu, hooks);
    return { ...s, layout, layoutHooks: hooks, sizes, measure: state };
}

// 메뉴 노드를 세로/가로/장식 등의 상태로 직접 채운다(생성 처리기는 별도 단위이므로 시험이 필요한 필드만 쓴다).
export function prepared(s, token, fields = {}) { const node = s.nodes.get(token); Object.assign(node, fields); return node; }
// 기록에서 한 종류만 거른다.
export const only = (log, name) => log.filter((entry) => entry[0] === name);
