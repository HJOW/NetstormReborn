// 독립 원문 입력을 실제 커서 전역에 공급하며 기대 출력은 fixture에서만 읽는다.
#pragma once
#include "client/PreviewDraw.h"
#include "client/RawCursorPlacement.h"
#include "o/RawBridgeConnect.h"

namespace netstorm::test::previewbinding {
// 실제 배치 검사 격자와 커서 상태를 테스트 수명 동안 소유한다.
struct Scene {
    client::CursorDropState drop; // 내려놓기 허용과 표시 허용을 구별하는 실제 상태다.
    client::CursorLifecycleState lifecycle; // 임시 목록·부모·경고 치수의 실제 저장소다.
    client::CursorPlacementState placement; // 현재 설명·판정·화면 원점이다.
    o::BridgeConnectState surfaces; // 수집과 표시가 공유하는 실제 타입 전역이다.
    o::CanonPlacementPipelineState pipeline; // 미리보기 격자는 복사본 대신 이 실제 저장소를 참조한다.
    // 원본 고정 입력의 공통 전역을 공급한다. 기대 출력은 계산하지 않는다.
    void Collection(const client::PreviewCollectionState& c) {
        drop.lockDepth=c.lockDepth;drop.localPlayer=c.localPlayer;drop.x=c.x;drop.y=c.y;drop.legal=!c.legal;
        lifecycle.abstractType=c.type;lifecycle.currentGump=c.parentGump;lifecycle.previewCount=c.pieceCount;
        std::copy(c.pieces.begin(),c.pieces.end(),lifecycle.previews.begin());
        placement.placementAllowed=c.legal;surfaces.islandType=c.specialTypeA;surfaces.stalagType=c.specialTypeB;
    }
    // 표시기의 고정 입력을 해당 원본 주소의 실제 저장소에 공급한다.
    void Drawing(const client::PreviewCollectionState& c,const client::PreviewDrawState& s) {
        Collection(c);drop.type=s.selectedType;surfaces.bridgeType=s.destroyType;surfaces.connectorType=s.noOutlineType;
        placement.screenOrigin={s.cameraX,s.cameraY};placement.labelSpacing=s.spacing;
        placement.bounds={s.labelOffsetX,s.labelOffsetY,s.labelWidth,s.labelHeight};drop.cursorWidth=s.priceWidth;
        lifecycle.costTextWidth=s.priceIconOffset;lifecycle.costHotspotX=s.priceIconX;lifecycle.costHotspotY=s.priceIconY;
        drop.costFlags=s.flags;drop.inputFlags=~s.flags;placement.sourceAllowed=s.messageOverride;
        placement.placementFlags=s.blocked;placement.descriptionAllowed=s.powered;placement.spaceAllowed=s.reachable;
        pipeline.preview.blocked=s.grid;placement.description=s.labels;lifecycle.warningWidths=s.messageWidths;lifecycle.warningHeights=s.messageHeights;
    }
    // 같은 풀의 실제 참조만 반환하며 수집·표시 호출 도중 값 변경을 숨기지 않는다.
    client::CursorPreviewSource Source(const o::SidPool& pool) const { return {pool,drop,lifecycle,placement,surfaces,pipeline.preview.blocked}; }
};
}
