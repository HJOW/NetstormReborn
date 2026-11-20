// 실제 Client 설정·전역·Player·월드 저장소에서 캠페인 Guide 생성과 교체·반납을 관리한다.
// 사용: guide.begin('Tutorial1'); guide.current(); 사용 뒤 guide.dispose(). 단계 진행과 화면 전환은 호출자의 책임이다.
import { asciiLower } from '../assets/config.js';
import { decodeWindows1252, encodeOriginalText } from '../assets/text-encoding.js';
import { ClientState } from './client-state.js';
import { RawMissionGuide } from './raw-mission-guide.js';
import { CampaignGuideRuntime, releaseCampaignGuideMission } from './campaign-guide-release.js';
import { beginGuideMission, closeGuideMission } from './guide-mission-transitions.js';
import { runTutorialMissionStage } from './mission-lifecycle.js';
import { createGuideMissionScript, releaseGuideMissionScript } from './mission-script.js';
import { initializeTutorialMission } from './tutorial-init.js';
import { resetMissionBattleOptions } from './raw-mission-battle-options.js';
import { applyMissionTechnologyToRestrictions } from './mission-technology.js';
import { captureMissionFortZone, createMissionFortLoadRequest, writeMissionFortRequest } from './mission-fort-request.js';

// 현재 미션 Script·Runtime과 지연 요청 및 같은 Client 전역을 소유한다.
export class ClientCampaignGuide {
    // 동일 Client 설정·게임 전역·Player와 원본 Guide 소유 상태다.
    #data; #gameplay; #players; #storage; #pending = new ClientState(); #owned; #nextToken = 0; #disposed = false;
    // 튜토리얼 초기화의 같은 Client 장치와 원본 안내 표시 전역이다.
    #tutorial = null; #tutorialSeen = { value: 0 }; #runner = null;

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
    #create(name, demo, mode, source = null) {
        if (this.#owned.mission.value || this.#owned.runtime.value) throw new Error('Guide 생성 전 현재 미션이 남아 있음');
        const mission = createGuideMissionScript(this.#data, name, this.#owned.nativeMission, source); this.#owned.mission.value = mission;
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

    // Guide 단계 1이 사용할 실제 장기 Template·State·Subscriber·Fort·TYPE을 한 번 결합한다.
    bindTutorialInit(devices) {
        this.#live();
        if (this.#tutorial || !devices?.templates || !devices.local || !devices.subscriber || !devices.types || typeof devices.selectFort !== 'function') throw new TypeError('Guide Tutorial Init의 실제 Client 장치 누락 또는 중복');
        this.#tutorial = devices;
    }
    // 원본 Init이 선택한 Fort를 실제 State7·Zone 이미지와 다음 루프 큐에 예약한다.
    #requestFort(name) {
        if (this.#pending.pending() || this.#owned.request.value) throw new Error('Guide Fort 요청 전 다른 전환 또는 이미지가 남아 있음');
        const devices = this.#tutorial, request = createMissionFortLoadRequest(), local = devices.local, next = local.state().next;
        const address = Uint8Array.of(122, 108, 111, 99, 97, 108, 44, 42, 0);
        writeMissionFortRequest(name, { next, sequence: local.zonePending(), localAddress: address, port: () => this.#gameplay.gameServerPort().value }, {
            selectFort: (value) => { request.fort = devices.selectFort(value); }, duplicate: (packet) => captureMissionFortZone(request, packet),
        });
        request.next.set(next); request.sequence = local.zonePending().value; this.#owned.request.value = request;
        this.#pending.post({ command: 'LoadMissionFort', argument: request.fort });
    }
    // 실제 Script·Client 장치로 Tutorial Init을 실행한다. 지연 요청은 다음 상태가 소비한다.
    initializeTutorial() {
        this.#live(); const runtime = this.#owned.runtime.value, mission = this.#owned.mission.value, devices = this.#tutorial;
        if (!runtime?.guided || !mission || !devices) throw new Error('Guide Tutorial Init의 현재 미션/장치 누락');
        const flags = { get value() { return runtime.state.flags; }, set value(value) { runtime.state.flags = value; } };
        const demo = { get value() { return runtime.gate.demo; }, set value(value) { runtime.gate.demo = value; } };
        const tutorialNumber = { get value() { return runtime.gate.tutorialNumber; }, set value(value) { runtime.gate.tutorialNumber = value; } };
        const get = (key, fallback) => {
            const value = mission.get(key) ?? ''; const narrow = Array.from(encodeOriginalText(value), (byte) => String.fromCharCode(byte)).join('').split('\0', 1)[0];
            return narrow || fallback || '';
        };
        initializeTutorialMission(runtime.init, flags, demo, { tutorialNumber, missionNumber: this.#owned.missionNumber, tutorialSeen: this.#tutorialSeen,
            loginSubscriber: devices.subscriber.mySubscriber(), initialSubscriber: () => devices.subscriber.loginSubscriber().value }, {
            clearTemplates: () => devices.templates.clearAll(), resetOptions: () => resetMissionBattleOptions(this.#gameplay.battleOptionsOwner()), get,
            set: (key, value) => mission.set(key, decodeWindows1252(Uint8Array.from(value, (character) => character.charCodeAt(0)))),
            requestFort: (name) => this.#requestFort(name), header: () => applyMissionTechnologyToRestrictions(mission, devices.types, this.#owned.restrictions),
        });
    }

    // 실제 창·시계·월드 연결 상태를 빌려 원본 Guide 반복기를 만든다.
    bindLoop(devices) {
        this.#live();
        if (this.#runner || !devices || ['dialogsClear', 'gameSeconds', 'worldConnecting', 'tellGlobal'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('Guide 반복기의 실제 Client 장치 누락 또는 중복');
        this.#runner = new RawMissionGuide({
            current: () => this.#owned.runtime.value?.guided ? this.#owned.runtime.value.state : null,
            pending: () => this.#pending.pending() || this.#tutorial.local.state().next[0] !== 0 || devices.worldConnecting(this.#owned.runtime.value?.saveToken),
            dialogsClear: () => devices.dialogsClear(),
            stage: (state, sequence) => runTutorialMissionStage(state, sequence, {
                initialize: () => this.initializeTutorial(),
                tellGlobal: (name, mode) => devices.tellGlobal(name, mode),
                showStage: () => { const tutorial = this.#owned.runtime.value?.tutorial; if (!tutorial) throw new Error('Guide First의 현재 안내 연결 누락'); tutorial.tellStage(); },
                start: () => { const start = this.#owned.runtime.value?.start; if (!start) throw new Error('Guide 단계3 실제 World Start 연결 누락'); start.start(); },
                frame: () => { const tutorial = this.#owned.runtime.value?.tutorial; if (!tutorial) throw new Error('Guide 프레임의 실제 미션 연결 누락'); tutorial.frame(); },
                tellMission: (name) => { if (typeof devices.tellMission !== 'function') throw new Error('Guide 종료 안내의 실제 미션 Tell 연결 누락'); return devices.tellMission(name); },
                closeTransient: () => { if (typeof devices.closeTransient !== 'function') throw new Error('Guide 팝업 닫기 연결 누락'); devices.closeTransient(); },
                pause: () => { const tutorial = this.#owned.runtime.value?.tutorial; if (!tutorial) throw new Error('Guide 안내의 실제 자기 정지 연결 누락'); tutorial.gate().pauseForDialog(); },
            }),
            postpone: () => {
                const runtime = this.#owned.runtime.value; if (!runtime) throw new Error('Guide 재지연의 현재 미션 없음');
                if (runtime.tutorial) runtime.tutorial.gate().postpone();
                else if (runtime.gate.nextStageSeconds !== 0) runtime.gate.nextStageSeconds = runtime.gate.delaySeconds + devices.gameSeconds();
            },
            release: () => this.#release(),
            invalidStage: () => { throw new Error('Guide 현재 미션 순번0..6 위반'); },
        });
    }
    // 한 차례 Guide 반복을 실행한다. State나 창이 막으면 초기화를 앞당기지 않는다.
    run() { this.#live(); if (!this.#runner || !this.#tutorial) throw new Error('Guide 반복기의 실제 Client 장치 미설치'); this.#runner.run(); }

    // 기존 미션의 종료·해제 뒤 새 미션을 만들며 단계 진행이나 World 생성은 앞당기지 않는다. source 는 선택적 명시 파일 읽기 함수 (경로) => 바이트|null 이며, 생략하면 이전 미션의 명시 source 를(없으면 기본 자산 읽기를) 이어 쓴다(원문 BeginMission 이 이전 FileSource 를 넘김). 12차 스커미시가 생성한 가상 대본·fort 를 읽게 하는 데 쓴다. (2026-11-20 WB04-E1105)
    begin(name, demo = 0, mode = 0, source = null) {
        this.#live(); const inherited = source ?? this.#owned.mission.value?.fileSource() ?? null;
        beginGuideMission(name, { current: () => Boolean(this.#owned.runtime.value), finish: () => this.#finish(), release: () => this.#release(), create: (value) => this.#create(value, demo >>> 0, mode >>> 0, inherited) });
    }
    // 현재 미션을 종료 가상 호출 없이 반납하며 이후 새 미션을 만들 수 있다.
    close() { this.#live(); closeGuideMission({ current: () => Boolean(this.#owned.runtime.value), release: () => this.#release() }); }
    // 현재 미션을 조회한다. 객체와 Script는 이 Guide보다 오래 살지 않아야 한다.
    current() { this.#live(); return this.#owned.runtime.value ? { script: this.#owned.mission.value, runtime: this.#owned.runtime.value } : null; }
    // Session과 같은 256칸 금지 표를 빌린다.
    restrictions() { this.#live(); return this.#owned.restrictions; }
    // 현재 지연 Fort 요청 원본을 빌린다. 취소나 소비 여부는 같은 객체에서 관찰한다.
    fortRequest() { this.#live(); return this.#owned.request.value; }
    // 지연 Fort 요청을 소유하는 실제 상태 큐를 빌린다.
    pending() { this.#live(); return this.#pending; }
    // Guide 반납은 현재 미션만 해제하며 World와 Kernel은 상위 Client가 정리한다.
    dispose() { if (this.#disposed) return; this.close(); this.#disposed = true; }
}
