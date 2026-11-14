// Slider/돌 화살표/배경 손잡이의 전체 정상 생성과 실제 가상 Draw/scalar 삭제를 기존 계층(기본 Gump/돌 버튼/배경/Slider)에 결합한다.
// (cpppj client/ScrollGumpWidgets.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 vtable·손잡이 할당 크기(0xd4)·FILD 중간값 보존은 옮기지 않는다)
// 부분 초기화·signed 정렬 BYTE·원본 new 크기·현재 자식 재조회와 돌 버튼/배경의 실제 출력 순서를 유지한다. 자동 Client 조립은 상위 계층 책임이다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E426: RawMenuControls 선행 ScrollGumpWidgets 이식)
import { StoneButtonGumpNode } from './stone-button-gump.js';

// 실제 파생 가상 표와 new 요청 크기다(주소는 근거 식별자이며 호스트 함수 포인터로 쓰지 않는다).
const SLIDER_VTABLE = 0x5116f8; const ARROW_VTABLE = 0x5116a8; const THUMB_VTABLE = 0x511668; const ARROW_BYTES = 0x178; const THUMB_BYTES = 0xd8;
// 원본 손잡이 바탕은 눌림 질감/전체 테두리, 내부 사각형은 일반 질감/전체 테두리다. 키보드 입력 제외 해제 비트와 팝업 유지 비트다.
const TRACK_FLAGS = 0x03ec0000; const HANDLE_FLAGS = 0x03e00000; const THUMB_FLAGS = 0x201; const KEYBOARD = 0x200; const KEEP_POPUPS = 0x20;
// 원본 기본 크기와 초기 스크롤 값이다. 두 축 중 값 0 인 축만 40 으로 치환한다.
const DEFAULT_SIZE = 40; const INITIAL_MAXIMUM = 100; const INITIAL_PAGE = 10; const INITIAL_POSITION = 50;
// 원본 정수 크기/좌표 산술은 하위 DWORD 만 유지한다.
const add = (first, second) => (first + second) | 0;
const subtract = (first, second) => (first - second) | 0;
// signed BYTE 정렬의 확장 결과를 원본 DWORD 비트로 보관한다.
const alignment = (value) => ((value << 24) >> 24) >>> 0;
// CRT _ftol 의 signed 64비트 절삭 결과의 하위 DWORD 다. 무효 변환의 하위 DWORD 는 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 산술 결과는 새 float 저장이다. 원문 직접 복사의 신호 NaN 비트를 재사용하지 않는다.
function setDimension(node, field, value) { node[field] = Math.fround(value); if (node.floatBits) delete node.floatBits[field]; }
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }

// 원본 StoneButton 파생 +170 방향이다. 버튼/배경/글자 문맥과 동일 객체를 공유한다.
export class ScrollGumpArrowNode extends StoneButtonGumpNode {
    // 화살표 방향(0 위, 1 오른쪽, 2 아래, 3 왼쪽 — 삼각형 선 분기의 기준 값)이다.
    direction = 0;
}

// 현재 +20 종류 BYTE 와 손잡이 기본 배경 전역이다. 초기 종류는 두 원본에서 39/51/1 이며 위치 정렬 +28 과 구별한다.
// defaultType 은 원본 자산 로딩 후의 타입 토큰을 공급해야 한다(배포 PE 초기값은 0, frame 은 1).
export class ScrollGumpWidgetState {
    // Slider/손잡이/화살표의 종류 BYTE(signed)다.
    sliderAlignment = 39; thumbAlignment = 51; arrowAlignment = 1;
    // 손잡이 기본 배경의 타입 토큰과 프레임이다.
    defaultType = 0; defaultFrame = 1;
}

// Slider 전체 생성과 화살표/손잡이 Draw 를 기존 기본 Gump/돌 버튼/배경/Slider 처리기에 위임한다.
// hooks: { node, arrow, thumb, base, allocate(bytes)→token(0 실패), attach(token,x,y,parent,mode) }. node/arrow/thumb/base 는 기존 생성·트리·버튼·배경·Slider 와 같은 객체를 반환해야 한다.
// 사용: new ScrollGumpWidgets(create, state, factory, stones, backgrounds, sliders, lifetime, hooks).construct(10, 9, 100, type, frame).
export class ScrollGumpWidgets {
    // 같은 생성 상태·전역·처리기와 노드 저장소·가상 경계다.
    #create; #state; #factory; #stones; #backgrounds; #sliders; #lifetime; #hooks;
    // 조회 경계만 필수이며 new/가상 Attach/폰트/픽셀/자산은 실제 도달 시 기존 처리기가 검사한다.
    constructor(create, state, factory, stones, backgrounds, sliders, lifetime, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.arrow !== 'function' || typeof hooks.thumb !== 'function' || typeof hooks.base !== 'function') throw new TypeError('Slider 생성/표시 공유 저장소 필요');
        this.#create = create; this.#state = state; this.#factory = factory; this.#stones = stones; this.#backgrounds = backgrounds; this.#sliders = sliders; this.#lifetime = lifetime; this.#hooks = hooks;
    }

    // 동일 Slider 의 값/자식/기본 등록 필드를 읽는다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || node !== this.#hooks.base(token)) throw new Error('Slider 생성 공유 노드 없음'); return node; }
    // 기존 돌 버튼 처리기가 같은 파생 주소의 글자 문맥/배경을 사용해야 한다.
    #arrow(token) { const node = token ? this.#hooks.arrow(token) : null; if (!node || node !== this.#hooks.base(token)) throw new Error('Slider 화살표 공유 노드 없음'); return node; }
    // 손잡이는 실제 기본 배경을 상속하므로 shape/타입/타일/클립을 복제하지 않는다.
    #thumb(token) { const node = token ? this.#hooks.thumb(token) : null; if (!node || node !== this.#hooks.base(token)) throw new Error('Slider 손잡이 공유 노드 없음'); return node; }
    // scalar 위임은 세 파생 객체 모두 같은 기본 저장소로 처리한다.
    #base(token) { const node = token ? this.#hooks.base(token) : null; if (!node) throw new Error('Slider 기본 노드 없음'); return node; }

    // 빈 라벨/모든 8슬롯을 초기화한 명시 문맥 {0,0,color,0,0,0,1,1}으로 기존 돌 버튼 전체 생성자를 호출한 뒤 파생 가상 표·팝업 유지 비트·정렬·방향을 쓴다.
    // 원본 Slider 생성 내부에서는 type 인자가 `unused`, frame 인자가 `color` 로 전달된다.
    constructArrow(token, width, height, direction, unused, color) {
        this.#arrow(token);
        this.#stones.construct(token, width, height, null, unused, Uint32Array.from([0, 0, color >>> 0, 0, 0, 0, 1, 1])); const node = this.#arrow(token);
        node.vtable = ARROW_VTABLE; node.buttonFlags = (node.buttonFlags | KEEP_POPUPS) >>> 0; node.command = alignment(this.#state.arrowAlignment); node.direction = direction >>> 0;
        return token;
    }

    // 기본 배경 (0,0,0x3ec0000,0x201)을 전역 기본 type/frame 으로 생성한 뒤 파생 표/rect 0/정렬/크기/인자 사본을 쓴다. 기본 배경 생성에 넘기는 type/frame 은 손잡이의 추가 인자 사본과 별개다.
    // 이전 y/x 와 type 0 일 때 기본 shape/타일 필드는 원본처럼 미작성 상태를 보존한다.
    constructThumb(token, width, height, type, frame) {
        this.#thumb(token); const defaultType = this.#state.defaultType; const defaultFrame = this.#state.defaultFrame;
        this.#backgrounds.construct(token, 0, 0, TRACK_FLAGS, THUMB_FLAGS, defaultType, defaultFrame); const node = this.#thumb(token);
        node.vtable = THUMB_VTABLE; Object.assign(node.handle, { left: 0, top: 0, right: 0, bottom: 0 }); node.command = alignment(this.#state.thumbAlignment); node.detailType = type >>> 0; node.detailFrame = frame >>> 0;
        width |= 0; height |= 0;
        setDimension(node, 'baseWidth', width); setDimension(node, 'width', width * node.scale); setDimension(node, 'baseHeight', height); setDimension(node, 'height', height * node.scale);
        node.flags = (node.flags | KEYBOARD) >>> 0;
        return token;
    }

    // 성공한 new 만 파생 생성자로 진행한다. 실패(0)는 원본 null 토큰을 돌려준다.
    #newArrow(width, height, direction, type, frame) {
        const token = need(this.#hooks.allocate, 'Slider 자식 할당 경계 필요')(ARROW_BYTES) >>> 0;
        return token ? this.constructArrow(token, width, height, direction, type, frame) : 0;
    }
    #newThumb(width, height, type, frame) {
        const token = need(this.#hooks.allocate, 'Slider 자식 할당 경계 필요')(THUMB_BYTES) >>> 0;
        return token ? this.constructThumb(token, width, height, type, frame) : 0;
    }

    // 처음 자식 부착에서도 실제 가상 함수를 쓰고 원본 null 접근은 호스트 예외로 처리한다. 부모 token/mode 0 이다.
    #attach(token, x, y, parent) { this.#base(token); need(this.#hooks.attach, 'Slider 자식 가상 Attach 경계 필요')(token, x, y, parent, 0); }

    // 기본 (0,0,0,1) 생성 뒤 Slider 필드를 초기화한다: 0 크기는 40, 높이<폭이면 가로(scrollFlags 1), 키보드 비트·step 1, float32 크기(배율 높이는 저장한 float 를 다시 읽음).
    // 두께는 가로면 저장한 높이, 세로면 저장 전 폭(패치 x87 값)의 CRT 절삭이다. 세 자식(감소/증가 화살표·손잡이)을 현재 토큰 순서로 할당·부착한 뒤 position 0 → 범위 0..100 → page 10 → position 50 순서로 값을 통지한다.
    // 할당 실패(0)는 자식 토큰 0 이 되고 이어지는 부착이 호스트 예외로 거부한다. +bc 와 미작성 자식 파생 필드는 보존한다.
    construct(token, width, height, type, frame) {
        this.#node(token); this.#factory.construct(token, 0, 0, 0, 1); const node = this.#node(token);
        node.vtable = SLIDER_VTABLE; node.minimum = node.maximum = node.position = node.page = node.step = 0;
        node.reservedB0 = 0; node.scrollFlags = 0; node.reserved[0] = 0; node.reserved[1] = 0; node.decrement = node.increment = node.thumb = 0;
        node.command = alignment(this.#state.sliderAlignment);
        width |= 0; height |= 0;
        if (!width) width = DEFAULT_SIZE; if (!height) height = DEFAULT_SIZE;
        const horizontal = height < width; node.scrollFlags = horizontal ? 1 : 0; node.flags = (node.flags | KEYBOARD) >>> 0; node.step = 1;
        setDimension(node, 'baseWidth', width); setDimension(node, 'width', width * node.scale);
        setDimension(node, 'baseHeight', height); setDimension(node, 'height', node.baseHeight * node.scale);
        const thickness = ftol(horizontal ? node.baseHeight : width); const twice = add(thickness, thickness);
        // C++17 대입 순서(우변이 먼저)를 따른다: 할당/생성 뒤에 현재 노드를 조회해 토큰을 쓴다.
        if (horizontal) {
            let child = this.#newArrow(thickness, height, 3, type, frame); this.#node(token).decrement = child;
            child = this.#newArrow(thickness, height, 1, type, frame); this.#node(token).increment = child;
            child = this.#newThumb(subtract(width, twice), height, type, frame); this.#node(token).thumb = child;
            this.#attach(this.#node(token).decrement, 0, 0, token);
            this.#attach(this.#node(token).increment, Math.fround(subtract(width, thickness)), 0, token);
            this.#attach(this.#node(token).thumb, Math.fround(thickness), 0, token);
        } else {
            let child = this.#newArrow(width, thickness, 0, type, frame); this.#node(token).decrement = child;
            child = this.#newArrow(width, thickness, 2, type, frame); this.#node(token).increment = child;
            child = this.#newThumb(width, subtract(height, twice), type, frame); this.#node(token).thumb = child;
            this.#attach(this.#node(token).decrement, 0, 0, token);
            this.#attach(this.#node(token).increment, 0, Math.fround(subtract(height, thickness)), token);
            this.#attach(this.#node(token).thumb, 0, Math.fround(thickness), token);
        }
        this.#node(token).position = 0;
        this.#sliders.setRange(token, 0, INITIAL_MAXIMUM); this.#sliders.setPage(token, INITIAL_PAGE); this.#sliders.setPosition(token, INITIAL_POSITION);
        return token;
    }

    // 실제 돌 버튼 Draw 뒤 현재 Bounds/기본 글자 색/방향으로 삼각형 세 선을 그린다. 안쪽 여백 2, 중점의 합은 DWORD 로 감기고 나눗셈은 0 방향으로 절삭한다.
    // 방향 1 은 오른쪽을, 2 는 아래를, 3 은 왼쪽을, 그 밖(0)은 위를 가리킨다. 사용: arrow.drawArrow(token, collection, text, target).
    drawArrow(token, collection, text, target) {
        this.#arrow(token); this.#stones.draw(token, collection, text, target); const bounds = collection.bounds(token);
        const color = this.#arrow(token).textContext[2]; const direction = this.#arrow(token).direction;
        const left = add(bounds.left, 2); const top = add(bounds.top, 2); const right = subtract(bounds.right, 2); const bottom = subtract(bounds.bottom, 2);
        const middleX = Math.trunc(add(left, right) / 2); const middleY = Math.trunc(add(top, bottom) / 2);
        const line = (x1, y1, x2, y2) => ({ first: { x: x1, y: y1 }, last: { x: x2, y: y2 }, color });
        let lines;
        if (direction === 1) lines = [line(left, top, left, bottom), line(left, top, right, middleY), line(left, bottom, right, middleY)];
        else if (direction === 2) lines = [line(left, top, right, top), line(left, top, middleX, bottom), line(right, top, middleX, bottom)];
        else if (direction === 3) lines = [line(right, top, right, bottom), line(right, top, left, middleY), line(right, bottom, left, middleY)];
        else lines = [line(left, bottom, right, bottom), line(left, bottom, middleX, top), line(right, bottom, middleX, top)];
        target.lines(lines);
    }

    // 실제 배경 Draw 가 갱신한 현재 위치/타입/프레임을 다시 읽어 현재 Bounds+handle 을 기본 배경 type/frame 으로 다시 그린다.
    // 원본은 양 끝 포함 선택 rect 의 right/bottom 을 +1 하지 않고 half-open 배경 함수에 그대로 전달한다.
    drawThumb(token, collection, backgrounds) {
        this.#thumb(token); backgrounds.draw(token); const bounds = collection.bounds(token); const node = this.#thumb(token);
        const rect = { left: add(bounds.left, node.handle.left), top: add(bounds.top, node.handle.top), right: add(bounds.left, node.handle.right), bottom: add(bounds.top, node.handle.bottom) };
        backgrounds.background(rect, HANDLE_FLAGS, node.type, node.frame);
    }

    // 원본 세 가상 scalar 삭제는 추가 소유 자원이 없어 기본 Gump 소멸/flags bit 0 반납에 위임한다. 자식은 기존 수명 처리기가 먼저 지운다.
    deleteScalar(token, flags) { this.#base(token); return this.#lifetime.deleteScalar(token, flags); }
}

// Slider 의 빈 Draw 와 화살표/손잡이 Draw 를 같은 kind 2 클립/픽셀 처리기로 분배하고 기존 처리를 보존한다.
// 원본 Slider 가상 Draw 는 ret 뿐인 빈 몸체다. resolve 는 호출 때마다 평가한다.
export function bindScrollGumpWidgetDraw(hooks, collection, text, target, backgrounds, slider, arrow, thumb) {
    if (typeof slider !== 'function' || typeof arrow !== 'function' || typeof thumb !== 'function') throw new TypeError('Slider Draw 분류 경계 필요');
    const previous = hooks.draw;
    hooks.draw = (token) => {
        if (slider(token)) return;
        const arrowHandler = arrow(token); if (arrowHandler) { arrowHandler.drawArrow(token, collection, text, target); return; }
        const thumbHandler = thumb(token); if (thumbHandler) { thumbHandler.drawThumb(token, collection, backgrounds); return; }
        if (!previous) throw new TypeError('Slider 외 Draw 처리기 필요');
        previous(token);
    };
}

// 현재 파생 토큰의 처리기를 지연 조회해 공통 자식 우선 Destroy/Cleanup 과 결합한다. 파생 가상 분배를 재호출하지 않고 기본 scalar 소멸 몸체에 직접 위임한다.
export function bindScrollGumpWidgetLifetime(hooks, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('Slider 수명 분류 경계 필요');
    const previous = hooks.deletingDestructor;
    hooks.deletingDestructor = (token, flags) => {
        const handler = resolve(token); if (handler) { handler.deleteScalar(token, flags); return; }
        if (!previous) throw new TypeError('Slider 외 scalar 소멸자 필요');
        previous(token, flags);
    };
}
