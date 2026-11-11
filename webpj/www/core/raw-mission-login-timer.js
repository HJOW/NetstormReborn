// PATCH 로그인 타이머의 같은 Player9 집계·미션 종료 선택·Begin/Abort 순서와 일반 로그인 진행 문구를 옮긴 모듈이다(cpppj `o/RawMissionLoginTimer.cpp/.h`, 10.78: 004b67c0 Run, 004180a0 Expired, 004b61f0 Begin, 004b6780 Abort, 004b68d6..004b6b95 진행 문구, 집계 00490970·00490a00·00490ab0·00490b20·00490bc0·00490c50·00490cf0).
// 사용: const counts = new MissionLoginPlayerCounts(players); const timer = new RawMissionLoginTimer(state, counts, reads, hooks); timer.run(true|false); timer.begin(); timer.abort(); timer.progressText(frameWall, translate);
//   state = 기존 게임 상태의 실제 저장소(속성으로 읽고 쓰는 객체): end(005c85f0 의 실제 벽시각 예약 double)·timeout(005c8938 signed)·begin(005c7a74)·scriptBreak(00542544)·requestPhase(005c85ac)·clock(00594fb4).
//   reads = 원문이 분기마다 다시 읽는 현재 모드 함수: savedGame(005c85d4)·server(00540bc0)·mission(00594fa0)·missionPost(00594fa4)·initialGeysers(00540bc4)·checkingAssertions(005e4794). bool 을 DWORD 참조로 재해석하지 않는다.
//   hooks = 실제 시간/전송/사제/UI 소비자만 외부 경계: configInt·wallSeconds·clock·connection·remoteStatus·broadcast·priestRefresh·saveReconnect·closeProgress·resetCamera·abortBattle·progress·serverAssertion·log(kind, text)·initializeGeneral·createInitialGeysers. 선택하지 않은 경계는 호출하지 않고 가짜 성공으로 채우지 않는다.
// 집계(`MissionLoginPlayerCounts`)는 로그인/타이머가 공유하는 같은 Player9 의 owner 1..8 subscriber(+0x2c)/flags(+0x50)만 읽고 사본을 두지 않는다.
// 기준선 뒤 cpppj 변경(a60d547f0·25e32b7a9, 3.6절 ②): 일반 로그인 완료 초기화(initializeGeneral/createInitialGeysers)와 일반 대기 화면/강제 시작 진행 문구(progressText)를 반영했다. CD(10.72)는 옮기지 않았다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E318: 4-C RawMissionLoginTimer 이식)

// 같은 실제 Player9 의 물리 폭/개수이다. (2026-11-11 WB04-E318)
const PLAYER_STRIDE = 0xb4;
const PLAYER_COUNT = 9;

// 실제 Begin 8BYTE(version 1 / type 0x6e / length 8 / payload 0x80000000)이다. 송신 장치마다 새 사본을 준다. (2026-11-11 WB04-E318)
const BEGIN_PACKET = [1, 0x6e, 8, 0, 0, 0, 0, 0x80];

// 서로 다른 원본 로그 몸체를 같은 성공 효과로 합치지 않도록 호출 종류를 전달한다: 0045ad00 시작 진입과 004a68e0 사제/카메라 진행 로그. (2026-11-11 WB04-E318)
export const MISSION_LOGIN_TIMER_LOG = Object.freeze({ TimerStart: 0, Status: 1 });

// 실제로 선택된 경계만 확인한다. inactive 타이머에서 사용하지 않는 효과를 호출하거나 성공으로 채우지 않는다. (2026-11-11 WB04-E318)
function need(device, message) {
    if (typeof device !== 'function') throw new Error(message);
    return device;
}

// 원본 로그인이 읽는 1..8 의 현재 subscriber/flags 만 읽는 집계이다. 로그인/타이머가 같은 실제 9 Player 를 공유한다. (2026-11-11 WB04-E318)
export class MissionLoginPlayerCounts {
    // 사용: 정확한 9×0xb4 바이트를 빌린다. 저장소는 모든 로그인/타이머보다 오래 살아야 하며 다른 크기는 정상 입력으로 간주하지 않는다. (2026-11-11 WB04-E318)
    constructor(players) {
        if (!(players instanceof Uint8Array) || players.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('로그인 집계의 실제 PATCH9 Player 기록 누락');
        this.players = players;
    }

    // 원본 DWORD 를 정렬 가정 없이 낮은 BYTE 부터 읽는다. (2026-11-11 WB04-E318)
    read(owner, offset) {
        const base = owner * PLAYER_STRIDE + offset;
        return (this.players[base] | (this.players[base + 1] << 8) | (this.players[base + 2] << 16) | (this.players[base + 3] << 24)) >>> 0;
    }

    // owner 1..8 중 조건을 만족하는 기록 수를 센다(현재 값을 매번 읽고 Player 0 은 방문하지 않음). (2026-11-11 WB04-E318)
    count(predicate) {
        let count = 0;
        // 현재 1..8 만 방문한다.
        for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
            if (predicate(this.read(owner, 0x2c), this.read(owner, 0x50))) count += 1;
        }
        return count;
    }

    // 00490ab0: subscriber 가 0 이 아닌 기록 수이며 flags 를 요구하지 않는다. (2026-11-11 WB04-E318)
    subscribers() {
        return this.count((subscriber) => subscriber !== 0);
    }

    // 00490a00: subscriber 가 있고 flags 2(사람 참여)인 기록 수이다(로그인된 것과 별개). (2026-11-11 WB04-E318)
    participating() {
        return this.count((subscriber, flags) => subscriber !== 0 && (flags & 2) !== 0);
    }

    // 00490bc0: subscriber 와 flags 2 와 flags 0x100 이 모두 있을 때만 센다(100 만 있는 기록을 참여로 추정하지 않음). (2026-11-11 WB04-E318)
    disconnected() {
        return this.count((subscriber, flags) => subscriber !== 0 && (flags & 2) !== 0 && (flags & 0x100) !== 0);
    }

    // 00490b20: subscriber, flags 2, flags 1, flags 0x40 이 모두 있어야 센다. (2026-11-11 WB04-E318)
    ready() {
        return this.count((subscriber, flags) => subscriber !== 0 && (flags & 2) !== 0 && (flags & 1) !== 0 && (flags & 0x40) !== 0);
    }

    // 00490970: subscriber, flags 2, flags 1(로그인)이며 Ready 와 별개이다. (2026-11-11 WB04-E318)
    loggedIn() {
        return this.count((subscriber, flags) => subscriber !== 0 && (flags & 2) !== 0 && (flags & 1) !== 0);
    }

    // 00490c50: subscriber 와 flags 2 가 있고 flags 1 과 0x100 이 모두 없는(미로그인/미단절 참여자) 기록 수이다. (2026-11-11 WB04-E318)
    pending() {
        return this.count((subscriber, flags) => subscriber !== 0 && (flags & 0x103) === 2);
    }

    // 00490cf0: subscriber 가 있고 flags 2 가 없는(참여 전 연결 중) 기록 수이다. (2026-11-11 WB04-E318)
    connecting() {
        return this.count((subscriber, flags) => subscriber !== 0 && (flags & 2) === 0);
    }

    // 진행 문구가 같은 현재 이름/flags 를 읽도록 실제 한 기록을 빌린다(사본이 아님). (2026-11-11 WB04-E318)
    record(owner) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= PLAYER_COUNT) throw new RangeError('로그인 진행 Player 범위');
        return this.players.subarray(owner * PLAYER_STRIDE, (owner + 1) * PLAYER_STRIDE);
    }
}

// C `%s`/`%d` 서식(플래그 - + 공백 # 0, 폭, 정밀도)을 번역이 바꾼 서식에도 그대로 적용한다. 호출자가 서식의 인자 종류를 먼저 검증한다. (2026-11-11 WB04-E318)
function formatPrintf(pattern, values) {
    let output = '';
    let argument = 0;
    // 서식 문자열을 앞에서부터 한 글자씩 소비한다.
    for (let at = 0; at < pattern.length; at += 1) {
        if (pattern[at] !== '%') {
            output += pattern[at];
            continue;
        }
        at += 1;
        if (pattern[at] === '%') {
            output += '%';
            continue;
        }
        let flags = '';
        while ('-+ #0'.includes(pattern[at]) && at < pattern.length) {
            flags += pattern[at];
            at += 1;
        }
        let width = '';
        while (at < pattern.length && pattern[at] >= '0' && pattern[at] <= '9') {
            width += pattern[at];
            at += 1;
        }
        let precision = null;
        if (pattern[at] === '.') {
            at += 1;
            precision = '';
            while (at < pattern.length && pattern[at] >= '0' && pattern[at] <= '9') {
                precision += pattern[at];
                at += 1;
            }
        }
        const value = values[argument];
        argument += 1;
        let text;
        if (pattern[at] === 's') {
            text = precision === null ? value : value.slice(0, Number(precision || 0));
            const pad = Math.max(Number(width || 0) - text.length, 0);
            output += flags.includes('-') ? text + ' '.repeat(pad) : ' '.repeat(pad) + text;
        } else {
            const negative = value < 0;
            let digits = String(Math.abs(value));
            if (precision !== null) digits = (Number(precision || 0) === 0 && value === 0) ? '' : digits.padStart(Number(precision || 0), '0');
            const sign = negative ? '-' : flags.includes('+') ? '+' : flags.includes(' ') ? ' ' : '';
            const pad = Math.max(Number(width || 0) - sign.length - digits.length, 0);
            if (flags.includes('-')) output += sign + digits + ' '.repeat(pad);
            else if (flags.includes('0') && precision === null) output += sign + '0'.repeat(pad) + digits;
            else output += ' '.repeat(pad) + sign + digits;
        }
    }
    return output;
}

// 실제 타이머/Begin/Abort 와 일반 진행 문구를 계산하고 선택된 효과만 실제 장치에 위임한다. (2026-11-11 WB04-E318)
export class RawMissionLoginTimer {
    // 사용: 모든 참조/현재 조회 대상은 타이머보다 오래 살아야 한다. 필수 모드 조회 여섯 개는 생성 때 확인한다. (2026-11-11 WB04-E318)
    constructor(state, players, reads, hooks = {}) {
        if (!(players instanceof MissionLoginPlayerCounts) || !state) throw new TypeError('로그인 타이머의 상태/집계 누락');
        for (const name of ['savedGame', 'server', 'mission', 'missionPost', 'initialGeysers', 'checkingAssertions']) {
            if (typeof reads?.[name] !== 'function') throw new TypeError('로그인 타이머의 PATCH 현재 모드 조회 누락');
        }
        this.state = state;
        this.players = players;
        this.reads = reads;
        this.hooks = hooks;
    }

    // 004180a0: end <= 현재 벽시각이다. 게임 시각/프레임 캐시를 대신 쓰지 않고 wall 효과 뒤 같은 end 를 다시 읽는다. (2026-11-11 WB04-E318)
    expired() {
        const now = need(this.hooks.wallSeconds, '로그인 실제 벽시각 미연결')();
        return this.state.end <= now;
    }

    // 004b61f0: (확인 중이고 서버가 아니면 진단) → end 0 → 실제 Begin 8BYTE 송신 → begin 1. 송신 효과 뒤의 begin 쓰기를 앞당기지 않는다. (2026-11-11 WB04-E318)
    begin() {
        if (this.reads.checkingAssertions() && !this.reads.server()) need(this.hooks.serverAssertion, '로그인 Begin 서버 진단 미연결')();
        this.state.end = 0;
        need(this.hooks.broadcast, '로그인 Begin 실제 송신 미연결')(Uint8Array.from(BEGIN_PACKET));
        this.state.begin = 1;
    }

    // 004b6780: begin 0 → end 0 → scriptBreak 1 → 요청 phase 4 → 실제 004b6350 → 진행 창 해제. phase 는 하위가 다시 바꿀 수 있으므로 마지막에 4 로 덮어쓰지 않는다. (2026-11-11 WB04-E318)
    abort() {
        const state = this.state;
        state.begin = 0;
        state.end = 0;
        state.scriptBreak = 1;
        state.requestPhase = 4;
        need(this.hooks.abortBattle, '로그인 Abort 실제 상태 정리 미연결')();
        need(this.hooks.closeProgress, '로그인 진행 창 해제 미연결')();
    }

    // 004b67c0: start 면 설정 timeout 과 현재 벽시각으로 예약하고, 아니면 현재 예약의 실제 선택 분기를 실행한다. missionPost 와 Begin 은 별개이며 미ready/미만료가 항상 진행 창 대기를 뜻하지 않는다. (2026-11-11 WB04-E318)
    run(start) {
        const state = this.state;
        const reads = this.reads;
        const hooks = this.hooks;
        if (start) {
            need(hooks.log, '로그인 시작 로그 미연결')(MISSION_LOGIN_TIMER_LOG.TimerStart, 'login status timer start\n');
            // 원본 stack local 의 선택된 정수값을 저장한 뒤 실제 wall 효과를 수행한다.
            const timeout = need(hooks.configInt, '로그인 현재 설정 미연결')('loginTimeout') | 0;
            state.timeout = timeout;
            const now = need(hooks.wallSeconds, '로그인 실제 벽시각 미연결')();
            state.end = timeout + now;
            return;
        }
        if (state.end === 0) return;
        if (!reads.savedGame()) state.clock = need(hooks.clock, '로그인 실제 CRT 시각 미연결')() >>> 0;
        // 연결 조회 효과가 서버 모드를 바꾸면 원문 재조회 결과로 서버 사슬을 선택한다.
        let serverBranch = true;
        if (!reads.server()) {
            if (!need(hooks.connection, '로그인 현재 서버 연결 조회 미연결')(9)) {
                this.abort();
                return;
            }
            if (!reads.server()) {
                need(hooks.remoteStatus, '로그인 실제 원격 상태 요청 미연결')(9, 0, 0);
                serverBranch = false;
            }
        }
        if (serverBranch) {
            // 미ready/미만료이면 Begin 만 생략하고 아래 실제 missionPost 종료 판정으로 간다.
            let begin = true;
            if (state.begin === 0) {
                const dropped = this.players.disconnected();
                const subscribers = this.players.subscribers();
                const ready = this.players.ready();
                if (((subscribers - dropped) >>> 0) !== ready) {
                    if (!this.expired()) begin = false;
                    else if (this.players.loggedIn() < 2 && !reads.savedGame()) {
                        this.abort();
                        return;
                    }
                }
            }
            if (begin) this.begin();
        }
        if (reads.missionPost() || state.begin !== 0) {
            if (!reads.savedGame()) {
                if (!reads.mission()) need(hooks.initializeGeneral, '로그인 일반 전투 초기화 부모 미연결')();
                if (reads.initialGeysers() && !reads.mission()) {
                    need(hooks.log, '일반 초기 간헐천 로그 미연결')(MISSION_LOGIN_TIMER_LOG.Status, 'Creating initial geysers\n');
                    need(hooks.createInitialGeysers, '일반 초기 간헐천 실제 부모 미연결')();
                    need(hooks.log, '일반 초기 간헐천 완료 로그 미연결')(MISSION_LOGIN_TIMER_LOG.Status, 'Done\n');
                }
            }
            need(hooks.log, '로그인 사제 상태 로그 미연결')(MISSION_LOGIN_TIMER_LOG.Status, 'Seting Priest State\n');
            // 같은 현재 Player/사제 상태의 실제 owner 1..8 을 원본 순서로 재조회한다.
            for (let owner = 1; owner < 9; owner += 1) need(hooks.priestRefresh, '로그인 실제 사제 상태 재설정 미연결')(owner);
            need(hooks.log, '로그인 상태 완료 로그 미연결')(MISSION_LOGIN_TIMER_LOG.Status, 'Done\n');
            state.begin = 0;
            state.scriptBreak = 0;
            need(hooks.saveReconnect, '로그인 실제 재접속 저장 경계 미연결')();
            state.end = 0;
            need(hooks.closeProgress, '로그인 진행 창 해제 미연결')();
            need(hooks.log, '로그인 카메라 로그 미연결')(MISSION_LOGIN_TIMER_LOG.Status, 'Moveing Camera\n');
            if (!reads.missionPost()) need(hooks.resetCamera, '로그인 실제 카메라 초기화 미연결')();
            need(hooks.log, '로그인 상태 완료 로그 미연결')(MISSION_LOGIN_TIMER_LOG.Status, 'Done\n');
            return;
        }
        need(hooks.progress, '로그인 일반 진행 창 꼬리004b68d6은 아직 미연결')();
    }

    // 004b68d6..004b6b95: 원문 cached wall(frameWall)과 현재 Player 집계, 번역된 네 인자 서식(제목 %s, 명단 %s, 남은 시간 %d, 추가 안내 %s)으로 진행 HTML 을 만든다. 번역이 미공급 인자를 읽게 하는 서식은 명시 거부하고 %% 와 상수 폭/정밀도는 유지하며 결과가 1024바이트 이상이면 거부한다. (2026-11-11 WB04-E318)
    progressText(frameWall, translate) {
        let list = '';
        // subscriber 가 있는 owner 1..8 의 이름 뒤 현재 flags 의 상태 문구와 LF 를 붙인다.
        for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
            const record = this.players.record(owner);
            const subscriber = (record[0x2c] | (record[0x2d] << 8) | (record[0x2e] << 16) | (record[0x2f] << 24)) >>> 0;
            if (!subscriber) continue;
            const name = record.subarray(12, 12 + 32);
            const end = name.indexOf(0);
            if (end < 0) throw new RangeError('로그인 진행 이름의 NUL 누락');
            list += String.fromCharCode(...name.subarray(0, end));
            const flags = (record[0x50] | (record[0x51] << 8) | (record[0x52] << 16) | (record[0x53] << 24)) >>> 0;
            const key = !(flags & 2) ? ': ~iConnecting~.' : !(flags & 1) ? (flags & 0x100 ? ': ~iDisconnected~.' : ': ~iPending~.') : !(flags & 0x40) ? ': ~iTransferring~.' : ': ~iReady~.';
            list += need(translate, '로그인 진행 번역 미연결')(key);
            list += '\n';
            if (list.length >= 1024) throw new RangeError('로그인 진행 명단1024BYTE 초과');
        }
        // PE 4b6a8b/93 은 현재 end 와 0055b4d8 의 차이를 x87 에 남긴다: 64비트 절삭의 하위 DWORD 를 signed 로 읽는다.
        const difference = this.state.end - frameWall;
        const bits = (!Number.isFinite(difference) || difference >= 2 ** 63 || difference < -(2 ** 63)) ? 0 : Number(BigInt.asUintN(32, BigInt(Math.trunc(difference))));
        let remaining = bits | 0;
        if (remaining < 0) remaining = 0;
        let extra = '';
        if (remaining < 1) extra = need(translate, '로그인 지연 번역 미연결')('\n\nThere may be Internet traffic, or the Server\nmay have a slow Internet connection which is\ntaking longer than expected. You can abort\nthis game, or continue waiting to get in. Most\ngames reconnect correctly even after long delays.\n$ButtonInstant=Abort,LeaveBattle,3\n');
        // 최초 비서버 선택은 제목을 다시 서버 조회로 바꾸지 않는다.
        let titleKey = 'Starting Game';
        if (this.reads.server()) {
            // 원문 bc0/ab0/c50/cf0/b20 순서를 유지한다.
            const disconnected = this.players.disconnected();
            const subscribers = this.players.subscribers();
            const pending = this.players.pending();
            const connecting = this.players.connecting();
            const ready = this.players.ready();
            if ((((subscribers - disconnected - pending - connecting) >>> 0) === ready) && (((this.state.timeout >>> 0) - (remaining >>> 0)) | 0) > 10) {
                extra = need(translate, '로그인 강제 시작 번역 미연결')('\n\nSome players are not connected and may\nnot make it back. Press Start to begin now.$ButtonInstant=Start,ServerForceBattleStart,0 ');
            }
            // 강제 시작 안내의 번역 효과 뒤에만 현재 서버를 다시 읽는다.
            if (this.reads.server()) titleKey = 'Becoming Server';
        }
        const title = need(translate, '로그인 제목 번역 미연결')(titleKey);
        const pattern = need(translate, '로그인 진행 서식 번역 미연결')('~.~3%s~*~.\n\n%s\nTime Remaining: ~i%d~.%s');
        let argument = 0;
        // CRT 에 미공급 인자를 읽게 하는 번역은 명시 거부하고 %% 와 상수 폭/정밀도는 유지한다.
        for (let at = 0; at < pattern.length; at += 1) {
            if (pattern[at] !== '%') continue;
            at += 1;
            if (at < pattern.length && pattern[at] === '%') continue;
            while (at < pattern.length && '-+ #0.0123456789'.includes(pattern[at])) at += 1;
            if (argument >= 4 || at === pattern.length || pattern[at] !== (argument === 2 ? 'd' : 's')) throw new TypeError('로그인 진행 번역의 네 인자 서식 오류');
            argument += 1;
        }
        if (argument !== 4) throw new TypeError('로그인 진행 번역의 인자 개수 오류');
        const output = formatPrintf(pattern, [title, list, remaining, extra]);
        if (output.length >= 1024) throw new RangeError('로그인 진행 HTML1024BYTE 초과');
        return output;
    }
}
