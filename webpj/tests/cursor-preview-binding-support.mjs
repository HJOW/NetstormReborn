// cpppj CursorPreviewBindingSupport와 같은 독립 입력을 실제 커서 전역에 공급한다. 기대값은 계산하지 않는다. (2026-11-14 WB04-E404)
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { ClientCursorGlobals } from '../www/core/client-cursor-globals.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { createCanonPlacementPipelineState } from '../www/core/raw-canon-placement-pipeline.js';

// 실제 커서 소유자와 Canon 격자를 시험 수명 동안 유지한다.
export class CursorPreviewBindingScene {
    // 최종 내려놓기와 별도 표시 허용을 구분하는 현재 전역이다.
    drop = new CursorDropState(); owner = new ClientCursorGlobals(); lifecycle = this.owner.lifecycle(); placement = this.owner.placement(); surfaces = new BridgeConnectState();
    // 실제 배치 검사기의 격자를 같은 주소로 빌린다.
    pipeline = createCanonPlacementPipelineState();

    // 원문 수집 입력을 해당 주소의 저장소에 준다. 중복 독립 상태는 나중에 다르게 바꿀 수 있다.
    collection(c) {
        Object.assign(this.drop, { lockDepth: c.lockDepth, localPlayer: c.localPlayer, x: c.x, y: c.y, legal: c.legal ? 0 : 1 });
        Object.assign(this.lifecycle, { abstractType: c.type, currentGump: c.parentGump, previewCount: c.pieceCount }); this.lifecycle.previews.set(c.pieces);
        this.placement.placementAllowed = c.legal; this.surfaces.islandType = c.specialTypeA; this.surfaces.stalagType = c.specialTypeB;
    }

    // 표시 입력을 실제 커서 상태와 Canon 격자에 공급한다.
    drawing(c, s) {
        this.collection(c); this.drop.type = s.selectedType; this.surfaces.bridgeType = s.destroyType; this.surfaces.connectorType = s.noOutlineType;
        Object.assign(this.placement.screenOrigin, { x: s.cameraX, y: s.cameraY }); this.placement.labelSpacing = s.spacing;
        this.placement.bounds.set([s.labelOffsetX, s.labelOffsetY, s.labelWidth, s.labelHeight]); this.drop.cursorWidth = s.priceWidth;
        Object.assign(this.lifecycle, { costTextWidth: s.priceIconOffset, costHotspotX: s.priceIconX, costHotspotY: s.priceIconY });
        Object.assign(this.drop, { costFlags: s.flags, inputFlags: ~s.flags }); Object.assign(this.placement, { sourceAllowed: s.messageOverride, placementFlags: s.blocked, descriptionAllowed: s.powered, spaceAllowed: s.reachable });
        this.pipeline.preview.blocked.set(s.grid); this.placement.description = String.fromCharCode(...s.labels);
        this.lifecycle.warningWidths.set(s.messageWidths); this.lifecycle.warningHeights.set(s.messageHeights);
    }

    // 같은 풀의 실제 참조만 전달한다.
    source(pool) { return { pool, drop: this.drop, lifecycle: this.lifecycle, placement: this.placement, surfaces: this.surfaces, grid: this.pipeline.preview.blocked }; }
}
