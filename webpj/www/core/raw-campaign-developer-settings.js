// 31h 개발자 명령 중 현재 미션 설정과 즉시 건설 전역을 조작한다(원본 ExecuteDeveloperCommand).
// 사용: executeDeveloperSettings(text, gameAux, trace, services) → 처리한 명령이면 true, 다른 명령이면 false.

// 첫 토큰 뒤의 연속 %d 입력을 차례로 읽는다. 정수 범위 밖의 미정 변환은 명시 오류로 둔다.
export function readDeveloperIntegers(rest, count) {
    const result = []; let remaining = rest;
    // 앞 변환이 멈춘 위치에서 다음 정수 변환을 시작한다.
    for (let index = 0; index < count; index += 1) {
        const match = /^[\t\n\v\f\r ]*([+-]?\d+)/.exec(remaining);
        if (!match) break;
        const value = BigInt(match[1]);
        if (value < -2147483648n || value > 2147483647n) throw new RangeError('개발자 명령 정수의 실제 signed DWORD 범위');
        result.push(Number(value)); remaining = remaining.slice(match[0].length);
    }
    return result;
}

// 원본 단일 %d 변환의 실패를 null로 돌려준다.
export function firstDeveloperInteger(rest) { return readDeveloperIntegers(rest, 1)[0] ?? null; }

// 첫 NUL 앞에서 원본 255BYTE 명령 토큰과 나머지 입력을 분리한다.
export function parseDeveloperCommand(text) {
    if (typeof text !== 'string') throw new TypeError('개발자 명령 문자열 계약');
    const input = text.split('\0', 1)[0], token = /^[\t\n\v\f\r ]*([^\t\n\v\f\r ]+)/.exec(input);
    if (!token) throw new TypeError('개발자 명령의 미정 첫 토큰');
    return { key: token[1].slice(0, 255).replace(/[A-Z]/g, (letter) => letter.toLowerCase()), rest: input.slice(token[0].length) };
}

// 같은 GameAux의 일곱 설정과 quick 전역을 원본 분기 순서로 변경한다. (2026-11-18 WB04-E740)
export function executeDeveloperSettings(text, gameAux, trace, services = {}) {
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
    if (key === 'settracelevel' || key === 'stl') {
        const token = /^[\t\n\v\f\r ]*([^\t\n\v\f\r ]+)/.exec(rest);
        if (!token) throw new TypeError('개발자 tracelevel의 원문 미정 문자열 입력');
        if (typeof services.install?.setConfig !== 'function') throw new Error('개발자 tracelevel 실제 Config 미연결');
        const value = token[1].slice(0, 255);
        services.install.setConfig('tracelevel', Uint8Array.from(`${value}\0`, (letter) => letter.charCodeAt(0))); return true;
    }
    if (key === 'combatgumpcolsrows' || key === 'cr') {
        const parsed = readDeveloperIntegers(rest, 2), columns = parsed.length === 2 ? parsed[0] : 3, rows = parsed.length === 2 ? parsed[1] : 10;
        values[5] = Math.max(1, Math.min(3, columns)); values[6] = Math.max(1, Math.min(16, rows));
        if (values[5] * values[6] > 8) values[0] = 0;
        if (typeof services.developerQueue !== 'function') throw new Error('개발자 cr의 실제 입력 큐 미연결');
        services.developerQueue(gameAux.developerCombatResizeCommand, values[5], values[6]); return true;
    }
    return false;
}
