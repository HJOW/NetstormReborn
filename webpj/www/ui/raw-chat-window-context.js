// 실제 ChatWindow 공장이 빌리는 장기 owner와 선택 장치 경계다(cpppj client/RawChatWindowContext.h, 기준선1f00a25a).
import { createRawTextEntryContext } from './text-entry-gump.js';

// globals/commands는 참조를 빌리고 editor는 C++ 값 구조체처럼 필드를 복사한다. 콜백은 생성한 UI의 입력/소멸보다 오래 살아야 한다.
// read/translate/storeText/text/recipient/sendChat/message/resolve/diagnostic/config/fileValid/writeLog/quit는 선택 시 실제 장치를 요구하는 소비자가 호출한다.
// 사용: createRawChatWindowContext({globals,commands,editor:{now},translate}). (2026-11-14 WB04-E455)
export function createRawChatWindowContext(fields = {}) {
    return {
        globals: null, commands: null, read: null, translate: null, storeText: null, text: null,
        recipient: null, sendChat: null, message: null, resolve: null, diagnostic: null, config: null,
        fileValid: null, writeLog: null, quit: null, ...fields, editor: createRawTextEntryContext(fields.editor),
    };
}
