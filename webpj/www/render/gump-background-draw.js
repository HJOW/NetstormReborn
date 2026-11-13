// 배경의 실제 생성 자산/현재 pass/clip·색·픽셀을 같은 저장소에 연결한다(cpppj client/GumpBackgroundDraw.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E415)
import { clipRect } from './indexed-surface.js';
import { GumpBackgroundFlags as F, planGumpTexture } from './gump-visual.js';
import { setGumpDrawClip } from './gump-draw.js';
import { buildTypeRecords } from '../assets/type-flags.js';

// 실제 도달한 자산/출력 경계만 요구한다.
function need(fn) { if (typeof fn !== 'function') throw new TypeError('배경 Draw 출력/자산 경계 필요'); return fn; }
// 원문 DWORD 좌표/색 덧셈이다.
const add = (first, second) => (first + second) | 0;
// 초기 clip 설정 실패와 출력 예외 모두 원래 clip을 복구한다. 색/픽셀은 복구하지 않는다.
function textureClip(set, saved, next, body) { set = need(set); try { set({ ...next }); } catch (error) { set({ ...saved }); throw error; } try { return body(); } finally { set({ ...saved }); } }

// 현재 디버그와 마지막 파일 팔레트의 가까운/먼 선 색이다.
export class GumpBackgroundDrawState {
    // debug는 체크 fill 경로이며 debugColor는 출력 전에 DWORD로 증가한다.
    debug = false; debugColor = 0;
    // 단색/체크/무배경의1픽셀 선 명명색이다.
    nearColor = 0; farColor = 0;
}
// 일시 팔레트에서 재검색하지 않고 마지막 파일의 흰색/회색 번호를 연결한다.
export function setGumpBackgroundColors(state, palette) { state.nearColor = palette.basic[7]; state.farColor = palette.basic[8]; }

// 같은 Position/Bounds·half-open/VFX clip과 현재 출력 장치를 빌린다.
export class GumpBackgroundDraw {
    // 현재 위치/clip/배경 전역과 실제 가상 경계다.
    #collection; #clip; #state; #hooks;
    // node만 생성 때 필수이며 단색/체크는 자산 없이 출력할 수 있다.
    constructor(collection, clip, state, hooks = {}) { need(hooks.node); this.#collection = collection; this.#clip = clip; this.#state = state; this.#hooks = hooks; }
    // Bounds 뒤 현재 debug/파생 필드를 읽는다. debug 증가는 fill보다 먼저다.
    draw(token) { const rect = this.#collection.bounds(token); if (this.#state.debug) { const color = this.#state.debugColor >>> 0; this.#state.debugColor = (color + 1) >>> 0; need(this.#hooks.fill)(rect, color, true); return; } const node = need(this.#hooks.node)(token); if (!node) throw new Error('배경 Draw 토큰/저장소 없음'); this.background(rect, node.backgroundFlags, node.type, node.frame); }
    // 체크→단색→질감 우선순위다. 평면의 왼쪽/위/오른쪽/아래 색은 각 호출 때 다시 읽는다.
    background(rect, flags, type, frame) {
        if (flags & F.CHECKER) need(this.#hooks.fill)(rect, flags & 255, true); else if (flags & F.SOLID) need(this.#hooks.fill)(rect, flags & 255, false);
        else if (flags & F.TEXTURE) {
            this.texture(rect, flags, type, frame); const saved = { ...this.#clip.clip }; const near = flags & F.PRESSED ? F.DARK : F.BRIGHT; const far = flags & F.PRESSED ? F.BRIGHT : F.DARK;
            if (flags & F.LEFT) this.texture(clipRect({ left: rect.left, top: rect.top, right: add(rect.left, 2), bottom: rect.bottom }, this.#clip.clip), flags | near, type, frame);
            if (flags & F.RIGHT) this.texture(clipRect({ left: add(rect.right, -2), top: rect.top, right: rect.right, bottom: rect.bottom }, saved), flags | far, type, frame);
            if (flags & F.TOP) this.texture(clipRect({ left: rect.left, top: rect.top, right: rect.right, bottom: add(rect.top, 2) }, saved), flags | near, type, frame);
            if (flags & F.BOTTOM) this.texture(clipRect({ left: rect.left, top: add(rect.bottom, -2), right: rect.right, bottom: rect.bottom }, saved), flags | far, type, frame); return;
        }
        if (flags & F.LEFT) need(this.#hooks.line)({ first: { x: rect.left, y: rect.top }, last: { x: rect.left, y: add(rect.bottom, -1) }, color: this.#state.nearColor });
        if (flags & F.TOP) need(this.#hooks.line)({ first: { x: rect.left, y: rect.top }, last: { x: add(rect.right, -1), y: rect.top }, color: this.#state.nearColor });
        if (flags & F.RIGHT) need(this.#hooks.line)({ first: { x: add(rect.right, -1), y: rect.top }, last: { x: add(rect.right, -1), y: add(rect.bottom, -1) }, color: this.#state.farColor });
        if (flags & F.BOTTOM) need(this.#hooks.line)({ first: { x: rect.left, y: add(rect.bottom, -1) }, last: { x: add(rect.right, -1), y: add(rect.bottom, -1) }, color: this.#state.farColor });
    }
    // 자산 조회 뒤 현재 clip을 저장한다. 유효하지 않은 타일은 clip/색/shape 장치를 읽지 않는다.
    texture(rect, flags, type, frame) {
        const metrics = need(this.#hooks.frame)(type, frame); const saved = { ...this.#clip.clip }; const width = metrics.width << 16 >> 16; const height = metrics.height << 16 >> 16; const plan = planGumpTexture(rect, saved, width, height, flags); if (!plan.validTile) return; const bodyStart = this.#hooks.repeatBody ? this.#hooks.repeatBody(type, frame) : 0; const firstPeriod = height - 1; const bodyRows = firstPeriod - bodyStart; const extra = clipRect(plan.clip, { left: plan.clip.left, top: firstPeriod, right: plan.clip.right, bottom: plan.clip.bottom });
        if (bodyStart > 0 && bodyRows > 0 && extra.left < extra.right && extra.top < extra.bottom) {
            const legacy = { left: plan.clip.left, top: plan.clip.top, right: plan.clip.right, bottom: Math.min(plan.clip.bottom, firstPeriod) };
            if (legacy.top < legacy.bottom) textureClip(this.#hooks.clip, saved, legacy, () => { if (plan.shade) need(this.#hooks.shade)(plan.shade); for (const point of plan.origins) need(this.#hooks.shape)({ type, frame, position: point, mapped: plan.shade !== 0 }); });
            if (!plan.origins.length) return; const firstY = plan.origins[0].y; const first = firstPeriod + Math.trunc((extra.top - firstPeriod) / bodyRows) * bodyRows; const columns = plan.origins.filter((p) => p.y === firstY).length; const rows = Math.trunc((extra.bottom - first + bodyRows - 1) / bodyRows); if (columns * rows > 1000000) throw new RangeError('와이드 패널 본문 반복 범위 초과');
            // 첫 원문 타일 뒤 실제 본문만 반복한다. 가로 원문 위상은 유지한다.
            for (let top = first; top < extra.bottom; top += bodyRows) { const cut = { left: extra.left, top: Math.max(extra.top, top) | 0, right: extra.right, bottom: Math.min(extra.bottom, top + bodyRows) | 0 }; textureClip(this.#hooks.clip, saved, cut, () => { if (plan.shade) need(this.#hooks.shade)(plan.shade); for (const point of plan.origins) if (point.y === firstY) need(this.#hooks.shape)({ type, frame, position: { x: point.x, y: (top - bodyStart) | 0 }, mapped: plan.shade !== 0 }); }); }
            return;
        }
        textureClip(this.#hooks.clip, saved, plan.clip, () => { if (plan.shade) need(this.#hooks.shade)(plan.shade); for (const point of plan.origins) need(this.#hooks.shape)({ type, frame, position: point, mapped: plan.shade !== 0 }); });
    }
}

// 같은 배경만 가상 Draw로 보내며 기존 파생 처리기를 보존한다.
export function bindGumpBackgroundDraw(hooks, draw, isBackground) { if (typeof isBackground !== 'function') throw new TypeError('배경 Draw 분배 식별기 필요'); const previous = hooks.draw; hooks.draw = (token) => { if (isBackground(token)) draw.draw(token); else need(previous)(token); }; }

// 실제 GameAssets 또는 명시 SHP 바인딩을 같은 헤더/프레임 검증에 연결한다.
export class GumpBackgroundAssets {
    // 장기 실제 ShapeDatabase와 안정된 타입/물리 메타 표다.
    #shapes; #views = new Map();
    // 사용: new GumpBackgroundAssets(gameAssets) 또는 (shapeDatabase,bindings).
    constructor(source, bindings = null) {
        this.#shapes = bindings === null ? source.shapes : source;
        if (bindings !== null) { for (const binding of bindings) this.#add(binding); return; }
        const records = buildTypeRecords(source.typeTable.definitions); const panel = source.find('fortGump').block;
        // 내장 무자산 타입은 제외하고 원본70번부터의 로딩 순서를 유지한다.
        for (const asset of source.types()) { const type = asset.block + 70; this.#add({ type, block: asset.block, frameCount: asset.definition.clusters.length, flags: records[type].flags1, repeatFrame: asset.block === panel ? 2 : -1 }); }
    }
    // 메타는 자체 표에 복사하며 VFX 픽셀은 장기 DB에서 읽는다.
    #add(binding) {
        const type = binding.type >>> 0; const count = binding.frameCount | 0; const block = binding.block; const source = this.#shapes.blocks[block]; if (!type || this.#views.has(type) || !source || count < 1 || count > source.frames.length) throw new TypeError('배경 자산 바인딩 오류'); const frames = [];
        // 공유/추가 레이어를 포함한 모든 물리 프레임의 signed WORD를 보존한다.
        for (let i = 0; i < source.frames.length; i += 1) { const m = this.#shapes.squidMetrics(block, i); frames.push(Object.freeze({ width: m.width << 16 >> 16, height: m.height << 16 >> 16 })); }
        const view = { type: Object.freeze({ token: type, shape: type }), block, count, flags: binding.flags >>> 0, frames, repeatFrame: -1, repeatBodyStart: 0 };
        if (binding.repeatFrame >= 0) {
            const index = binding.repeatFrame | 0; if (index >= count) throw new TypeError('와이드 패널의 실제 논리 프레임 범위 오류'); const frame = source.frames[index]; const image = this.#shapes.decode(block, index); const m = frames[index];
            if (!frame.special && frame.rect.left === 0 && frame.rect.top === 0 && image.width === m.width - 1 && image.height === m.height - 1) {
                let row = 0;
                // 명시 패널에서만 연속 불투명 색0 행을 실제 RLE 결과로 센다.
                for (; row < image.height; row += 1) { let black = true; for (let x = 0; x < image.width; x += 1) if (image.indices[row * image.width + x] !== 0 || image.opacity[row * image.width + x] !== 255) { black = false; break; } if (!black) break; }
                if (row > 0 && row < image.height) { view.repeatFrame = index; view.repeatBodyStart = row; }
            }
        }
        this.#views.set(type, view);
    }
    // 안정된 타입 토큰만 제공하며 내장/없는 타입은 거부한다.
    type(type) { const view = this.#views.get(type >>> 0); if (!view) throw new RangeError('배경 자산 타입 없음'); return view.type; }
    // 논리 수/추가 레이어 허용 비트와 실제 물리 헤더를 함께 검사한다.
    frame(type, abstract, frame) { if (abstract) throw new TypeError('배경 자산 abstract 인자는0이어야 함'); this.type(type); const view = this.#views.get(type >>> 0); frame >>>= 0; const maximum = view.count * ((view.flags & 0x440000) ? 2 : 1); if (frame >= maximum || frame >= view.frames.length) throw new RangeError('배경 SHP 프레임 범위 오류'); return view.frames[frame]; }
    // 각 실제 출력에서 프레임을 다시 검사하며 생성 노드의 이전 shape 필드는 읽지 않는다.
    shape(type, frame) { this.frame(type, 0, frame); return { database: this.#shapes, block: this.#views.get(type >>> 0).block }; }
    // 명시한 실제 패널/프레임만 선두 행 제외 정책을 사용한다.
    repeatBodyStart(type, frame) { this.frame(type, 0, frame); const view = this.#views.get(type >>> 0); return view.repeatFrame >= 0 && (frame >>> 0) === view.repeatFrame ? view.repeatBodyStart : 0; }
}

// 생성과 각 pass의 현재 헤더를 같은 자산 어댑터에서 공급한다.
export function bindGumpBackgroundAssets(create, draw, assets) { create.type = (type) => assets.type(type); create.frame = (type, abstract, frame) => assets.frame(type, abstract, frame); draw.frame = (type, frame) => assets.frame(type, 0, frame); draw.repeatBody = (type, frame) => assets.repeatBodyStart(type, frame); }

// 현재 GumpPixelTarget/같은 VFX 색 표에 실제 배경을 출력한다.
export class GumpBackgroundPixelTarget {
    // 장기 표면/자산/명암 표/공통 색과 해독 그림 캐시다.
    #target; #assets; #maps; #colors; #images = new WeakMap();
    // 색 표 객체는 글리프와 공유할 수 있어 그 주소를 바꾸지 않는다.
    constructor(target, assets, maps, colors) { if (colors.length !== 256) throw new TypeError('배경 현재 색표256칸 필요'); this.#target = target; this.#assets = assets; this.#maps = maps; this.#colors = colors; }
    // 규칙/자산 조회는 보존하고 실제 출력만 같은 표면에 연결한다.
    hooks(state, hooks = {}) { return { ...hooks, clip: (rect) => setGumpDrawClip(state, rect), shade: (shade) => { if (shade !== 1 && shade !== 2) throw new TypeError('배경 명암 표 선택 오류'); this.#colors.set(shade === 1 ? this.#maps.bright : this.#maps.dark); }, shape: (request) => this.#shape(request), fill: (rect, color, checker) => this.#target.fill(rect, color, checker), line: (line) => this.#target.lines([line]) }; }
    // 특별 빈 프레임은 쓰지 않고 명암은 소스 색만 치환한다.
    #shape(request) { const { database, block } = this.#assets.shape(request.type, request.frame); const frame = database.blocks[block].frames[request.frame]; if (frame.special) return; let cache = this.#images.get(database); if (!cache) { cache = new Map(); this.#images.set(database, cache); } const key = `${block}:${request.frame}`; let image = cache.get(key); if (!image) { image = database.decode(block, request.frame); cache.set(key, image); } this.#target.image(image, { x: add(request.position.x, frame.rect.left), y: add(request.position.y, frame.rect.top) }, request.mapped ? this.#colors : null); }
}
