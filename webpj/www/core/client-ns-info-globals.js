// NSInfo(준비 창)가 읽는 현재 전역(NSInfo 포인터 토큰·zone·서버 선택·서버 이름 80바이트 버퍼·시작 서버 행·종류 BYTE·zone 형식)을 한 Client 수명에 보관한다(cpppj client/ClientNsInfoGlobals.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 원본의 짧은 시작 서버 파일 행 읽기와 현재 서버 이름 해석(LAN 번역 / 무효 선택은 기존 바이트 보존 / 유효 행은 80바이트를 지우고 세미콜론 사이 이름을 최대 79바이트까지 복사)을 같은 바이트 규칙으로 옮긴다.
// 서버 행은 "바이트 문자열"(문자 하나 = 바이트 하나, 0..255)이고 이름 버퍼는 Uint8Array(80) 이다. 파일 쓰기/네트워크 편집은 이 단위의 일이 아니다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const globals = new ClientNsInfoGlobals(); globals.loadServerNames(fileBytes); globals.serverSelection = 0; const name = globals.resolveServerName((key) => translated); // name 은 이름 버퍼의 NUL 앞 부분을 빌린 Uint8Array 다.
// (2026-11-14 WB04-E444: RawMenuControls 선행 NSInfo 전역 이식)

// 원본 서버 이름 버퍼 크기(594970 의 공유 80바이트)와 시작 서버 파일의 한 행 최대 길이(미복원 긴 원본 행 읽기 경계)다.
const SERVER_NAME_BYTES = 80; const MAX_SERVER_LINE = 256;

// 호출자가 준 문자열/바이트 배열을 바이트 문자열로 맞춘다(문자 코드는 0..255 로 본다).
function byteString(value) {
    if (typeof value === 'string') return value;
    let text = ''; for (let index = 0; index < value.length; index += 1) text += String.fromCharCode(value[index]);
    return text;
}

// 현재 NSInfo 전역 묶음이다. 모든 필드는 읽는 시점의 현재 값이다.
export class ClientNsInfoGlobals {
    // 593150 의 현재 NSInfo 포인터 토큰과 5467b8 의 zone DWORD 다.
    current = 0; zone = 0;
    // 54db4c 의 서버 선택(signed, BSS 0)이다. Interpret Options 의 curServer 가 갱신한다. -1 은 LAN 서버를 뜻한다.
    serverSelection = 0;
    // 594970 의 공유 80바이트 서버 이름 버퍼다. 무효 서버 조회는 기존 바이트를 보존한다.
    serverName = new Uint8Array(SERVER_NAME_BYTES);
    // 532714/71c 의 읽기 전용 시작 서버 행(바이트 문자열)이다.
    servers = [];
    // 540421/440/457 의 보호 10.78 초기 종류 BYTE 와 540bac 의 zone 형식 문자열 토큰의 초기값이다.
    nsKind = 6; playerKind = 37; battleKind = 60; zoneFormat = 0x50c894;

    // 시작 서버 파일의 현재 행을 읽는다(보호 파일에 쓰지 않는다). 개행으로 나눈 비어 있지 않은 행을 순서대로 보관하고 모든 CR 을 지운다.
    // 256바이트(CR 포함)를 넘는 행은 RangeError(C++ length_error)이며 그때까지 읽은 행은 버려져 기존 목록이 유지된다.
    loadServerNames(file) {
        const loaded = []; let start = 0;
        // 각 행을 읽는다. 마지막 개행 뒤가 비어 있으면 행이 더 없다(std::getline 이 실패한다).
        while (start < file.length) {
            let end = file.indexOf(0x0a, start); const hasNewline = end >= 0; if (!hasNewline) end = file.length;
            if (end - start > MAX_SERVER_LINE) throw new RangeError('시작 서버 목록의 긴 원본 행 미연결');
            let line = '';
            // 한 행의 바이트를 CR 만 건너뛰며 바이트 문자열로 모은다.
            for (let index = start; index < end; index += 1) if (file[index] !== 0x0d) line += String.fromCharCode(file[index]);
            if (line.length > 0) loaded.push(line);
            start = hasNewline ? end + 1 : file.length;
        }
        this.servers = loaded;
    }

    // 47c430 → 현재 이름 출력 경계: 선택이 -1 이면 translate("LAN Server") 를 NUL 앞까지 복사(80바이트 이상이면 RangeError, 번역기 없으면 Error)하고 NUL 을 쓰되 뒤의 기존 바이트는 지우지 않는다.
    // 유효 행(0 이상·목록 안)이면 버퍼를 0 으로 채운 뒤 행(NUL 앞)의 첫 세미콜론 뒤~다음 세미콜론 앞(없으면 끝)을 최대 79바이트까지 복사한다. 그 밖의 선택은 버퍼를 건드리지 않는다.
    // 반환은 버퍼의 NUL 앞 부분을 빌린 Uint8Array 다(복사하지 않는다).
    resolveServerName(translate) {
        const selection = this.serverSelection | 0;
        if (selection === -1) {
            if (typeof translate !== 'function') throw new Error('현재 LAN 서버 이름 번역 미연결');
            const value = byteString(translate('LAN Server')); const nul = value.indexOf('\0'); const size = nul < 0 ? value.length : nul;
            if (size >= SERVER_NAME_BYTES) throw new RangeError('LAN 서버 이름의 원본80BYTE 경계 초과');
            // 이름 바이트를 그대로 복사한 뒤 종결 NUL 을 쓴다.
            for (let index = 0; index < size; index += 1) this.serverName[index] = value.charCodeAt(index) & 0xff;
            this.serverName[size] = 0;
        } else if (selection >= 0 && selection < this.servers.length) {
            this.serverName.fill(0);
            const record = this.servers[selection]; const cut = record.indexOf('\0'); const value = cut < 0 ? record : record.slice(0, cut); const first = value.indexOf(';');
            if (first >= 0) {
                const end = value.indexOf(';', first + 1); const name = value.slice(first + 1, end < 0 ? value.length : end); const count = Math.min(name.length, SERVER_NAME_BYTES - 1);
                // 이름의 앞 79바이트까지만 복사한다.
                for (let index = 0; index < count; index += 1) this.serverName[index] = name.charCodeAt(index) & 0xff;
            }
        }
        const nulAt = this.serverName.indexOf(0);
        return this.serverName.subarray(0, nulAt < 0 ? SERVER_NAME_BYTES : nulAt);
    }
}
