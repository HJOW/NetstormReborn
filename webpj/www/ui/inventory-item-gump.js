// 생산 내용물 항목(5068f4)의 실제 SHP 자식 배치·수량 글자·클릭·장식·비용 판정을 옮긴다(cpppj `client/InventoryItemGump.cpp/.h`, 기준선 1f00a25a, 10.78 전용). 원본 항목은 부착 뒤 자식을 다시 만들고 실제 내용물
// flags·품질·수량을 현재 시점에 읽는다: Attach(43ce40)는 dead 진단과 자식 정리 뒤 아이콘 경계·배율·중심을 float32 중간 저장과 배정도 계산으로 정해 TYPE 패턴의 칸마다 ImageGump 자식(0xb4)과 수량 TextGump(0xa4)을 만들고,
// Draw(43cdc0)는 장식만, Handle(43d4c0)는 선택/우클릭/반복 입력을, Affordable(4af010)은 비용 float→현재 SP→에디터 판정을 수행한다.
// 사용: const item = new InventoryItemGump(pool, state, hooks); item.attach(token, x, y, parent, mode); item.draw(token); item.handle(token, event, first, second); item.affordable(contentSid).
//   state = { localOwner, editor, stockEnabled, inputDisabled, lastType(쓰기 가능), bridgeType, pixelsX, pixelsY, backgroundType, backgroundFrame }(원본 전역의 현재 값, 호출마다 읽고 lastType 만 쓴다),
//   hooks = { node(token)→InventoryItemNode, base(token)→GumpCreateNode, type(kind)→{ gumpFrame, frames }(frames = FrameCodeTable), baseAttach, clearChildren(token), cleanup(), robust(sid, allowDead), assertion(text), measure(kind, argument, explicitFrame)→{ bounds:{left,top,right,bottom} },
//   walk(query, visit)(visit({ frame, x, y })), allocate(size)→token, image(token, kind, frame, mode), text(token), styledText(token, text), fitText(token), attach(token,x,y,parent,mode), costUnits(kind), money(owner)→float, position(token)→{x,y}, background(rect, flags, type, frame),
//   local(token, event, first, second)→[x,y], knowledgeOpen(), imageHit(token, x, y), denied(first, flags), tell(text), forward(token, event, first, second), about(token, first, second) }(도달한 경계만 요구).
// 실제 자산(TYPE 표·패턴 측정·디코더)은 bindInventoryItemAssets로 연결한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E473: 5-B InventoryItemGump 이식, 기준선 뒤 cpppj 변경 없음)
import { cameraFtolLow } from '../render/camera-view.js';
import { FrameCodeTable } from '../assets/terrain-tiles.js';
import { decodeCanonType } from '../core/raw-canon-type.js';

// 실제 원본 사건 DWORD(선택·우클릭·반복)와 자식 할당 크기(그림 0xb4, 글자 0xa4)이다.
const CHOOSE = 0x02000008; const RIGHT = 0x02000018; const REPEAT = 0x02000107; const IMAGE_BYTES = 0xb4; const TEXT_BYTES = 0xa4;

// 도달한 실제 소비자만 필수로 요구한다(C++ logic_error → Error).
function need(callback) { if (typeof callback !== 'function') throw new Error('생산 항목 실제 하위 미연결'); return callback; }
// 내용물의 비정렬 WORD 를 현재 시점에 읽는다.
const word = (raw, offset) => raw[offset] | (raw[offset + 1] << 8);
// 장식 좌표의 정수 덧셈은 DWORD 로 감긴다.
const add = (a, b) => (a + b) | 0;

// 생산 항목 처리기이다. 모든 현재 상태/노드·소비자는 각 호출과 처리기보다 오래 살아야 한다. (2026-11-15 WB04-E473)
export class InventoryItemGump {
    // 단일 내용물 저장소(10.78), 원본 현재 전역(live 객체), 선택된 외부 효과이다.
    #pool; #state; #hooks;

    // 원본 10.78 내용물 저장소만 받는다(C++ invalid_argument → TypeError). 사용: new InventoryItemGump(pool, state, hooks).
    constructor(pool, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('생산 항목 PATCH 자료 필요');
        this.#pool = pool; this.#state = state; this.#hooks = hooks;
    }

    // 파생 노드의 동일 주소를 현재 조회한다(없으면 RangeError, C++ out_of_range).
    #node(token) {
        const node = token ? need(this.#hooks.node)(token) : null;
        if (!node) throw new RangeError('생산 항목 저장소 없음');
        return node;
    }

    // 자식 가상 부착 전 실제 공통 필드를 쓴다(그림/글자 자식의 기본 노드).
    #base(token) {
        const node = token ? need(this.#hooks.base)(token) : null;
        if (!node) throw new RangeError('생산 항목 자식 저장소 없음');
        return node;
    }

    // 원본 포인터 산술 전에 실제 물리 범위를 확인한다.
    #content(sid) {
        if (!sid || sid >= this.#pool.capacity || sid > 65535) throw new RangeError('생산 항목 실제 내용물SID 오류');
        return sid;
    }

    // 4af010: 비용표 복호 값은 원본처럼 먼저 float 으로 저장한다. 에디터여도 SP 조회를 생략하지 않는다(비용 → 현재 SP 순서). 반환은 1/0 이다.
    affordable(content) {
        const kind = word(this.#pool.slot(this.#content(content)), 18); const cost = Math.fround(need(this.#hooks.costUnits)(kind)); const money = Math.fround(need(this.#hooks.money)(this.#state.localOwner));
        return this.#state.editor || cost <= money ? 1 : 0;
    }

    // 43ce40: 원본의 dead 진단 네 곳과 삭제 큐 정리는 아이콘 계산 전에 실행한다. 내용물이 비면(종류 0) 자식 없이 끝난다. 아이콘 경계가 노드 크기보다 크고 항목 배율이 0 이면 큰 쪽 비율의 역수로 줄이며(배정도 나눗셈→float32),
    // 중심은 (노드 크기 − 경계×배율)×0.5+2 이다. 패턴의 칸마다 다리 TYPE 은 품질(1→0x20, 3→0x40)을 더한 플래그로 프레임을 다시 찾고, flags 0x100 은 모드 1, flags 0x418 이 있고 살 수 없으면 모드 2 로 ImageGump 자식을 만들어
    // 배율/크기를 현재 기본 크기에서 정하고 부착하며, 재고 표시가 켜져 있고 flags 8 이면 `~w~E<품질>` 글자 자식을 그림 오른쪽 아래에 붙인다.
    attach(token, x, y, parent, mode) {
        const node = this.#node(token); const hooks = this.#hooks; const state = this.#state;
        // 각 효과 이후의 현재 플래그를 다시 확인한다.
        const alive = () => { if (node.flags & 0x10000) need(hooks.assertion)('!(flags & GF_DEAD)'); };
        alive(); need(hooks.baseAttach)(token, x, y, parent, mode); alive(); need(hooks.clearChildren)(token); alive(); need(hooks.cleanup)(); alive();
        if (!need(hooks.robust)(node.content, true)) need(hooks.assertion)('isSidRobust( contentSid )');
        const sid = this.#content(node.content); const raw = this.#pool.physical(sid); const kind = word(raw, 18); let argument = word(raw, 12);
        const explicitFrame = need(hooks.type)(kind).gumpFrame !== -1;
        if (explicitFrame) argument = need(hooks.type)(kind).gumpFrame >>> 0;
        if (!kind) return;
        const shape = need(hooks.measure)(kind, argument, explicitFrame); const bounds = shape.bounds; let scale = 1;
        if (node.iconScale === 0) {
            const rx = Math.fround((bounds.right - bounds.left + 2.0) / node.width); const ry = Math.fround((bounds.bottom - bounds.top + 2.0) / node.height);
            if (rx > 1 || ry > 1) { const maximum = rx <= ry ? ry : rx; scale = Math.fround(1.0 / maximum); }
        } else scale = node.iconScale;
        const centerX = Math.fround((node.width - (bounds.right - bounds.left) * scale) * 0.5 + 2.0); const centerY = Math.fround((node.height - (bounds.bottom - bounds.top) * scale) * 0.5 + 2.0);
        // 패턴의 현재 칸마다 호출되는 방문자이다(실제 decoder 가 반환한 한 유효 칸).
        need(hooks.walk)({ type: kind, argument: argument | 0, direction: 0, x: 0, y: 0, explicitFrame }, (cell) => {
            const px = Math.fround(centerX + cell.x * state.pixelsX * scale); const py = Math.fround(centerY + cell.y * state.pixelsY * scale); let frame = cell.frame;
            if (word(raw, 18) === state.bridgeType) {
                const bridge = need(hooks.type)(state.bridgeType);
                if (!bridge.frames || frame < 0 || frame >= bridge.frames.codes.length) throw new RangeError('생산 항목 현재 다리 프레임 코드 오류');
                const code = bridge.frames.codes[frame]; const quality = raw[30];
                const flags = ((code.flags << 24) >> 24) | (quality === 1 ? 0x20 : quality === 3 ? 0x40 : 0);
                frame = bridge.frames.findFlags(code.side, code.variant, flags);
            }
            const itemFlags = word(raw, 20); const imageMode = (itemFlags & 0x100) ? 1 : ((itemFlags & 0x418) && !this.affordable(sid)) ? 2 : 0;
            const image = need(hooks.allocate)(IMAGE_BYTES); if (!image) throw new Error('생산 항목 그림 할당 실패');
            need(hooks.image)(image, kind, frame, imageMode);
            const child = this.#base(image); child.scale = scale; child.width = Math.fround(scale * child.baseWidth); child.height = Math.fround(scale * child.baseHeight); need(hooks.attach)(image, px, py, token, 0);
            if (state.stockEnabled && (word(raw, 20) & 8)) {
                const text = need(hooks.allocate)(TEXT_BYTES); if (!text) throw new Error('생산 항목 글자 할당 실패');
                need(hooks.text)(text); need(hooks.styledText)(text, `~w~E${raw[30]}`); need(hooks.fitText)(text);
                const tx = Math.fround(px + child.width); const ty = Math.fround(py + child.height - 10.0); need(hooks.attach)(text, tx, ty, token, 0);
            }
        });
    }

    // 43cdc0: 장식만 그리며 아이콘은 자식이 그린다. 현재 화면점+3 에서 시작하고 끝점은 기본 크기+화면점을 CRT 절삭한다.
    draw(token) {
        const node = this.#node(token); if (!node.decoration) return;
        const point = need(this.#hooks.position)(token);
        need(this.#hooks.background)({ left: add(point.x, 3), top: add(point.y, 3), right: cameraFtolLow(point.x + node.baseWidth), bottom: cameraFtolLow(point.y + node.baseHeight) }, 0x100000, this.#state.backgroundType, this.#state.backgroundFrame);
    }

    // 43d4c0: 사건을 로컬 좌표로 먼저 바꾸고 현재 지식/입력 관문을 읽는다. 반복 입력은 마지막 선택과 같은 종류일 때만 선택을 전달한다. 좌/우클릭 누름만 처리하며 현재 자식 목록에서(그림 hit 요구가 있으면 그림 자식의 hit 가
    // 성공한 첫 자식) 좌클릭은 4c2790(1,0x41) 정책이 허용하면 마지막 선택을 갱신하고 선택을 전달하며 거부면 NoBridgeYet 안내를, 우클릭은 부모 전달이 0 이고 지식이 열리지 않았으면 정보 메뉴를 연다. 자식 순환은 거부한다.
    handle(token, event, first, second) {
        const node = this.#node(token); const hooks = this.#hooks; const state = this.#state;
        const point = need(hooks.local)(token, event, first, second);
        if (need(hooks.knowledgeOpen)() || state.inputDisabled) return 0;
        const code = (event >>> 0) & 0x03ffffff;
        if (code === REPEAT) {
            if (state.lastType === word(this.#pool.slot(this.#content(node.content)), 18)) return need(hooks.forward)(token, CHOOSE, node.content, node.serial);
            return 0;
        }
        if ((code !== 0x10000 && code !== 0x20000) || ((event >>> 0) & 0x80000000)) return 0;
        const visited = new Set();
        // 현재 자식 목록에서 실제 이미지 hit 를 요구하며 실패 뒤에는 현재 next 를 읽는다.
        for (let child = node.child; child; child = this.#base(child).next) {
            if (visited.has(child)) throw new Error('생산 항목 자식 순환');
            visited.add(child); const current = this.#base(child);
            if (node.requireImageHit && (current.command !== 3 || !need(hooks.imageHit)(child, point[0], point[1]))) continue;
            if (code === 0x10000) {
                if (!need(hooks.denied)(1, 0x41)) { state.lastType = word(this.#pool.slot(this.#content(node.content)), 18); return need(hooks.forward)(token, CHOOSE, node.content, node.serial); }
                need(hooks.tell)('NoBridgeYet');
            } else if (!need(hooks.forward)(token, RIGHT, node.content, node.serial) && !need(hooks.knowledgeOpen)()) return need(hooks.about)(token, first, second);
            return 1;
        }
        return 0;
    }
}

// 현재 실제 자산의 Gump 프레임·코드, 픽셀 측정과 원본 Canon 순회만 항목에 연결한다.
export function bindInventoryItemAssets(hooks, assets, geometry, measure) {
    if (assets?.typeTable?.definitions?.length !== 116 || measure?.pool?.edition !== 'patch1078' || geometry?.frames?.length !== 188 || !geometry.geometry) throw new TypeError('생산 항목 실제 자산 판본 오류');
    // 내장 타입은 Gump 프레임 0이며 그림 타입은 같은 현재 자산 정의를 읽는다.
    hooks.type = (type) => {
        type >>>= 0; if (type >= geometry.frames.length) throw new RangeError('생산 항목 실제 TYPE 프레임 범위 오류');
        const frame = type < 70 ? 0 : assets.types()[type - 70].definition.gumpFrame;
        return { gumpFrame: frame, frames: new FrameCodeTable(geometry.frames[type].frames) };
    };
    // 전체 패턴의 실제 SHP 픽셀 경계를 같은 측정기에서 얻는다.
    hooks.measure = (type, argument, explicitFrame) => measure.measure(type, argument, 0, explicitFrame);
    // 현재 타입의 원본 Canon 유효 칸을 원래 순서대로 모두 방문한다.
    hooks.walk = (query, visit) => {
        const type = query.type >>> 0; if (type >= geometry.frames.length) throw new RangeError('생산 항목 실제 decoder 타입 오류');
        const meta = geometry.frames[type], current = geometry.geometry;
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame, current.patternTypes, query, current.decoderDiagnostics);
        while (decoder.valid) { visit({ frame: decoder.frame, x: decoder.x, y: decoder.y }); decoder.advance(); }
    };
}
