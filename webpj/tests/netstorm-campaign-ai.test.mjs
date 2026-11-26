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
    const allocation = Uint8Array.of(7, 8); ai.allocationBytes(allocation); assert.deepEqual([...allocation], [7, 8]);
    const translated = ai.names.translate('Priest');
    assert.equal(displayTextCodec.decode(Uint8Array.from(translated, (item) => item.charCodeAt(0))), '사제');
    assert.equal(ai.names.language(), 7); assert.equal(ai.names.substitute('Bot'), 'Bot!'); assert.equal(ai.defaultName(), 'Bot');
    assert.equal(ai.difficulty(2, true), 2); values.skirmishRun = 1; assert.equal(ai.difficulty(2, true), 3); assert.equal(ai.skirmishPlayers(), 3);
    app._clientSubscriber.mySubscriber().value = 5; ai.participation.addChat(5, 'Self', 0x20); assert.equal(chat[0x2c], 0);
    ai.participation.addChat(6, 'Bot', 0x20); assert.equal(chat[0x2c], 6); assert.equal(chat[0x28], 0x20);
    ai.participation.clearChat(0x20); assert.equal(chat[0x28], 0); assert.equal(chat[0x2c], 6);
    // 로그인 수신기도 같은 현재 장부를 읽어 채널만 지우고 이름·다른 채널은 유지한다.
    const login = app._campaignLoginServices();
    ai.participation.addChat(6, 'Bot', 0x60); login.removeChat(6, 0x20);
    assert.equal(chat[0x28], 0x40); assert.equal(chat[0x2c], 6); assert.equal(chat[0], 66);
});

// Session이 받는 창·무시 목록은 앱이 장기 소유하는 같은 객체다.
test('앱 미션 채팅: Client 창 전역 대여', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    const shared = app._campaignSharedServices(), windows = app._clientChatWindows;
    assert.equal(shared.chatGlobals, windows); windows.window.current = 17; shared.chatGlobals.ignored.push(42);
    assert.equal(app._campaignSharedServices().chatGlobals.window.current, 17); assert.deepEqual(windows.ignored, [42]);
});
