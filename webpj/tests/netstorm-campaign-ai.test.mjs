// 앱 Client의 AI 서비스가 같은 채팅 기록과 현재 설정·번역을 사용하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { displayTextCodec } from '../www/assets/display-text.js';

// 설정과 자기 Subscriber를 바꿔도 AI 서비스는 호출 시점의 같은 소유자를 읽는다.
test('앱 미션 AI: 채팅·언어·이름·현재 난이도', () => {
    const app = new Netstorm({ language: 'korean' }); app._initializeClientRuntime(false);
    const values = { difficulty: 2, skirmishRun: 0, skirmishAi2Diff: 3, skirmishAiCount: 2 };
    app._gameData = { getInt: (key) => values[key] ?? 0, pathSpec: () => 'Bot', substituteLocal: (text) => `${text}!` };
    app._resources = { translate: () => '사제' };
    const ai = app._campaignGameplayServices().ai, chat = app._clientChatRecords;
    const translated = ai.names.translate('Priest');
    assert.equal(displayTextCodec.decode(Uint8Array.from(translated, (item) => item.charCodeAt(0))), '사제');
    assert.equal(ai.names.language(), 7); assert.equal(ai.names.substitute('Bot'), 'Bot!'); assert.equal(ai.defaultName(), 'Bot');
    assert.equal(ai.difficulty(2, true), 2); values.skirmishRun = 1; assert.equal(ai.difficulty(2, true), 3); assert.equal(ai.skirmishPlayers(), 3);
    app._clientSubscriber.mySubscriber().value = 5; ai.participation.addChat(5, 'Self', 0x20); assert.equal(chat[0x2c], 0);
    ai.participation.addChat(6, 'Bot', 0x20); assert.equal(chat[0x2c], 6); assert.equal(chat[0x28], 0x20);
    ai.participation.clearChat(0x20); assert.equal(chat[0x28], 0); assert.equal(chat[0x2c], 6);
});
