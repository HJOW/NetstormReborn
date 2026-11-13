// Client 수명의 커서 수명/위치 전역을 소유한다(cpppj client/ClientCursorGlobals.h, 10.78).
// lifecycle/placement/옵션 상자의 주소는 같은 Client 수명 동안 유지한다. 실행기·Session 콜백을 새로 저장하지 않는다.
// resetForGeneralWorld는 배열과 상태 객체를 제자리 초기화하고 현재 진단/카메라 참조와 회전 옵션을 보존한다. NativeCancel 효과와 구별한다.
// 이 모듈은 DOM을 쓰지 않는다. (2026-11-14 WB04-E402: 커서 기반 부품 뒤의 장기 소유자)
import { CursorLifecycleState } from './raw-cursor-lifecycle.js';
import { CursorPlacementState } from './raw-cursor-placement.js';

// 같은 DWORD를 빌리는 주소 고정 상자다. 저장은 unsigned32 폭으로 정규화한다.
function wordBox(words, index) {
    return { get value() { return words[index]; }, set value(next) { words[index] = next; } };
}

// 원본 aggregate 대입을 주소가 바뀌지 않는 제자리 필드/배열 복사로 옮긴다.
function resetState(target, fresh) {
    // TypedArray와 화면 점은 기존 대여 주소를 보존하고 스칼라/문자열은 값을 교체한다.
    for (const key of Object.keys(fresh)) {
        if (ArrayBuffer.isView(fresh[key]) && ArrayBuffer.isView(target[key])) target[key].set(fresh[key]);
        else if (key === 'screenOrigin') { target[key].x = fresh[key].x; target[key].y = fresh[key].y; }
        else target[key] = fresh[key];
    }
}

// 짧은 World/Session보다 오래 사는 단일 커서 데이터 소유자다.
export class ClientCursorGlobals {
    // 최초 PE/BSS 이미지로만 만든 두 상태다.
    #lifecycle = new CursorLifecycleState(); #placement = new CursorPlacementState();
    // 별도 두 옵션 DWORD와 주소 고정 상자다. rotationMode는 일반 초기화로 지우지 않는다.
    #options = new Uint32Array(2); #snap = wordBox(this.#options, 0); #rotation = wordBox(this.#options, 1);

    // 실제30DWORD/count/측정/현재 Gump 상태를 같은 주소로 빌린다.
    lifecycle() { return this.#lifecycle; }
    // 현재 위치/경고/그림/설명과 카메라 참조를 같은 주소로 빌린다.
    placement() { return this.#placement; }
    // 현재 snapToGrid DWORD다. 옵션/입력은 같은 상자를 공유한다.
    snapToGrid() { return this.#snap; }
    // 현재 회전 모드 DWORD다. 일반 reset이나 Session 반납으로 지우지 않는다.
    rotationMode() { return this.#rotation; }

    // 일반 Runtime의 fresh/반납 호환 초기화다. 진단/카메라 대여는 보존하고 snap만0으로 돌린다.
    resetForGeneralWorld() {
        const pixels = this.#placement.sharedScreenOrigin; const checking = this.#lifecycle.checking;
        resetState(this.#lifecycle, new CursorLifecycleState()); this.#lifecycle.checking = checking;
        resetState(this.#placement, new CursorPlacementState()); this.#placement.sharedScreenOrigin = pixels;
        this.#snap.value = 0;
    }
}
