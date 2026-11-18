// 31h 개발자 명령 중 현재 미션 설정과 즉시 건설 전역을 조작한다(원본 ExecuteDeveloperCommand).
// 사용: executeDeveloperSettings(text, gameAux, trace) → 처리한 명령이면 true, 다른 명령이면 false.

// 첫 토큰 뒤의 원본 %d 입력을 읽는다. 정수 범위 밖의 미정 변환은 명시 오류로 둔다.
export function firstDeveloperInteger(rest) {
    const match = /^[\t\n\v\f\r ]*([+-]?\d+)/.exec(rest);
    if (!match) return null;
    const value = BigInt(match[1]);
    if (value < -2147483648n || value > 2147483647n) throw new RangeError('개발자 명령 정수의 실제 signed DWORD 범위');
    return Number(value);
}

// 첫 NUL 앞에서 원본 255BYTE 명령 토큰과 나머지 입력을 분리한다.
export function parseDeveloperCommand(text) {
    if (typeof text !== 'string') throw new TypeError('개발자 명령 문자열 계약');
    const input = text.split('\0', 1)[0], token = /^[\t\n\v\f\r ]*([^\t\n\v\f\r ]+)/.exec(input);
    if (!token) throw new TypeError('개발자 명령의 미정 첫 토큰');
    return { key: token[1].slice(0, 255).replace(/[A-Z]/g, (letter) => letter.toLowerCase()), rest: input.slice(token[0].length) };
}

// 같은 GameAux의 일곱 설정과 quick 전역을 원본 분기 순서로 변경한다. (2026-11-18 WB04-E740)
export function executeDeveloperSettings(text, gameAux, trace) {
    const { key, rest } = parseDeveloperCommand(text), values = gameAux.missionSettings();
    // 원본 로그를 선택한 분기에서만 실제 Client 추적 장치로 전달한다.
    const log = (message) => { if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결'); trace(29, message); };
    // 필수 정수가 빠지면 원문의 미정 스택을 합성하지 않는다.
    const required = () => { const value = firstDeveloperInteger(rest); if (value === null) throw new TypeError('개발자 명령의 원문 미정 정수 입력'); return value; };
    if (key === 'sethackmove' || key === 'shm') { values[1] = Number(required() !== 0); log(`hackmover now ${values[1]}\n`); return true; }
    if (key === 'setnodamage' || key === 'snd') {
        values[2] = Number(required() !== 0); log(`nodamagedebug now ${values[2]}\n`);
        log('requested server to do same.\n***Not echoed to other clients.  This will FUCK EVERYTHING!***\n'); return true;
    }
    if (key === 'shootmyfriends' || key === 'smf') { values[3] = Number((firstDeveloperInteger(rest) ?? 1) !== 0); return true; }
    if (key === 'fillmytank' || key === 'fmt' || key === 'generatortimer' || key === 'gt') {
        const value = firstDeveloperInteger(rest);
        if (value === null || value < 0) { log('bad command\n'); return true; }
        if (key === 'fillmytank' || key === 'fmt') values[4] = value; else values[0] = Number(value !== 0);
        return true;
    }
    if (key === 'quickvortex' || key === 'qv') { gameAux.quickVortex = Number(required() !== 0); return true; }
    if (key === 'quickbuild' || key === 'qb') { gameAux.quickBuild = Number((firstDeveloperInteger(rest) ?? 1) !== 0); return true; }
    if (key === 'roundtotile' || key === 'rtt') return true;
    return false;
}
