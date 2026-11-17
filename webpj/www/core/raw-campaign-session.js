// RawCampaignSession 외곽 생성자4318..4330과 소멸자 앞단의 typed 조립이다(225553df, WB04-E638).
// 사용: new RawCampaignSession(game, files, fort, mission, services) → 후속 Load → dispose.
// 아직 Load/Login/State 반납 증빙은 구현하지 않는다. 생성 자체는 미션을 시작하거나 loaded를 켜지 않는다.
import { RawCampaignSessionOwners } from './raw-campaign-session-owners.js';
import { learnCampaignNewTechnology } from './raw-campaign-callbacks.js';
import { refreshLocalRitualData } from './raw-ritual-refresh.js';

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
    // 생성과 실제 설치 완료를 구별한다. 이 단계는 항상 false다.
    get loaded() { return this.#live().loaded; }
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
