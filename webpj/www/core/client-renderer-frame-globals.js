// Client 수명 동안 Renderer의 구름 사각형·행 스캔·표시 옵션을 같은 저장소에서 빌린다.
// cpppj ClientRendererFrameGlobals.h의 BSS 최초값이며 Session이나 화면 교체 때 초기화하지 않는다.

// unsigned DWORD를 같은 주소의 상자로 제공한다.
function wordBox(words, index) {
    return { get value() { return words[index]; }, set value(next) { words[index] = next; } };
}

// 화면 장치보다 오래 사는 구름 큐와 19 DWORD 스캔 소유자다.
export class ClientRendererFrameGlobals {
    // 행 index·16행·초기화·이전 flags의 연속 저장소다.
    #scan = new Uint32Array(19);
    // 구름 4칸과 개수, 묶음 숨김은 일반 Renderer dirty 표와 별도다.
    #rectangles = Array.from({ length: 4 }, () => ({ left: 0, top: 0, right: 0, bottom: 0 }));
    #words = new Uint32Array(2);
    #count = wordBox(this.#words, 0);
    #hideGroups = wordBox(this.#words, 1);
    #queue = { rectangles: this.#rectangles, count: this.#count };

    // 실제 19 DWORD를 빌린다. 스캔 표는 복사하거나 매 프레임 지우지 않는다.
    scan() { return this.#scan; }
    // 추가와 배경 갱신에 같은 네 칸과 현재 개수를 빌린다.
    cloudQueue() { return this.#queue; }
    // 진단 및 표시에서 현재 네 칸을 읽는다.
    cloudRectangles() { return this.#rectangles; }
    // 구름 큐의 개수 DWORD다.
    cloudDirtyCount() { return this.#count; }
    // 건물 숨김과 구별되는 묶음 숨김 DWORD다.
    hideGroups() { return this.#hideGroups; }
}
