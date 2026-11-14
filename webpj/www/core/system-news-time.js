// 서버별 뉴스 시각의 공유128BYTE 키와 Config 표현식을 옮긴다(cpppj client/SystemNewsTime.cpp/.h, 8efc76eb).
// 네트워크/파일 쓰기는 하지 않는다. globals={key:Uint8Array,value:get/set DWORD}, hooks={serverName,read,configure}이며 문자열은 원문 바이트 문자열이다.
// 사용: readSystemNewsTime(globals,hooks), writeSystemNewsTime(globals,hooks). (2026-11-14 WB04-E455)

// 선택한 하위만 요구하며 키 작성 뒤 실패해도 앞선 공유 메모리를 되돌리지 않는다.
function need(hook) { if (typeof hook !== 'function') throw new Error('시스템 뉴스 시각의 실제 장치 미연결'); return hook; }

// 첫 NUL 앞 서버 이름에 NewsTime을 붙이고 '.'만 '_'로 바꾼다. 용량 오류는 쓰기 전이며 NUL 뒤 공유 꼬리는 보존한다.
export function buildSystemNewsTimeKey(globals, hooks) {
    const name = need(hooks.serverName)().split('\0', 1)[0]; const formatted = `${name}NewsTime`;
    if (formatted.length >= 256 || formatted.length >= globals.key.length) throw new RangeError('시스템 뉴스 키의 실제 로컬/공유 물리 부족');
    // 원문 strcpy가 쓰는 바이트와 NUL만 현재 공유 키에 작성한다.
    for (let index = 0; index < formatted.length; index += 1) globals.key[index] = formatted.charCodeAt(index) & 0xff;
    globals.key[formatted.length] = 0;
    // 이번에 작성한 문자열만 순회하며 키 뒤의 이전 흔적은 건드리지 않는다.
    for (let index = 0; index < formatted.length; index += 1) if (globals.key[index] === 0x2e) globals.key[index] = 0x5f;
    return Array.from(globals.key.subarray(0, formatted.length), (byte) => String.fromCharCode(byte)).join('');
}

// Config의 signed 반환 비트를 같은 DWORD에 기록한다(누락은 read 장치의 원래0이다).
export function readSystemNewsTime(globals, hooks) {
    const key = buildSystemNewsTimeKey(globals, hooks); globals.value = need(hooks.read)(key) >>> 0;
}

// 현재 시각을 서버 이름 getter보다 먼저 캡처하고 원문처럼 키를 별도로 이스케이프하지 않은 설정문을 전달한다.
export function writeSystemNewsTime(globals, hooks) {
    const value = globals.value >>> 0; const key = buildSystemNewsTimeKey(globals, hooks); const expression = `${key}="${value | 0}"`;
    if (expression.length >= 256) throw new RangeError('시스템 뉴스 설정문256BYTE 물리 부족');
    need(hooks.configure)(expression);
}
