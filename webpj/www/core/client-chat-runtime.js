// 같은 Client의 장기 owner/현재 장치를 빌려 Chat 명령·Identity·서식·패킷을 연결한다(cpppj client/ClientChatRuntime.cpp/.h).
// 기준선1f00a25a의 전 함수를 이식하고 서버 뉴스 Config 경계는8efc76eb를 반영한다. 메타 로비 전용 RefreshBattleChatChannel/Members는 웹 제외다.
// Client 본체/RawMenuControls 조립은 후속이다. client의 camelCase getter들은 실제 기존 owner를 반환해야 하며 별도 게임 상태를 만들지 않는다.
// encodeDisplayText(UTF16)→Uint8Array와 decodeDisplayText(Uint8Array)→UTF16는 명시 플랫폼 경계다. 원문 좁은 문자열을 한국어 '?'로 손실 변환하지 않는다.
// 사용: initializeClientIssueCommands(client); initializeClientChatManager(client); bindClientChatRecipients(context,client,surface,nameGetter);
// bindClientChatMessage(context,client,surface,nameGetter,packetHooks); bindClientChatSubmit(context,client,menus). (2026-11-14 WB04-E455)
import { configParseLong } from '../assets/config.js';
import { makeClientChatManagerHooks, makeClientChatSubmitHooks } from './client-chat-manager-globals.js';
import { initializeChatManager, refreshChatManagerRankPrefix, CHAT_RANK_OUTPUT_BYTES } from './raw-chat-manager-prefix.js';
import { RawPlayerIdentity, PLAYER_IDENTITY_RECORD_SIZE } from './raw-player-identity.js';
import { readSystemNewsTime, writeSystemNewsTime } from './system-news-time.js';
import { checkChatRecipients } from '../ui/chat-recipients.js';
import { completeChatName } from '../ui/chat-name-completion.js';
import { applyChatFormat } from '../ui/raw-chat-format.js';
import { sayChatMessage, CHAT_MESSAGE_NAME_BYTES } from '../ui/raw-chat-message.js';
import { sendChatPacket } from '../ui/raw-chat-packet.js';
import { submitChatInput, CHAT_SUBMIT_WORK_BYTES } from '../ui/raw-chat-submit.js';
import { dispatchDotCommand } from '../ui/dot-command-dispatch.js';
import { invokeIssueCommand, registerIssueCommands } from '../ui/issue-commands.js';
import { applyScriptConfig } from '../ui/script-command.js';

// 선택한 장치가 없으면 그 위치에서 중단하며 미선택 UI/음향을 선행 요구하지 않는다.
function need(hook) { if (typeof hook !== 'function') throw new Error('Client Chat의 실제 선택 장치 미연결'); return hook; }
// 원문 clog 진단 뒤 계속하는 경계를 브라우저 console로 보낸다(실제 Client trace는 별도 writeDiagnosticLog 경계다).
function chatAssertion(line) { console.warn(`Chat 원문 진단:${line >>> 0}`); }
// 좁은 원문 바이트만 변환하며 UTF16을 낮은 BYTE로 자르는 암묵 변환을 거부한다.
function bytes(text, nul = false) {
    const result = new Uint8Array(text.length + Number(nul));
    // 원문 문자열의 각 BYTE를 같은 순서로 작성한다.
    for (let index = 0; index < text.length; index += 1) {
        const code = text.charCodeAt(index); if (code > 255) throw new RangeError('Client Chat의 원문 바이트 문자열 필요'); result[index] = code;
    }
    return result;
}
// 실제 NUL이 있는 구간만 문자열로 읽는다. 원문 지역의 미작성 꼬리는 관측하지 않는다.
function text(bytes) {
    const end = bytes.indexOf(0); if (end < 0) throw new RangeError('Client Chat 문자열의 실제 NUL 없음');
    return Array.from(bytes.subarray(0, end), (byte) => String.fromCharCode(byte)).join('');
}
// 현재 표시 인코더가 반환한 바이트를 별도 재부호화 없이 좁은 문자열로 빌린다.
function encode(client, value) {
    const encoded = need(client.encodeDisplayText).call(client, value);
    if (!(encoded instanceof Uint8Array)) throw new TypeError('Client Chat 표시 인코더의 실제 BYTE 배열 필요');
    return Array.from(encoded, (byte) => String.fromCharCode(byte)).join('');
}
// 현재 표시 디코더는 Config UTF16 경계에서만 사용한다.
function decode(client, value) { return need(client.decodeDisplayText).call(client, bytes(value)); }
// 실제 subscriber DWORD만 지정한60BYTE 지역의 +4에 쓴다.
function subscriber(record, value) { for (let index = 0; index < 4; index += 1) record[4 + index] = (value >>> (index * 8)) & 0xff; }
// 원문 네 BYTE의 little endian owner를 읽는다.
function ownerOf(record) { return (record[0] | (record[1] << 8) | (record[2] << 16) | (record[3] << 24)) >>> 0; }
// 같은 owner의 current 상자/현재 ID/활성 DWORD를 원문 사용 위치에서 다시 읽는다.
function managerGlobals(client) {
    const owner = client.chatManagerGlobals(); const core = client.coreDataGlobals();
    return { current: owner.current(), subscriber: () => client.currentSubscriber(), coreDataActive: () => core.active };
}
// 원문TYPE번호 첫70에 현재 icon 블록을 더하고 같은 frames의 B/C run 개수를 매번 조회한다.
function iconCount(assets, surface, letter) {
    const kind = (assets.find('icon').block + 70) >>> 0;
    if (kind >= surface.frames.length) throw new RangeError('Chat icon 현재TYPE 물리 범위');
    return surface.frames[kind].run(letter).count | 0;
}
// 서버 이름/Config의 원문 표시 바이트와 현재 UTF16 저장 경계를 같은 Client 장치에 연결한다.
function newsTimeHooks(client) {
    return {
        serverName: () => client.nsInfoGlobals().resolveServerName((source) => encode(client, client.translate(source))),
        read: (key) => configParseLong(client.configuration().get(decode(client, key)) ?? ''),
        configure: (expression) => { applyScriptConfig(decode(client, expression), (key, value) => client.configuration().set(key, value)); },
    };
}
// 값 사본이 아닌 같은 장기 뉴스 DWORD/128BYTE 키의 get/set 참조다.
function newsTimeGlobals(news) { return { key: news.newsKey, get value() { return news.newsTime; }, set value(value) { news.newsTime = value >>> 0; } }; }
// 서버별 현재 뉴스 시각을 읽으며 별도 저장/네트워크를 실행하지 않는다.
export function loadClientSystemNewsTime(client) { readSystemNewsTime(newsTimeGlobals(client.chatWindowGlobals()), newsTimeHooks(client)); }
// 수신한 현재 시각을 같은 Config 해석기에 쓰고 영속화 시점은 기존 Client에 맡긴다.
export function saveClientSystemNewsTime(client) { writeSystemNewsTime(newsTimeGlobals(client.chatWindowGlobals()), newsTimeHooks(client)); }
// 초기 issueCommand 네 등록만 수행한다. Chat manager 재생성이 반복 등록하지 않는다.
export function initializeClientIssueCommands(client) {
    registerIssueCommands((name, callback, flag) => client.dotCommandGlobals().register(bytes(name, true), callback, flag));
}
// 실제 장기 할당/명령표와 UTF16 Config의 두 원문 서식을 연결한다. 누락한 설정은 같은 빈 지역 문자열이다.
export function initializeClientChatManager(client) {
    const hooks = makeClientChatManagerHooks(client.chatManagerGlobals()); hooks.assertion = chatAssertion;
    hooks.registerCommand = (name, callback, flag) => client.dotCommandGlobals().register(bytes(name, true), callback, flag);
    hooks.readConfig = (key) => encode(client, client.configuration().get(key) ?? '');
    initializeChatManager(managerGlobals(client), hooks);
}
// manager0/active0이면 Core/icon 하위를 조회하지 않는다. 실제 최대랭크는 두 현재TYPE 조회의 DWORD 곱/감김이다.
export function refreshClientChatRankPrefix(client, assets, surface) {
    const hooks = makeClientChatManagerHooks(client.chatManagerGlobals()); hooks.assertion = chatAssertion;
    hooks.core = () => { const metamap = Number(surface.ownerMode.challenge); return client.coreDataGlobals().read(client.gameplayGlobals().player, metamap, { assertion: chatAssertion }); };
    hooks.minorCount = () => iconCount(assets, surface, 'B');
    hooks.maximumRank = () => { const minor = iconCount(assets, surface, 'B') >>> 0; const major = iconCount(assets, surface, 'C') >>> 0; return (Math.imul(major, minor) - 1) | 0; };
    refreshChatManagerRankPrefix(managerGlobals(client), hooks, new Uint8Array(CHAT_RANK_OUTPUT_BYTES), new Uint8Array(CHAT_RANK_OUTPUT_BYTES));
}
// 매조회 현재Playerbase/Chat100행을 빌린다. 로컬 일치 전에는 메타 표를 요구하지 않으며 웹 제외 메타 검색은 기존 명시 오류를 유지한다.
export function resolveClientChatIdentity(client, surface, currentName, record) {
    const reads = {
        localOwner: () => client.gameplayGlobals().player.battleOwner, subscriber: () => client.currentSubscriber(),
        address: () => client.gameplayGlobals().localAddress(), name: () => currentName,
        battle: () => surface.ownerMode.battle, editor: () => client.gameplayGlobals().world.editor !== 0, metamap: () => surface.ownerMode.challenge,
    };
    return new RawPlayerIdentity(client.coreDataGlobals().players, new Uint8Array(0), client.chatRecords(), reads).resolve(record);
}
// 같은 실제 이름 getter/번역을 두 UI 소비자와 자동완성에 연결한다. bind 때의 번역/resolve 캡처와 매조회 현재 owner를 구별한다.
export function bindClientChatRecipients(context, client, surface, currentName) {
    if (typeof currentName !== 'function' || typeof context.translate !== 'function' || !surface) throw new TypeError('Chat 대상 검사의 현재 이름/번역/표면 장치 필요');
    context.resolve = (record) => resolveClientChatIdentity(client, surface, currentName(), record);
    const resolve = context.resolve; const translate = context.translate;
    context.recipient = (value) => checkChatRecipients(client.chatManagerGlobals().current().value, value, new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE), { resolve, translate });
    context.editor.recipient = context.recipient;
    context.editor.complete = (callback, word, length, backward) => {
        if ((callback >>> 0) !== 0x42e540) throw new Error('실제 Console의 미지원 자동완성 callback');
        return completeChatName(client.chatWindowGlobals().completion, word, length, backward, { translate, records: () => client.chatRecords(), assertion: chatAssertion });
    };
    context.diagnostic = (level, format, parameter) => {
        if (format !== 'Could not lookup a name for subId == %d\n') throw new Error('Chat 이름 조회 진단의 미지원 원문 서식');
        client.writeDiagnosticLog(level | 0, `Could not lookup a name for subId == ${parameter | 0}\n`);
    };
}
// 같은 manager/공통 캡처를 쓰고 좁은 바이트 서식을 현재 UTF16 Config 경계에서만 해독한다.
export function applyClientChatFormat(client, front, input) {
    const owner = client.chatManagerGlobals(); const memory = makeClientChatManagerHooks(owner);
    return applyChatFormat(front, { current: owner.current().value, subscriber: client.currentSubscriber() }, input, owner.formatCapture(), {
        memory: memory.memory, copyFormat: memory.copyFormat, assertion: chatAssertion,
        writeConfig: (key, value) => client.configuration().set(key, decode(client, value)),
        illegalFormat: (value) => client.writeDiagnosticLog(29, `Illegal format "${value}"\n`),
    });
}
// flags2 지역Identity에 subscriber만 쓰고 실패해도 원문 초기 owner0을 읽는다.
export function findClientChatSenderOwner(client, surface, currentName, value) {
    const record = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE); RawPlayerIdentity.initialize(record, 2); subscriber(record, value);
    resolveClientChatIdentity(client, surface, currentName, record); return ownerOf(record);
}
// 실제 현재이름/Identity/manager를 같은 일반 본문 조합에 빌린다. 목적 이름 getter 반환1은 조회 성공 여부가 아닌 포인터 별칭이다.
export function makeClientChatMessageHooks(client, surface, currentName) {
    if (!surface || typeof currentName !== 'function') throw new TypeError('Chat 일반 조합의 실제 표면/현재 이름 getter 필요');
    const owner = client.chatManagerGlobals();
    return {
        memory: (token) => owner.bytes(token), currentName,
        nameForSubscriber: (output, value) => {
            const record = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE); RawPlayerIdentity.initialize(record, 0); subscriber(record, value);
            const name = resolveClientChatIdentity(client, surface, currentName(), record) ? text(record.subarray(8)) : 'SOMEONE';
            if (output.length <= name.length) throw new RangeError('Chat 대상 이름 getter의 실제 목적 물리 부족');
            output.set(bytes(name)); output[name.length] = 0; return 1;
        },
    };
}
// 같은 현재 manager/ID와 실제 패킷 writer를 일반 say에 연결한다. 빈 본문의 표시 장치는 제출 분배기가 선행 처리한다.
export function bindClientChatMessage(context, client, surface, currentName, packet) {
    const hooks = makeClientChatMessageHooks(client, surface, currentName); const captured = { ...packet };
    hooks.deliver = (manager, recipients, body) => sendChatPacket(manager, recipients, body, captured);
    context.message = (input) => sayChatMessage(input, new Uint8Array(CHAT_MESSAGE_NAME_BYTES), { current: client.chatManagerGlobals().current().value, subscriber: client.currentSubscriber() }, hooks);
}
// 실제 제출→공유 명령표→issue/clear/quit/서식/say를 같은 장기 owner와 현재 UI에 잇는다. 입력 인자는 원문 바이트 문자열이다.
export function bindClientChatSubmit(context, client, menus) {
    if (typeof context.resolve !== 'function' || typeof context.translate !== 'function') throw new TypeError('Chat 제출의 실제 Identity/번역 장치 필요');
    const resolve = context.resolve; const translate = context.translate; const message = context.message;
    context.sendChat = (manager, input) => {
        const hooks = makeClientChatSubmitHooks(client.chatManagerGlobals()); hooks.assertion = chatAssertion; hooks.resolve = resolve; hooks.translate = translate; hooks.records = () => client.chatRecords();
        hooks.inputVisible = (visible) => {
            const current = client.chatWindowGlobals().window.current; if (!current) return;
            const geometry = menus.chatGeometryBackend(); if (!geometry) throw new Error('Chat 제출의 현재 입력 표시 장치 없음'); geometry.setInputVisible(current, visible);
        };
        hooks.dispatch = (body) => {
            const commands = {
                invoke: (callback, argument) => {
                    const value = text(argument); const energy = client.gameplayGlobals().energy;
                    const cheat = { get value() { return energy.userCheat; }, set value(value) { energy.userCheat = value >>> 0; } };
                    const result = invokeIssueCommand(callback, value, cheat, {
                        flash: (brightness) => client.requestThunderFlash(brightness),
                        sound: (name) => { client.worldAudio().sounds().playByName(name, 0, 0, 0, 0, 0); },
                    });
                    if (result !== null) return result;
                    if (callback === 0x42f680 || callback === 0x42e520) return menus.invokeChatCommand(callback);
                    if (callback === 0x42ddf0 || callback === 0x42def0) return applyClientChatFormat(client, callback === 0x42def0, value);
                    if (callback === 0x42f1e0) {
                        if (!client.chatManagerGlobals().current().value) return 0;
                        if (!argument.length) throw new RangeError('Chat say의 실제 인자 물리 없음');
                        if (!argument[0]) { hooks.inputVisible(0); return 0; }
                        if (argument[0] === 0x20) return 0;
                        return need(message)(argument);
                    }
                    throw new Error(`Chat 제출의 실제 명령 callback 미연결:${callback}`);
                },
                error: (name, code) => menus.printChat(`Error in command ".${name}": code == ${(code >>> 0).toString(16)}\n`),
            };
            const owner = client.dotCommandGlobals(); dispatchDotCommand(owner.bytes(), owner.dispatchState(), body, commands);
        };
        submitChatInput(manager, input, new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE), new Uint8Array(CHAT_SUBMIT_WORK_BYTES), hooks);
    };
}
