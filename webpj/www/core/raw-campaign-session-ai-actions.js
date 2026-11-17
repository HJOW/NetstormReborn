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
    deploy(owner, type, x, y, argument, direction) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiDeployment.place(token, type, Math.fround(x), Math.fround(y), argument, direction); return true; }
    // 현재 AI의 수집자 확보 행동을 진행한다.
    ensureCollectors(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiCollectors.ensure(token); return true; }
    // 현재 AI의 시설 생산 행동을 진행한다.
    manageFactories(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiFactoryProduction.manage(token); return true; }
    // 에너지 부족 좌표를 실제 본체의 목록에 기록한다.
    recordDrySpot(owner, x, y, type) { const token = this.#brain(owner); return token ? this.#owner.aiDrySpots.add(token, Math.fround(x), Math.fround(y), type) : false; }
    // 부족 좌표를 한 항목 검사하고 제거된 TYPE을 돌려준다.
    recheckDrySpots(owner) { const token = this.#brain(owner); return token ? this.#owner.aiDrySpots.maintain(token) : 0; }
    // 같은 게임 난수로 부족 좌표 후보를 생성한다.
    seedDrySpot(owner) { const token = this.#brain(owner); if (!token) return false; const s = this.#owner; s.aiDrySpots.seed(token, s.random); return true; }
    // 실제 표면과 경계로 가짜 부족 좌표 후보를 계산한다.
    computeDryCandidate(source, target) { this.#loaded(); return this.#owner.aiDryGeometry.candidate(source, target); }
    // 같은 본체의 가짜 부족 좌표 갱신을 한 번 진행한다.
    extendFakeDrySpots(owner) { const token = this.#brain(owner); if (!token) return false; this.#owner.aiFakeDrySpots.extend(token); return true; }
}
