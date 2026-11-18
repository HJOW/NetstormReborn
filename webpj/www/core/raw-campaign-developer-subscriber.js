// 31h Subscriber 명령의 웹 캠페인 분기와 현재 사용자 진단을 연결한다.
import { parseDeveloperCommand, readDeveloperIntegers } from './raw-campaign-developer-settings.js';

// 기존 Fort 편집이 아닌 현재 Session에서는 원문의 manager/fort 관문 뒤 현재 ID와 이름을 출력한다.
export function executeDeveloperSubscriber(text, owner, trace) {
    const { key, rest } = parseDeveloperCommand(text);
    if (key !== 'setsubid' && key !== 'ssi') return false;
    const integer = /^[\t\n\v\f\r ]*[+-]?\d+/.exec(rest);
    const name = integer && /^[\t\n\v\f\r ]*[^\t\n\v\f\r ]+/.exec(rest.slice(integer[0].length));
    if (integer && name) {
        readDeveloperIntegers(rest, 1);
        const slot = owner.playerState.templateOwner;
        if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('Subscriber 변경 Template9 범위');
        const headers = owner.templateStorage.headers(), view = new DataView(headers.buffer, headers.byteOffset, headers.byteLength);
        if (view.getUint32(slot * 20, true) && owner.core.ownerMode.fort) throw new Error('웹에서 제외된 기존 Fort Subscriber 변경');
    }
    const handle = owner.handle, end = handle.indexOf(0);
    if (end < 0) throw new RangeError('개발자 Subscriber 현재 handle NUL 누락');
    if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결');
    trace(29, `subid = ${owner.globals.mySubscriber | 0} handle = ${String.fromCharCode(...handle.subarray(0, end))}\n`);
    return true;
}
