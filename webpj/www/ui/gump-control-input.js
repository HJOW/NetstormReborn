// 현재 메뉴/돌 버튼의 사각형 입력 어댑터를 이식한다(cpppj client/Gump.cpp/.h, 기준선 1f00a25a).
// 복원된 GumpInput 트리 처리기와는 다른 타입/저장소를 쓰는 화면 단위 판정 영역 목록이며, 자동 검사(메뉴 스모크)도 같은 좌표·입력 경로를 쓴다.
// 사용: const input = new GumpControlInput(); input.setControls([{ id: 1, rect: { left: 0, top: 0, right: 10, bottom: 10 } }]); input.event({ code: InputCode.leftButton, x: 1, y: 1 }).
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E433: RawMenuControls 선행 Gump 입력 어댑터 이식)
import { InputCode } from '../core/input-queue.js';

// 입력 종류 마스크(키/버튼 종류 비트만)다. 수식·뗌·창 밖 비트는 마스크 밖이다.
const KIND_MASK = 0x03ffffff;

// Gump 입력 대상의 식별자·사각형·동작 상태다. menu 는 목록 행(누르는 순간 실행·호버 있음), instant 는 뗌을 기다리지 않는 버튼, enabled 는 입력 가능 여부다.
export class GumpControl {
    // 대상을 구별하는 식별 번호, 출력·적중·배치 검사에 쓰는 사각형(오른쪽/아래 끝 제외)이다.
    id = 0; rect = { left: 0, top: 0, right: 0, bottom: 0 };
    // 메뉴 행 여부, 입력 가능 여부(기본 true), 누름 즉시 동작 여부다.
    menu = false; enabled = true; instant = false;
    // 일부 필드만 주어도 원본 집합 초기화와 같이 나머지는 기본값이다.
    constructor(fields = {}) { Object.assign(this, fields); }
}

// 현재 메뉴의 사각형 입력 어댑터다. 화면 판정 영역을 바꾸면 이전 화면의 누름 캡처를 해제한다.
export class GumpControlInput {
    // 입력 대상 목록(앞의 항목이 우선)과 버튼 해제까지 입력을 받는 캡처 대상 번호, 눌린 돌 버튼 번호, 강조된 목록 행 번호(없으면 undefined)다.
    #controls = []; #capture; #pressed; #hovered;
    // 마지막으로 받은 포인터 위치다(사건/Move 마다 갱신).
    #point = { x: 0, y: 0 };

    // 004248e0: 오른쪽/아래 끝은 제외한다. 호출자가 버튼에 세로 1픽셀을 더한다. 사각형은 정수 화면 좌표다.
    static contains(rect, point) { return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom; }

    // 현재 화면의 판정 영역을 바꾸며 이전 화면의 누름 캡처를 해제한다(이전 화면에서 누른 상태가 새 화면의 같은 번호를 실행하지 않게 한다).
    setControls(controls) { this.#controls = Array.from(controls, (control) => (control instanceof GumpControl ? control : new GumpControl(control))); this.cancel(); }

    // 004249a0·00476820: 돌 버튼은 같은 곳에서 뗄 때, 목록/즉시 버튼은 누르는 순간 실행해 번호를 돌려준다(없으면 undefined). 왼쪽 버튼 이외의 누름은 돌 버튼을 실행하지 않는다.
    event(event) {
        if (((event.code >>> 0) & InputCode.outside) !== 0) { this.cancel(); return undefined; }
        if ((((event.code >>> 0) & KIND_MASK) >>> 0) !== InputCode.leftButton) return undefined;
        this.move({ x: event.x, y: event.y });
        if (((event.code >>> 0) & InputCode.release) !== 0) {
            const activated = this.#pressed; this.#capture = undefined; this.#pressed = undefined; return activated;
        }
        // 가장 앞의 활성 판정 영역만 현재 사건을 받는다.
        for (const control of this.#controls) if (GumpControlInput.contains(control.rect, this.#point)) {
            if (!control.enabled) return undefined;
            if (control.menu || control.instant) return control.id;
            this.#capture = control.id; this.#pressed = control.id; return undefined;
        }
        return undefined;
    }

    // 누른 버튼에서 나갔다 돌아오는 모양과 목록 행 강조를 폴링 위치에 맞춘다. 활성 메뉴에는 호버가 있지만 돌 버튼은 누름 캡처 중에만 모습이 변한다. 상태가 바뀌었는지 돌려준다.
    move(point, active = true) {
        const oldPressed = this.#pressed; const oldHovered = this.#hovered; this.#point = { x: point.x, y: point.y };
        this.#pressed = undefined; this.#hovered = undefined;
        if (!active) { this.#capture = undefined; return oldPressed !== this.#pressed || oldHovered !== this.#hovered; }
        // 새 좌표에서 눌림/강조를 계산한다.
        for (const control of this.#controls) if (control.enabled && GumpControlInput.contains(control.rect, point)) {
            if (this.#capture === control.id) this.#pressed = control.id;
            if (control.menu) this.#hovered = control.id;
        }
        return oldPressed !== this.#pressed || oldHovered !== this.#hovered;
    }

    // 현재 눌린 모양의 돌 버튼 번호다(실제 돌 버튼 그리기에서 사용한다).
    pressed() { return this.#pressed; }

    // 현재 강조된 목록 행 번호다(목록 강조 색을 적용할 번호).
    hoveredMenu() { return this.#hovered; }

    // 포커스 상실/전환·화면 변화 뒤 남은 캡처와 강조를 지운다.
    cancel() { this.#capture = undefined; this.#pressed = undefined; this.#hovered = undefined; }

    // 자동 검사에서 실제 화면의 판정 영역을 읽는다(같은 저장 목록의 읽기용 복사본이 아닌 현재 목록).
    controls() { return this.#controls; }
}
