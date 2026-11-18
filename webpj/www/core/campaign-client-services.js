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
        refreshSp: () => { client.renderer().invalidateAll(); const menu = client.menu(); if (menu) menu.refresh(); },
        refreshCommandGump,
        randomGeysers: () => world.session().generateWorldGeysers(),
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
