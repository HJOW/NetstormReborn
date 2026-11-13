// 원본 일반 Squid 객체 Draw(10.78 00498220 — 공통 표시 목록의 kind 0 항목과 kind 1 묶음 원소)를 옮긴 모듈이다: 본체·그림자·가상 출력·표식·선택 선·상태 막대(cpppj `client/SpriteDraw.cpp/.h`, 10.78 전용. CD 00455f60 경로는 범위 밖).
//  - SpriteDraw.draw(entry): 항목 토큰(`tokenBase + SID × 50`)을 같은 raw 풀 SID 로 되돌리고(0·stride 배수 아님·용량 밖·TYPE 표 밖은 거부), 그림이 로드되지 않은 타입이면 아무것도 하지 않는다. raw 의 부호 있는 화면 WORD 와 월드 투영(x×16+0.5·y×11+0.5 x87 절삭)이
//    다르면 원본 assert 에 해당하는 계약 오류를 낸다. 화면 기준점은 화면 위치에서 TYPE 기준점을 뺀 값이다.
//      tail 비트 4(그림자 레이어): abstract(extra 0x10)면 끝, 패치판은 진단 전역(공유면 정확히 0, 독립이면 alias 고정 안 함)일 때 그림자 alias 타입을 현재 타입으로 기록한다. 프레임이 논리 프레임 수 이상이면 한 번 빼고, 특수 모드이거나 죽은 객체(extra 1)면 끝, 아니면 `프레임 + 논리 프레임 수` 를 그림자 모드로 낸다.
//      그 밖(본체): extra 비트 2 가 없으면 죽음→죽음 그림, abstract→로컬 소유자일 때 abstract 그림, 특수 모드이고 필터에 걸리면 abstract 그림, 아니면 일반 그림(소유자 변환표가 있고 소유자가 0 이 아니면 소유자 색 문맥 뒤 소유자 그림, 아니면 원색).
//        extra 비트 2 가 있으면 가상 프레임 조회(-1 이면 기본 프레임) 뒤 현재 필터로 같은 갈래를 고르고 가상 Draw 를 호출한다. 이어서 abstract 가 아니면 표식(배너 옵션·TYPE flags1 0x400·표식 표시·소유자·0단계 표면 SID 의 TYPE 이 제외 타입이 아닐 때 소유자 팔레트 − 1 프레임)과 상태 표시(extra 0x20)를 낸다.
//      마지막으로 extra 비트 4(현재 슬롯을 다시 읽음, 가상 Draw 가 바꿀 수 있음)이고 선택이 꺼져 있지 않으면 선택 헤더 두 겹 선 + 상태 막대들(HP·건설·자원·타이머 셋·abstract/상태 지속·비용)을 낸다.
//  - 선택 막대는 `bar(색, 현재, 최대)` 가 배경 fill 과 본체 fill 을 낸다(행마다 3픽셀 위로 쌓음). HP 는 E377 `readSpriteHitPointBar` 로 읽는다. 지속 막대는 실제 Kernel 순서의 상태 프로세스(0번 슬롯 제외, 첫 SID 일치에서 멈춤, flag 1 이 2 보다 우선)에서 남은 시간을 읽는다.
//  - SpritePixelTarget: 규칙 조회/가상 Draw 훅은 그대로 두고 픽셀 출력(setOwnerColors/shape/line/fill)만 현재 8비트 표면에 연결한다 — 원색은 그대로, 소유자는 소스 색 치환(현재 색 표), 죽음·그림자·밝게(160%)·abstract 는 고정 대상 변환표(translated 커널), scaled 요청은 회전 없는 VFX 배율 출력.
//    선택 선은 양 끝 포함 직교 선분(대각선 거부), fill 은 끝 좌표 제외이며 빈/역전 사각형은 아무것도 바꾸지 않는다.
//  - makeSpriteAssetViews(assets): 실제 자산의 SHP 추가 헤더/프레임 수와 .type 의 기본 프레임·casttime·praytime·effecttime 을 로딩 순서 그대로 준비한다. makeSpriteAssetShapes(assets, state): 그림자 alias 타입을 현재 값으로 SHP 블록에 연결.
//    makeSpriteSurfaceLookup(hash): 표식용 +0.9999 float 상수와 CRT 절삭으로 0단계 표면 SID 를 찾는다(경계 밖 0). bindSpriteDraw(hooks, draw): 공통 분배의 kind 0 과 묶음 내부 항목을 같은 Draw 로 연결.
// 상태(`SpriteDrawState`)의 공유 전역(sharedPatchDiagnostic·sharedUseBanners)은 `{ value }` 상자이며 독립 입력만 지역 초기값(shadowAliasPinned·markerEnabled)을 쓴다. 가상 Draw 가 바꾸는 raw 는 풀의 살아 있는 보기(`pool.physical`)로 다시 읽는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E388: 5-A 타입별 Draw로 추가, 기준선 뒤 cpppj 변경 없음)
import { clipRect, drawIndexedImage } from './indexed-surface.js';
import { drawScaledVfxImage, drawTranslatedVfxImage } from './vfx-transform.js';
import { squidRendererShapes } from './squid-renderer.js';
import { readSpriteHitPointBar } from '../core/sprite-hit-point-bar.js';
import { FIRST_ASSET_TYPE_NUMBER, typeNumber } from '../assets/type-table.js';

// VFX 호출의 원색·현재 소유자 표·명시 변환 표를 구분하는 모드이다(fixture 의 숫자 그대로). 밝게(160%)는 555e38 의 1.6배 명도표로 소스 마스크 아래 대상 화면을 치환한다. (2026-11-13 WB04-E388)
export const SpriteShapeMode = Object.freeze({ plain: 0, owner: 1, abstract: 2, dead: 3, shadow: 4, bright160: 5 });
// 풀 레이아웃(10.78): SID 슬롯 50바이트, 토큰 범위 검사의 32비트 공간이다. (2026-11-13 WB04-E388)
const STRIDE = 50;
const TOKEN_SPACE = 0x100000000;
// 항목 안의 토큰/tail 위치와 tail 의 그림자 비트이다. (2026-11-13 WB04-E388)
const ENTRY_TOKEN = 8;
const ENTRY_TAIL = 15;
const SHADOW_LAYER = 4;

// Draw 가 읽는 원본 전역의 현재 값이다. 선택 SID·그림자 alias 만 쓰기 대상이다. 사용: new SpriteDrawState({ shadowAliasType: 69 }). (2026-11-13 WB04-E388)
export class SpriteDrawState {
    // 화면 원점, 로컬 플레이어·특수 모드·필터 플래그, 넓은 소유자/필터 제외/그림자 alias 타입, 그림자 alias 고정·1/4 HP·선택 끔·표식 켬/표시 여부이다.
    cameraX = 0;
    cameraY = 0;
    localPlayer = 0;
    specialMode = 0;
    filteredFlags = 0;
    wideOwnerType = 0;
    unfilteredType = 0;
    shadowAliasType = 0;
    shadowAliasPinned = false;
    quarterHitPoints = false;
    selectionDisabled = false;
    markerEnabled = false;
    markerVisible = false;
    // 표식 제외/표식/상태/abstract 상태/타이머 프로세스/상태 프로세스/비용 타입 번호와 타이머 타입 셋(20/60/60 최대, 색 0xd9/9/0x6b 순서)이다.
    markerExcludeType = 0;
    markerType = 0;
    statusType = 0;
    abstractStatusType = 0;
    timerProcessType = 0;
    statusProcessType = 0;
    costType = 0;
    timerTypes = [0, 0, 0];
    // 타입별 소유자 변환표 존재 여부(독립 입력)와 실제 557940 의 현재 표(우선), 실제 5319c0 배너 옵션 상자, 소유자 팔레트·선택 색·정상/절반/1/4 HP 색이다.
    ownerMaps = [];
    sharedOwnerMaps = [];
    sharedUseBanners = null;
    ownerPalettes = [];
    selectionColors = [];
    hitPointColors = [0, 0, 0];
    // 막대 배경 색, 선택된 SID(쓰기), 그림자 alias 의 현재 그림 타입(없으면 null), 현재 시각, 상태 프로세스 목록, 실제 005318ec 진단 상자(없으면 독립 입력)이다.
    barBackground = 0;
    selectedSid = 0;
    shadowShapeType = null;
    now = 0;
    processes = [];
    sharedPatchDiagnostic = null;

    // 일부 필드를 덮어쓴다.
    constructor(init = {}) { Object.assign(this, init); }
}

// 실행한 분기의 필수 조회/출력이 빠지면 조용히 그림을 생략하지 않는다. (2026-11-13 WB04-E388)
function required(hook) {
    if (typeof hook !== 'function') throw new TypeError('Sprite Draw hook required');
    return hook;
}

// signed DWORD 의 넘침은 원본 하위 비트로 보존한다. (2026-11-13 WB04-E388)
const add = (a, b) => (a + b) | 0;
const sub = (a, b) => (a - b) | 0;
const mul = (a, b) => Math.imul(a, b);
// 0 방향 정수 나눗셈이다. (2026-11-13 WB04-E388)
const divide = (a, b) => Math.trunc(a / b) | 0;

// raw 필드의 little endian 폭만 읽는다(주변 packed 필드를 읽지 않음). (2026-11-13 WB04-E388)
function readField(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// native CRT 의 signed 64비트 절삭 결과 중 하위 DWORD 를 유지한다(비유한·범위 밖은 거부). (2026-11-13 WB04-E388)
function truncate(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) throw new RangeError('Sprite Draw finite coordinate/time');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 일반 객체 Draw 이다. 풀/타입/그림/전역의 수명은 호출자가 유지하며 tokenBase 는 수집기와 같아야 한다. 사용: new SpriteDraw(pool, types, views, state, hooks, tokenBase). (2026-11-13 WB04-E388)
export class SpriteDraw {
    // 객체 슬롯 풀, 타입 표(hotspot/foot/flags/maxHitPoints/cost), 타입별 그림 정보(shape/gumpFrame/capacity/constructionTime/duration), 현재 전역, 훅, 토큰 기준이다.
    #pool;
    #types;
    #views;
    #state;
    #hooks;
    #tokenBase;

    // 표시/자산/풀 번호를 같은 전체 타입 표에 맞추고 토큰 범위를 검증한다. (2026-11-13 WB04-E388)
    constructor(pool, types, views, state, hooks = {}, tokenBase = 0) {
        if (types.length === 0 || types.length !== views.length || tokenBase + pool.capacity * STRIDE > TOKEN_SPACE) throw new TypeError('Sprite Draw tables/token range');
        this.#pool = pool;
        this.#types = types;
        this.#views = views;
        this.#state = state;
        this.#hooks = hooks;
        this.#tokenBase = tokenBase >>> 0;
    }

    // 타입 +0xdc 만 바꾼 원본 부작용을 실제 선택 헤더와 loaded 검사에도 적용한다(그림자 alias). (2026-11-13 WB04-E388)
    #shape(type) {
        const s = this.#state;
        if (type === s.shadowAliasType && s.shadowShapeType !== null && s.shadowShapeType !== undefined) type = s.shadowShapeType;
        if (type >= this.#views.length) throw new RangeError('Sprite Draw shape alias');
        return this.#views[type].shape;
    }

    // 공통 표시 목록의 한 항목(16바이트)을 그린다. entry 의 x/y(+0/+4 float)는 표식의 표면 조회에, 토큰(+8)·tail(+15)은 대상과 그림자 레이어 선택에 쓴다. (2026-11-13 WB04-E388)
    draw(entry) {
        const view = new DataView(entry.buffer, entry.byteOffset, entry.byteLength);
        const token = view.getUint32(ENTRY_TOKEN, true);
        const tail = entry[ENTRY_TAIL];
        if (token < this.#tokenBase) throw new RangeError('Sprite Draw token');
        const offset = token - this.#tokenBase;
        if (!offset || offset % STRIDE || offset / STRIDE >= this.#pool.capacity) throw new RangeError('Sprite Draw SID');
        const sid = offset / STRIDE;
        const s = this.#state;
        const hooks = this.#hooks;
        // 가상 Draw 가 슬롯을 바꿀 수 있으므로 살아 있는 보기를 다시 읽는다.
        const raw = this.#pool.physical(sid);
        const type = raw[10];
        if (type >= this.#types.length) throw new RangeError('Sprite Draw type');
        const record = this.#types[type];
        const typeView = this.#views[type];
        if (!this.#shape(type).loaded) return;
        const frame = readField(raw, 36, 4);
        const extra = raw[40];
        const x = sub((readField(raw, 22, 2) << 16) >> 16, s.cameraX);
        const y = sub((readField(raw, 24, 2) << 16) >> 16, s.cameraY);
        const rawView = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        const wx = rawView.getFloat32(14, true);
        const wy = rawView.getFloat32(18, true);
        if (sub(truncate(wx * 16 + 0.5), s.cameraX) !== x || sub(truncate(wy * 11 + 0.5), s.cameraY) !== y) throw new TypeError('Sprite Draw world/screen mismatch');
        const point = { x: sub(x, record.hotspotX), y: sub(y, record.hotspotY) };
        // 모든 출력은 원본 프레임과 화면 기준점 그대로 전달한다.
        const shape = (key, index, position, mode) => { required(hooks.shape)({ type: key >>> 0, frame: index >>> 0, position, mode, scaled: false, scaleX: 65536, scaleY: 65536 }); };
        if (tail & SHADOW_LAYER) {
            if (extra & 0x10) return;
            if (s.sharedPatchDiagnostic ? (s.sharedPatchDiagnostic.value >>> 0) === 0 : !s.shadowAliasPinned) s.shadowShapeType = type;
            // 그림자 레이어로의 보정은 반복 modulo 가 아니라 한 번의 signed 비교/뺄셈이다.
            let index = frame;
            if (typeView.shape.frameCount <= (index | 0)) index = (index - typeView.shape.frameCount) >>> 0;
            if ((s.specialMode >>> 0) || (extra & 1)) return;
            shape(type, (index + typeView.shape.frameCount) >>> 0, point, SpriteShapeMode.shadow);
            return;
        }
        const owner = type === (s.wideOwnerType >>> 0) ? readField(raw, 12, 2) : raw[34];
        const abstract = (extra & 0x10) !== 0;
        const dead = (extra & 1) !== 0;
        const flags2 = record.flags2;
        // 가상 프레임 조회 뒤에 바뀐 필터 전역도 원본처럼 현재 값으로 읽는다.
        const filtered = () => ((s.filteredFlags & flags2) !== 0) || type === (s.wideOwnerType >>> 0) || type === (s.unfilteredType >>> 0);
        // 소유자 표가 있는 소유자만 current map 을 바꾸고 이어서 mapped 그림을 호출한다.
        const normal = (index) => {
            const shared = s.sharedOwnerMaps.length > 0;
            if (type >= (shared ? s.sharedOwnerMaps.length : s.ownerMaps.length)) throw new RangeError('Sprite Draw owner maps');
            if ((shared ? s.sharedOwnerMaps[type] : s.ownerMaps[type]) && owner) {
                required(hooks.setOwnerColors)(type, owner);
                shape(type, index, point, SpriteShapeMode.owner);
            } else shape(type, index, point, SpriteShapeMode.plain);
        };
        const localPlayer = () => s.localPlayer >>> 0;
        if (!(extra & 2)) {
            if (dead) shape(type, frame, point, SpriteShapeMode.dead);
            else if (abstract) { if (owner === localPlayer()) shape(type, frame, point, SpriteShapeMode.abstract); }
            else if ((s.specialMode >>> 0) && filtered()) shape(type, frame, point, SpriteShapeMode.abstract);
            else normal(frame);
        } else {
            // 가상 프레임 -1 은 기본 gumpFrame 을 유지하며 alive 일반 경로는 별도 가상 Draw 만 호출한다.
            const custom = required(hooks.virtualFrame)(sid) | 0;
            const fallback = custom === -1 ? typeView.gumpFrame : custom >>> 0;
            if (dead) shape(type, fallback, point, SpriteShapeMode.dead);
            else if (!(s.specialMode >>> 0)) {
                if (custom !== -1) {
                    if (!abstract) normal(custom >>> 0);
                    else if (owner === localPlayer()) shape(type, custom >>> 0, point, SpriteShapeMode.abstract);
                }
                required(hooks.virtualDraw)(sid, point);
            } else if (abstract) { if (owner === localPlayer()) shape(type, custom >>> 0, point, SpriteShapeMode.abstract); }
            else if (filtered()) shape(type, fallback, point, SpriteShapeMode.abstract);
            else {
                if (custom !== -1) normal(custom >>> 0);
                required(hooks.virtualDraw)(sid, point);
            }
        }
        if (!abstract) {
            if ((s.sharedUseBanners ? (s.sharedUseBanners.value >>> 0) !== 0 : s.markerEnabled) && (record.flags1 & 0x400) && s.markerVisible && owner) {
                const surface = required(hooks.surface)(view.getFloat32(0, true), view.getFloat32(4, true));
                if (surface && this.#pool.slot(surface)[10] !== (s.markerExcludeType >>> 0)) {
                    if (owner >= s.ownerPalettes.length) throw new RangeError('Sprite Draw owner palette');
                    shape(s.markerType, sub(s.ownerPalettes[owner], 1) >>> 0, point, SpriteShapeMode.plain);
                }
            }
            if (raw[40] & 0x20) {
                if (s.statusType >= this.#views.length) throw new RangeError('Sprite Draw status type');
                shape(s.statusType, this.#views[s.statusType].gumpFrame, { x: sub(point.x, divide(record.footX, 2)), y: sub(point.y, divide(record.footY, 2)) }, SpriteShapeMode.plain);
            }
        }
        // extra 는 가상 Draw 에서 바뀔 수 있으므로 선택/후속 상태 판단은 현재 슬롯을 다시 읽는다.
        if ((raw[40] & 4) && !s.selectionDisabled) this.#selection(sid, type, owner, frame, { x, y });
    }

    // 원본 선택 헤더의 아래 두 모서리를 겹쳐 그리고 상태 막대를 추가한다. (2026-11-13 WB04-E388)
    #selection(sid, type, owner, frame, screen) {
        const s = this.#state;
        const hooks = this.#hooks;
        const typeView = this.#views[type];
        const record = this.#types[type];
        const raw = this.#pool.physical(sid);
        const physical = this.#shape(type);
        if (frame >= physical.frames.length || (frame | 0) < 0 || frame >= typeView.shape.frameCount * ((record.flags1 & 0x440000) ? 2 : 1) || typeView.shape.frameCount <= 0
            || record.maxHitPoints === (0xdddddddd | 0) || record.maxHitPoints === (0xcdcdcdcd | 0)) throw new RangeError('Sprite Draw selection frame');
        const header = physical.frames[frame];
        s.selectedSid = sid;
        if (owner >= s.ownerPalettes.length || s.ownerPalettes[owner] < 0 || s.ownerPalettes[owner] >= s.selectionColors.length) throw new RangeError('Sprite Draw selection colors');
        const color = s.selectionColors[s.ownerPalettes[owner]];
        let left = sub(screen.x, header.hotspotX);
        let right = add(left, header.width);
        let bottom = add(sub(screen.y, header.hotspotY), header.height);
        const horizontal = divide(header.width, 4);
        const vertical = divide(sub(header.height, 1), 4);
        // 바깥 네 선 다음 안쪽 네 선의 호출 순서를 유지한다.
        for (let layer = 0; layer < 2; layer += 1) {
            required(hooks.line)({ x: left, y: sub(bottom, vertical) }, { x: left, y: bottom }, color);
            required(hooks.line)({ x: left, y: bottom }, { x: add(left, horizontal), y: bottom }, color);
            required(hooks.line)({ x: right, y: sub(bottom, vertical) }, { x: right, y: bottom }, color);
            required(hooks.line)({ x: sub(right, horizontal), y: bottom }, { x: right, y: bottom }, color);
            left = add(left, 1);
            right = sub(right, 1);
            bottom = sub(bottom, 1);
        }
        let row = 0;
        // 막대는 폭 전체와 3픽셀 행 간격을 사용한다. 배경/본체 fill 은 half-open 이다.
        const bar = (barColor, current, maximum) => {
            const divisor = Math.max(maximum, 1);
            const amount = Math.max(0, Math.min(current, maximum));
            const bx = sub(screen.x, header.hotspotX);
            const by = sub(sub(screen.y, mul(row, 3)), header.hotspotY);
            required(hooks.fill)({ left: sub(bx, 1), top: sub(by, 5), right: add(add(bx, 1), header.width), bottom: sub(by, 1) }, s.barBackground);
            required(hooks.fill)({ left: bx, top: sub(by, 4), right: add(bx, divide(mul(amount, header.width), divisor)), bottom: sub(by, 2) }, barColor);
            row += 1;
        };
        const hasBar = (record.flags2 & 0x400000) ? (raw[40] & 1) !== 0 : (record.flags1 & 0x10) !== 0;
        if (hasBar) {
            const hpHooks = {
                current: () => readField(raw, 26, 4) | 0,
                // 실제 표시기는 공유 getter, 기존 독립 fixture 는 같은 선택 TYPE/상태를 사용한다.
                maximum: () => {
                    if (hooks.typeHitPoints) return hooks.typeHitPoints(type);
                    return s.quarterHitPoints && type === (s.shadowAliasType >>> 0) ? divide(record.maxHitPoints, 4) : record.maxHitPoints;
                },
            };
            const hpBar = readSpriteHitPointBar(s.hitPointColors, hpHooks);
            bar(hpBar.color, hpBar.current, hpBar.maximum);
        }
        if (raw[10] === (s.shadowAliasType >>> 0)) {
            const target = readField(raw, 42, 4);
            if (target >= this.#views.length) throw new RangeError('Sprite Draw construction type');
            if (required(hooks.constructionActive)(sid)) bar(0x7b, sub(this.#views[target].constructionTime, readField(raw, 46, 4) | 0), this.#views[target].constructionTime);
        }
        const resource = required(hooks.resource)(sid);
        if (resource) {
            if (resource.capacityType >= this.#views.length) throw new RangeError('Sprite Draw resource type');
            const capacity = mul(this.#views[resource.capacityType].capacity, 10);
            bar(0xbe, sub(capacity, resource.remaining), capacity);
        }
        // 같은 타입 번호가 여러 타이머 전역에 쓰인 경우에도 각각 독립 조회/막대를 유지한다.
        for (let index = 0; index < s.timerTypes.length; index += 1) {
            if (raw[10] === (s.timerTypes[index] >>> 0)) {
                const times = required(hooks.timers)(sid, s.timerProcessType);
                if (times) bar(index === 0 ? 0xd9 : index === 1 ? 9 : 0x6b, truncate(times[index] - s.now), index === 0 ? 20 : 60);
            }
        }
        if (raw[40] & 0x30) {
            let abstractTime = 0;
            let statusTime = 0;
            const processes = hooks.statusProcesses ? hooks.statusProcesses() : s.processes; // 본체 가상 Draw 뒤의 현재 실제 목록을 읽는다.
            // 실제 Kernel 의 예약 슬롯 0 은 검사하지 않는다. 현재 입력 표의 끝까지 원래 순서로 읽는다.
            for (let index = 1; index < processes.length; index += 1) {
                const process = processes[index];
                if (process.type !== (s.statusProcessType >>> 0)) continue;
                // 목록 내 첫 SID 일치에서 멈추며 flag 1 이 flag 2 보다 우선한다.
                for (const member of process.members) {
                    if (member === sid) {
                        if (process.flags & 1) abstractTime = Math.max(abstractTime, truncate(process.endTime - s.now));
                        else if (process.flags & 2) statusTime = Math.max(statusTime, truncate(process.endTime - s.now));
                        break;
                    }
                }
            }
            if (raw[40] & 0x10) {
                if (s.abstractStatusType >= this.#views.length) throw new RangeError('Sprite Draw abstract status');
                bar(0xc3, abstractTime, this.#views[s.abstractStatusType].duration);
            }
            if (raw[40] & 0x20) {
                if (s.statusType >= this.#views.length) throw new RangeError('Sprite Draw status duration');
                bar(0xfe, statusTime, this.#views[s.statusType].duration);
            }
        }
        if (raw[10] === (s.costType >>> 0)) bar(0xf0, readField(raw, 12, 2), truncate(record.cost));
    }
}

// kind 0 과 묶음 내부 kind 0 을 같은 실제 Draw 로 연결한다. (2026-11-13 WB04-E388)
export function bindSpriteDraw(hooks, draw) {
    hooks.sprite = (entry) => { draw.draw(entry); };
}

// 로딩 순서 그대로 SHP 선택 헤더와 기본 프레임·casttime·praytime·effecttime 을 연결한다(전체 타입 번호 유지, 내장 타입은 로드되지 않은 그림). (2026-11-13 WB04-E388)
export function makeSpriteAssetViews(assets) {
    const shapes = squidRendererShapes(assets.typeTable, assets.shapes);
    const result = shapes.map((shape) => ({ shape, gumpFrame: 0, capacity: 0, constructionTime: 0, duration: 0 }));
    assets.types().forEach((type, index) => {
        const definition = type.definition;
        const view = result[FIRST_ASSET_TYPE_NUMBER + index];
        // 원본 498220 은 TYPE+118 의 기본 프레임을 읽는다(기존 gumpFrame 멤버 이름과 다름).
        view.gumpFrame = definition.defaultFrame >>> 0;
        view.capacity = truncate(typeNumber(definition, 'casttime') ?? 0);
        view.constructionTime = truncate(typeNumber(definition, 'praytime') ?? 0);
        view.duration = truncate(typeNumber(definition, 'effecttime') ?? 0);
    });
    return result;
}

// 타입 +0xdc 의 alias 쓰기를 실제 자산 출력에서도 현재 값으로 읽도록 SHP 블록에 연결한다. 반환 함수: type → { database, block }. (2026-11-13 WB04-E388)
export function makeSpriteAssetShapes(assets, state) {
    return (type) => {
        if (type === state.shadowAliasType && state.shadowShapeType !== null && state.shadowShapeType !== undefined) type = state.shadowShapeType;
        const types = assets.types();
        if (type < FIRST_ASSET_TYPE_NUMBER || type - FIRST_ASSET_TYPE_NUMBER >= types.length) throw new RangeError('Sprite Draw asset type');
        return { database: assets.shapes, block: types[type - FIRST_ASSET_TYPE_NUMBER].block };
    };
}

// 0040eaf0 와 CD inline 의 경계 좌표/0단계 표 조회를 실제 해시에 연결한다(좌표 + float 0.9999 의 CRT 절삭, 경계 밖은 0). (2026-11-13 WB04-E388)
export function makeSpriteSurfaceLookup(hash) {
    const bias = Math.fround(0.9999);
    return (x, y) => {
        const column = truncate(x + bias);
        const row = truncate(y + bias);
        if (column < 0 || row < 0 || column >= 256 || row >= 256) return 0;
        return hash.cell(0, column, row);
    };
}

// 실제 SHP 합성·직교 선택 선·막대 fill 을 클립된 8비트 표면으로 출력한다. 사용: new SpritePixelTarget(pixels, 8, 8, clip, shapeOf, colorsOf).hooks(rules). (2026-11-13 WB04-E388)
export class SpritePixelTarget {
    // 출력 픽셀·행 간격·높이·원본 dirty clip, 그림 바인딩과 색 표 콜백, 마지막으로 고른 소유자 색 표이다.
    #pixels;
    #pitch;
    #height;
    #clip;
    #shape;
    #colors;
    #current = null;

    // 표면 크기와 clip 을 검증해 실제 픽셀 쓰기 범위를 제한한다. shape 는 alias 를 포함한 현재 바인딩, colors 는 (모드, 타입, 소유자) → 256 색 표이다. (2026-11-13 WB04-E388)
    constructor(pixels, pitch, height, clip, shape, colors) {
        if (pitch <= 0 || height <= 0 || pitch * height > pixels.length || clip.left > clip.right || clip.top > clip.bottom) throw new TypeError('Sprite Draw surface/clip');
        this.#pixels = pixels;
        this.#pitch = pitch;
        this.#height = height;
        this.#clip = clipRect(clip, { left: 0, top: 0, right: pitch, bottom: height });
        this.#shape = shape;
        this.#colors = colors;
    }

    // 게임 규칙/가상 처리기는 보존하고 픽셀 관련 출력만 이 표면에 연결한 훅 사본을 만든다. (2026-11-13 WB04-E388)
    hooks(rules = {}) {
        return {
            ...rules,
            setOwnerColors: (type, owner) => { this.#current = required(this.#colors)(SpriteShapeMode.owner, type, owner); },
            shape: (request) => { this.#drawShape(request); },
            line: (first, last, color) => { this.#drawLine(first, last, color); },
            fill: (rect, color) => { this.#drawFill(rect, color); },
        };
    }

    // 004dab90 의 고정 표는 소스 마스크 아래의 대상 색을 바꾼다. 소유자 RLE 의 소스 색 치환과 구별한다. (2026-11-13 WB04-E388)
    #drawShape(request) {
        const binding = required(this.#shape)(request.type);
        if (!binding?.database) throw new TypeError('Sprite Draw shape binding');
        const blocks = binding.database.blocks;
        if (binding.block >= blocks.length || request.frame >= blocks[binding.block].frames.length) throw new RangeError('Sprite Draw frame');
        const frame = blocks[binding.block].frames[request.frame];
        if (frame.special) return;
        const image = binding.database.decode(binding.block, request.frame);
        let colors = null;
        if (request.mode === SpriteShapeMode.owner) {
            if (!this.#current) throw new Error('Sprite Draw owner colors missing');
            colors = this.#current;
        } else if (request.mode !== SpriteShapeMode.plain) colors = required(this.#colors)(request.mode, request.type, 0);
        const scaleX = request.scaled ? request.scaleX : 65536;
        const scaleY = request.scaled ? request.scaleY : 65536;
        if (request.mode !== SpriteShapeMode.plain && request.mode !== SpriteShapeMode.owner) {
            drawTranslatedVfxImage(this.#pixels, this.#pitch, this.#height, image, frame, request.position, scaleX, scaleY, this.#clip, colors);
        } else if (request.scaled) drawScaledVfxImage(this.#pixels, this.#pitch, this.#height, image, frame, request.position, request.scaleX, request.scaleY, this.#clip, colors);
        else drawIndexedImage(this.#pixels, this.#pitch, this.#height, image, add(request.position.x, frame.rect.left), add(request.position.y, frame.rect.top), this.#clip, colors);
    }

    // 양 끝 포함 직교 선분을 정규화해 자른다. 대각선 입력은 이 Draw 의 계약 밖이다. (2026-11-13 WB04-E388)
    #drawLine(first, last, color) {
        const clip = this.#clip;
        if (first.x !== last.x && first.y !== last.y) throw new TypeError('Sprite Draw orthogonal line');
        if (first.y === last.y) {
            if (first.y < clip.top || first.y >= clip.bottom) return;
            const low = Math.max(Math.min(first.x, last.x), clip.left);
            const high = Math.min(Math.max(first.x, last.x), clip.right - 1);
            for (let x = low; x <= high; x += 1) this.#pixels[first.y * this.#pitch + x] = color;
        } else {
            if (first.x < clip.left || first.x >= clip.right) return;
            const low = Math.max(Math.min(first.y, last.y), clip.top);
            const high = Math.min(Math.max(first.y, last.y), clip.bottom - 1);
            for (let y = low; y <= high; y += 1) this.#pixels[y * this.#pitch + first.x] = color;
        }
    }

    // 원본 fill 의 끝 좌표를 제외해 배경/막대 픽셀을 쓴다. 빈/역전 폭·높이에는 쓰지 않는다. (2026-11-13 WB04-E388)
    #drawFill(rect, color) {
        const visible = clipRect(rect, this.#clip);
        for (let y = visible.top; y < visible.bottom; y += 1) {
            for (let x = visible.left; x < visible.right; x += 1) this.#pixels[y * this.#pitch + x] = color;
        }
    }
}
