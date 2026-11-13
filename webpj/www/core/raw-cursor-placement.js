// 커서 위치 판정445440·연관 지역 검사445340·표시 준비4458e0을 같은 커서 상태로 실행한다(cpppj client/RawCursorPlacement.cpp/.h, 10.78).
// 사용: new RawCursorPlacement(pool, types, surfaceMap, drop, lifecycle, state, hooks).query()/sourceAllowed()/prepare().
// describe는 description의 살아 있는 {value} 참조를 받는다. 그 밖의 가상/공간/그림 경계는 원문 위치에서만 요구한다.
// 이 모듈은 DOM을 쓰지 않는다. (2026-11-14 WB04-E401: 커서 전역·미리보기 표시의 선행)
import { cursorPreviewSid } from './raw-cursor-lifecycle.js';
import { cameraFtolLow } from '../render/camera-view.js';
import { decodeCanonType } from './raw-canon-type.js';

// 실제 float32 0.9999를 double 중간값의 칸 절삭에 사용한다.
const CELL_BIAS = Math.fround(0.9999);
// 현재 DWORD 산술을 signed 결과로 해석한다.
const add = (left, right) => (left + right) | 0;
const sub = (left, right) => (left - right) | 0;
// 선택된 경계만 요구하고 누락을 성공으로 바꾸지 않는다.
function need(callback) { if (typeof callback !== 'function') throw new Error('커서 위치 의존 동작 누락'); return callback; }

// 내려놓기/수명과 겹치지 않는 현재 허용 DWORD·표시 범위·화면 원점이다.
export class CursorPlacementState {
    // Take 호출 flags와 다른 배치 경고 플래그 및 네 원문 허용 DWORD다.
    placementFlags = 0; placementAllowed = 0; descriptionAllowed = 0; spaceAllowed = 0; sourceAllowed = 0;
    // 현재 공간 장애와 Gump 수신 이벤트의 PE 기본값이다.
    obstruction = 0; acceptEvent = 0x0200003f;
    // 설명 한 글자의 여백과 기준점X/Y·전체 폭/높이의 signed 네 칸이다(좌상/우하가 아님).
    labelSpacing = 16; bounds = new Int32Array(4);
    // 첫 그림의 signed WORD 기준점·크기다.
    hotspotX = 0; hotspotY = 0; imageWidth = 0; imageHeight = 0;
    // 현재 설명과 독립 화면 원점 또는 같은 카메라 snapshot의 살아 있는 두 칸이다.
    description = ''; screenOrigin = { x: 0, y: 0 }; sharedScreenOrigin = null;

    // 비용 표시 소비 위치에서 현재 카메라 원점을 읽는다.
    screenOriginX() { return (this.sharedScreenOrigin ? this.sharedScreenOrigin[0] : this.screenOrigin.x) | 0; }
    screenOriginY() { return (this.sharedScreenOrigin ? this.sharedScreenOrigin[1] : this.screenOrigin.y) | 0; }
}

// 세 몸체가 같은 TYPE·미리보기·GUI·지역 저장소를 읽도록 묶는다.
export class RawCursorPlacement {
    // 장기 소유자가 빌려 주는 실제 풀/표면/TYPE와 세 커서 상태·가상 경계다.
    #pool; #types; #surface; #drop; #life; #state; #hooks;

    // 표면 지도는 SourceAllowed의 유효 칸에서만 필요하다. 생성 때 강제로 채우지 않는다.
    constructor(pool, types, surfaceMap, drop, lifecycle, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('커서 위치10.78 판본 계약 오류');
        this.#pool = pool; this.#types = types; this.#surface = surfaceMap; this.#drop = drop; this.#life = lifecycle; this.#state = state; this.#hooks = hooks;
    }

    // 어댑터의 같은 풀 계약을 확인한다.
    get pool() { return this.#pool; }

    // 현재 abstract TYPE를 읽는다. 내려놓기의 별도 type으로 대체하지 않는다.
    #type() {
        const type = this.#life.abstractType >>> 0;
        if (type >= this.#types.length) throw new RangeError('커서 위치 abstract TYPE 범위');
        return this.#types[type];
    }

    // SP가 아닌 연관 객체 지역과 현재 Canon 패턴의 겹침이다. 일치하면 Advance 전에 끝낸다.
    sourceAllowed() {
        const d = this.#drop; const h = this.#hooks;
        if (!(d.costFlags & 0x20)) return 1;
        const source = cursorPreviewSid(need(h.source)(cursorPreviewSid(d.preview)));
        const region = need(h.region)(source) | 0; let allowed = 0;
        need(h.walk)({ type: d.type >>> 0, argument: d.argument | 0, direction: d.direction | 0, x: Math.fround(d.x), y: Math.fround(d.y), explicitFrame: false }, (cell) => {
            const x = cameraFtolLow(Math.fround(cell.x) + CELL_BIAS); const y = cameraFtolLow(Math.fround(cell.y) + CELL_BIAS);
            if (x >= 0 && y >= 0 && x < 256 && y < 256) {
                if (this.#surface.length !== 65536) throw new Error('커서 연관 지역 표면 지도 누락');
                const sid = this.#surface[y * 256 + x];
                if (sid && (need(h.region)(sid) | 0) === region) { allowed = 1; return false; }
            }
            return true;
        });
        return allowed;
    }

    // 현재 설명·패턴의 그림 범위와 월드/Gump/연관 지역의 허용을 원문 순서로 계산한다.
    query() {
        const d = this.#drop; const l = this.#life; const s = this.#state; const h = this.#hooks;
        const description = { get value() { return s.description; }, set value(next) { s.description = next; } };
        d.legal = need(h.describe)(description, Math.fround(d.x), Math.fround(d.y), d.type >>> 0, d.localPlayer >>> 0) >>> 0;
        s.descriptionAllowed = d.legal;
        const nul = s.description.indexOf('\0'); const textLength = (nul < 0 ? s.description.length : nul) >>> 0;
        let left = Math.fround((-(this.#type().footX << 4)) | 0); let top = Math.fround(Math.imul(this.#type().footY, -11));
        let right = 0; let bottom = 0; let index = 0; s.hotspotX = 0; s.hotspotY = 0;
        need(h.walk)({ type: d.type >>> 0, argument: d.argument | 0, direction: d.direction | 0, x: 0, y: 0, explicitFrame: false }, (cell) => {
            const image = need(h.image)(l.abstractType >>> 0, cell.frame | 0);
            if (!index) { s.hotspotX = image.hotspotX; s.hotspotY = image.hotspotY; s.imageWidth = image.width; s.imageHeight = image.height; }
            const px = Math.fround(Math.fround(cell.x) * 16 - image.hotspotX);
            const py = Math.fround(Math.fround(cell.y) * 11) - image.hotspotY;
            if (!(px > left)) left = px; if (!(py > top)) top = Math.fround(py);
            const farX = px + image.width; const farY = py + image.height;
            if (!(right > farX)) right = Math.fround(farX); if (!(bottom > farY)) bottom = Math.fround(farY);
            index += 1; return true;
        });
        let finalLeft;
        if (this.#type().flags2 & 4) {
            finalLeft = left - 16; top = Math.fround(top - 11); right = Math.fround(right + 16); bottom = Math.fround(bottom + 11);
        } else {
            finalLeft = left - (s.labelSpacing | 0);
            const extent = sub(s.imageHeight, s.hotspotY); const textExtent = Math.imul(s.labelSpacing, textLength);
            const labelTop = bottom - (extent < 0 ? 0 : extent) - textExtent;
            if (!(labelTop > top)) top = Math.fround(labelTop);
        }
        if ((this.#type().flags1 & 0x400) && !(bottom > 30)) bottom = 30;
        s.bounds.set([cameraFtolLow(-finalLeft), cameraFtolLow(-top), cameraFtolLow(right - finalLeft), cameraFtolLow(bottom - top)]);
        const oldGump = l.currentGump >>> 0; d.legalGump = 0; l.currentGump = 0;
        l.currentGump = need(h.gumpAt)(d.screenX | 0, d.screenY | 0) >>> 0;
        if (!(l.currentGump >>> 0)) {
            const x = Math.fround(Math.floor(Math.fround(d.x) + 0.5)); const y = Math.fround(Math.floor(Math.fround(d.y) + 0.5));
            s.placementAllowed = need(h.mayPlace)({ type: l.abstractType >>> 0, argument: d.argument >>> 0, x, y, flags: d.direction >>> 0, owner: d.localPlayer & 255, mode: 1 }) >>> 0;
            s.spaceAllowed = (s.obstruction >>> 0) === 0 ? 1 : 0;
        } else {
            s.placementAllowed = 0; d.legalGump = need(h.accept)(l.currentGump >>> 0, s.acceptEvent >>> 0, 0, 0, 0) >>> 0;
            if (d.legalGump >>> 0) {
                s.placementAllowed = 1; s.descriptionAllowed = 1; d.legal = 1;
                if (!(need(h.parent)(d.legalGump >>> 0) >>> 0)) throw new Error('cursorLegalGump->parentSid 원본 진단');
            }
        }
        d.legal = (d.legal >>> 0) && (s.placementAllowed >>> 0) ? 1 : 0;
        s.sourceAllowed = this.sourceAllowed(); d.legal = (d.legal >>> 0) && (s.sourceAllowed >>> 0) ? 1 : 0;
        s.placementAllowed = (s.placementAllowed >>> 0) && (s.sourceAllowed >>> 0) ? 1 : 0;
        if ((d.cursorGump >>> 0) && (!oldGump !== !(l.currentGump >>> 0))) need(h.processEvent)(d.cursorGump >>> 0, 1, (l.currentGump >>> 0) ? 1 : 0, 0);
    }

    // 본체·경고·비용의 변경 사각형을 signed 나눗셈과 DWORD 감김으로 순서대로 보낸다.
    prepare() {
        const d = this.#drop; const l = this.#life; const s = this.#state; const h = this.#hooks;
        const point = need(h.project)(Math.fround(d.x), Math.fround(d.y)); const b = s.bounds;
        const rect = { left: sub(sub(point.x, b[0]), 2), top: sub(sub(point.y, b[1]), 2), right: add(add(sub(b[2], b[0]), 6), point.x), bottom: add(add(sub(b[3], b[1]), 14), point.y) };
        need(h.invalidate)(rect, 2);
        if (((d.costFlags & 0x20) && !(s.sourceAllowed >>> 0)) || !(s.descriptionAllowed >>> 0) || (!(s.placementAllowed >>> 0) && (s.placementFlags >>> 0)) || !(s.spaceAllowed >>> 0)) {
            const x = add(sub(Math.trunc(b[2] / 2), Math.trunc((l.warningWidth | 0) / 2)), rect.left);
            const y = add(sub(Math.trunc(b[3] / 2), Math.trunc((l.warningHeight | 0) / 2)), rect.top);
            need(h.invalidate)({ left: x, top: y, right: add(add(l.warningWidth, 2), x), bottom: add(add(l.warningHeight, 2), y) }, 0);
        }
        if (d.costFlags & 0x40) {
            const raw = this.#pool.slot(cursorPreviewSid(l.previews[0]));
            const cellX = ((raw[22] | (raw[23] << 8)) << 16) >> 16; const cellY = ((raw[24] | (raw[25] << 8)) << 16) >> 16;
            const foot = this.#type().footX << 4;
            const x = sub(sub(sub(cellX, Math.trunc(foot / 2)), Math.trunc((d.cursorWidth | 0) / 2)), s.screenOriginX()); const y = sub(cellY, s.screenOriginY());
            need(h.invalidate)({ left: x, top: y, right: add(add(d.cursorWidth, 2), x), bottom: add(add(l.costHeight, 2), y) }, 0);
        }
    }
}

// 취득/취소/내려놓기의 판정과 표시 준비를 같은 몸체로 제자리 연결한다.
export function bindCursorPlacement(lifecycle, drop, placement) {
    lifecycle.queryCursor = drop.queryCursor = () => placement.query(); lifecycle.prepareCursor = drop.prepareCursor = () => placement.prepare();
}

// 같은 자산의 현재 프레임/Canon 반복자와 SHP 추가 헤더만 연결한다. 현재188TYPE는 세션 소유 표를 빌린다.
export function bindCursorPlacementAssets(hooks, assets, types, geometry) {
    // TYPE 번호를 현재 자산 블록으로 바꾸며 내장 타입의 그림 조회는 거부한다.
    const asset = (type) => { type >>>= 0; if (type < 70 || type - 70 >= assets.types().length) throw new RangeError('커서 위치 자산 TYPE 범위'); return assets.types()[type - 70]; };
    // false를 반환한 현재 칸에서는 Advance를 호출하지 않는다.
    hooks.walk = (query, visit) => {
        const current = asset(query.type); const definition = current.definition;
        const decoder = decodeCanonType(definition.clusters.map((cluster) => cluster.code), definition.defaultFrame, geometry.patternTypes, query, geometry.decoderDiagnostics);
        // 실제 Canon의 유효 칸을 원래 순서로 전달한다.
        while (decoder.valid) { if (!visit({ frame: decoder.frame, x: decoder.x, y: decoder.y })) return; decoder.advance(); }
    };
    // 현재 flags1의 frameCheck 뒤 같은 SHP의 signed WORD를 읽는다.
    hooks.image = (type, frame) => {
        const current = asset(type); const count = current.definition.clusters.length;
        if (frame < 0 || frame >= count * ((types[type >>> 0].flags1 & 0x440000) ? 2 : 1)) throw new Error('커서 위치 frameCheck 원본 진단');
        const image = assets.shapes.squidMetrics(current.block, frame);
        return { token: (current.block + 1) >>> 0, width: image.width, height: image.height, hotspotX: image.hotspotX, hotspotY: image.hotspotY };
    };
}

// 같은 실제 배치 파이프라인과 Renderer 변경 표를 연결한다. GUI/설명/지역은 호출자 장치를 보존한다.
export function bindCursorPlacementWorld(hooks, pool, assets, types, geometry, placement, renderer) {
    if (pool?.edition !== 'patch1078' || placement?.pool !== pool || types?.length !== 188 || assets?.typeTable?.definitions?.length !== 116) throw new TypeError('커서 위치 실제 월드 계약 오류');
    bindCursorPlacementAssets(hooks, assets, types, geometry);
    hooks.mayPlace = (query) => placement.mayPlace(query) ? 1 : 0; hooks.invalidate = (rect, flags) => renderer.invalidate(rect, flags);
}
