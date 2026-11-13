// 커서 내려놓기 004473e0의 전체 분기·효과 순서를 옮긴다(cpppj client/RawCursorDrop.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 사용: new RawCursorDrop(pool, types, state, hooks).drop(). state는 같은 CursorDropState를 수명/배치/선택 소유자와 공유한다.
// GUI/Form/실제 로컬 배치 경계는 호출자가 공급하며 성공 함수로 대체하지 않는다. 미리보기 DWORD는 호스트가 소비할 때만 SID16으로 검사한다.
// makeCursorDropHooks의 sound는 playByName(name, repeat, volume, pan, priority, limit) 장치다. 브라우저 음향 장치 연결은 ClientAudio 단위 몫이다.
// 이 모듈은 DOM을 쓰지 않는다. (2026-11-14 WB04-E398: 4-D/5-A가 요구하는 6단계 커서 선행 부품)
import { cameraFtolLow } from '../render/camera-view.js';

// 생성 때 확인하는 원문 가상/호스트 경계다. 이후 선택된 호출 순서를 그대로 보존한다.
const HOOK_NAMES = Object.freeze(['queryCursor', 'prepareCursor', 'highlight', 'playDrop', 'previewSource', 'destroy', 'pop',
    'record', 'placeLocal', 'addLocalSp', 'showSp', 'battleRefresh', 'invalidGui', 'guiFlags', 'guiParent', 'detach', 'attach',
    'guiReceive', 'returnPiece', 'activatePreview', 'selectCursor', 'refresh', 'closeGump']);

// 내려놓기·커서 수명에서 공유하는 현재 전역이다. Gump 토큰은 서로 다른 원문 슬롯을 유지한다.
export class CursorDropState {
    // 중첩 잠금·현재 배치 판정·TYPE 번호·명령 인자·방향·입력 WORD 원본·비용 플래그다.
    lockDepth = 0; legal = 0; type = 0; argument = 0; direction = 0; inputFlags = 0; costFlags = 0;
    // 0055a2a8의 전체 DWORD와 단정도 월드 좌표다. 준비 효과가 preview를 바꾸어도 캡처 번호는 유지한다.
    preview = 0; x = 0; y = 0;
    // 현재 허용 Gump와 마나/사정거리 Process 토큰, 기록기·전투 상태·로컬 소유자다.
    legalGump = 0; cursorGump = 0; recording = 0; recorder = 0; battle = 0; blocked = 0; localPlayer = 0;
    // 원시 마우스와 구별한 hotspot 보정 화면 좌표와 가격 표시 폭(signed DWORD)이다.
    screenX = 0; screenY = 0; cursorWidth = 0;
}

// double 중간값의 floor(float + 0.5)를 단정도로 저장한다. 비유한 원본 좌표는 소비 위치에서 거부한다.
function roundCoordinate(value) {
    const single = Math.fround(value);
    if (!Number.isFinite(single)) throw new TypeError('커서 좌표가 유한하지 않습니다');
    return Math.fround(Math.floor(single + 0.5));
}

// 현재 커서 전역과 같은 SID 풀을 빌려 월드 배치 또는 GUI 전달을 실행한다.
export class RawCursorDrop {
    // 현재 풀·TYPE 표·공유 전역·가상 경계다. 자체 월드나 별도 미리보기 사본을 만들지 않는다.
    #pool; #types; #state; #hooks;

    // 모든 필수 효과를 확인하고 참조를 보관한다. 생성은 원본 상태를 초기화하지 않는다.
    constructor(pool, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types?.length || !state || HOOK_NAMES.some((name) => typeof hooks[name] !== 'function')) {
            throw new TypeError('커서 내려놓기 자료/효과 연결 오류');
        }
        this.#pool = pool; this.#types = types; this.#state = state; this.#hooks = hooks;
    }

    // 실제 로컬 배치·보상 어댑터와 같은 풀인지 확인한다.
    get pool() { return this.#pool; }

    // 현재 TYPE를 매 소비 위치에서 다시 읽는다. SP 가산 등의 효과가 바꾼 비용도 반영한다.
    #type() {
        const number = this.#state.type >>> 0;
        if (number >= this.#types.length) throw new RangeError('커서 타입 번호 오류');
        return this.#types[number];
    }

    // 연관 SID를 검사한다. 미리보기의 예약 번호 제한과 구별한다.
    #sourceSid(value) {
        const number = value >>> 0;
        if (!number || number >= this.#pool.capacity || number > 0xffff) throw new RangeError('커서 연관 SID 오류');
        return number;
    }

    // 실제 Pool을 쓰는 위치에서만 DWORD를 SID16으로 검사하며 하위 WORD로 별칭하지 않는다.
    #previewSid(value) {
        const number = value >>> 0;
        if (number < 5 || number >= this.#pool.capacity || number > 0xffff) throw new RangeError('커서 미리보기 DWORD의 실제Pool/SID16범위');
        return number;
    }

    // 처음 잠글 때만 미리보기·하드웨어 커서를 바꾸고 현재 깊이를 DWORD로 증가한다.
    #lock() {
        const s = this.#state;
        if (!(s.lockDepth >>> 0)) { this.#hooks.activatePreview(0); this.#hooks.selectCursor(1); }
        s.lockDepth = (s.lockDepth + 1) >>> 0;
    }

    // 월드 분기의 비용 차감 뒤 현재 비용·폭·화면 좌표를 다시 읽고 원문 SP 표시를 요청한다.
    #charge() {
        const s = this.#state; const h = this.#hooks;
        if (!(s.costFlags & 0x40)) return;
        h.addLocalSp(Math.fround(-Math.fround(this.#type().cost)));
        const type = this.#type();
        const pixels = (type.footX << 4) | 0;
        const x = ((s.screenX >>> 0) - (Math.trunc(pixels / 2) >>> 0) - (Math.trunc((s.cursorWidth | 0) / 2) >>> 0)) | 0;
        h.showSp(cameraFtolLow(Math.fround(type.cost)), x, s.screenY | 0, 0);
        if ((s.battle >>> 0) && !(s.blocked >>> 0)) h.battleRefresh();
    }

    // 잠금/불법 위치를 먼저 생략하고, 현재 raw·콜백 뒤 전역을 원문 순서대로 소비한다.
    drop() {
        const s = this.#state; const h = this.#hooks;
        if (s.lockDepth >>> 0) return;
        h.queryCursor(); if (!(s.legal >>> 0)) return;
        const preview = s.preview >>> 0;
        h.prepareCursor(); s.x = roundCoordinate(s.x); s.y = roundCoordinate(s.y); h.prepareCursor();
        h.highlight(0); h.playDrop();
        if (!(s.legalGump >>> 0)) {
            if (s.inputFlags & 0x80) {
                const captured = this.#previewSid(preview);
                const source = this.#sourceSid(h.previewSource(captured));
                h.pop(source, Math.fround(s.x), Math.fround(s.y), 0); h.destroy(captured, 0);
            } else {
                if (s.recording >>> 0) h.record(s.recorder >>> 0, this.#type().name, Math.fround(s.x), Math.fround(s.y), s.argument >>> 0, s.direction >>> 0);
                const x = roundCoordinate(s.x); const y = roundCoordinate(s.y);
                this.#lock(); h.placeLocal({ preview: this.#previewSid(preview), x, y, argument: s.argument >>> 0, direction: s.direction >>> 0 });
            }
            this.#charge();
        } else {
            const allowed = h.guiFlags(s.legalGump >>> 0) >>> 0;
            const captured = this.#previewSid(preview);
            const first = h.previewSource(captured) >>> 0; const second = h.previewSource(captured) >>> 0;
            const raw = this.#pool.slot(captured);
            const flags = raw[20] | (raw[21] << 8); const quality = raw[30];
            if (allowed & flags) { h.invalidGui(); return; }
            h.detach(captured, 0);
            const writable = this.#pool.allocated(captured);
            writable[20] = allowed & 0xff; writable[21] = (allowed >>> 8) & 0xff;
            const parent = h.guiParent(s.legalGump >>> 0) >>> 0;
            if (!parent) throw new Error('cursorLegalGump->parentSid 원본 assert 계약 위반');
            h.attach(captured, parent, 0x10); h.guiReceive(s.legalGump >>> 0, captured);
            if (s.inputFlags & 0x80) h.destroy(this.#sourceSid(first), 0);
            else if (flags & 8) h.returnPiece(flags, second, quality);
        }
        this.#lock(); h.refresh(0, 0, 1);
        if (s.cursorGump >>> 0) { h.closeGump(s.cursorGump >>> 0, 3, 0, 0); s.cursorGump = 0; }
        s.preview = 0; h.activatePreview(1);
    }
}

// 같은 풀의 로컬 배치·SP 보상·SoundPlayer 장치를 연결한 새 훅 묶음이다. 호출자 묶음은 보존한다.
export function makeCursorDropHooks(pool, local, reward, sound, state, hooks = {}) {
    if (local?.pool !== pool || reward?.pool !== pool) throw new TypeError('커서/로컬 배치/보상 SID 풀 연결 오류');
    return {
        ...hooks,
        placeLocal: (request) => local.place(request),
        addLocalSp: (delta) => reward.addSp(state.localPlayer >>> 0, delta),
        playDrop: () => sound.playByName('droppiece.wav', 0, 0, 0, 0, 0),
    };
}
