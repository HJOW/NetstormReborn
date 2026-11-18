// 실제 캠페인 미션 Start의 Client/World 외부 효과를 연결한다(10.78 단독 미션).
// 사용: makeCampaignMissionStartEffects(client, world, refreshCommandGump). 호출자와 실제 Client/World는 효과보다 오래 살아야 한다.
import { currentAllocationBytes } from './current-allocation-bytes.js';
import { DIFFICULTY_CONFIG_KEY } from './ai-difficulty.js';

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
