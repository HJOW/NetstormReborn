// 실제 Client 설정·전역·Player·월드 저장소에서 캠페인 Guide 생성과 교체·반납을 관리한다.
// 사용: guide.begin('Tutorial1'); guide.current(); 사용 뒤 guide.dispose(). 단계 진행과 화면 전환은 호출자의 책임이다.
import { asciiLower } from '../assets/config.js';
import { ClientState } from './client-state.js';
import { CampaignGuideRuntime, releaseCampaignGuideMission } from './campaign-guide-release.js';
import { beginGuideMission, closeGuideMission } from './guide-mission-transitions.js';
import { runTutorialMissionStage } from './mission-lifecycle.js';
import { createGuideMissionScript, releaseGuideMissionScript } from './mission-script.js';

// 현재 미션 Script·Runtime과 지연 요청 및 같은 Client 전역을 소유한다.
export class ClientCampaignGuide {
    // 동일 Client 설정·게임 전역·Player와 원본 Guide 소유 상태다.
    #data; #gameplay; #players; #storage; #pending = new ClientState(); #owned; #nextToken = 0; #disposed = false;

    // 원본 Native 미션 모드 참조를 같은 장기 월드 저장소에 연결한다.
    constructor(data, gameplay, players, storage) {
        if (!data || !gameplay || !players || !storage || storage.authority !== gameplay.authority()) throw new TypeError('Guide의 실제 Client 공통 소유자 불일치');
        this.#data = data; this.#gameplay = gameplay; this.#players = players; this.#storage = storage;
        const nativeMission = { get value() { return storage.links.mission; }, set value(value) { storage.links.mission = Boolean(value); } };
        this.#owned = { mission: { value: null }, runtime: { value: null }, request: { value: null }, restrictions: new Uint32Array(256), missionNumber: { value: 0 }, worldMissionToken: { value: 0 }, nativeMission };
    }

    // 이미 반납한 Guide로 현재 미션을 다시 만들거나 빌리지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Client Guide'); }
    // 실제 Client 장기 전역과 현재 Player만 기존 해제 본문에 빌린다.
    #release() {
        releaseCampaignGuideMission({ gameplayGlobals: () => this.#gameplay, playerRecords: () => this.#players, configuration: () => this.#data }, this.#pending, this.#owned);
    }
    // 공장은 Script 생성 뒤 판정하고 새 Runtime만 같은 Guide에 둔다. 실패한 생성은 등록 객체를 반납한다.
    #create(name, demo, mode) {
        if (this.#owned.mission.value || this.#owned.runtime.value) throw new Error('Guide 생성 전 현재 미션이 남아 있음');
        const mission = createGuideMissionScript(this.#data, name, this.#owned.nativeMission); this.#owned.mission.value = mission;
        try {
            if (!mission.loaded) throw new Error(`미션 스크립트 없음: ${name}`);
            const type = asciiLower(mission.missionType());
            if (type !== 'tutorial' && type !== 'battle') throw new Error(`지원하지 않는 미션 유형: ${mission.missionType()}`);
            if (this.#nextToken === 0xffffffff) throw new RangeError('Guide 미션 토큰 소진');
            this.#owned.runtime.value = new CampaignGuideRuntime(mission, demo, mode, ++this.#nextToken);
        } catch (error) { releaseGuideMissionScript(this.#owned.mission, this.#owned.nativeMission, this.#data); throw error; }
    }
    // 현재 Tutorial의 실제 종료 가상 단계 6을 수행한다. 기본 미션은 별도 효과가 없다.
    #finish() {
        const runtime = this.#owned.runtime.value; if (!runtime) throw new Error('현재 Guide 미션 없음');
        if (runtime.guided) runTutorialMissionStage(runtime.state, 6, {});
    }

    // 기존 미션의 종료·해제 뒤 새 미션을 만들며 단계 진행이나 World 생성은 앞당기지 않는다.
    begin(name, demo = 0, mode = 0) {
        this.#live(); beginGuideMission(name, { current: () => Boolean(this.#owned.runtime.value), finish: () => this.#finish(), release: () => this.#release(), create: (value) => this.#create(value, demo >>> 0, mode >>> 0) });
    }
    // 현재 미션을 종료 가상 호출 없이 반납하며 이후 새 미션을 만들 수 있다.
    close() { this.#live(); closeGuideMission({ current: () => Boolean(this.#owned.runtime.value), release: () => this.#release() }); }
    // 현재 미션을 조회한다. 객체와 Script는 이 Guide보다 오래 살지 않아야 한다.
    current() { this.#live(); return this.#owned.runtime.value ? { script: this.#owned.mission.value, runtime: this.#owned.runtime.value } : null; }
    // Session과 같은 256칸 금지 표를 빌린다.
    restrictions() { this.#live(); return this.#owned.restrictions; }
    // 지연 Fort 요청을 소유하는 실제 상태 큐를 빌린다.
    pending() { this.#live(); return this.#pending; }
    // Guide 반납은 현재 미션만 해제하며 World와 Kernel은 상위 Client가 정리한다.
    dispose() { if (this.#disposed) return; this.close(); this.#disposed = true; }
}
