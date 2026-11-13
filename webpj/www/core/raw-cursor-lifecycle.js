// 커서 취득00447000·취소00446e70·미리보기 정리00445aa0을 같은 내려놓기 상태에 연결한다(cpppj client/RawCursorLifecycle.cpp/.h, 10.78).
// 사용: new RawCursorLifecycle(pool, types, drop, state, hooks).take(preview, flags)/cancel()/activate(force).
// 도달한 외부 장치만 요구한다. 큰 DWORD를 SID로 소비할 때 검사하며 취소/준비/진단이 바꾼 현재 값과 부분 효과를 보존한다.
// 자산 바인딩은 같은 세션의 현재188칸 types를 명시 인자로 빌린다. GameAssets의 그림 정의에서 별도 규칙 표를 만들지 않는다.
// 이 모듈은 DOM을 쓰지 않는다. (2026-11-14 WB04-E400: 커서 전역·표시의 6단계 선행)
import { cameraFtolLow } from '../render/camera-view.js';

// 원본005325cc의 다섯 배치 거부 문구다. 현재 번역·실제 측정기를 통해 치수를 얻는다.
const WARNINGS = Object.freeze(["Priest can't get here.", 'Must be placed over constructing artifact', 'Cannot place here', 'Energy not satisfied', 'Only on Controlled Island']);

// 선택한 가상/호스트 경계를 실행한다. 선택하지 않은 경계는 없어도 된다.
function need(callback) {
    if (typeof callback !== 'function') throw new Error('실제 커서 수명 경계 미연결');
    return callback;
}

// 원본 DWORD를 호스트 SID16으로 바꾸기 직전 검사한다. 실제 풀 범위는 각 소비자가 확인한다.
export function cursorPreviewSid(number) {
    const value = number >>> 0;
    if (value > 65535) throw new RangeError('커서원문DWORD의호스트SID범위');
    return value;
}

// 정렬되지 않은 원본 Form WORD다.
const word = (raw, offset) => raw[offset] | (raw[offset + 1] << 8);

// 내려놓기와 겹치지 않는 현재 커서 전역이다. 목록/문맥은 원문 DWORD 폭의 장기 저장소다.
export class CursorLifecycleState {
    // TYPE 논리 포인터와 방향 인자, 마나/사정거리 Process와 구별한 현재 Gump 토큰이다.
    abstractType = 0; directionArgument = 0; currentGump = 0;
    // 120바이트/30DWORD 목록과 현재 signed 개수다. SID16 배열로 줄이지 않는다.
    previews = new Uint32Array(30); previewCount = 0;
    // 두 색/글꼴 전역 이름·비용 그림 TYPE92·독립 진단 입력 및 실제 Client 진단 대여다.
    warningFont = 0; costFont = 0; costType = 92; teleportBlocked = 0; checking = null;
    // 원본32바이트 문맥이며 +8만 현재 warningFont/costFont로 바뀐다.
    warningContext = new Uint32Array([2, 0, 0, 0, 0, 0, 1, 1]);
    costContext = new Uint32Array([2, 3, 0, 0, 0, 0, 1, 1]);
    // 개별 경고 치수와 signed 최대 경고/가격 치수다.
    warningWidths = new Int32Array(5); warningHeights = new Int32Array(5);
    warningWidth = 0; warningHeight = 0; costTextWidth = 0; costHeight = 0;
    // 원본 비용 그림의 signed WORD 기준점/높이와 같은 SHP 블록 토큰이다.
    costHotspotX = 0; costHotspotY = 0; imageHeight = 0; costImage = 0;
    // CRT 절삭 비용 문자열이다.
    costText = '';

    // 효과 뒤 실제 진단 DWORD를 다시 읽는다. 함수 또는 같은 {value} 상자를 빌릴 수 있다.
    checkingValue() {
        return (this.checking === null ? this.teleportBlocked : typeof this.checking === 'function' ? this.checking() : this.checking.value) >>> 0;
    }
}

// 취득/취소/정리를 같은 풀·현재188TYPE·내려놓기 상태·목록 상태로 실행한다.
export class RawCursorLifecycle {
    // 장기 소유자에서 빌린 실제 저장소와 외부 가상 효과다.
    #pool; #types; #drop; #state; #hooks;

    // 다른 판본/TYPE 크기는 첫 효과 전에 거부한다. 실제 선택 장치는 각 소비 위치에서 확인한다.
    constructor(pool, types, drop, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188) throw new TypeError('커서 수명10.78 Pool/TYPE 계약 오류');
        this.#pool = pool; this.#types = types; this.#drop = drop; this.#state = state; this.#hooks = hooks;
    }

    // 어댑터가 같은 실제 풀을 사용하는지 확인한다.
    get pool() { return this.#pool; }

    // 현재 SID의 살아 있는 raw를 읽기용으로 빌린다. 콜백 뒤 같은 span의 변경을 보존하며 잠금 등의 조기 반환은 접근하지 않는다.
    #raw(sid) {
        if (sid < 5 || sid >= this.#pool.capacity) throw new RangeError('커서 수명 현재 SID 범위 오류');
        return this.#pool.physical(sid);
    }

    // 선택 TYPE DWORD와 다른 abstractType 논리 포인터를 현재 상태에서 다시 읽는다.
    #type() {
        const type = this.#state.abstractType >>> 0;
        if (type >= this.#types.length) throw new RangeError('커서 수명 현재 TYPE 범위 오류');
        return this.#types[type];
    }

    // 준비 뒤 현재 flags/목록/개수를 읽고, 각 삭제 효과 뒤 다음 현재 개수/번호를 다시 읽는다.
    activate(force) {
        const s = this.#state; need(this.#hooks.prepareCursor)();
        if ((this.#drop.costFlags & 0x200) || (force >>> 0)) {
            // 정리 도중 콜백이 목록이나 signed 개수를 바꿀 수 있다.
            for (let index = 0; index < (s.previewCount | 0); index += 1) {
                if (index >= s.previews.length) throw new RangeError('커서 임시 목록 원본30칸 범위 오류');
                const sid = cursorPreviewSid(s.previews[index]); this.#raw(sid); need(this.#hooks.destroy)(sid, 0);
            }
            s.previewCount = 0; s.previews.fill(0);
        }
    }

    // 캡처한 실제 Form과 효과 뒤 현재 미리보기 전역을 구별해 반환/삭제/진단/정리를 수행한다.
    cancel() {
        const d = this.#drop; const s = this.#state; const h = this.#hooks;
        if (!(d.lockDepth >>> 0) && (d.preview >>> 0)) {
            need(h.highlight)(0);
            if (!(d.lockDepth >>> 0)) { this.activate(0); need(h.selectCursor)(1); }
            d.lockDepth = (d.lockDepth + 1) >>> 0;
            const preview = cursorPreviewSid(d.preview); const raw = this.#raw(preview);
            if (!(raw[11] & 1)) {
                const bytes = this.#pool.allocated(preview); bytes[20] = d.inputFlags & 255; bytes[21] = (d.inputFlags >>> 8) & 255;
                if (d.inputFlags & 0x80) {
                    const source = cursorPreviewSid(need(h.source)(preview)); const sourceRaw = this.#raw(source);
                    if (!(sourceRaw[11] & 4)) need(h.detach)(source, 0);
                    need(h.unpop)(source, 0);
                }
                if (d.costFlags & 0x100) need(h.destroy)(preview, 0);
                else {
                    const source = cursorPreviewSid(need(h.source)(preview)); this.#raw(source);
                    need(h.sourceNotice)(source, cursorPreviewSid(d.preview), 4);
                    if (d.costFlags & 0x80) {
                        need(h.sound)('teleporterActive.wav');
                        if (s.checkingValue()) need(h.assertion)('false', '.\\Cursor.cpp', 0x3f1);
                        need(h.addLocalSp)(Math.fround(-Math.fround(this.#type().cost)));
                    }
                }
            }
        }
        this.activate(1);
        if (d.cursorGump >>> 0) { need(h.removeProcess)(d.cursorGump >>> 0, 0); d.cursorGump = 0; }
        s.previews.fill(0); d.preview = 0; d.lockDepth = 1; s.previewCount = 0;
    }

    // 팝업 정리부터 현재 마우스 위치 꼬리 호출까지 취득의 원문 순서를 보존한다.
    take(preview, flags) {
        const d = this.#drop; const s = this.#state; const h = this.#hooks;
        preview = cursorPreviewSid(preview); flags >>>= 0;
        this.cancel(); if (d.cursorGump >>> 0) throw new Error('cursorManaProcess == NULL 원본 진단');
        need(h.closePopups)(); d.preview = preview;
        const raw = this.#raw(preview); d.type = word(raw, 18); s.abstractType = word(raw, 18); this.#type();
        d.argument = word(raw, 12); if (!(d.argument >>> 0)) d.argument = need(h.defaultArgument)(s.abstractType >>> 0) >>> 0;
        d.direction = 0; s.directionArgument = 0; d.inputFlags = word(raw, 20);
        this.#pool.allocated(preview)[21] |= 1;
        const source = cursorPreviewSid(need(h.source)(preview)); this.#raw(source); need(h.sourceNotice)(source, cursorPreviewSid(d.preview), 4);
        d.costFlags = (this.#type().flags2 & 4) ? (flags | 0x200) >>> 0 : flags;
        d.lockDepth = 0; need(h.createPreviews)(); need(h.selectCursor)((d.legalGump >>> 0) ? 3 : 2);
        d.legalGump = 0; s.currentGump = 0;
        if (this.#type().flags2 & 0xa00) {
            d.cursorGump = need(h.createMana)(cursorPreviewSid(s.previews[0]), 0) >>> 0;
            if ((s.currentGump >>> 0) && (d.cursorGump >>> 0)) need(h.processEvent)(d.cursorGump >>> 0, 1, 1, 0);
        }
        const typeNumber = need(h.typeNumber)(s.abstractType >>> 0) >>> 0;
        if (need(h.hasRange)(typeNumber)) d.cursorGump = need(h.createRange)(cursorPreviewSid(s.previews[0]), 0) >>> 0;
        need(h.queryCursor)(); s.warningContext[2] = s.warningFont; s.warningWidth = 0; s.warningHeight = 0;
        // 번역/측정 효과 뒤 개별 signed 치수와 현재 최대값을 원문 순서로 저장한다.
        for (let index = 0; index < WARNINGS.length; index += 1) {
            const text = need(h.translate)(WARNINGS[index]); const size = need(h.measure)(text, s.warningContext);
            s.warningWidths[index] = size[0]; s.warningHeights[index] = size[1];
            if ((s.warningWidth | 0) <= s.warningWidths[index]) s.warningWidth = s.warningWidths[index];
            if ((s.warningHeight | 0) <= s.warningHeights[index]) s.warningHeight = s.warningHeights[index];
        }
        s.costContext[2] = s.costFont; s.costText = String(cameraFtolLow(Math.fround(this.#type().cost)));
        const size = need(h.measure)(s.costText, s.costContext); s.costTextWidth = size[0] | 0; s.costHeight = size[1] | 0;
        const image = need(h.costImage)(s.costType >>> 0, 3); s.costImage = image.token >>> 0; s.costHotspotX = image.hotspotX; s.costHotspotY = image.hotspotY;
        d.cursorWidth = (image.width + (s.costTextWidth >>> 0)) | 0;
        if ((s.costHeight | 0) <= image.height) s.costHeight = image.height;
        s.imageHeight = image.height;
        const mouse = need(h.mouse)(); need(h.updatePosition)(mouse.x | 0, mouse.y | 0);
    }
}

// 사제 메뉴의 취득과 내려놓기의 활성화/GUI 거부를 같은 실제 수명에 제자리로 연결한다.
export function bindCursorLifecycle(priest, drop, lifecycle) {
    priest.takePreview = (sid, flags) => lifecycle.take(sid, flags);
    drop.activatePreview = (force) => lifecycle.activate(force); drop.invalidGui = () => lifecycle.cancel();
}

// 원본00474240의 네 그룹과 flags2 bit0x100 판정이다. 현재 TYPE 레코드를 매번 읽는다.
export function hasCursorRange(types, typeNumber) {
    typeNumber >>>= 0;
    if (typeNumber >= types.length) throw new RangeError('커서 사정거리 현재 TYPE 범위 오류');
    const type = types[typeNumber];
    return type.group === 0 || type.group === 1 || (type.flags2 & 0x100) !== 0 || type.group === 6 || type.group === 3;
}

// 같은 GameAssets의 그림/기본 프레임과 실제 현재 TYPE 표를 제자리로 연결한다. fonts(FontSet)를 생략하면 기존 측정 장치를 유지한다.
export function bindCursorLifecycleAssets(hooks, assets, types, fonts = null) {
    if (types?.length !== 188 || assets?.typeTable?.definitions?.length !== 116 || !assets.shapes) throw new TypeError('커서 자산10.78 판본 계약 오류');
    // TYPE 번호와 SHP 블록 번호를 구별한다. 내장 타입은 그림 조회에만 사용할 수 없다.
    const asset = (type) => {
        type >>>= 0;
        if (type < 70 || type - 70 >= assets.types().length) throw new RangeError('커서 현재 자산 TYPE 범위 오류');
        return assets.types()[type - 70];
    };
    hooks.defaultArgument = (type) => asset(type).definition.defaultFrame >>> 0;
    // 번호 변환은 내장 타입도 허용하고 실제 그림을 읽지 않는다.
    hooks.typeNumber = (type) => {
        type >>>= 0;
        if (type >= types.length) throw new RangeError('커서 현재 TYPE 번호 범위 오류');
        return type;
    };
    hooks.hasRange = (type) => hasCursorRange(types, type);
    // frameCheck는 현재 TYPE 비트로 검사한 뒤 같은 SHP 추가 헤더를 읽는다.
    hooks.costImage = (type, frame) => {
        const current = asset(type); const count = current.definition.clusters.length;
        if ((frame >>> 0) >= count * ((types[type >>> 0].flags1 & 0x440000) ? 2 : 1)) throw new Error('커서 비용 그림 frameCheck 원본 진단');
        const metrics = assets.shapes.squidMetrics(current.block, frame >>> 0);
        return { token: (current.block + 1) >>> 0, width: metrics.width, height: metrics.height, hotspotX: metrics.hotspotX, hotspotY: metrics.hotspotY };
    };
    if (fonts !== null) {
        // C++의 NUL 종료 문자열/8DWORD 문맥을 기존 FontSet의 실제 측정 입력으로 바꾼다.
        hooks.measure = (value, words) => {
            const bytes = fonts.encode(String(value).split('\0', 1)[0]);
            const result = fonts.measure(bytes, { flags: words[0], font: words[1], color: words[2], style: words[3], extra: words[4], shadowColor: words[5], shadowX: words[6] | 0, shadowY: words[7] | 0 });
            return [result.width, result.height];
        };
    }
}
