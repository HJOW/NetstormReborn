// 실제 장기 Chat/Core/Config/명령표와 명시 원문 입력을 공유하는 창 없는 Runtime 검사 장면이다. Client/UI 본체를 대체하는 제품 코드가 아니다.
import { ClientChatManagerGlobals } from '../www/core/client-chat-manager-globals.js';
import { ClientChatWindowGlobals } from '../www/core/client-chat-window-globals.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';
import { ClientDotCommandGlobals } from '../www/ui/dot-command-globals.js';
import { Config, ConfigRegistry } from '../www/assets/config.js';
import { encodeOriginalText, decodeWindows1252 } from '../www/assets/text-encoding.js';
import { createRawChatWindowContext } from '../www/ui/raw-chat-window-context.js';
import { bindClientChatRecipients, bindClientChatSubmit, initializeClientChatManager } from '../www/core/client-chat-runtime.js';

// 원문 little endian DWORD만 쓰고 인접 바이트는 보존한다.
export function put(bytes, at, value) { for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255; }
// 실제 네 BYTE를 부호 없는 DWORD로 읽는다.
export function word(bytes, at) { return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0; }
// 영어/원문1252 입력의 NUL 포함 BYTE 배열이다.
export function bin(value) { return Uint8Array.from([...value, '\0'], (character) => character.charCodeAt(0)); }
// 실제 첫 NUL 앞 바이트만 읽는다.
export function z(bytes, at = 0) { const end = bytes.indexOf(0, at); if (end < 0) throw new Error('검사 입력 NUL 없음'); return Buffer.from(bytes.subarray(at, end)).toString('latin1'); }
// 이름 입력과 NUL만 원문 물리에 쓴다.
export function name(bytes, at, value) { bytes.set(bin(value), at); }

// ClientChatRuntimeTests/ChatWindowClientTests의 같은 장기 owner와 독립 Player/Chat 입력이다. 선택 효과는 trace로 관측한다.
export function runtimeScene() {
    const manager = new ClientChatManagerGlobals(); const chat = new ClientChatWindowGlobals(); const core = new ClientCoreDataGlobals(); const commands = new ClientDotCommandGlobals();
    const config = new Config(new ConfigRegistry()); config.assign(''); core.players = new Uint8Array(1620);
    const state = { subscriber: 100, currentName: 'LocalUser', address: 0, records: new Uint8Array(4800), server: 'ns.example.org' };
    const gameplay = { player: { battleOwner: 1, templateOwner: 1, playerKnown: false }, world: { editor: 0 }, energy: { userCheat: 0 }, localAddress: () => state.address };
    name(core.players, 192, 'LocalUser'); put(core.players, 224, 100); name(core.players, 372, 'Alice'); put(core.players, 404, 200);
    name(state.records, 0, 'Bob'); put(state.records, 44, 300); name(state.records, 48, 'B'.repeat(40)); put(state.records, 92, 400);
    const surface = { ownerMode: { battle: false, challenge: false }, frames: [] }; const trace = [];
    const client = {
        chatManagerGlobals: () => manager, chatWindowGlobals: () => chat, coreDataGlobals: () => core, dotCommandGlobals: () => commands,
        currentSubscriber: () => state.subscriber, configuration: () => config, gameplayGlobals: () => gameplay, chatRecords: () => state.records,
        encodeDisplayText: encodeOriginalText, decodeDisplayText: decodeWindows1252, translate: (key) => key,
        nsInfoGlobals: () => ({ resolveServerName: () => state.server }), writeDiagnosticLog: (...args) => trace.push(['log', ...args]),
        requestThunderFlash: (brightness) => trace.push(['flash', brightness]), worldAudio: () => ({ sounds: () => ({ playByName: (...args) => trace.push(['sound', ...args]) }) }),
    };
    const menus = { chatGeometryBackend: () => ({ setInputVisible: (...args) => trace.push(['visible', ...args]) }), invokeChatCommand: (callback) => { trace.push(['command', callback]); return 0; }, printChat: (value) => trace.push(['print', value]) };
    // 실제 장기 참조를 빌리는 문맥이며 이름은 매조회 변경될 수 있다.
    const context = () => createRawChatWindowContext({ globals: chat, commands, translate: (key) => key });
    // 공통 실제 owner 초기화 뒤 대상 검사와 제출을 묶는다(메시지는 명시 외부 장치다).
    const submit = (message = null) => {
        initializeClientChatManager(client); const result = context(); result.message = message;
        bindClientChatRecipients(result, client, surface, () => state.currentName); bindClientChatSubmit(result, client, menus); return result;
    };
    return { client, manager, chat, core, commands, config, state, gameplay, surface, trace, menus, context, submit };
}
