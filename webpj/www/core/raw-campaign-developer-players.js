// 31h의 Player·동맹 진단 명령을 현재 9개 Player 기록과 추적 장치에 연결한다.
// 사용: executeDeveloperPlayers(text, players, trace) → 처리한 명령이면 true, 다른 명령이면 false.
import { firstDeveloperInteger, parseDeveloperCommand } from './raw-campaign-developer-settings.js';

// 현재 Player 기록의 비정렬 DWORD를 물리 경계 확인 후 읽는다.
function field(players, owner, offset) {
    const at = owner * 180 + offset;
    if (!(players instanceof Uint8Array) || at < 0 || at + 4 > players.length) throw new RangeError('개발자 Player 기록 물리 범위');
    return new DataView(players.buffer, players.byteOffset + at, 4).getUint32(0, true);
}

// 현재 Player 이름의 첫 NUL 앞 바이트를 원문 로그 문자열로 읽는다.
function playerName(players, owner) {
    if (!Number.isInteger(owner) || owner < 0 || owner >= 9) throw new RangeError('개발자 Player 이름의 물리 범위');
    const start = owner * 180 + 12;
    if (!(players instanceof Uint8Array) || start + 32 > players.length) throw new RangeError('개발자 Player 이름의 물리 범위');
    const bytes = players.subarray(start, start + 32), end = bytes.indexOf(0);
    if (end < 0) throw new RangeError('개발자 Player 이름의 NUL 부족');
    return String.fromCharCode(...bytes.subarray(0, end));
}

// Player 1~8과 현재 동맹 비트의 원본 수준29 출력을 수행한다. (2026-11-18 WB04-E741)
export function executeDeveloperPlayers(text, players, trace) {
    const { key, rest } = parseDeveloperCommand(text);
    // 실제 줄을 출력하는 위치에서만 추적 장치를 요구한다.
    const log = (message) => { if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결'); trace(29, message); };
    if (key === 'dumpplayers' || key === 'dp') {
        // 각 Player의 현재 플래그와 이름을 출력 직전에 읽는다.
        for (let owner = 1; owner < 9; owner += 1) {
            const flags = field(players, owner, 0x50);
            if (flags & 0x22) log(`id:${owner} ${playerName(players, owner)} ${(flags & 1) ? '' : 'Not'}Logged in\n`);
        }
        return true;
    }
    if (key === 'dumpally' || key === 'dally') {
        const value = firstDeveloperInteger(rest);
        if (value === null) return true;
        const owner = value >>> 0;
        log(`Ally bits for ${value} ${playerName(players, owner)}\n`);
        // 앞선 출력이 변경한 현재 동맹 비트와 대상 이름을 매번 읽는다.
        for (let target = 1; target < 9; target += 1) {
            if (field(players, owner, 0x54) & (1 << target)) log(` ${target} ${playerName(players, target)}\n`);
        }
        return true;
    }
    return false;
}
