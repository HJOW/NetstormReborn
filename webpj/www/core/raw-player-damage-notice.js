// PATCH 10.78 의 현재 Player AI 조회와 피해 통지 래퍼(00416760)를 옮긴 모듈이다(cpppj `o/RawPlayerDamageNotice.cpp/.h`).
// 순서: challenge 모드이거나 owner<1 이면 반환 → owner 가 9 이상이면 RangeError → Player(0xb4 폭)+0xb0 의 AI 토큰(opaque32)을 읽어 0 이면 반환 → AI 소비자(00415460)가 없으면 Error → ai(토큰, 피해자 SID, 공격자 SID).
// 이 래퍼는 Player 기록을 쓰지 않는다. AI 주소는 호스트 포인터가 아닌 opaque32 이며 CD 의 AI 피해 몸체와 경계가 달라 10.78 전용이다. 모드는 호출마다 다시 읽는다.
// (2026-11-08 WB04-E8: Player 피해 통지 래퍼 이식으로 추가)

// 같은 월드의 9×0xb4 Player 바이트와 현재 모드를 빌려 통지를 전달하는 처리기이다. 사용: new RawPlayerDamageNotice(players, mode, (token, victim, attacker) => ...). (2026-11-08 WB04-E8)
export class RawPlayerDamageNotice {
    #players;
    #mode;
    #ai;

    // 실제 9기록의 폭을 확인하며 합성 AI 목록을 만들지 않는다. ai 는 비영 AI 토큰이 있을 때만 필수이다.
    constructor(players, mode, ai = null) {
        if (!(players instanceof Uint8Array) || players.length !== 9 * 0xb4) throw new RangeError('피해 통지 Player9 바이트 크기 오류');
        this.#players = players;
        this.#mode = mode;
        this.#ai = ai;
    }

    // challenge 또는 owner<1 또는 현재 AI 가 0 이면 원본대로 반환한다. owner 는 signed 32비트이다.
    notice(owner, victim, attacker) {
        const signed = owner | 0;
        if (this.#mode.challenge || signed < 1) return;
        if (signed >= 9) throw new RangeError('피해 통지 Player 번호가 실제 9기록 범위 밖임');
        const offset = signed * 0xb4 + 0xb0;
        // 비정렬 opaque32 AI 주소의 little endian 비트를 현재 네 BYTE 에서 읽는다.
        const token = (this.#players[offset] | (this.#players[offset + 1] << 8) | (this.#players[offset + 2] << 16) | (this.#players[offset + 3] << 24)) >>> 0;
        if (!token) return;
        if (typeof this.#ai !== 'function') throw new Error('실제 Player AI 피해 반응 00415460 미연결');
        this.#ai(token, victim, attacker);
    }
}
