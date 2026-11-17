// RawCampaignSession 외곽 생성자4318..4330과 소멸자 앞단의 typed 조립이다(225553df, WB04-E638).
// 사용: new RawCampaignSession(game, files, fort, mission, services) → 후속 Load → dispose.
// 생성 자체는 미션을 시작하거나 loaded를 켜지 않는다. Login/State 반납 증빙은 후속이다.
import { RawCampaignSessionOwners } from './raw-campaign-session-owners.js';
import { learnCampaignNewTechnology } from './raw-campaign-callbacks.js';
import { refreshLocalRitualData } from './raw-ritual-refresh.js';
import { createCampaignAi, crankCampaignAiMoney, setCampaignAiParameter, prepareCampaignAi, initializeCampaignAi, makeCampaignAi } from './raw-campaign-session-ai-public.js';
import { RawCampaignSessionAiActions } from './raw-campaign-session-ai-actions.js';

// Impl990..1865의 이미 이식한 생성 순서다. 각 단계는 같은 owner의 실제 하위 모듈을 한 번 만든다.
const CONSTRUCTOR_STEPS = Object.freeze([
    'PlayerTablesAndPackets', 'IslandsAndPriests', 'PlayersAndOwnership', 'BodiesAndTypeTables', 'GunAndCursorViews',
    'HitBodies', 'GunBodies', 'Particles', 'Teleport', 'Combat', 'Vortex', 'Assets', 'WorldRuntimeHooks',
    'ContentNotices', 'ContentLifetime', 'Production', 'Factories', 'Flare', 'Construction', 'Damageable', 'IceTower',
    'CarrierSteal', 'BombPostPop', 'AcidZap', 'AcidFence', 'CarrierRegular', 'SunAviary', 'WindAviary', 'RainAviary',
    'ArcSpire', 'Flyer', 'Fence', 'GemReward', 'WorldDeletion', 'WorldRegular', 'WorldTransmission', 'Terrain',
]);

// 외곽 주소를 게시하고 같은 Impl만 소유한다. Client 앱 수명 연결은4D-6·4D-7에서 수행한다.
export class RawCampaignSession {
    // 실제 Impl 소유자와 외곽 반납 관문이다. 외부에는 원시 owner를 공개하지 않는다.
    #owner = null; #disposed = false;

    // 모든 Impl 모듈이 구성된 뒤 외곽 callback을 연결하고 마지막에 현재 Session을 게시한다.
    constructor(game, files, fort, mission, services = {}) {
        try {
            const login = services.localLogin; // C++ optional 함수 표의 값형 사본이며 공유 transport owner 자체는 유지한다.
            const input = { ...services, localLogin: login && typeof login === 'object' ? { ...login, transport: login.transport ? { ...login.transport } : login.transport } : login };
            const o = new RawCampaignSessionOwners(game, files, mission, input); this.#owner = o;
            o.initializePlayersAndSp();
            // source 생성자의 실제 조립 순서를 유지하며 초기화 누락을 성공으로 넘기지 않는다.
            for (const name of CONSTRUCTOR_STEPS) o[`connect${name}`]();
            o.connectWorldInstall(fort); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction();
            o.learnNewTechnology = (type) => this.learnNewTechnology(type);
            o.vortexSnapshot = () => this.#snapshotCurrent();
            o.writeCurrentFortCore = (slot) => this.#live().writeFortCoreDataBody(slot);
            o.createWorldGeysers = () => this.generateWorldGeysers();
            o.placeBattleNuggets = (rectangle, quantity) => this.placeBattleNuggets(rectangle, quantity);
            o.refreshLocalRitual = () => {
                const current = this.#live();
                refreshLocalRitualData({ // 현재 Core/owner/CoreData와 두 독립 snapshot을 같은 외곽에 연결한다.
                    core: () => current.refreshCurrentPlayerCore(), templateOwner: () => current.playerState.templateOwner,
                    coreData: (slot) => current.writeFortCoreDataBody(slot), snapshot: () => this.#snapshotCurrent(),
                });
            };
            o.gameplayUse.publish(this);
        } catch (error) {
            this.#disposed = true; this.#owner?.dispose(); throw error;
        }
    }

    // 반납한 외곽 주소가 낡은 Impl이나 다음 세대의 전역을 다시 사용하지 못하게 한다.
    #live() { if (this.#disposed || !this.#owner) throw new Error('반납한 외곽 Session'); return this.#owner; }
    // 호출 시 살아 있는 같은 owner로 AI 행동 전달기를 구성한다.
    #aiActions() { return new RawCampaignSessionAiActions(this.#live()); }
    // 두 독립 if마다 현재 모드/owner를 다시 읽는다. manager 부재의 선행 생략을 추가하지 않는다.
    #snapshotCurrent() {
        const o = this.#live();
        if (o.core.ownerMode.battle) this.snapshotBattle(o.playerState.templateOwner);
        if (o.core.ownerMode.fort) this.snapshotFort(o.playerState.templateOwner);
    }

    // 같은 실제 World/Core를 빌리는 최소 공개 조회다. 나머지 공개 API는4D-6c에서 옮긴다.
    world() { return this.#live().world; }
    // C++ Core의 값형 aggregate를 돌려주되 Pool/권한/장부의 실제 참조는 그대로 빌린다(6382).
    core() { return { ...this.#live().core }; }
    // 원문의 설치 순서로 같은 실제 월드와 모든 모듈을 연결한다. 오류 시 앞선 효과를 되돌리지 않는다.
    load() {
        const o = this.#live();
        o.connectBuilding(); o.connectMotion(); o.runtime.load();
        o.connectDevastation(); o.connectCarrierDrop(); o.connectPriestPrayer(); o.connectBuilder();
        o.connectGameplayPlacement(); o.connectCursor(); o.loaded = true;
    }
    // 생성과 실제 설치 완료를 구별한다.
    get loaded() { return this.#live().loaded; }
    // 현재 방향별 동맹 DWORD를 복사하지 않고 빌린다.
    alliances() { return this.#live().alliances; }
    // 현재 Player 참여와 AI 소유자 수명을 빌린다.
    participation() { return this.#live().participation; }
    // 같은 옵션 저장소의 숨김 Player 바이트를 빌린다.
    hiddenPlayers() { return this.#live().hiddenPlayers; }
    // 현재 87바이트 옵션의 유효한 위치를 조회한다.
    battleOption(offset) { const options = this.#live().battleOptions; if (!Number.isInteger(offset) || offset < 0 || offset >= options.length) throw new RangeError('Session 전투 옵션 범위'); return options[offset]; }
    // 설치된 미션의 Player에 실제 AI를 생성한다.
    createAi(owner, settings) { return createCampaignAi(this.#live(), owner, settings); }
    // 일반 AI 설정과 객체 이전을 현재 Session에서 실행한다.
    makeAi(owner) { makeCampaignAi(this.#live(), owner); }
    // 현재 생존 AI 지갑에 원본 직접 명령을 실행한다.
    crankAiMoney() { crankCampaignAiMoney(this.#live()); }
    // packed 입력의 선택된 AI 필드에 원본 직접 명령을 실행한다.
    setAiParameter(packed) { setCampaignAiParameter(this.#live(), packed); }
    // 현재 Player의 AI 준비 행동을 한 번 진행한다.
    prepareAi(owner) { return prepareCampaignAi(this.#live(), owner); }
    // 현재 Player의 초기 전략이 준비되었는지 확인한다.
    initializeAi(owner) { return initializeCampaignAi(this.#live(), owner); }
    // 현재 기술 부모의 AI 다리 재고를 보충한다.
    refreshAiBridges(owner) { return this.#aiActions().refreshBridges(owner); }
    // 호출자 scratch로 AI 다리 경로를 한 번 진행한다.
    advanceAiBridge(owner, target, scratch) { return this.#aiActions().advanceBridge(owner, target, scratch); }
    // 같은 본체의 AI 경제 정리를 진행한다.
    reconcileAiEconomy(owner) { return this.#aiActions().reconcileEconomy(owner); }
    // 선택된 생산물을 현재 월드에 배치한다.
    deployAi(owner, type, x, y, argument, direction) { return this.#aiActions().deploy(owner, type, x, y, argument, direction); }
    // 같은 본체의 수집자 확보 행동을 진행한다.
    ensureAiCollectors(owner) { return this.#aiActions().ensureCollectors(owner); }
    // 같은 본체의 시설 생산 행동을 진행한다.
    manageAiFactories(owner) { return this.#aiActions().manageFactories(owner); }
    // 현재 부족 좌표 목록에 배치 후보를 기록한다.
    recordAiDrySpot(owner, x, y, type) { return this.#aiActions().recordDrySpot(owner, x, y, type); }
    // 부족 좌표 목록 한 항목을 재검사한다.
    recheckAiDrySpots(owner) { return this.#aiActions().recheckDrySpots(owner); }
    // 같은 난수로 부족 좌표 후보를 선택한다.
    seedAiDrySpot(owner) { return this.#aiActions().seedDrySpot(owner); }
    // 실제 표면과 경계에서 가짜 부족 좌표를 계산한다.
    computeAiDryCandidate(source, target) { return this.#aiActions().computeDryCandidate(source, target); }
    // 같은 AI 본체의 가짜 부족 좌표를 갱신한다.
    extendAiFakeDrySpots(owner) { return this.#aiActions().extendFakeDrySpots(owner); }
    // 현재 이동체의 실제 경로 Form을 조회한다.
    aiMovementForm(actor) { return this.#aiActions().movementForm(actor); }
    // 현재 이동체의 실제 경로를 중단한다.
    stopAiMovement(actor) { this.#aiActions().stopMovement(actor); }
    // 현재 AI의 대상 이동 명령을 실행한다.
    moveAiToObject(owner, actor, target, sacrifice) { return this.#aiActions().moveToObject(owner, actor, target, sacrifice); }
    // 현재 AI의 좌표 이동 명령을 실행한다.
    moveAiToPoint(owner, actor, point, sacrifice) { return this.#aiActions().moveToPoint(owner, actor, point, sacrifice); }
    // 현재 AI 이동체의 신전 복귀를 실행한다.
    returnAiHome(owner, actor) { return this.#aiActions().returnHome(owner, actor); }
    // 현재 운반자의 수집 또는 귀환을 판단한다.
    collectAiOrReturnHome(owner, actor) { return this.#aiActions().collectOrHome(owner, actor); }
    // 끊어진 AI 간헐천 목록을 재분류한다.
    recheckAiGeysers(owner) { return this.#aiActions().recheckGeysers(owner); }
    // 현재 AI의 전체 운반 행동을 실행한다.
    manageAiCarriers(owner) { return this.#aiActions().manageCarriers(owner); }
    // 현재 AI 본체의 경계와 회피 좌표로 건설 탐색을 시작한다.
    startAiBuildSearch(owner, type) { return this.#aiActions().startBuildSearch(owner, type); }
    // 현재 AI 건설 탐색을 진행한다.
    advanceAiBuildSearch(owner, budgeted) { return this.#aiActions().advanceBuildSearch(owner, budgeted); }
    // 현재 AI의 다음 건설 TYPE을 조회한다.
    selectAiBuildType(owner) { return this.#aiActions().selectBuildType(owner); }
    // 현재 AI 적의 자금 부족 조건을 조회한다.
    aiEnemyLowFunds(owner) { return this.#aiActions().enemyLowFunds(owner); }
    // 현재 AI 사제의 전체 행동을 실행한다.
    manageAiPriest(owner, priest) { return this.#aiActions().managePriest(owner, priest); }
    // 현재 AI 표적 점수를 조회한다.
    scoreAiTarget(owner, target) { return this.#aiActions().scoreTarget(owner, target); }
    // 실제 총기 표적 점수와 대상 출력을 조회한다.
    scoreAiGun(owner, type, x, y, direction, result) { return this.#aiActions().scoreGun(owner, type, x, y, direction, result); }
    // 실제 장애물 점수와 대상 출력을 조회한다.
    scoreAiBlockers(owner, type, x, y, direction, result) { return this.#aiActions().scoreBlockers(owner, type, x, y, direction, result); }
    // 현재 배치 점수와 대상 출력을 조회한다.
    scoreAiDrop(owner, type, x, y, direction, target) { return this.#aiActions().scoreDrop(owner, type, x, y, direction, target); }
    // 최고 배치 후보를 호출자의 상자에 반영한다.
    considerAiDrop(owner, type, x, y, direction, bestPoint, bestScore, target) { return this.#aiActions().considerDrop(owner, type, x, y, direction, bestPoint, bestScore, target); }
    // 현재 배치 후보 스캔을 진행한다.
    advanceAiDropScan(owner, type, direction) { return this.#aiActions().advanceDropScan(owner, type, direction); }
    // 선택한 AI 배치 후보를 제거한다.
    removeAiDropChoice(owner, index) { return this.#aiActions().removeDropChoice(owner, index); }
    // 현재 AI 배치 TYPE을 선택한다.
    selectAiDropType(owner) { return this.#aiActions().selectDropType(owner); }
    // 현재 AI 배치 행동을 실행한다.
    manageAiDrops(owner) { return this.#aiActions().manageDrops(owner); }
    // 현재 실제 객체 이름을 조회한다.
    objectName(parent) { return this.#aiActions().objectName(parent); }
    // 설치된 이름 장치에서 기본 이름을 조회한다.
    objectBaseName(parent) { return this.#aiActions().objectBaseName(parent); }
    // 현재 Player의 AI 본체를 명시 해제한다.
    removeAi(owner) { this.#aiActions().remove(owner); }
    // 현재 Session의 AI 메모리 소유자를 빌린다.
    aiMemory() { return this.#aiActions().memory(); }
    // 외곽 callback이 같은 현재 기술 부모/안내 본문을 선택한다.
    learnNewTechnology(type) { learnCampaignNewTechnology(this.#live(), type); }
    // 전체 저장의 설치 관문도 기존 실제 owner가 유지한다.
    snapshotFort(slot) { this.#live().snapshotFort(slot); }
    // Battle은 Fort 전용 자식을 덧붙이지 않는 기존 본문에 연결한다.
    snapshotBattle(slot) { this.#live().snapshotBattle(slot); }
    // 현재 요청 상태를 먼저 준비하는 실제 로컬 간헐천이다.
    generateLocalGeysers() { this.#live().generateLocalGeysers(); }
    // 현재 장기 옵션/공간을 빌리는 실제 세계 간헐천이다.
    generateWorldGeysers() { this.#live().generateWorldGeysers(); }
    // Impl callback 슬롯과 실제 typed 배치 본문을 구별한다.
    placeBattleNuggets(rectangle, quantity) { this.#live().placeBattleNuggetsBody(rectangle, quantity); }

    // 4822: 같은 World/건설 장치의 외곽 pin을 먼저 연결하고 장기 칸에도 보관한다.
    bindRuntimePin(pin = null) {
        const o = this.#live(); o.world.bindRuntimePin(pin); o.constructionRuntimePin = pin;
        if (o.constructionRuntime) o.constructionRuntime.pin = o.constructionRuntimePin;
    }
    // 4158..4160: 아직 Login이 없어도 같은 services가 보관한 실제 전송 대여를 반납한다.
    detachConnectionTransport() {
        const o = this.#live(), transport = o.services.localLogin?.transport;
        if (transport) {
            if (typeof transport.detachSession !== 'function') throw new Error('외곽 Session의 실제 전송 반납 장치 미연결');
            transport.detachSession();
        }
    }
    // State 증빙/Withdraw의 앞단만 옮긴다. 일반 Impl 반납은 기존 owner가 끝까지 수행한다.
    dispose() {
        if (this.#disposed) return;
        const o = this.#live();
        if (o.gameplayGlobals.runtimeCallActive(this)) throw new Error('실제 Runtime 배송 종료 전 외곽 Session 반납');
        try { this.bindRuntimePin(null); this.detachConnectionTransport(); }
        finally { this.#disposed = true; o.dispose(); }
    }
}
