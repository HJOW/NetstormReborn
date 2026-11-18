// 일반 전투의 참가자·동맹·사제에서 종료 관문을 조회한다(cpppj RawBattleCompletion, 10.78).
// 사용: new RawBattleCompletion({ players, options, alliances, battle, missionPost, boss, scriptBreak, loginEnd }, hooks).
// 전역 값은 각 조회 시점의 속성이며 hooks.currentPlayers가 있으면 같은 현재 Player 저장소를 다시 읽는다.

// 선택한 분기에서 필요한 실제 장치가 없을 때만 오류를 낸다.
function need(device) {
    if (typeof device !== 'function') throw new Error('일반 전투 종료의 실제 조회 미연결');
    return device;
}

// 9개 Player, 87BYTE 옵션, 방향별 동맹의 현재 물리를 빌리는 전투 종료 판정기다.
export class RawBattleCompletion {
    // 실제 전역과 조회 장치를 보관하며 내용을 복제하지 않는다.
    constructor(globals, hooks = {}) {
        if (!(globals.players instanceof Uint8Array) || globals.players.length !== 9 * 180 || !(globals.options instanceof Uint8Array) || globals.options.length !== 87 || !(globals.alliances instanceof Uint32Array) || globals.alliances.length !== 81) throw new TypeError('일반 전투 종료의 실제9 Player/동맹/옵션 누락');
        this.globals = globals; this.hooks = hooks;
    }

    // 효과 뒤에도 현재 Player 저장소를 조회해 정렬되지 않은 DWORD를 읽는다.
    read(owner, offset) {
        const players = this.hooks.currentPlayers ? this.hooks.currentPlayers() : this.globals.players;
        if (!(players instanceof Uint8Array) || players.length !== 9 * 180 || !Number.isInteger(owner) || owner < 0 || owner >= 9 || !Number.isInteger(offset) || offset < 0 || offset > 176) throw new RangeError('전투 종료의 현재 Player DWORD 물리 범위');
        return new DataView(players.buffer, players.byteOffset, players.byteLength).getUint32(owner * 180 + offset, true);
    }

    // 두 방향의 동맹 표가 모두 참인지 확인한다.
    allies(first, second) { const table = this.globals.alliances; return table[second * 9 + first] !== 0 && table[first * 9 + second] !== 0; }

    // 참가자와 비관전자가 둘 이상이며 모두 대표와 상호 동맹인지 확인한다.
    allAllied() {
        let subscribers = 0, nonWatching = 0, representative = 0;
        // 현재 subscriber가 있는 행만 세고 관전자는 대표에서 제외한다.
        for (let owner = 1; owner < 9; owner += 1) if (this.read(owner, 0x2c)) {
            subscribers += 1; if (this.globals.options[0x3e + owner]) continue; nonWatching += 1;
            if (!representative) representative = owner; else if (!this.allies(owner, representative)) return false;
        }
        return subscribers >= 2 && nonWatching > 1;
    }

    // 살아 있는 비관전자 팀이 하나 이하일 때 일반 전투의 팀 종료를 판정한다.
    teamFinished() {
        const g = this.globals, h = this.hooks;
        if (!g.battle || g.missionPost || g.scriptBreak || !need(h.serverConnection)()) return false;
        if (this.allAllied()) return true;
        let subscribers = 0, representative = 0, enemy = 0;
        // 관전자 여부보다 먼저 사제의 현재 생존 상태를 조회한다.
        for (let owner = 1; owner < 9; owner += 1) if (this.read(owner, 0x2c)) {
            subscribers += 1;
            if (!need(h.priestDead)(owner) && !g.options[0x3e + owner]) {
                if (!representative) representative = owner; else if (!this.allies(owner, representative)) enemy = owner;
            }
        }
        return subscribers > 1 && (!representative || !enemy);
    }

    // 로그인된 생존 참가자의 기권 비트와 boss·모드로 기권 종료를 판정한다.
    flagFinished(mode) {
        const g = this.globals, h = this.hooks;
        if (!g.battle || g.missionPost || g.scriptBreak || !need(h.serverConnection)() || g.loginEnd !== 0) return false;
        let all = true, flagged = false, logged = 0;
        // 사제 조회의 부수 효과 뒤 같은 행의 기권 비트를 다시 읽는다.
        for (let owner = 1; owner < 9; owner += 1) if (this.read(owner, 0x2c) && (this.read(owner, 0x50) & 1)) {
            logged += 1;
            if (!need(h.priestDead)(owner)) { if (this.read(owner, 0x50) & 0x80) flagged = true; else all = false; }
        }
        if (g.boss && mode === 0 && all) return true;
        if (flagged && all && logged > 0) { if (g.boss && mode !== 0) return logged === 1; return logged > 1; }
        return false;
    }

    // 팀 종료가 참이면 기권 판정과 그 외부 조회를 생략한다.
    finished() { return this.teamFinished() || this.flagFinished(1); }

    // 로컬 사제가 죽은 boss의 미접속·기권·관전자 집계를 계산한다.
    unloggedFinished(localOwner) {
        const g = this.globals, h = this.hooks;
        if (!need(h.serverConnection)()) return false;
        if (!need(h.priestDead)(localOwner) || !g.boss) return false;
        let subscribers = 0, alive = 0, offlineFlagged = 0, offlineTeam = 0, nonWatching = 0, offlineNonWatching = 0;
        // 각 행과 팀 판정의 부수 효과 후 다음 현재 행을 읽는다.
        for (let owner = 1; owner < 9; owner += 1) if (this.read(owner, 0x2c)) {
            subscribers += 1; if (!g.options[62 + owner]) nonWatching += 1;
            if (!(this.read(owner, 0x50) & 1) && !g.options[62 + owner]) offlineNonWatching += 1;
            if (!need(h.priestDead)(owner)) { alive += 1; const flags = this.read(owner, 0x50); if (!(flags & 1) && (flags & 0x80)) offlineFlagged += 1; }
            if (this.teamFinished() && !(this.read(owner, 0x50) & 1)) offlineTeam += 1;
        }
        return subscribers > 1 && (offlineFlagged === alive || nonWatching === offlineNonWatching || offlineTeam === subscribers - 1);
    }
}
