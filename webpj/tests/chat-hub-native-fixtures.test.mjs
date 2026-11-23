// 채팅 허브의 원본 관측 두 개를 읽는 시험이다: chat-input-recipients(입력줄 대상 포함 검사 720행: 실제 Client 의 현재 Identity·대상 검사와 공통 ChatWindow 입력의 접합)와
// chat-callbacks(채팅 창 callback 29행: clear/로그/quit). 두 파일은 cpppj ChatWindowClientTests 가 읽는 원본 10.78 관측이며 웹 시험이 읽지 않던 것을 채운다.
// 입력 열만 웹 장치에 공급하고 관측 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-23 WB04-E1265)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { runtimeScene } from './client-chat-runtime-support.mjs';
import { bindClientChatRecipients } from '../www/core/client-chat-runtime.js';
import { ChatWindow, ChatWindowState } from '../www/ui/chat-window.js';
import { RawPlayerIdentity, PLAYER_IDENTITY_RECORD_SIZE } from '../www/core/raw-player-identity.js';
import { ClientChatWindow } from '../www/ui/client-chat-window.js';

// 머리글을 뺀 TSV 행이다(빈 열도 보존한다).
function rows(name) {
    const lines = fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0);
    return { header: lines[0].split('\t'), body: lines.slice(1).map((line) => line.split('\t')) };
}
// 16진 문자열('-' 은 빈 입력)을 한 글자가 한 바이트인 문자열로 바꾼다.
const bytesOf = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
// 레코드의 +8 이름을 첫 NUL 앞까지 읽는다.
function recordName(record) { let end = 8; while (end < record.length && record[end] !== 0) end += 1; return Buffer.from(record.subarray(8, end)).toString('latin1'); }

// 원본 720행: 모드(0 일반·1 전투·2 편집)·manager 현재값·입력줄 내용·대상 이름이 다른 입력에서 `ChatContainsRecipient` 의 반환이 같다. 'subscriber' 행은 Subscriber 번호만 넣은 새 식별 기록을 실제로 조회한 뒤 그 이름으로 검사한다.
test('chat-input-recipients 원본 720행: 입력줄 대상 포함 검사가 현재 Identity·대상 검사와 접합해 같은 반환을 낸다', () => {
    const { header, body } = rows('chat-input-recipients-x86.tsv');
    assert.deepEqual(header, ['operation', 'scene', 'manager', 'input', 'target', 'subscriber', 'result', 'trace', 'rawEax', 'instructions']);
    assert.equal(body.length, 720);
    const scene = runtimeScene(); const context = scene.context();
    bindClientChatRecipients(context, scene.client, scene.surface, () => scene.state.currentName);
    // 입력줄 본문은 행마다 바꾸는 한 상자이며 창의 Console 복사 장치가 128바이트에 NUL 과 함께 복사해 준다(원문 0042e9e0 의 지역 사본).
    const input = { text: '' }; const node = { console: 77 };
    const hooks = {
        chat: () => node, base: () => node, recipient: context.recipient,
        entry: { copyText: (token, bytes) => { assert.equal(token, 77); bytes.fill(0); for (let index = 0; index < input.text.length && index < 127; index += 1) bytes[index] = input.text.charCodeAt(index) & 0xff; return input.text.length; } },
    };
    const window = new ChatWindow(new ChatWindowState(), hooks); const failures = []; const counts = { contains: 0, subscriber: 0 };
    for (const [index, row] of body.entries()) {
        assert.equal(row.length, 10, `행 ${index}`);
        const mode = Number(row[1]);
        scene.surface.ownerMode.battle = mode === 1; scene.gameplay.world.editor = mode === 2 ? 1 : 0; scene.manager.current().value = Number(row[2]) >>> 0; input.text = bytesOf(row[3]);
        let result;
        if (row[0] === 'contains') { counts.contains += 1; result = window.containsRecipient(1, bytesOf(row[4])); }
        else {
            assert.equal(row[0], 'subscriber'); counts.subscriber += 1;
            // 원문 42f640 의 사슬: 조회가 실패하면 입력 복사를 먼저 하지 않는다. 기록은 0xa5 로 채우고 식별 초기화 뒤 Subscriber 한 DWORD 만 쓴다.
            const record = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE).fill(0xa5); RawPlayerIdentity.initialize(record, 0);
            const subscriber = Number(row[5]) >>> 0; for (let at = 0; at < 4; at += 1) record[4 + at] = (subscriber >>> (8 * at)) & 0xff;
            result = Boolean(context.resolve(record)) && window.containsRecipient(1, recordName(record));
        }
        if (String(result ? 1 : 0) !== row[6]) failures.push(`${index}/${row[0]}/${mode}`);
        assert.equal(input.text, bytesOf(row[3]), `행 ${index} 입력줄 보존`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
    assert.deepEqual(counts, { contains: 432, subscriber: 288 });
});

// 원본 29행: clear 10·로그 15·quit 4. 웹은 clear 를 e2e(`chat-window.spec.js` 의 `.clear`)와 `ClientChatWindow.invokeChatCommand` 로, quit 을 같은 함수의 안내 반환으로 대응하며
// 로그 파일 쓰기(file valid·write 호출)와 quit 큐 예약(RequestQuitApp)은 브라우저에 대응이 없어 웹 제외다. 이 시험은 구조·반환 열과 웹이 돌려주는 값(0)이 같음을 확인한다.
test('chat-callbacks 원본 29행: 세 callback 의 반환은 모두 0 이고 웹 quit/미지원 callback 처리가 같은 반환·오류를 낸다', () => {
    const { header, body } = rows('chat-callbacks-x86.tsv');
    assert.deepEqual(header, ['operation', 'current', 'file', 'valid', 'mutation', 'text', 'event', 'trace', 'finalCurrent', 'finalFile', 'result', 'instructions']);
    assert.equal(body.length, 29);
    const counts = {}; for (const row of body) { assert.equal(row.length, 12); counts[row[0]] = (counts[row[0]] ?? 0) + 1; assert.equal(row[10], '0', `${row[0]}/${row[4]} 반환`); }
    assert.deepEqual(counts, { clear: 10, log: 15, quit: 4 });
    // quit(0x42e520): 웹은 게임을 끝내지 않고 콘솔에 안내만 남기며 0 을 돌려준다(this 를 쓰지 않는 경로).
    const info = console.info; const logged = []; console.info = (...args) => logged.push(args.join(' '));
    try {
        for (let count = 0; count < counts.quit; count += 1) assert.equal(ClientChatWindow.prototype.invokeChatCommand.call({}, 0x42e520), 0);
        // 원문 세 callback 밖의 값은 조용히 넘기지 않고 오류다.
        assert.throws(() => ClientChatWindow.prototype.invokeChatCommand.call({}, 0x42f590), /지원하지 않는 원문 callback/);
    } finally { console.info = info; }
    assert.equal(logged.length, 4);
});
