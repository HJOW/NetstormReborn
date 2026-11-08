// 초기 요새 자산의 postPop 재정의(Gun·제네레이터·거주 구역·바리케이드·간헐천·신전·Factory 아웃포스트)가 공유하는 전역과 목록을 모은 상태 모듈이다(cpppj `o/RawAssetPostPop.h` 의 AssetPostPopPlayer·AssetPostPopState, 10.78 전용).
// 실제 World/Session 은 서버·boss 권한(RawAuthorityState)과 거주 구역 50칸·신전 20칸·간헐천 100칸·아웃포스트 100칸 목록을 Session 보다 긴 owner 에서 빌려 쓰고(shared*),
// 독립 입력은 이 객체가 소유한 값/목록을 쓴다. 읽기 접근자(readServer·readBoss·currentResidences 등)는 소비 위치마다 현재 값을 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E61: Factory 후처리 이식에 필요한 공유 상태로 추가. 자산 후처리 본체 RawAssetPostPop 은 이후 단위에서 이 상태를 그대로 쓴다)

import { SquidPostPopList } from './squid-postpop-state.js';

// 원본 Player 의 신전 네 필드이다. 복사본을 읽지 않도록 실행 월드가 이 저장소를 공유한다. (2026-11-08 WB04-E61)
export class AssetPostPopPlayer {
    // 초기 신전, 초기 신전 복사, 새 신전, 새 신전 복사이다.
    initialTemple = 0;
    initialTempleCopy = 0;
    newTemple = 0;
    newTempleCopy = 0;
}

// Player 번호 1..8 + 0번 칸이다. (2026-11-08 WB04-E61)
const PLAYER_COUNT = 8;

// Gun/Residence/Geyser/Vortex/Factory 재정의가 사용하는 전역과 목록이다. (2026-11-08 WB04-E61)
export class AssetPostPopState {
    // 내부 전투 여부, boss 여부, Fort 모드 여부, 검사 Form 판정 getter(기본 0), 거주 건물 애니메이션 여부이다.
    inBattle = false;
    boss = true;
    fortMode = false;
    checkingForms = () => 0;
    residenceAnimation = false;
    // 로딩 깊이, 신전 개수, 현재 선택한 객체·컨트롤의 번호 또는 토큰, 로컬 소유자이다.
    loadingDepth = 0;
    templeCount = 0;
    selected = 0;
    localOwner = 1;
    // island 자산·islandStalag 받침 자산·slot 자산을 타입 표에서 찾는 번호이다.
    islandType = 94;
    stalagType = 95;
    slotType = 133;
    // Player 0..8 의 신전 필드이다.
    players = Array.from({ length: PLAYER_COUNT + 1 }, () => new AssetPostPopPlayer());
    // 독립 입력의 거주 건물·신전·간헐천·아웃포스트(DAT0055a70c: 신전/아웃포스트의 섬 소유권 재계산용 별도 목록) 목록이다.
    residences = new SquidPostPopList();
    temples = new SquidPostPopList();
    geysers = new SquidPostPopList();
    outposts = new SquidPostPopList();
    // 실제 제품이 같은 Client 에서 빌리는 권한(RawAuthorityState)과 장기 목록이다. 없으면 null 이다.
    sharedAuthority = null;
    sharedResidences = null;
    sharedTemples = null;
    sharedGeysers = null;
    sharedOutposts = null;
    // WORD setter(004ac220)가 통지를 선택하는 독립 Bridge TYPE DWORD 와 실제 World 의 현재 BridgeState TYPE getter(없으면 null)이다.
    bridgeType = 82;
    currentBridgeType = null;

    // 540bc0 서버 권한이다. 독립 옛 fixture 의 battle 열은 역사적으로 같은 서버 입력이었다. (2026-11-08 WB04-E61)
    readServer() {
        return this.sharedAuthority ? Boolean(this.sharedAuthority.server) : this.inBattle;
    }

    // 실제 서버 권한을 쓴다(공유 권한이 있으면 그 객체에 쓴다). (2026-11-08 WB04-E61)
    writeServer(value) {
        if (this.sharedAuthority) this.sharedAuthority.server = Boolean(value);
        else this.inBattle = Boolean(value);
    }

    // 540bc4 boss 권한이다. Vortex/Factory 의 실제 권한 소비 시점에서 현재 값을 읽는다. (2026-11-08 WB04-E61)
    readBoss() {
        return this.sharedAuthority ? Boolean(this.sharedAuthority.boss) : this.boss;
    }

    // boss 권한을 쓴다(공유 권한이 있으면 그 객체에 쓴다). (2026-11-08 WB04-E61)
    writeBoss(value) {
        if (this.sharedAuthority) this.sharedAuthority.boss = Boolean(value);
        else this.boss = Boolean(value);
    }

    // 현재 생성/삭제가 같은 목록에 직접 쓰는 거주 건물 목록이다(실제 Session 은 50칸). (2026-11-08 WB04-E61)
    currentResidences() { return this.sharedResidences ?? this.residences; }

    // 현재 신전 목록이다(실제 20칸, 아웃포스트 100칸과 구별). (2026-11-08 WB04-E61)
    currentTemples() { return this.sharedTemples ?? this.temples; }

    // 현재 간헐천 Pop/삭제의 실제 목록이다(100칸). (2026-11-08 WB04-E61)
    currentGeysers() { return this.sharedGeysers ?? this.geysers; }

    // 지역 소유권과 캐리어의 현재 아웃포스트 목록이다(원본의 별도 100칸 정체성). (2026-11-08 WB04-E61)
    currentOutposts() { return this.sharedOutposts ?? this.outposts; }

    // WORD 저장 뒤 현재 TYPE 비교에서만 읽는 Bridge TYPE 의 전체 DWORD 이다. (2026-11-08 WB04-E61)
    readBridgeType() {
        return this.currentBridgeType ? this.currentBridgeType() >>> 0 : this.bridgeType;
    }
}
