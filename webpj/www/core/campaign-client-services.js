// 실제 캠페인 미션 Start의 Client/World 외부 효과를 연결한다(10.78 단독 미션).
// 사용: makeCampaignMissionStartEffects(client, world, refreshCommandGump). 호출자와 실제 Client/World는 효과보다 오래 살아야 한다.
import { currentAllocationBytes } from './current-allocation-bytes.js';
import { DIFFICULTY_CONFIG_KEY } from './ai-difficulty.js';
import { captureLocalMissionStarting, RawLocalMissionState } from './raw-local-mission-state.js';

// 같은 실제 Client 시계·화면 및 현재 World의 카메라·전체 간헐천 생성기를 빌린다.
export function makeCampaignMissionStartEffects(client, world, refreshCommandGump) {
    if (typeof refreshCommandGump !== 'function') throw new TypeError('미션 Start의 현재 명령 Gump 조회 미연결');
    return {
        settingsBytes: (_owner, bytes) => currentAllocationBytes(bytes),
        gameSeconds: () => client.time().game,
        refreshSp: () => { const menu = client.menu(); if (menu) menu.refreshStormPower(); },
        refreshCommandGump,
        randomGeysers: () => world.session().generateWorldGeysers(),
        // 12차 스커미시: 계획이 있을 때만 MP 규칙으로 섬을 만든다(일반 미션은 계획이 없어 아무 일도 하지 않는다). (2026-11-20 WB04-E1105)
        prepareWorld: () => world.session().buildSkirmishWorld(),
        resetCamera: () => world.resetCampaignCamera(),
        unknownAbilityAssertion: (name) => console.info(`미션 AI의 원본 능력 이름 진단: ${name}`),
        unknownAbilityLog: (name) => console.info(`미등록 미션 AI 능력: ${name}`),
    };
}

// 원본 좁은 문자열을 같은 BYTE 배열로 바꾸어 표시 디코더에 전달한다.
function decodeDisplay(client, text) {
    const bytes = new Uint8Array(text.length);
    // 원본 문자열의 각 문자는 한 BYTE이며 물리 범위 밖은 거부한다.
    for (let index = 0; index < text.length; index += 1) { const value = text.charCodeAt(index); if (value > 255) throw new RangeError('Client AI 원문 바이트 범위'); bytes[index] = value; }
    return client.decodeDisplayText(bytes);
}
// 현재 표시 인코더의 바이트를 원본과 같은 좁은 문자열로 보관한다.
function encodeDisplay(client, text) {
    const bytes = client.encodeDisplayText(text); if (!(bytes instanceof Uint8Array)) throw new TypeError('Client AI 표시 인코더 BYTE 배열 누락');
    return Array.from(bytes, (byte) => String.fromCharCode(byte)).join('');
}

// 같은 Client의 현재 언어·설정·채팅 및 미션/스커미시 AI 난이도 조회를 연결한다.
export function bindCampaignAiClientServices(client, ai) {
    ai.names ??= {}; ai.participation ??= {};
    ai.names.language = () => client.languageNumber() >>> 0;
    ai.names.translate = (source) => encodeDisplay(client, client.translate(decodeDisplay(client, source)));
    ai.names.substitute = (source) => encodeDisplay(client, client.configuration().substituteLocal(decodeDisplay(client, source)));
    ai.participation.clearChat = (channel) => { client.chatChannels().clear(channel); };
    ai.participation.addChat = (subscriber, name, channel) => { client.chatChannels().addToChannel(subscriber, name, channel); };
    ai.skirmishPlayers = () => {
        const config = client.configuration(); return config.getInt('skirmishRun') !== 0 ? 1 + Math.max(1, Math.min(7, config.getInt('skirmishAiCount'))) : 0;
    };
    ai.difficulty = (owner, enemy) => {
        const config = client.configuration();
        if (config.getInt('skirmishRun') !== 0 && owner >= 2 && owner <= 8) return config.getInt(`skirmishAi${owner}Diff`) >>> 0;
        return enemy ? config.getInt(DIFFICULTY_CONFIG_KEY) >>> 0 : 0;
    };
    ai.defaultName = () => encodeDisplay(client, client.configuration().pathSpec('defaultAiName'));
}

// 개발자 명령과 신전 이탈이 같은 Client의 현재 입력 큐에 signed 좌표로 예약되게 한다.
export function bindCampaignClientInputServices(client, services) {
    services.developerQueue = (code, first, second) => client.input().pushCommand(code >>> 0, first | 0, second | 0);
}

// 지역 도장이 호출될 때마다 실제 Client 설정의 원문 키를 읽고, 키가 없으면 원본 기본값 1을 사용한다. (2026-11-19 WB04-E873)
export function bindCampaignOwnershipTheme(client, services) {
    services.ownership ??= {};
    services.ownership.themeMode = () => client.configuration().readInt('theammode', 1).value >>> 0;
}

// 신전 소유권과 자산 PostPop의 생산 갱신을 같은 Gump 입력 트리에 전달한다. 숨겨진 노드 포함과 캡처 우선은 원본 Route가 맡는다. (2026-11-19 WB04-E875)
export function bindCampaignProductionRefresh(input, services) {
    if (!input || typeof input.route !== 'function') throw new TypeError('생산 갱신의 실제 Gump 입력 미연결');
    services.ownership ??= {}; services.assets ??= {}; services.assets.postPop ??= {};
    const refresh = () => { input.route(0x02000003, 0, 0, true); };
    services.ownership.refreshProduction = refresh; services.assets.postPop.refreshProduction = refresh;
}

// 로그인 안내와 타이머가 선택한 시점의 Client 메뉴·시계·번역을 조회한다.
export function bindCampaignLoginProgressServices(client, controls, login) {
    if (client.edition() !== 'patch1078') return;
    login.closeProgress = () => {
        const menus = controls.menus(); if (!menus) throw new Error('로그인 진행 안내의 실제 메뉴 미설치');
        menus.closeMissionLoading();
    };
    login.progress = (kind, remaining) => {
        const menu = client.menu(); if (!menu) throw new Error('현재 로그인 안내의 실제 UberGump 미연결');
        menu.showLoginProgress(kind, remaining);
    };
    login.translate = (key) => encodeDisplay(client, client.translate(decodeDisplay(client, key)));
    login.frameWallSeconds = () => client.time().wall;
    login.showTimerProgress = (text) => {
        const menu = client.menu(); if (!menu) throw new Error('일반 대기 안내의 실제 UberGump 미연결');
        return menu.showLoginWaiting(text);
    };
    login.closeTimerProgress = () => {
        const menus = controls.menus(); if (!menus) throw new Error('일반 대기 안내의 실제 메뉴 미설치');
        menus.closeHtmlSerial(client.localStateGlobals().loginProgressSerial());
    };
}

// 현재 State7과 요청의 실제 Zone을 검사한 뒤 기본 분모와 정리 전 지역 값을 캡처한다.
export function captureCampaignMissionStarting(client, request) {
    const local = client.localStateGlobals(), state = local.state();
    if (client.edition() !== 'patch1078' || request.consumed || request.starting || request.zone.length !== 86 || state.current[0] !== 7 || state.current[32] !== 1 || request.next[32] !== 1) throw new Error('실제 현재7/Zone/미캡처 앞단 계약 오류');
    client.gameplayGlobals().camera().selectDenominator(0, client.kencloudGlobals().worldRedraw());
    const title = client.htmlWindowGlobals().text.background;
    const globals = {
        get zonePending() { return local.zonePending().value; },
        get gameTicks() { return local.gameTicks().value; },
        get sourceType() { return local.sourceType().value; },
        set titleInitial(value) { title[0] = value; },
    };
    request.starting = captureLocalMissionStarting(state.current, globals);
}

// 다음 상태 7의 실제 블록을 현재로 복사하고 첫 DWORD만 지운 뒤 같은 앞단을 캡처한다.
export function prepareCampaignMissionStarting(client, request) {
    const state = client.localStateGlobals().state();
    if (client.edition() !== 'patch1078' || request.consumed || request.starting || request.zone.length !== 86 || state.next[0] !== 7 || state.next[32] !== 1 || request.next[32] !== 1) throw new Error('실제 State7 현재 요청/Zone/새 앞단 계약 오류');
    state.current.set(state.next); state.next[0] = 0; captureCampaignMissionStarting(client, request);
}

// World 교체 중에도 같은 Client·표면·State를 빌려 State7과 후속 State8을 공통 부모에서 확정한다.
export function runCampaignMissionStarting(client, body, connect) {
    const storage = client.surfaceStorage();
    if (client.edition() !== 'patch1078' || !storage || typeof body !== 'function' || typeof connect !== 'function') throw new Error('실제 미션 부모의 판본/장기 표면/몸체/접속 장치 계약 오류');
    const local = client.localStateGlobals(), game = client.gameplayGlobals(), title = client.htmlWindowGlobals().text.background;
    const globals = {
        get server() { return game.authority().server; }, set server(value) { game.authority().server = Boolean(value); },
        get boss() { return game.authority().boss; }, set boss(value) { game.authority().boss = Boolean(value); },
        get inBattle() { return storage.ownerMode.battle; }, set inBattle(value) { storage.ownerMode.battle = Boolean(value); },
        get missionPost() { return game.world.missionPost; }, get missionWorld() { return storage.links.mission; },
        get savedGame() { return local.savedGame().value; }, set savedGame(value) { local.savedGame().value = value; },
        get simulconnect() { return local.simulconnect().value; }, get metamap() { return storage.ownerMode.challenge; }, player: game.player,
        get zonePending() { return local.zonePending().value; }, set zonePending(value) { local.zonePending().value = value; },
        get gameTicks() { return local.gameTicks().value; }, set gameTicks(value) { local.gameTicks().value = value; },
        get sourceType() { return local.sourceType().value; }, set sourceType(value) { local.sourceType().value = value; },
        get cdPresent() { return local.cdPresent().value; }, set cdPresent(value) { local.cdPresent().value = value; },
        get metaRefresh() { return local.metaRefresh().value; }, set metaRefresh(value) { local.metaRefresh().value = value; },
        get metaRebuild() { return local.metaRebuild().value; }, set metaRebuild(value) { local.metaRebuild().value = value; },
        get titleInitial() { return title[0]; }, set titleInitial(value) { title[0] = value; }, get battleByte() { return local.battleByte().value; },
        get connectEnd() { return local.connectEnd().value; }, set connectEnd(value) { local.connectEnd().value = value; },
        get activityEnd() { return local.activityEnd().value; }, set activityEnd(value) { local.activityEnd().value = value; },
        get gameSeconds() { return client.time().game; }, get wallCached() { return client.time().wall; },
    };
    const hooks = { logState: (value) => console.info(`State ${value}`), wallSeconds: () => client.wallSeconds(), connectServer: connect,
        invalidState: (value) => { throw new Error(`미션 부모의 선택된 다른 Native 상태 미연결:${value}`); } };
    new RawLocalMissionState(local.state(), globals, hooks).pollStarting(body);
}
