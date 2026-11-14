// 원본 배경 GIF 창(BackgroundGif, VT 00501b60)의 생성·교체·종료 입력·부착·원시 복사 출력·소멸을 옮긴다(원문 0041a030/00419ec0/00419e10/00419df0/00419e40/00419f50/00419d90, cpppj client/BackgroundGifGump.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 원본 GIF 창은 HTML 부모와 달리 기본 Gump 160바이트 뒤에 종료 입력 3개와 소유 원시 GIF 버퍼를 가진 0xb0 바이트 파생이다. 기본 생성/부착/수명/위치/입력 위임은 같은 공통 Gump 장치가 하고, 이 모듈은 파생 필드의 쓰기 순서와 현재 노드 재조회 순서를 보존한다.
// 파일 조회·GIF 해독·소유 버퍼 반납·실제 복사(004a1fe0)·종료 입력 큐(00452f90)는 모두 호스트 훅이다. 실제 연결(파일 시스템, 소유 이미지 표, GumpPixelTarget.rawBitmap)은 RawBackgroundGifControls 조립 단위가 한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const gump = new BackgroundGifGump(state, hooks); const token = gump.construct(token, name); gump.draw(token); gump.handle(token, event, first, second); gump.destruct(token).
// (2026-11-14 WB04-E444: RawMenuControls 선행 배경 GIF Gump 이식)
import { GumpCreateNode } from './gump-create.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 원본 가상 표 주소(실행 포인터가 아닌 근거 식별자)와 입력의 해제 비트/종류 마스크다.
const VTABLE = 0x501b60; const RELEASE_BIT = 0x80000000; const CODE_MASK = 0x03ffffff;
// 종료 입력으로 취급하는 세 입력 코드다(왼쪽 버튼 0x10000, 오른쪽 버튼 0x20000, ESC 27).
const LEFT_CODE = 0x10000; const RIGHT_CODE = 0x20000; const ESCAPE_CODE = 27;
// SetEvent 가 켜는 표시 플래그(재그리기/재배치 요구 0x200|0x400)다.
const DIRTY_FLAGS = 0x600;

// 도달한 실제 장치만 필수로 검사한다(C++ logic_error → Error). 검사는 호출 직전에 하므로 쓰이지 않는 경로의 장치는 비어 있어도 된다.
function need(hook) { if (typeof hook !== 'function') throw new Error('BackgroundGif 실제 장치 미연결'); return hook; }

// 원본 +a0/+a4/+a8 의 종료 입력과 +ac 의 소유 원시 GIF 버퍼 토큰을 가진 노드다. 기본 필드는 공통 Gump 와 같은 객체를 공유한다.
export class BackgroundGifGumpNode extends GumpCreateNode {
    // +a0 종료 입력 코드(0이면 종료 입력 없음), +a4/+a8 그 입력과 함께 전달할 첫째/둘째 DWORD 다.
    event = 0; first = 0; second = 0;
    // +ac 소유 원시 GIF 색 번호 버퍼의 호스트 토큰이다(0이면 버퍼 없음).
    bitmap = 0;
}

// 현재 종류 BYTE(원문 0054041e, signed)다. 10.78 보호 초기값은 3이며 호출 시점의 값을 읽는다.
export class BackgroundGifGumpState {
    // signed 8비트 종류 값이다. Construct 가 부호 확장한 DWORD 를 command 로 기록한다.
    kind = 3;
}

// 한 Client 수명의 실제 파생 노드/장치를 빌려 쓰는 GIF 창 본체다.
// hooks = { node(token), base(token), constructBase(token), destructBase(token), pathSpec(name) → path, exists(path) → bool,
//   load(path, outputs) → bitmap 토큰(outputs.width/outputs.height 는 0 으로 시작하며 실제 두 출력 DWORD 다), releaseBitmap(bitmap), position(token) → {x,y},
//   copy(request), attach(token, x, y, parent, mode), baseHandle(token, event, first, second) → DWORD, dispatch(event, first, second) } 이다.
// copy 요청 = { position:{x,y}, bitmap, stride, source:{left,top,right,bottom} } 이며 원문 004a1fe0 의 원시 복사에 대응한다(배율 확대와 구별).
export class BackgroundGifGump {
    // 현재 종류 상태와 훅 묶음이다(둘 다 호출보다 오래 유지된다).
    #state; #hooks;
    // 같은 파생/기본 노드 조회만 미리 요구한다(나머지 장치는 도달한 분기에서 확인한다). C++ invalid_argument → TypeError.
    constructor(state, hooks) {
        if (typeof hooks?.node !== 'function' || typeof hooks.base !== 'function') throw new TypeError('BackgroundGif 같은 실제 노드 필요');
        this.#state = state; this.#hooks = hooks;
    }

    // 파생 노드를 조회하되 표시 사본을 만들지 않는다. 토큰이 0이거나 저장소가 없거나 기본 노드 조회와 다른 객체면 Error 다(C++ logic_error).
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.base(token)) throw new Error('BackgroundGif 공유 노드 없음');
        return node;
    }

    // 0041a030: 같은 파생 주소를 확인한 뒤 기본 생성(0,0,0,1) → 가상 표/종류(부호 확장)/bitmap 0 → 현재 GIF 적재(Refresh) → 종료 입력 3개를 0 으로 쓴다. 적재 효과 뒤에 세 필드를 쓴다.
    construct(token, name) {
        this.#node(token); need(this.#hooks.constructBase)(token);
        const node = this.#node(token); node.vtable = VTABLE; node.command = ((this.#state.kind << 24) >> 24) >>> 0; node.bitmap = 0;
        this.refresh(token, name);
        this.#node(token).event = 0; this.#node(token).first = 0; this.#node(token).second = 0;
        return token;
    }

    // 00419ec0: 이전 버퍼를 반납하고 0 으로 쓴 뒤, 현재 경로 규칙·파일 존재 여부를 조회해 적재하고 기본/표시 크기를 쓴다. 표시 크기는 정수 치수를 먼저 float 로 좁히지 않고 현재 배율과 곱한 배정도 값을 float32 로 저장한다.
    // 파일이 없으면 bitmap 은 0 이고 치수는 0 이다(오류가 아니다). 노드는 각 쓰기 전에 다시 조회한다(반납/적재 효과가 저장소를 바꿀 수 있다).
    refresh(token, name) {
        const node = this.#node(token); const outputs = { width: 0, height: 0 };
        if (node.bitmap) { need(this.#hooks.releaseBitmap)(node.bitmap); this.#node(token).bitmap = 0; }
        const path = need(this.#hooks.pathSpec)(name);
        if (need(this.#hooks.exists)(path)) { const bitmap = need(this.#hooks.load)(path, outputs) >>> 0; this.#node(token).bitmap = bitmap; }
        const width = outputs.width | 0; const height = outputs.height | 0;
        this.#node(token).baseWidth = Math.fround(width); this.#node(token).width = Math.fround(width * this.#node(token).scale);
        this.#node(token).baseHeight = Math.fround(height); this.#node(token).height = Math.fround(height * this.#node(token).scale);
    }

    // 00419e10: 종료 입력 코드/둘째/첫째 DWORD 를 쓰고 재그리기 플래그(0x600)를 켠다. 쓰기 순서는 event→second→first→flags 다.
    setEvent(token, event, first, second) {
        const node = this.#node(token); node.event = event >>> 0; node.second = second >>> 0; node.first = first >>> 0; node.flags = (node.flags | DIRTY_FLAGS) >>> 0;
    }

    // 00419df0: 같은 현재 x/y 로 부모 0/모드 0 인 가상 부착을 다시 호출한다.
    reattach(token) { const node = this.#node(token); need(this.#hooks.attach)(token, node.x, node.y, 0, 0); }

    // 00419e40: 종료 입력이 있고 해제 입력이 아니며 코드(마스크 후)가 왼쪽/오른쪽 버튼/ESC 이면 그 노드의 입력 세 DWORD 를 큐에 그대로 넘기고 1 을 돌려준다(큐가 창을 반납해도 노드를 다시 읽지 않는다).
    // 그 밖의 입력은 모두 기본 입력 처리에 위임하고 그 DWORD 를 돌려준다.
    handle(token, event, first, second) {
        const node = this.#node(token); const input = event >>> 0; const code = input & CODE_MASK;
        if (node.event && !(input & RELEASE_BIT) && (code === LEFT_CODE || code === RIGHT_CODE || code === ESCAPE_CODE)) {
            const pending = node.event; const a = node.first; const b = node.second;
            need(this.#hooks.dispatch)(pending, a, b); return 1;
        }
        return need(this.#hooks.baseHandle)(token, event, first, second) >>> 0;
    }

    // 00419f50: bitmap 이 없으면 아무것도 하지 않는다. 있으면 현재 위치를 먼저 얻고 기본 크기(배율 크기가 아님)를 ftol 해 폭/높이로 원시 복사 요청을 보낸다(행 간격 = 폭, 소스 사각형 = 0,0,폭,높이).
    // 위치 조회 뒤에 노드를 다시 읽으므로 위치 장치가 bitmap/기본 크기를 바꾸면 그 값이 쓰인다.
    draw(token) {
        if (!this.#node(token).bitmap) return;
        const position = need(this.#hooks.position)(token);
        const width = cameraFtolLow(this.#node(token).baseWidth); const height = cameraFtolLow(this.#node(token).baseHeight); const bitmap = this.#node(token).bitmap;
        need(this.#hooks.copy)({ position, bitmap, stride: width, source: { left: 0, top: 0, right: width, bottom: height } });
    }

    // 00419d90: 가상 표를 다시 쓰고 bitmap 이 있으면 반납한 뒤 기본 소멸을 호출한다. 원문은 반납 뒤 bitmap 필드를 0 으로 쓰지 않는다.
    destruct(token) {
        const node = this.#node(token); node.vtable = VTABLE;
        if (node.bitmap) need(this.#hooks.releaseBitmap)(node.bitmap);
        need(this.#hooks.destructBase)(token);
    }
}
