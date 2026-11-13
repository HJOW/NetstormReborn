// 돌 버튼의 부분 초기화·라벨·실제 측정/배경·가상 부착/출력/수명을 연결한다(cpppj client/StoneButtonGump.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E417)
import { ButtonGumpNode, ButtonGumpKind } from './button-gump.js';
import { GumpBackgroundFlags, planButtonDraw } from '../render/gump-visual.js';

// 10.78 VT 식별값·실제 배경 new 요청 크기·라벨 복사 한도다.
const PATCH_STONE_VTABLE = 0x503898; const BACKGROUND_BYTES = 0xb8; const LABEL_COPY_BYTES = 63;
// 선택된 실제 처리기가 없으면 앞선 효과를 보존한 채 실패한다.
function need(fn) { if (typeof fn !== 'function') throw new TypeError('돌 버튼 선택 가상/장치 경계 필요'); return fn; }
// 산술 결과는 새 float 저장이다. 원문 직접 복사의 신호NaN 비트를 재사용하지 않는다.
function setDimension(node, field, value) { node[field] = Math.fround(value); if (node.floatBits) delete node.floatBits[field]; }
// 새 메모리의32BYTE 문맥 기본값이다. 생성자는 이 중 앞 네DWORD만 초기화한다.
const emptyContext = () => ({ flags: 0, font: 0, color: 0, style: 0, extra: 0, shadowColor: 0, shadowX: 0, shadowY: 0 });

// 기본/버튼과 같은 주소의 label+c0·세 문맥+104·측정값+164/+168이다.
export class StoneButtonGumpNode extends ButtonGumpNode {
    // 원문64BYTE 라벨이다. 웹 유니코드 장치는 생성 전에 같은64칸 Uint16Array를 명시 공급할 수 있다.
    label = new Uint8Array(64);
    // 미작성 extra/disabled shadow 필드도 현재 객체에 보존한다.
    contexts = [emptyContext(), emptyContext(), emptyContext()];
    // 현재 기본 문맥으로 측정한 signedDWORD 폭/높이다.
    textWidth = 0; textHeight = 0;
}

// 같은 Client가 생성 시점에 공급하는 현재 색/문맥/배경/팔레트다.
export class StoneButtonGumpState {
    // signed 색 BYTE와 현재 기본32BYTE 문맥이다.
    color = 0; defaultContext = new Uint32Array(8);
    // 마지막 파일 팔레트의 검정/흰색/외곽선 번호다.
    palette = { black: 0, white: 0, edge: 0 };
    // 실제 GameAssets 타입/논리 프레임이다.
    backgroundType = 0; backgroundFrame = 0;
}

// 실제 Button/GumpCreate/Background/Tree/Lifetime을 빌리며 자동 Client 조립은 상위 계층이 담당한다.
export class StoneButtonGump {
    // 같은 현재 상태/처리기와 안정된 파생 노드·가상 장치다.
    #create; #state; #factory; #buttons; #backgrounds; #tree; #life; #hooks;
    // measure(token,label,words,output,length)의 output은 실제 측정 필드의 읽기/쓰기 별칭이다.
    constructor(create, state, factory, buttons, backgrounds, tree, lifetime, hooks = {}) { need(hooks.node); this.#create = create; this.#state = state; this.#factory = factory; this.#buttons = buttons; this.#backgrounds = backgrounds; this.#tree = tree; this.#life = lifetime; this.#hooks = hooks; }
    // 파생/기본 생성 저장소의 주소를 실제 호출 때 확인한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || this.#factory.collectionHooks().node(token) !== node) throw new Error('돌 버튼 토큰/공유 저장소 없음'); return node; }
    // 배경은 별도 파생 타입이므로 같은 기본 저장소로 조회한다.
    #baseNode(token) { const node = token ? this.#factory.collectionHooks().node(token) : null; if (!node) throw new Error('돌 버튼 배경/기본 저장소 없음'); return node; }
    // NUL 뒤는 소스를 더 읽지 않고0으로 채우며 label[63]은 보존한다.
    #copyLabel(node, label) { if (label === null) { node.label[0] = 0; return; } if (!(label instanceof Uint8Array || label instanceof Uint16Array) || node.label.length !== 64 || (label instanceof Uint16Array && !(node.label instanceof Uint16Array))) throw new TypeError('돌 버튼 라벨의 명시 바이트/UTF16 저장소 필요'); let ended = false;
        // 원문 strncpy63의 쓰기 순서와 유효 소스 범위만 따른다.
        for (let i = 0; i < LABEL_COPY_BYTES; i += 1) { if (!ended && i >= label.length) throw new RangeError('돌 버튼 라벨 소스 범위 오류'); const value = ended ? 0 : label[i]; node.label[i] = value; if (!value) ended = true; }
    }
    // 원문 unused는 읽지 않고 측정값/문맥 꼬리·라벨 마지막 바이트도 초기화하지 않는다.
    construct(token, width, height, label, unused = 0, context = null) { width |= 0; height |= 0; const n = this.#node(token); this.#buttons.constructBase(token, width, height, 0); void unused; n.vtable = PATCH_STONE_VTABLE; n.kind = ButtonGumpKind.STONE;
        // 앞 네DWORD만 초기화하며 extra/비활성 shadow는 보존한다.
        for (const text of n.contexts) Object.assign(text, { flags: 0, font: 0, color: 0, style: 0 });
        n.command = ((this.#state.color << 24) >> 24) >>> 0; n.contexts[2].color = this.#state.palette.black; Object.assign(n.contexts[1], { color: this.#state.palette.white, shadowColor: this.#state.palette.black, flags: 2, shadowX: 1, shadowY: 1 }); Object.assign(n.contexts[0], { color: this.#state.palette.white, flags: 2, shadowX: 1, shadowY: 1, shadowColor: this.#state.palette.black }); const selected = context ?? this.#state.defaultContext; if (selected.length !== 8) throw new TypeError('돌 버튼 기본 문맥8DWORD 필요'); n.textContext.set(selected); this.#copyLabel(n, label); n.baseWidth = Math.fround(width); n.width = Math.fround(width * n.scale); n.baseHeight = Math.fround(height); n.height = Math.fround(height * n.scale);
        let background = need(this.#hooks.allocate)(BACKGROUND_BYTES); if (background) { const frame = this.#state.backgroundFrame; const type = this.#state.backgroundType; background = this.#backgrounds.construct(background, Math.fround((width - 2) | 0), Math.fround((height - 2) | 0), GumpBackgroundFlags.BUTTON, 1, type, frame); } n.background = background; this.#baseNode(background).secondaryDepth = 1; need(this.#hooks.attach)(n.background, 1, 1, token, 0); return token;
    }
    // 최대 길이DWORD -1과 실제 현재 기본 문맥/두 출력 별칭을 공급한다.
    #measure(token) { const n = this.#node(token); const output = { get width() { return n.textWidth; }, set width(value) { n.textWidth = value | 0; }, get height() { return n.textHeight; }, set height(value) { n.textHeight = value | 0; } }; need(this.#hooks.measure)(token, n.label, n.textContext, output, 0xffffffff); }
    // 라벨 변경은 측정만 수행하며 크기/배경/재그리기를 추가하지 않는다.
    setLabel(token, label) { if (label === null) throw new TypeError('돌 버튼 라벨 변경의 원본 non-null 계약 필요'); this.#copyLabel(this.#node(token), label); this.#measure(token); }
    // 측정 뒤 현재 출력/배율을 읽고 DWORD 덧셈 감김 후 float 저장 지점을 유지한다.
    fitToText(token) { this.#measure(token); const n = this.#node(token); const width = (n.textWidth + 12) | 0; setDimension(n, 'baseWidth', width); setDimension(n, 'width', width * n.scale); const height = (n.textHeight + 6) | 0; setDimension(n, 'baseHeight', height); setDimension(n, 'height', height * n.scale); }
    // Push 콜백 뒤 현재 배경/부모 크기와 높이 저장 전의 현재 배경을 다시 읽는다.
    attach(token, x, y, parent, mode, bits = {}) { const n = this.#node(token); need(this.#hooks.push)(n.background); const background = this.#baseNode(n.background); const width = n.baseWidth - 2; setDimension(background, 'baseWidth', width); setDimension(background, 'width', width * background.scale); const heightBackground = this.#baseNode(n.background); const height = n.baseHeight - 2; setDimension(heightBackground, 'baseHeight', height); setDimension(heightBackground, 'height', height * heightBackground.scale); need(this.#hooks.attach)(n.background, 1, 1, token, 0); this.#tree.attach(token, x, y, parent, mode, bits); }
    // 자식 배경은 공통 자식 우선 Destroy가 이미 파괴한다. 별도 free를 추가하지 않는다.
    deleteScalar(token, flags) { this.#node(token); return this.#life.deleteScalar(token, flags); }
    // 선택 문맥은 값형 계획에 복사하며 기본 측정 문맥과 구별한다.
    drawPlan(token) { const n = this.#node(token); const plan = planButtonDraw(this.#buttons.visualState(token, n.textWidth, n.textHeight), n.contexts, this.#state.palette.edge); return { ...plan, text: { ...plan.text }, lines: plan.lines.map((line) => ({ first: { x: line.x1, y: line.y1 }, last: { x: line.x2, y: line.y2 }, color: line.color })) }; }
    // 부모 선 뒤 현재 Node·Position과 라벨을 실제 글리프 출력기에 넘긴다. 배경은 별도 kind2 항목이다.
    draw(token, collection, text, target) { const plan = this.drawPlan(token); target.lines(plan.lines); const n = this.#node(token); text.drawGump(collection, token, n.label, plan.textOffset, n.textContext, plan.text); }
}

// 순환 의존 처리기를 만든 뒤 resolve를 연결할 수 있고 다른 파생 fallback은 보존한다.
export function bindStoneButtonGump(tree, lifetime, resolve) { need(resolve); const attach = tree.attach; const destroy = lifetime.deletingDestructor;
    // 실제 Attach의 마지막은 기본 GumpTree를 직접 호출하므로 가상 재귀를 만들지 않는다.
    tree.attach = (token, x, y, parent, mode, bits = {}) => { const stone = resolve(token); if (stone) stone.attach(token, x, y, parent, mode, bits); else need(attach)(token, x, y, parent, mode, bits); };
    // 등록 해제/자식 파괴는 같은 공통 수명이 담당한다.
    lifetime.deletingDestructor = (token, flags) => { const stone = resolve(token); if (stone) stone.deleteScalar(token, flags); else need(destroy)(token, flags); };
}

// 공통 GumpDraw의 clip 보호 안에서 돌 버튼 부모의 선/글자만 분배한다.
export function bindStoneButtonDraw(hooks, collection, text, target, resolve) { need(resolve); const previous = hooks.draw; hooks.draw = (token) => { const stone = resolve(token); if (stone) stone.draw(token, collection, text, target); else need(previous)(token); }; }
