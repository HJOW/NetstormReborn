// 수집/표시가 같은 PATCH 커서 전역을 사본 없이 읽는 참조 어댑터다(cpppj client/CursorPreviewSource.h).
// 사용: bindCursor({ pool, drop, lifecycle, placement, surfaces, grid }). 전역 객체/배열은 소비자보다 오래 살아야 한다.
// 최종 drop.legal과 placement.placementAllowed를 구분하고 출력/SP 재진입 뒤 현재 필드를 읽는다. (2026-11-14 WB04-E404)

// C++ 참조 구조체처럼 연결 시 참조 대상만 고정하며 다른 풀/판본은 거부한다.
export function captureCursorPreviewSource(pool, source, label) {
    if (source?.pool !== pool || pool?.edition !== 'patch1078') throw new TypeError(`${label} cursor pool/edition`);
    return { pool, drop: source.drop, lifecycle: source.lifecycle, placement: source.placement, surfaces: source.surfaces, grid: source.grid };
}

// 수집 본체의 독립 입력과 같은 이름으로 현재 커서 전역을 빌린다. SP 표시 피드백 상태는 기존 state에 남는다.
export function cursorPreviewCollectionInput(source) {
    const d = source.drop; const l = source.lifecycle; const p = source.placement; const s = source.surfaces;
    return {
        get lockDepth() { return d.lockDepth; }, get type() { return l.abstractType; }, get parentGump() { return l.currentGump; },
        get specialTypeA() { return s.islandType; }, get specialTypeB() { return s.stalagType; }, get legal() { return p.placementAllowed; },
        get pieceCount() { return l.previewCount; }, get pieces() { return l.previews; }, get x() { return d.x; }, get y() { return d.y; }, get localPlayer() { return d.localPlayer; },
    };
}

// 원래 std::string의 BYTE 문자 번호를 기존 Draw의 바이트 순회 계약에 연결한다.
function labelBytes(text) { return Uint8Array.from(text, (letter) => letter.charCodeAt(0) & 255); }

// 커서가 소유한 출력 필드는 현재 getter로, 고정 출력 설정/색/문자 표는 기존 state를 통해 읽는다.
export function cursorPreviewDrawInput(source, state) {
    const d = source.drop; const l = source.lifecycle; const p = source.placement; const s = source.surfaces;
    const current = {
        get selectedType() { return d.type; }, get destroyType() { return s.bridgeType; }, get noOutlineType() { return s.connectorType; },
        get cameraX() { return p.screenOriginX(); }, get cameraY() { return p.screenOriginY(); }, get spacing() { return p.labelSpacing; },
        get labelOffsetX() { return p.bounds[0]; }, get labelOffsetY() { return p.bounds[1]; }, get labelWidth() { return p.bounds[2]; }, get labelHeight() { return p.bounds[3]; },
        get priceWidth() { return d.cursorWidth; }, get priceIconOffset() { return l.costTextWidth; }, get priceIconX() { return l.costHotspotX; }, get priceIconY() { return l.costHotspotY; },
        get flags() { return d.costFlags; }, get messageOverride() { return p.sourceAllowed; }, get blocked() { return p.placementFlags; },
        get powered() { return p.descriptionAllowed; }, get reachable() { return p.spaceAllowed; }, get grid() { return source.grid; },
        get labels() { return labelBytes(p.description); }, get messageWidths() { return l.warningWidths; }, get messageHeights() { return l.warningHeights; },
    };
    // 이식 본체가 읽는 다른 설정은 소유자를 복사하지 않고 현재 값으로 전달한다.
    return new Proxy(current, { get(target, key) { return key in target ? target[key] : state[key]; } });
}
