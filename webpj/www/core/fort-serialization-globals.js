// Template 생성자와 객체 저장·로더가 같은 Client 수명으로 공유하는 원본 10.78 전역 묶음을 옮긴 모듈이다(cpppj `o/FortSerializationGlobals.h`, 기준선 1f00a25a 뒤 0d276d0d 에서 다섯 칸으로 확장).
// 다섯 DWORD 는 판본(005c89c0)·중첩 로딩 깊이(005c89b8)·객체 저장/로드 범위 scope(005c89b0, Terr 1/Chaff 2)·소유자 저장 flags2 마스크(00542644, 초기값 0x5d77cf00)·전투 저장 마스크(0054263c, 초기값 0x40480002)이다.
// Storage/Session/NoWorld 저장/객체 로더는 이 객체의 각 칸 주소를 **빌린다**: 값 사본을 만들지 않고, 빌린 쪽이 만들어지거나 사라질 때 초기값으로 되돌리지도 않는다(Session 교체/반납이 PE 초기값으로 덮지 않음).
// 칸은 모두 32비트 DWORD 이다(쓰기 때 `>>> 0`). 마스크 `00542640`(FortKeepGenus)은 이 묶음에 없는 별도 전역이다.
// 사용: const globals = new FortSerializationGlobals(); const state = createFortObjectLoadStateFromGlobals(globals)(fort-object-load.js). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-11 WB04-E279: 4-B Fort 저장 전역 묶음 이식)

// 소유자 저장 마스크와 전투 저장 마스크의 원본 초기값이다(두 PE 의 .data 초기값). (2026-11-11 WB04-E279)
export const FORT_OWNER_SAVED_MASK_INITIAL = 0x5d77cf00;
export const FORT_BATTLE_SAVED_MASK_INITIAL = 0x40480002;

// 다섯 DWORD 를 소유하는 묶음이다. 각 칸은 읽기/쓰기마다 현재 값을 그대로 돌려주고 저장하는 접근자이다.
// 사용: globals.version = 2; globals.scope = 1; 빌리는 쪽이 같은 객체를 받아 각 칸을 접근자로 읽고 쓴다. (2026-11-11 WB04-E279)
export class FortSerializationGlobals {
    // manager 생성자는 0, 전체 Objects 저장은 2 를 쓰는 판본이다.
    #version = 0;
    // Session/World 없는 메타 Core 로더가 빌리는 중첩 로딩 깊이이다.
    #loadingDepth = 0;
    // 실제 Terr1/Chaff2 로더와 writer 가 World 없이도 쓰는 현재 범위이다.
    #scope = 0;
    // 현재 flags2 저장 마스크와 전투 저장 마스크이다.
    #ownerSavedMask = FORT_OWNER_SAVED_MASK_INITIAL;
    #battleSavedMask = FORT_BATTLE_SAVED_MASK_INITIAL;

    get version() { return this.#version; }
    set version(value) { this.#version = value >>> 0; }

    get loadingDepth() { return this.#loadingDepth; }
    set loadingDepth(value) { this.#loadingDepth = value >>> 0; }

    get scope() { return this.#scope; }
    set scope(value) { this.#scope = value >>> 0; }

    get ownerSavedMask() { return this.#ownerSavedMask; }
    set ownerSavedMask(value) { this.#ownerSavedMask = value >>> 0; }

    get battleSavedMask() { return this.#battleSavedMask; }
    set battleSavedMask(value) { this.#battleSavedMask = value >>> 0; }
}
