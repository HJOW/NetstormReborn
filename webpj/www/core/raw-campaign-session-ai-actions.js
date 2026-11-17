// Session AI 공개 행동의 현재 Player·본체·World 전달이다(225553df, WB04-E674).

// 외곽이 매 호출마다 같은 살아 있는 owner를 제공한다.
export class RawCampaignSessionAiActions {
    // 실제 Session owner이며 별도 AI 상태를 만들지 않는다.
    #owner;
    // 현재 owner의 대여만 보관한다.
    constructor(owner) { this.#owner = owner; }
    // 실제 미션 설치 전 호출은 거부한다.
    #loaded() { if (!this.#owner.loaded) throw new Error('실제 Session 설치 후 AI 행동만 허용'); }
    // 현재 Player 1..8의 본체 토큰을 읽는다.
    #brain(owner) {
        this.#loaded(); if (!Number.isInteger(owner) || owner < 1 || owner >= 9) throw new RangeError('AI 행동 owner1..8 범위');
        return this.#owner.participation.brain(owner);
    }
    // 소유자의 현재 기술 부모로 다리 재고를 갱신한다.
    refreshBridges(owner) { const token = this.#brain(owner); if (!token) return false; const s = this.#owner, parent = s.aiProductionModules.technology(owner); s.aiStrategySetup.refreshBridges(token, parent); return true; }
    // 호출자의 scratch를 빌려 실제 다리 경로를 한 번 진행한다.
    advanceBridge(owner, target, scratch) { const token = this.#brain(owner); return token ? this.#owner.aiBridgePath(scratch).advance(token, { x: Math.fround(target.x), y: Math.fround(target.y) }) : false; }
    // 실제 AI 본체의 경제 정리를 한 번 진행한다.
    reconcileEconomy(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiEconomy.reconcile(token); return true; }
    // 현재 생산·지갑·월드로 요청한 유닛을 배치한다.
    deploy(owner, type, x, y, argument, direction) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiDeployment.place(token, type >>> 0, Math.fround(x), Math.fround(y), argument >>> 0, direction >>> 0); return true; }
    // 현재 AI의 수집자 확보 행동을 진행한다.
    ensureCollectors(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiCollectors.ensure(token); return true; }
    // 현재 AI의 시설 생산 행동을 진행한다.
    manageFactories(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiFactoryProduction.manage(token); return true; }
    // 에너지 부족 좌표를 실제 본체의 목록에 기록한다.
    recordDrySpot(owner, x, y, type) { const token = this.#brain(owner); return token ? this.#owner.aiDrySpots.add(token, Math.fround(x), Math.fround(y), type >>> 0) : false; }
    // 부족 좌표를 한 항목 검사하고 제거된 TYPE을 돌려준다.
    recheckDrySpots(owner) { const token = this.#brain(owner); return token ? this.#owner.aiDrySpots.maintain(token) : 0; }
    // 같은 게임 난수로 부족 좌표 후보를 생성한다.
    seedDrySpot(owner) { const token = this.#brain(owner); if (!token) return false; const s = this.#owner; s.aiDrySpots.seed(token, s.random); return true; }
    // 실제 표면과 경계로 가짜 부족 좌표 후보를 계산한다.
    computeDryCandidate(source, target) { this.#loaded(); return this.#owner.aiDryGeometry.candidate(source, target); }
    // 같은 본체의 가짜 부족 좌표 갱신을 한 번 진행한다.
    extendFakeDrySpots(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiFakeDrySpots.extend(token); return true; }
    // 실제 이동 Form을 같은 본체의 경로에서 찾는다.
    movementForm(actor) { this.#loaded(); return this.#owner.aiMovement.movementForm(actor); }
    // 현재 이동의 가상 삭제와 종속 정리를 실행한다.
    stopMovement(actor) { this.#loaded(); this.#owner.aiMovement.stop(actor); }
    // 실제 AI 본체로 대상 이동 명령을 보낸다.
    moveToObject(owner, actor, target, sacrifice) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiMovement.toTarget(token, actor, target, sacrifice >>> 0); return true; }
    // 단정도 좌표의 실제 이동 명령을 보낸다.
    moveToPoint(owner, actor, point, sacrifice) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiMovement.toPoint(token, actor, { x: Math.fround(point.x), y: Math.fround(point.y) }, sacrifice >>> 0); return true; }
    // 현재 신전으로 복귀하는 행동을 진행한다.
    returnHome(owner, actor) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiMovement.home(token, actor); return true; }
    // 같은 운반자의 수집 또는 귀환을 결정한다.
    collectOrHome(owner, actor) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiResourceChoice.collectOrHome(token, actor); return true; }
    // 끊어진 간헐천의 실제 목록을 재분류한다.
    recheckGeysers(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiResourceChoice.reclassifyDisconnected(token); return true; }
    // 실제 운반 행동 전체를 한 번 진행한다.
    manageCarriers(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiCarrierActions.act(token); return true; }
    // 본체의 현재 영역·회피 좌표 비트와 소유자로 건설 탐색을 시작한다.
    startBuildSearch(owner, type) {
        const token = this.#brain(owner); if (!token) return false;
        const raw = this.#owner.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        const bounds = { left: view.getFloat32(0x270, true), top: view.getFloat32(0x274, true), right: view.getFloat32(0x278, true), bottom: view.getFloat32(0x27c, true) };
        const avoid = { x: view.getFloat32(0x2b0, true), y: view.getFloat32(0x2b4, true), xBits: view.getUint32(0x2b0, true), yBits: view.getUint32(0x2b4, true) };
        this.#owner.aiBuildSearch.initialize(token, type >>> 0, bounds, avoid, view.getUint32(0x260, true)); return true;
    }
    // 현재 건설 탐색을 지정된 예산 정책으로 진행한다.
    advanceBuildSearch(owner, budgeted) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiBuildSearch.scan(token, budgeted >>> 0); return true; }
    // 다음 건설 TYPE을 현재 본체의 기술과 자금에서 선택한다.
    selectBuildType(owner) { const token = this.#brain(owner); return token ? this.#owner.aiBuildChoice.select(token) : 0; }
    // 현재 적의 낮은 자금 조건을 조회한다.
    enemyLowFunds(owner) { const token = this.#brain(owner); return token ? this.#owner.aiBuildChoice.enemyLowFunds(token) : false; }
}
