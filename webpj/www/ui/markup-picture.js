// StyleText와 같은 토큰/소유 표의 그림 조각·현재 메타/배율·실제 자산/픽셀을 이식한다(cpppj client/Markup.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E420)
import { StyleTextFragment, StyleTextFragmentKind as K, registerStyleTextFragment } from './style-text-fragments.js';
import { buildTypeRecords } from '../assets/type-flags.js';

// 원문 VFX 배율은16.16이며 기본 float 배율은1이다.
const FIXED_SCALE = 65536;
// 도달한 현재 자산/가상 장치만 요구한다.
function need(fn) { if (typeof fn !== 'function') throw new TypeError('Markup 선택 가상/장치 경계 필요'); return fn; }
// signed64 CRT 절삭의 하위 signedDWORD다. 무효 정수 결과의 하위값은0이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 원문 signed WORD와 DWORD 합의 하위 비트다.
const word = (value) => value << 16 >> 16;
const add = (first, second) => (first + second) | 0;
// 무자산 Lookup도 실제 여섯 WORD0을 반환한다.
const emptyFrame = () => ({ width: 0, height: 0, hotspotX: 0, hotspotY: 0, extraX: 0, extraY: 0 });

// 공통 next/metadata는 StyleTextState에 있고 이 표에는 실제 파생 필드만 둔다.
export class MarkupPicture {
    // 현재 원문 타입/프레임 DWORD다.
    type = 0; frame = 0;
    // float32 배율과 전체 하위 색 치환 BYTE다.
    scale = 1; mapped = 0;
}
// 현재 signed 타입 수와 안정된 파생 그림 표다.
export class MarkupState {
    // typeCount는 bool/자산 수로 좁히지 않는다.
    typeCount = 0;
    // 공통 조각과 동일 토큰을 사용하는 실제 파생 저장소다.
    pictures = new Map();
}

// 같은 공통 조각과 현재 그림/자산/출력 장치를 빌린다.
export class Markup {
    // 장기 공통/파생 표와 실제 현재 조회/색/출력 경계다.
    #fragments; #state; #hooks;
    // 생성과 색 BYTE 설정에는 자산 장치를 요구하지 않는다.
    constructor(fragments, state, hooks = {}) { this.#fragments = fragments; this.#state = state; this.#hooks = hooks; }
    // 공통 토큰을 다른 Gump 등록 ID나 메모리 주소로 해석하지 않는다.
    #fragment(token) { token >>>= 0; const node = token ? this.#fragments.fragments.get(token) : null; if (!node) throw new RangeError('Markup 공통 조각 저장소 없음'); return node; }
    // Base로 소멸한 그림의 직접 파생 호출은 거부한다.
    #picture(token) { const picture = this.#state.pictures.get(token >>> 0); if (!picture || this.#fragment(token).kind !== K.EXTERNAL) throw new RangeError('Markup 그림 조각 저장소 없음'); return picture; }
    // 두 번째 표 등록 실패는 새 공통 조각만 지운다. 이미 증가한 자동 토큰은 되돌리지 않는다.
    newPicture(type, frame, metadata = 0) { const fragment = Object.assign(new StyleTextFragment(), { kind: K.EXTERNAL, metadata: metadata >>> 0 }); const token = registerStyleTextFragment(this.#fragments, fragment); try { if (this.#state.pictures.has(token)) throw new Error('Markup 파생 토큰 중복'); this.#state.pictures.set(token, Object.assign(new MarkupPicture(), { type: type >>> 0, frame: frame >>> 0 })); } catch (error) { this.#fragments.fragments.delete(token); throw error; } return token; }
    // 복제는 type/frame/metadata만 복사하고 scale/mapped는1/0으로 새로 만든다.
    clonePicture(token) { const picture = this.#picture(token); return this.newPicture(picture.type, picture.frame, this.#fragment(token).metadata); }
    // 그림 필드/next를 비우지 않고 공통 가상 분류만 Base로 전환한다.
    destruct(token) { this.#picture(token); this.#fragment(token).kind = K.BASE; }
    // 공통 scalar 반납 직전에 그림 표만 정리한다. 그림 외 토큰은 무동작이다.
    release(token) { this.#state.pictures.delete(token >>> 0); }
    // Base로 전환한 그림도 실제 반납 전까지 이 표에 남아 있다.
    contains(token) { return this.#state.pictures.has(token >>> 0); }
    // 원문 하위BYTE를 보존하며 bool로 좁히지 않는다.
    setMapped(token, mapped) { this.#picture(token).mapped = mapped & 255; }
    // type를 먼저 저장하고 타입 조회 뒤 현재 frame·헤더 조회 뒤 현재 타입 기준점/SHP를 읽는다.
    lookup(token) { const type = this.#picture(token).type | 0; const result = { type: 0, shape: 0, frame: emptyFrame() }; if (type <= 0 || type >= (this.#state.typeCount | 0)) return result; const view = need(this.#hooks.type)(type >>> 0); if (!view || !view.shape) return result; const index = this.#picture(token).frame | 0; if (index < 0 || index >= (view.frameCount | 0)) return result; const header = need(this.#hooks.frame)(view.token, 0, index >>> 0); result.frame = { width: word(header.width), height: word(header.height), hotspotX: word(header.hotspotX), hotspotY: word(header.hotspotY), extraX: word(header.extraX), extraY: word(header.extraY) }; result.frame.hotspotX = word(result.frame.hotspotX - (view.hotspotX & 65535)); result.frame.hotspotY = word(result.frame.hotspotY - (view.hotspotY & 65535)); result.type = type >>> 0; result.shape = view.shape >>> 0; return result; }
    // 세로 비율을 먼저 float32로 저장하고 선택 비율이1 미만일 때만 기존 배율을 갱신한다.
    fit(token, width, height) { const frame = this.lookup(token).frame; const widthRatio = (width | 0) / frame.width; const heightRatio = Math.fround((height | 0) / frame.height); const selected = widthRatio < heightRatio ? widthRatio : heightRatio; if (selected < 1) this.#picture(token).scale = Math.fround(selected); }
    // 배율1에서는 signed WORD 그대로이며 높이와 보정된 y 기준점을 반환한다.
    baseline(token) { const frame = this.lookup(token).frame; const scale = this.#picture(token).scale; return scale === 1 ? [frame.height, frame.hotspotY] : [ftol(frame.height * scale), ftol(frame.hotspotY * scale)]; }
    // 저장 문맥 대신 현재 그림의 폭/높이와 배율을 사용한다.
    measure(token) { const frame = this.lookup(token).frame; const scale = this.#picture(token).scale; return scale === 1 ? [frame.width, frame.height] : [ftol(frame.width * scale), ftol(frame.height * scale)]; }
    // 이미 계산한 위치/배율/SHP·mapped를 보존하고 색 설치 후의 현재 frame만 다시 읽는다.
    draw(token, position) { const view = this.lookup(token); if (!view.shape) return; const scale = this.#picture(token).scale; const scaled = scale !== 1; const x = position.x | 0; const y = position.y | 0; const request = { type: view.type, shape: view.shape, frame: 0, position: scaled ? { x: ftol(view.frame.hotspotX * scale + x), y: ftol(view.frame.hotspotY * scale + y) } : { x: add(x, view.frame.hotspotX), y: add(y, view.frame.hotspotY) }, mapped: this.#picture(token).mapped !== 0, scaled, angle: 0, scaleX: scaled ? ftol(scale * FIXED_SCALE) : 0, scaleY: scaled ? ftol(scale * FIXED_SCALE) : 0 }; if (request.mapped) need(this.#hooks.color)(); request.frame = this.#picture(token).frame >>> 0; need(this.#hooks.shape)(request); }
}

// 그림 토큰의 가상 동작/반납만 연결하며 다른 외부 조각의 기존 분배를 각각 유지한다.
export function bindMarkup(hooks, markup) { const metrics = hooks.externalMetrics; const measure = hooks.externalMeasure; const draw = hooks.externalDraw; const destroy = hooks.externalDestroy; const release = hooks.release;
    // 전체 StyleText baseline/측정에서 같은 파생 그림 저장소를 조회한다.
    hooks.externalMetrics = (token) => markup.contains(token) ? markup.baseline(token) : need(metrics)(token);
    hooks.externalMeasure = (token) => markup.contains(token) ? markup.measure(token) : need(measure)(token);
    // 그림은 텍스트 override를 사용하지 않으며 다른 외부 조각에는 그대로 전달한다.
    hooks.externalDraw = (token, position, context) => { if (markup.contains(token)) markup.draw(token, position); else need(draw)(token, position, context); };
    // 원문 Base 전환 뒤의 공통 scalar 반납은 StyleText.Destroy가 수행한다.
    hooks.externalDestroy = (token, flags) => { if (markup.contains(token)) markup.destruct(token); else need(destroy)(token, flags); };
    // 기존 반납 관찰에서 아직 남아 있는 그림 필드를 볼 수 있도록 먼저 호출한다.
    hooks.release = (token) => { if (release) release(token); markup.release(token); };
}

// 실제 GameAssets의188타입/논리 클러스터·타입 기준점과 물리 SHP를 연결한다.
export class MarkupAssets {
    // 장기 실제 ShapeDatabase와 안정된 타입/블록 표·signed 타입 수다.
    #shapes; #views = new Map(); #typeCount;
    // 공통 레코드의 같은 기준점과 원문 자산 로딩 순서를 보존한다.
    constructor(assets) { this.#shapes = assets.shapes; const types = buildTypeRecords(assets.typeTable.definitions); this.#typeCount = types.length; if (types.length > 0x7fffffff) throw new RangeError('Markup 타입 수 범위 초과');
        // 자산 없는 내장 타입은 views에 등록하지 않는다.
        for (const asset of assets.types()) { const token = asset.block + 70; const count = asset.definition.clusters.length; if (token >= types.length || asset.block >= this.#shapes.blocks.length || count > 0x7fffffff) throw new RangeError('Markup 실제 자산 바인딩 범위 오류'); this.#views.set(token, { type: Object.freeze({ token, shape: token, frameCount: count, hotspotX: types[token].hotspotX | 0, hotspotY: types[token].hotspotY | 0 }), block: asset.block }); }
    }
    // 자산 수116과 구별하는 실제 원문 타입 수다.
    typeCount() { return this.#typeCount; }
    // 내장/무자산은 null이며 실제 타입 주소는 안정된다.
    type(token) { return this.#views.get(token >>> 0)?.type ?? null; }
    // abstract0의 논리 프레임과 실제 여섯 signed WORD만 공급한다.
    frame(type, abstract, frame) { type >>>= 0; frame >>>= 0; const view = this.type(type); if (abstract || !view || frame >= view.frameCount) throw new RangeError('Markup 논리 프레임 범위 오류'); const header = this.#shapes.squidHeader(this.#views.get(type).block, frame); const words = new DataView(header.buffer, header.byteOffset, header.byteLength); return { width: words.getInt16(0x18, true), height: words.getInt16(0x1a, true), hotspotX: words.getInt16(0x1c, true), hotspotY: words.getInt16(0x1e, true), extraX: words.getInt16(0x20, true), extraY: words.getInt16(0x22, true) }; }
    // 색 설치 후 바뀐 frame도 실제 물리 SHP 범위로 다시 검사한다.
    shape(shape, frame) { const view = this.#views.get(shape >>> 0); frame >>>= 0; if (!view || frame >= this.#shapes.blocks[view.block].frames.length) throw new RangeError('Markup VFX 물리 프레임 범위 오류'); return { database: this.#shapes, block: view.block }; }
}
// 같은 현재 타입 수와 두 실제 자산 조회 장치를 설치한다.
export function bindMarkupAssets(state, hooks, assets) { state.typeCount = assets.typeCount(); hooks.type = (type) => assets.type(type); hooks.frame = (type, abstract, frame) => assets.frame(type, abstract, frame); }

// 실제 현재 클립/공유 색 표·원색/치환/직교 배율을 같은 GumpPixelTarget에 출력한다.
export class MarkupPixelTarget {
    // 장기 실제 표면/자산·공유 표/그림 표와 해독 이미지 캐시다.
    #target; #assets; #colors; #pictureColors; #images = new WeakMap();
    // 두 표 주소를 유지하며 color 설치 시 현재 내용을 복사한다.
    constructor(target, assets, currentColors, pictureColors) { if (currentColors.length !== 256 || pictureColors.length !== 256) throw new TypeError('Markup 그림/현재 색표256칸 필요'); this.#target = target; this.#assets = assets; this.#colors = currentColors; this.#pictureColors = pictureColors; }
    // 일반/angle0 배율은 실제 표면에 연결하고 외부 회전 분배는 보존한다.
    hooks(hooks = {}) { const rotated = hooks.shape; return { ...hooks, color: () => this.#colors.set(this.#pictureColors), shape: (request) => { if (!request.scaled || request.angle === 0) this.#image(request); else need(rotated)(request); } }; }
    // 특별 빈 프레임은 출력하지 않고 현재 물리 프레임·그림 상대점/16.16 배율을 보존한다.
    #image(request) { const { database, block } = this.#assets.shape(request.shape, request.frame); const frame = database.blocks[block].frames[request.frame]; if (frame.special) return; let cache = this.#images.get(database); if (!cache) { cache = new Map(); this.#images.set(database, cache); } const key = `${block}:${request.frame}`; let image = cache.get(key); if (!image) { image = database.decode(block, request.frame); cache.set(key, image); } const colors = request.mapped ? this.#colors : null; if (request.scaled) this.#target.scaledImage(image, frame, request.position, request.scaleX, request.scaleY, colors); else this.#target.image(image, { x: add(request.position.x, frame.rect.left), y: add(request.position.y, frame.rect.top) }, colors); }
}
