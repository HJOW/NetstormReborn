// 실제 Guide 가 소유한 미션 상태와 해제를 공유하고 World·입력의 수명은 호출자에게 두는 모듈이다(cpppj `client/CampaignGuideRelease.cpp/.h`, 10.78 전용).
// 사용:
//   const runtime = new CampaignGuideRuntime(script, demo, mode, token); runtime.gate · runtime.state · runtime.init · runtime.start/tutorial(실제 Session 설치 뒤 생성) · runtime.guided · runtime.mode64 · runtime.saveToken
//   releaseCampaignGuideMission(client, pending, { mission, runtime, request, restrictions, missionNumber, worldMissionToken, nativeMission })
//     — 00463ab0 → 004c3210 → 00483210 을 실제 Client/AI/Config 에 연결한다. World·UserInput·시계·음악은 쓰지 않는다. 소유 참조는 `{ value }` 상자(C++ 참조 인자 대응)이고 restrictions 는 256칸 금지 표이다.
//     순서: 이전 Guide 의 지연 Fort 로드 요청 반납(LoadMissionFort/LoadMissionWorld 취소) → (실제 Tutorial 미션이면) 허용 표 전체 허용·회수/승급 금지·미션 번호·Early 분배 번호·단계·단계 표식 0 쓰기 →
//     현재 Guide AI 반납 → 기본 Mission 해제(실행기 Tutorial→Start 소멸, 세대 토큰 0, Config 해제·`global.inMission` 0). 기본 Mission 은 정상 꼬리/예외 unwind 에서 한 번만 해제한다. 실제 Tutorial 이 아니면 기본 해제만 한다.
// client 계약(실제 `Client` 이식 전의 최소 덕타입, 4-D 구획 E 에서 같은 이름으로 구현한다): { gameplayGlobals() → { denySalvage, denyAscend(현재 DWORD 읽기/쓰기 속성), borrowed(), requireSession() → { playerRecords() → Uint8Array,
//   participation() → { removeAllBrains() } } }, playerRecords() → { records() → Uint8Array(9×180) }, configuration() → { set(키, 값) } }.
// AI 반납: World 를 빌린 상태이면 같은 Session 의 Player 기록이 Client 의 것과 같아야 하고 실제 참여 상태에서 모든 AI 를 반납한다. 이미 반납된 상태이면 Player 1..8 의 +b0 brain 이 모두 NULL 임을 방문 순서대로 확인하며
//   하나라도 비NULL 이면 가짜 빈 Player 로 대체하지 않고 명시 오류로 둔다. 미초기화/손실된 Player 물리(1620바이트 아님)도 거부한다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E524: 4-D B 구획 CampaignGuideRelease 이식)
import { asciiLower } from '../assets/config.js';
import { encodeOriginalText } from '../assets/text-encoding.js';
import { releaseGuideMissionScript } from './mission-script.js';
import { releaseMissionFortLoad } from './mission-fort-request.js';
import { createMissionFrameGateState } from './raw-mission-frame-gate.js';
import { createMissionStartState } from './raw-mission-start.js';
import { releaseTutorialMission } from './raw-tutorial-mission-release.js';
import { setTotalMadeAllowAll } from './raw-total-made.js';

// 초기화된 Player 9 칸의 원문 크기와 칸 폭, 현재 AI 포인터(brain) 필드 위치이다. (2026-11-15 WB04-E524)
const INITIALIZED_PLAYER_BYTES = 1620;
const PLAYER_STRIDE = 180;
const BRAIN_OFFSET = 0xb0;

// 원문의 전역 AI 반납에 현재 Client 소유자만 연결한다. World 가 이미 반납됐으면 같은 Player+B0 가 모두 NULL 인지 방문 순서대로 확인한다. (2026-11-15 WB04-E524)
function removeCurrentGuideBrains(client) {
    const game = client.gameplayGlobals();
    if (game.borrowed()) {
        const session = game.requireSession();
        if (session.playerRecords() !== client.playerRecords().records()) throw new Error('Guide AI 반납의 실제 Client Player 소유자 불일치');
        // 일반 해제이며 State reset 증빙을 미리 발급하지 않는다.
        session.participation().removeAllBrains();
        return;
    }
    // 미초기화·손실된 물리를 가짜 빈 Player 로 대체하지 않는다.
    const players = client.playerRecords().records();
    if (players.length !== INITIALIZED_PLAYER_BYTES) throw new Error('Guide AI 반납의 현재 실제 Player9 물리 미초기화');
    // 원문 00413b20 처럼 중립 0 을 제외한 여덟 현재 brain 을 각각 읽는다.
    for (let owner = 1; owner < 9; owner += 1) {
        const offset = owner * PLAYER_STRIDE + BRAIN_OFFSET;
        const brain = (players[offset] | (players[offset + 1] << 8) | (players[offset + 2] << 16) | (players[offset + 3] << 24)) >>> 0;
        if (brain) throw new Error('Guide AI 반납의 현재 비NULL brain 실제 소유자 미연결');
    }
}

// 원본 Mission 필드와 그 필드를 빌리는 실행기를 한 객체에 보유한다. 생성은 번호·Start 효과를 앞당기지 않는다. (2026-11-15 WB04-E524)
export class CampaignGuideRuntime {
    // 현재 World 설치를 같은 미션과 연결하는 호스트 토큰(게임 규칙 값이 아님)이다.
    saveToken;
    // 원본 +4/+b0 및 Init 지역 문자열의 현재 입력이다.
    init = { name: '', namePrefix: 0, scratch: new Uint8Array(80) };
    // demo/started/시각/안내 단계의 유일한 현재 미션 저장소와 Start 의 순번·tellAt 항목/필드 상태이다.
    gate = createMissionFrameGateState();
    state = createMissionStartState();
    // 실제 Session 설치 뒤 생성하고 Tutorial 보다 오래 유지하는 Start 실행기, 현재 Mission+64 의 편집 복귀 모드(해제 중 임의 정규화하지 않음),
    // 실제 Tutorial 가상표를 쓰는 10.78 미션인지, 현재 Start/Session/Script 를 빌리는 안내 실행기이다.
    start = null;
    mode64 = 0;
    guided;
    tutorial = null;

    // 원문 00482bf0 의 초기 입력이며 번호 정규화는 단계 1 에서 실행한다. 미션 이름은 원문 부호화 바이트를 문자 하나로 담는다. (2026-11-15 WB04-E524)
    constructor(script, demo, mode, token) {
        this.saveToken = token >>> 0;
        this.mode64 = mode >>> 0;
        this.guided = asciiLower(script.missionType()) === 'tutorial';
        this.gate.demo = demo >>> 0;
        this.init.name = Array.from(encodeOriginalText(script.name), (byte) => String.fromCharCode(byte)).join('');
    }

    // 선언 역순으로 Tutorial 이 Start 보다 먼저 소멸한다(현재 참조를 먼저 반납). (2026-11-15 WB04-E524)
    dispose() {
        this.tutorial = null;
        this.start = null;
    }
}

// Guide 소멸의 실제 전역/AI/Config 순서를 World 소유권과 독립적으로 실행한다. 호스트의 이전 Guide 지연 요청만 닫고 Native State/World 는 유지한다. (2026-11-15 WB04-E524)
export function releaseCampaignGuideMission(client, pending, owned) {
    const { mission, runtime, request, restrictions, missionNumber, worldMissionToken, nativeMission } = owned;
    releaseMissionFortLoad(pending, request);
    // 소멸 본문이 실패해도 원문 unwind 처럼 같은 기본 Mission 의 Config 를 반납한다.
    const releaseBase = () => {
        // Tutorial→Start→상태가 Script 보다 먼저 끝나며 World 입력은 이 함수의 소유 대상이 아니다.
        if (runtime.value) runtime.value.dispose();
        runtime.value = null;
        worldMissionToken.value = 0;
        releaseGuideMissionScript(mission, nativeMission, client.configuration());
    };
    const current = runtime.value;
    if (current && current.guided) {
        const game = client.gameplayGlobals();
        const gate = current.gate;
        // C++ 은 여섯 전역의 참조를 넘긴다. JS 는 현재 값을 읽고 쓰는 속성으로 같은 DWORD 를 빌린다.
        const state = {
            get denySalvage() { return game.denySalvage; },
            set denySalvage(value) { game.denySalvage = value; },
            get denyAscend() { return game.denyAscend; },
            set denyAscend(value) { game.denyAscend = value; },
            get missionNumber() { return missionNumber.value; },
            set missionNumber(value) { missionNumber.value = value; },
            get tutorialNumber() { return gate.tutorialNumber; },
            set tutorialNumber(value) { gate.tutorialNumber = value; },
            get stage() { return gate.stage; },
            set stage(value) { gate.stage = value; },
            get stageDirty() { return gate.stageDirty; },
            set stageDirty(value) { gate.stageDirty = value; },
        };
        releaseTutorialMission(state, {
            setAllowAll: (allow) => { setTotalMadeAllowAll(restrictions, allow); },
            removeAllBrains: () => { removeCurrentGuideBrains(client); },
            releaseBaseMission: releaseBase,
        });
    } else releaseBase(); // 기본 Mission 에는 Tutorial 의 허용 표·번호·AI 소멸을 추가하지 않는다.
}
