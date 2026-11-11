// 단독 미션 상태 7~10/13/14 의 현재 블록 복사·확정·전송 수신 시점을 옮긴 모듈이다(cpppj `o/RawLocalMissionState.cpp/.h`, 10.78: 004b2bc0 Request, 004b88c0 Poll, 004b6dd0 Run 과 상태 7·8·9·10·13·15 의 작은 몸체).
// 사용: const state = new RawLocalMissionState({ old, phase, immediate, current, next }, globals, hooks); state.request(14); state.poll(); state.run(); state.enqueueReceive(() => {...}); state.dispatchQueued();
//   블록 = 정확히 36DWORD(0x90BYTE) `Uint32Array` 다섯 개이며 서로 겹치지 않는 실제 저장소이고 첫 DWORD 만 상태 번호다: old(005c8630: 마지막 확정 때의 이전 phase)·phase(005c8758: 마지막으로 공통부가 확정한 상태)·immediate(005c86c8: 같은 Run 에서 이어서 처리할 요청)·current(005c87e8: 실행 중인 명령)·next(005c8878: 다음 Poll 이 소비할 명령).
//   immediate+0x84 의 DWORD 는 원본 큰 진행창 전역(005c874c `menuWindow`)과 같은 칸이라 globals 에 따로 두지 않고 이 칸을 직접 읽고 쓴다(별칭을 생성 시점에 보장).
//   globals = 실제 전역 묶음이며 속성으로 매번 현재값을 읽고 쓴다(사본 없음): server·boss·inBattle·missionPost·savedGame(00540bc0/bc4/00594fbc/fa4/005c85d4)·missionWorld·simulconnect·metamap(읽기 전용 00594fa0/0054db50/00594fc8)·player(PlayerWorldState: playerKnown 과 두 owner)·
//   zonePending(005c85c0)·gameTicks(0055b4ac)·sourceType(0055a420)·cdPresent(005c85b8, CD 확인 결과)·metaRefresh(005c85d8)·metaRebuild(005c85dc)·titleInitial(00565e70 첫 BYTE)·battleByte(005c85d0, 읽기 전용)·connectEnd(005c85f8 벽시각 예약)·activityEnd(00544d58 벽시각 예약)·gameSeconds(0055b4d0)·wallCached(0055b4d8).
//   hooks = 큰 외부 몸체뿐이다(진행 창·연결·CD·Renderer·Fort 파일 등). 선택하지 않은 경계는 부르지 않고 도달한 경계에 몸체가 없으면 앞선 원문 쓰기만 남긴 채 명시 거부한다(가짜 성공을 만들지 않음). 송신 hook 은 수신기를 `enqueueReceive` 로 예약해야 하며 공통 확정 뒤 `dispatchQueued` 에서만 실행된다.
// 기준선 뒤 cpppj 변경(7f1ae00ee·14369776e·9986097ae·e064f1752·c1c8bd0ad·6bff48b8b·eb680d67d·23b16b54f, 3.6절 ②): 일반 시작 밀도/listener/연결 안내(State7), 일반 복귀 State6 의 큰 부모, State15 서버 인수와 State16 직행, 재접속 Fort 읽기와 진행 창, 메타 State19~42 의 큰 부모와 메타 Poll 선행 효과를 현재 소스대로 옮겼다.
//   웹은 10.78 만 옮기므로 판본 인자가 없고 CD(10.72)는 옮기지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E320: 4-C RawLocalMissionState 이식)
import { resetAmbientSoundDeadline } from './raw-ambient-sound.js';

// 상태 블록 하나의 DWORD 수(0x90BYTE)이며 첫 DWORD 만 상태 번호이다. (2026-11-11 WB04-E320)
const BLOCK_DWORDS = 36;
// 블록 안의 DWORD 위치이다: +0x54 연결 주소 20BYTE(5DWORD) 시작, +0x7c 연결 값, +0x80 Zone 핸들, +0x84 재접속 표식(= 큰 진행창 전역), +0x88 재접속 SP(int32). (2026-11-11 WB04-E320)
const ADDRESS_BYTES = 0x54;
const CONNECT_VALUE = 0x7c / 4;
const ZONE_HANDLE = 0x80 / 4;
const RECONNECT_MARK = 0x84 / 4;
const RECONNECT_SP = 0x88 / 4;
// 연결 주소 기록 전체 크기(BYTE)와 Fort Core 의 최소 크기이다. (2026-11-11 WB04-E320)
const ADDRESS_SIZE = 20;
const FORT_CORE_MIN = 24;
// 연결 시도 제한(초)이다. (2026-11-11 WB04-E320)
const CONNECT_TIMEOUT = 30.0;

// 큰 부모 상태가 돌려주는 실제 반환 종류이다: Commit 은 공통 확정을 하고 ReturnWithoutCommit 은 원문의 early return 이다. (2026-11-11 WB04-E320)
export const LocalMissionParentResult = Object.freeze({ Commit: 'Commit', ReturnWithoutCommit: 'ReturnWithoutCommit' });

// 큰 진행 창의 문구 선택이다. 실제 표시·번역·창 수명은 필수 소비자(hooks.progress)가 처리한다. (2026-11-11 WB04-E320)
export const LocalMissionProgress = Object.freeze({
    // 기존 미션 초기화·재접속·초 단위 대기·서버 상실 안내이다.
    MissionStarting: 'MissionStarting',
    ReconnectStarting: 'ReconnectStarting',
    ConnectingCountdown: 'ConnectingCountdown',
    ReconnectCountdown: 'ReconnectCountdown',
    ServerLost: 'ServerLost',
    // 일반 State7/8 과 로그인 송신 뒤 실제 전송 안내이다.
    ConnectingStarting: 'ConnectingStarting',
    Connecting: 'Connecting',
    Reconnecting: 'Reconnecting',
    Transferring: 'Transferring',
    ReconnectTransferring: 'ReconnectTransferring',
    MissionTransferring: 'MissionTransferring',
});

// 큰 메뉴 부모(공통 phase 확정 전에 실행되는 전체 부모)로 보내는 상태 번호이다: 1/2/3/6 과 메타 19~42. (2026-11-11 WB04-E320)
const PARENT_STATES = new Set([1, 2, 3, 6]);
// 메타 19~42 전체가 같은 큰 부모를 쓴다.
for (let meta = 19; meta <= 42; meta += 1) PARENT_STATES.add(meta);

// 선택된 실제 소비자가 없으면 앞선 원문 쓰기만 보존하고 중단한다. (2026-11-11 WB04-E320)
function need(hook, text) {
    if (typeof hook !== 'function') throw new Error(text);
    return hook;
}

// CRT _ftol 의 64비트 절삭값 하위 DWORD 이며 비유한/64비트 범위 밖은 하위 0 이다. (2026-11-11 WB04-E320)
function truncateDword(value) {
    if (!Number.isFinite(value) || value >= 2 ** 63 || value < -(2 ** 63)) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 정렬 가정 없이 원문 바이트를 지정 폭(little endian)으로 읽는다. (2026-11-11 WB04-E320)
function readBytes(bytes, offset, width) {
    let result = 0;
    // 낮은 바이트부터 조합해 주변 필드를 섞지 않는다.
    for (let index = 0; index < width; index += 1) result += bytes[offset + index] * 2 ** (index * 8);
    return result;
}

// 해당 폭만 수정해 같은 Template 의 다른 필드를 보존한다. (2026-11-11 WB04-E320)
function writeBytes(bytes, offset, value, width) {
    // 낮은 바이트부터 지정 폭만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = Math.floor(value / 2 ** (index * 8)) & 0xff;
}

// 7 이 정리 전에 보관하는 원문 지역 값이다: Zone 대기값과 재접속일 때만 보관하는 게임 시각/출처이다. (2026-11-11 WB04-E320)
export function captureLocalMissionStarting(current, globals) {
    // 기본 분모 선택 뒤: Zone 캡처 → 제목 첫 BYTE 0 → 조건부 시각/출처 보관 순서이다.
    const saved = { zonePending: globals.zonePending >>> 0, gameTicks: 0, sourceType: 0 };
    globals.titleInitial = 0;
    if (current[RECONNECT_MARK]) {
        saved.gameTicks = globals.gameTicks >>> 0;
        saved.sourceType = globals.sourceType >>> 0;
    }
    return saved;
}

// 정리/CD/명령 숨김 뒤 현재 재접속 표식을 다시 조회해 시각 → 출처 순서로 복원한다(최초 조건을 따로 캐시하지 않음). (2026-11-11 WB04-E320)
export function restoreLocalMissionStarting(current, saved, globals) {
    if (current[RECONNECT_MARK]) {
        globals.gameTicks = saved.gameTicks;
        globals.sourceType = saved.sourceType;
    }
}

// 실제 상태 확정과 비동기 수신 경계를 같은 수명으로 소유한다. (2026-11-11 WB04-E320)
export class RawLocalMissionState {
    // 실제 상태 블록 다섯과 전역/소비자를 빌린다. 이 모듈보다 오래 살아야 하며 블록이 36DWORD 가 아니거나 서로 겹치면 거부한다. (2026-11-11 WB04-E320)
    constructor(state, globals, hooks = {}) {
        const blocks = [state?.old, state?.phase, state?.immediate, state?.current, state?.next];
        // 모든 블록이 정확한 36DWORD 실제 저장소인지 확인한다.
        for (const block of blocks) {
            if (!(block instanceof Uint32Array) || block.length !== BLOCK_DWORDS) throw new TypeError('로컬 미션 상태 블록은 36DWORD Uint32Array 여야 함');
        }
        // 모든 블록이 서로 다른(겹치지 않는) 현재 저장소인지 확인한다.
        for (let first = 0; first < blocks.length; first += 1) {
            for (let second = first + 1; second < blocks.length; second += 1) {
                const a = blocks[first];
                const b = blocks[second];
                if (a.buffer === b.buffer && a.byteOffset < b.byteOffset + b.byteLength && b.byteOffset < a.byteOffset + a.byteLength) throw new RangeError('로컬미션상태블록중첩');
            }
        }
        if (typeof globals !== 'object' || globals === null || typeof globals.player !== 'object' || globals.player === null) throw new TypeError('로컬 미션 상태 전역 묶음 오류');
        this.#state = state;
        this.#globals = globals;
        this.#hooks = hooks;
    }

    // 빌린 실제 상태 블록 다섯, 현재 전역 묶음, 외부 소비자 묶음이다(사본을 만들지 않음). (2026-11-11 WB04-E320)
    #state;
    #globals;
    #hooks;
    // Run 이 실행 중인지이다(재진입 오류 검출). (2026-11-11 WB04-E320)
    #running = false;
    // 송신 callback 이 예약한 실제 수신기 순서 큐이다. (2026-11-11 WB04-E320)
    #receives = [];

    // 004b2bc0: 현재 immediate 만 쓰고 phase 를 앞당기지 않는다. 이미 요청이 있으면 진단 뒤에도 원문처럼 새 요청으로 덮어쓴다. (2026-11-11 WB04-E320)
    request(state) {
        if (this.#state.immediate[0]) need(this.#hooks.assertion, '즉시상태중복진단미연결')();
        this.#state.immediate[0] = state;
    }

    // 004b88c0: 메타맵 선행 효과 뒤 next 가 있을 때만 current 로 복사하고 Run 한다. next 0 은 자연 반환이며 기존 current/phase 를 임의로 0 으로 만들지 않는다. (2026-11-11 WB04-E320)
    poll() {
        this.#pollBody(null);
    }

    // 실제 next 7 을 같은 Poll/Run 공통부로 소비한다. 최초 복사/로그는 몸체의 cleanup 보다 먼저이며 새 상태 번호를 합성하지 않는다. (2026-11-11 WB04-E320)
    pollStarting(body) {
        if (this.#state.next[0] !== 7 || typeof body !== 'function') throw new Error('실제 next7/부모 몸체 계약 오류');
        this.#pollBody(body);
    }

    // 메타 선행 효과가 바꾼 현재 next 도 같은 위치에서 다시 읽는다. (2026-11-11 WB04-E320)
    #pollBody(starting) {
        const globals = this.#globals;
        const hooks = this.#hooks;
        if (globals.metaRebuild && !globals.metaRefresh && globals.metamap) {
            need(hooks.metaFirst, '메타맵첫효과미연결')();
            globals.metaRebuild = 0;
        }
        if (globals.metaRefresh && globals.metamap) {
            need(hooks.metaSecond, '메타맵둘째효과미연결')();
            need(hooks.metaThird, '메타맵셋째효과미연결')();
            globals.metaRefresh = 0;
        }
        if (this.#state.next[0]) {
            this.#state.current.set(this.#state.next);
            this.#state.next[0] = 0;
            this.#runBody(starting);
        }
    }

    // 004b6dd0: 작은 선택 상태와 전체 0x90BYTE 공통 확정을 실행한다. callback 실행 중에는 phase 가 이전 상태이며 모든 선택 효과가 끝난 뒤 확정한다. (2026-11-11 WB04-E320)
    run() {
        this.#runBody(null);
    }

    // 현재 7 의 실제 호스트 몸체를 호출하고 같은 공통부가 7/즉시 8 을 확정한다. 몸체 자체에서 상태 확정/중첩 Run 을 하지 않으며 다른 현재 상태를 7 로 강제하지 않는다. (2026-11-11 WB04-E320)
    runStarting(body) {
        if (this.#state.current[0] !== 7 || typeof body !== 'function') throw new Error('실제 State7 몸체/현재 상태 계약 오류');
        this.#runBody(body);
    }

    // 일반 상태와 외부 State7 모두 동일 144BYTE 확정과 예외 중간 쓰기를 보존한다. (2026-11-11 WB04-E320)
    #runBody(starting) {
        if (this.#running) throw new Error('상태실행중동기수신재진입: 실제수신큐를사용해야함');
        const state = this.#state;
        this.#running = true;
        try {
            // 즉시 요청이 있을 때만 같은 호출에서 다음 상태를 다시 처리한다.
            for (;;) {
                state.next.set(state.current);
                state.immediate.set(state.current);
                state.next[0] = 0;
                state.immediate[0] = 0;
                need(this.#hooks.logState, '현재상태로그미연결')(state.current[0]);
                if (state.current[0] === 7 && starting) starting();
                else if (this.#step()) break;
                const again = state.immediate[0] !== 0;
                state.old.set(state.phase);
                state.phase.set(state.current);
                state.current[0] = 0;
                if (!again) break;
                state.current.set(state.immediate);
            }
        } finally {
            this.#running = false;
        }
    }

    // 상태 7: 정리 전 Zone/시각/출처를 보관하고 전투 정리·CD 확인·명령 숨김 뒤 복원하며 Zone/Renderer/진행 창/팔레트를 원문 위치에서 조회한다. 같은 몸체를 호스트 State7 과 독립 상태 분배기가 공유한다. (2026-11-11 WB04-E320)
    #starting() {
        const state = this.#state;
        const globals = this.#globals;
        const hooks = this.#hooks;
        need(hooks.selectDefaultDenominator, '기본 배경 분모 선택 미연결')();
        const saved = captureLocalMissionStarting(state.current, globals);
        need(hooks.cleanupBattle, '현재전투정리미연결')(1);
        globals.cdPresent = need(hooks.cdPresent, '현재CD확인미연결')() >>> 0;
        need(hooks.refreshCommands, '명령창갱신미연결')();
        restoreLocalMissionStarting(state.current, saved, globals);
        globals.inBattle = true;
        need(hooks.stopMusic, '음악정리미연결')();
        if (globals.missionWorld && globals.missionPost) need(hooks.refreshMission, '현재미션가상갱신미연결')();
        if (globals.simulconnect) need(hooks.listenServer, '동시 접속의 실제 listener 미연결')();
        if (saved.zonePending) {
            need(hooks.installZone, '현재Zone설치미연결')(state.current[ZONE_HANDLE]);
            need(hooks.freeZone, '현재Zone해제미연결')(state.current[ZONE_HANDLE]);
            state.current[ZONE_HANDLE] = 0;
            state.next[ZONE_HANDLE] = 0;
            need(hooks.installMapHandlers, '실제맵처리기등록미연결')();
        }
        need(hooks.initializeRenderer, '현재Renderer초기화미연결')();
        globals.player.playerKnown = 0;
        need(hooks.uiMode, '현재UI모드미연결')(2);
        // 주변음 기한을 현재 벽시각 + 10 으로 다시 예약한다(State/Edit 가 쓰는 같은 기한).
        resetAmbientSoundDeadline({ set deadline(value) { globals.activityEnd = value; } }, hooks.wallSeconds);
        state.immediate[1] = 20;
        this.request(8);
        if (state.current[RECONNECT_MARK]) {
            globals.savedGame = true;
            need(hooks.loadReconnectFort, '재접속Fort읽기미연결')();
            need(hooks.progress, '재접속진행창미연결')(LocalMissionProgress.ReconnectStarting, 0);
        } else if (globals.missionPost) {
            need(hooks.progress, '미션진행창미연결')(LocalMissionProgress.MissionStarting, 0);
        } else {
            if (globals.server) need(hooks.initializeDensity, '일반 전투의 실제 밀도 writer 미연결')();
            need(hooks.clearChat, '일반 전투의 현재 Chat 채널 미연결')(0x40);
            need(hooks.clearChat, '일반 전투의 현재 Chat 채널 미연결')(0x80);
            need(hooks.progress, '일반 전투 연결 안내 미연결')(LocalMissionProgress.ConnectingStarting, 0);
        }
        const palette = need(hooks.battlePalette, '현재battlePal조회미연결')();
        need(hooks.applyPalette, '실제팔레트적용미연결')(palette);
    }

    // 상태 13: Template 의 작은 필드(Core 의 모드 비트·주소·포트·게임 시각·전투 BYTE·갱신 횟수)는 직접 쓰고 큰 파일/Builder/UI 몸체는 필수 소비자로 남긴다. (2026-11-11 WB04-E320)
    #connected() {
        const state = this.#state;
        const globals = this.#globals;
        const hooks = this.#hooks;
        globals.connectEnd = 0.0;
        need(hooks.closeProgress, '현재진행창전체해제미연결')();
        if (!globals.server) {
            const connection = need(hooks.serverConnection, '현재서버연결객체미연결')();
            need(hooks.attachCommander, '실제Commander연결미연결')(connection);
        }
        if (!need(hooks.fortAvailable, '현재Fortwrapper조회미연결')(globals.player.templateOwner)) need(hooks.loadCurrentFort, '현재Fort파일읽기미연결')();
        const core = need(hooks.fortCore, '현재FortCore조회미연결')(globals.player.templateOwner);
        if (!(core instanceof Uint8Array) || core.length < FORT_CORE_MIN) throw new RangeError('현재FortCore24BYTE누락');
        core[0] = (core[0] & 0xfb) | 2;
        const address = need(hooks.localAddress, '현재로컬연결주소미연결')();
        if (!(address instanceof Uint8Array) || address.length !== ADDRESS_SIZE) throw new RangeError('현재로컬연결주소20BYTE오류');
        writeBytes(core, 4, readBytes(address, 4, 4), 4);
        // 주소 종류 6 이면 12번, 아니면 2번 바이트의 포트를 읽어 네트워크 바이트 순서로 쓴다.
        const port = readBytes(address, readBytes(address, 0, 2) === 6 ? 12 : 2, 2);
        writeBytes(core, 8, ((port & 255) << 8) | (port >>> 8), 2);
        writeBytes(core, 12, truncateDword(globals.gameSeconds), 4);
        const battleByte = globals.battleByte;
        writeBytes(core, 0x16, readBytes(core, 0x16, 2) + 1, 2);
        core[0x10] = battleByte;
        need(hooks.touchFort, '현재Fort갱신미연결')(globals.player.templateOwner);
        need(hooks.uiMode, '현재UI모드미연결')(3);
        need(hooks.refreshFort, '실제Fort설정갱신미연결')();
        state.immediate[RECONNECT_MARK] = 0;
        state.next[RECONNECT_MARK] = 0;
        this.request(14);
        if (state.current[RECONNECT_MARK]) need(hooks.setReconnectSp, '실제재접속SP지정미연결')(Math.fround(state.current[RECONNECT_SP] | 0));
        if (!need(hooks.connectionOpen, '현재서버연결조회미연결')(9)) {
            globals.sourceType = 0;
            state.next[0] = 3;
        } else {
            need(hooks.loginTimer, '실제로그인타이머시작미연결')(true);
            need(hooks.inputMode, '실제입력모드전환미연결')(false);
        }
    }

    // 선택 상태의 일반 반환은 공통부로 돌아가고 원문 timeout 만 early return 한다(true 이면 공통 확정 없이 반환). (2026-11-11 WB04-E320)
    #step() {
        const state = this.#state;
        const globals = this.#globals;
        const hooks = this.#hooks;
        const selected = state.current[0];
        switch (selected) {
            case 7:
                this.#starting();
                break;
            case 8:
                // 연결 만료가 아직 예약되지 않았으면 현재 벽시각 + 30 으로 예약하고 일반 전투라면 연결/재접속 안내를 띄운다.
                if (globals.connectEnd === 0.0) {
                    const now = need(hooks.wallSeconds, '연결현재벽시각미연결')();
                    globals.connectEnd = now + CONNECT_TIMEOUT;
                    if (!globals.missionPost) need(hooks.progress, '일반 연결/재접속 안내 미연결')(globals.savedGame ? LocalMissionProgress.Reconnecting : LocalMissionProgress.Connecting, 0);
                }
                need(hooks.connectServer, '실제서버연결미연결')(new Uint8Array(state.current.buffer, state.current.byteOffset + ADDRESS_BYTES, ADDRESS_SIZE), state.current[CONNECT_VALUE]);
                state.next[0] = 9;
                break;
            case 9: {
                let connected = need(hooks.connectionStatus, '현재연결상태미연결')(9) | 0;
                if (connected < 1 && !globals.server) {
                    // 비서버가 pending(-1) 연결을 갖고 있으면 그 연결을 9 번으로 옮겨 연결된 것으로 본다.
                    if ((need(hooks.connectionStatus, '현재pending연결미연결')(-1) | 0) !== 0) {
                        const status = hooks.connectionStatus(-1) | 0;
                        need(hooks.logConnection, '연결전환진단미연결')(0x401, status);
                        need(hooks.closeConnection, '실제연결해제미연결')(9);
                        need(hooks.renameConnection, '실제연결번호전환미연결')(-1, 9);
                        connected = 1;
                    }
                }
                const now = need(hooks.wallSeconds, '연결만료벽시각미연결')();
                if (globals.connectEnd <= now) {
                    globals.sourceType = 0;
                    state.next[0] = 3;
                    return true;
                }
                const open = need(hooks.connectionOpen, '현재서버연결조회미연결')(9);
                if (!open || connected < 0) {
                    // 연결이 닫혔거나 오류이면 현재 열림 여부를 다시 조회해 진단하고 닫은 뒤 다시 8 을 요청한다.
                    const currentOpen = hooks.connectionOpen(9);
                    need(hooks.logConnection, '연결실패진단미연결')(0x40f, currentOpen ? 1 : 0);
                    need(hooks.closeConnection, '실제연결해제미연결')(9);
                    this.request(8);
                } else if (connected < 1) {
                    // 아직 연결 중이면 남은 초(현재 캐시 시각 기준)를 진행 창에 보이고 9 를 유지한다.
                    const remaining = truncateDword(globals.connectEnd - globals.wallCached) | 0;
                    need(hooks.progress, '실제연결대기창미연결')(globals.savedGame ? LocalMissionProgress.ReconnectCountdown : LocalMissionProgress.ConnectingCountdown, remaining);
                    state.next[0] = 9;
                } else {
                    // 연결되었으면 로그인을 송신하고 진행 창을 바꾼 뒤 10 을 요청한다.
                    need(hooks.sendLogin, '실제로그인송신미연결')();
                    need(hooks.connectedProgress, '로그인전송진행창미연결')();
                    this.request(10);
                }
                break;
            }
            case 10:
                if (!need(hooks.connectionOpen, '현재서버연결조회미연결')(9)) {
                    need(hooks.progress, '실제연결실패창미연결')(LocalMissionProgress.ServerLost, 0);
                    globals.sourceType = 0;
                    state.next[0] = 3;
                } else {
                    state.next[0] = 10;
                }
                break;
            case 13:
                this.#connected();
                break;
            case 15:
                need(hooks.takeServer, 'State15 실제 서버 인수 부모 미연결')();
                break;
            case 4:
            case 12:
            case 14:
            case 16:
                // 004b874c 의 State4 와 서버 인수 뒤 16 도 004b86bd 공통 확정으로 직행하며 다음 Poll 에서만 소비한다.
                break;
            default:
                if (PARENT_STATES.has(selected)) return need(hooks.parentState, '큰메뉴부모상태미연결')(selected) === LocalMissionParentResult.ReturnWithoutCommit;
                need(hooks.invalidState, '미복원상태진단미연결')(selected);
                break;
        }
        return false;
    }

    // 송신 callback 은 실제 수신기를 이 큐에 넣고 원문 공통 확정까지 반환해야 한다(즉시 실행하지 않음). (2026-11-11 WB04-E320)
    enqueueReceive(receive) {
        if (typeof receive !== 'function') throw new TypeError('빈로컬수신예약');
        this.#receives.push(receive);
    }

    // 상태 실행 중 분배를 명시 거부하여 reply 0x28 이 phase 10 확정 이전에 도착하지 않게 하고, 현재 큐의 실제 수신기를 순서대로 실행한다. (2026-11-11 WB04-E320)
    dispatchQueued() {
        if (this.#running) throw new Error('상태확정전수신분배금지');
        // 수신기가 새 예약을 추가해도 같은 호출에서 순서대로 모두 실행한다.
        while (this.#receives.length > 0) {
            const receive = this.#receives.shift();
            receive();
        }
    }
}
