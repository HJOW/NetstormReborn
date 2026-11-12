// Client 수명의 단독 미션 State 다섯 블록과 로그인/기한/시간 동기화/Zone/메타 구역 전역을 모두 같은 주소로 보관하는 소유자를 옮긴 모듈이다(cpppj `client/ClientLocalStateGlobals.h`, 10.78 전용, 기준선 1f00a25a 뒤 +48줄).
// 원본 전역의 수명은 Session/메뉴/Editor 보다 길다: 새 Client 에서만 최초 BSS 0 또는 PE 초기값을 만들고, Session 대여/반납·State 전환으로 되돌리지 않는다. 모든 접근자는 호출마다 **같은 객체**를 돌려주며(주소 고정) 값을 복제하지 않는다.
// 칸(주소는 보호 PE 의 원문 전역):
//  - State 다섯 블록 `blocks()`/`state()`: old(005c8630)·phase(005c8758)·immediate(005c86c8)·current(005c87e8)·next(005c8878) 각 36DWORD(144BYTE) `Uint32Array`, 서로 겹치지 않는 720BYTE 이다. `state()` 는 `RawLocalMissionState` 가 받는 {old, phase, immediate, current, next} 이다.
//  - 로그인/기한: begin(005c7a74)·zonePending(005c85c0)·sourceType(0055a420)·centralCreated(005c85a0)·loginProgressSerial(005c85b0)·scriptBreak(00542544, 최초 1)·abortPending(005c85e0)·restoreOwner(00542528)/restoreSubscriber(0054252c, 최초 0xffffffff)·
//    noRestartSerial(005c893c)·previousHandle(005c84c0, 64BYTE)·captureEnd(005c8618)·connectEnd(005c85f8)·loginEnd(005c85f0)·activityEnd(00544d58)·reconnectSaveEnd(005c8610) 는 double, savedGame(005c85d4)/simulconnect(0054db50) 는 불리언.
//  - State 보조: stateAuxCC(005c85cc)·metadataMode(005c85c8)·metaConnected(005c85c4)·connectionNotified(005c85b4)·cachedPeerIndex(00542534)/Value(00542538)/FieldA4(0054253c)/FieldA8(00542540, 최초 0xffffffff)·cdPresent(005c85b8)·metaCdPresent(005c85bc)·
//    metaPending(005c85e8)·metaRefresh(005c85d8)·metaRebuild(005c85dc)·localServerToken(005c85a8)·localServerSearchEnd(005c8600)/RetryEnd(005c8608)·battleTerritory(005c85d0, 한 DWORD)와 `battleByte()` 가 같은 DWORD 의 하위 BYTE 를 빌린다.
//  - 시간 동기화: `clockSynchronization()` 은 {connection(005c8bf4)·replyMode(005c8c00)·gameTicks(0055b4ac)} 접근자를 돌려주고(`resetClockSynchronization` 의 입력과 같은 형태), `clockSamples()` 는 70표본 `ClockSynchronizationData` 이다.
//  - Zone: `zoneWords()`(005c86c0, DWORD 2칸)와 `zone()`(앞 6BYTE 뷰)·`zonePackets()`(가변 Zone token 장부 `RawPacketCopies`)·`metaZones()`({ firstReset(00531760, 최초 1), records(00546908, 7344BYTE) })·`playerNoMakeWarnings()`(0054d4e8, 255 DWORD).
//  - 옵션/표시: helpWord(0054db38 `ClientHelpWord`)·debugBurst(0054dbc4)·joinTestBattleCommand(005427a4, 0x0200002b)·testBattleAddress(0054dae4, 64BYTE)·viewMetaCommand(00542744, 0x02000013)·invalidTerritory(00540a30, 127)·firstMenuPending(00542530, 1)·userUpdated(0054da8c)·
//    tellTips(0053190c, 1)·tipNumber(00531910, 1)·introAvailable(0054db98)·autoDemo(0054db90)·hudModeCommand(005427ec, 0x0200003d)·addIslandCommand(00542780, 0x02000022)·`monitorTime()`(00544d68/60·0054db9c/dba0 `MonitorTimeState`)·`ambientSound()`({ deadline(=activityEnd), kind(00544d50), forced(00544d54) }).
// 웹 범위: 메타(LAN/UDP)·로컬 서버 검색·Join 시험 전투 칸은 값 보관만 하고 해당 분기는 웹 제외 오류로 막힌 소비자가 쓴다. 이 모듈은 DOM 을 쓰지 않는다. 앱 수명 연결(Client 가 한 번 만듦)은 4-D 조립 몫이다.
// (2026-11-12 WB04-E335: 4-D ClientLocalStateGlobals 이식으로 추가)
import { Int32Box } from './client-ai-debug-globals.js';
import { DoubleBox } from './client-connection-activity-globals.js';
import { ClientHelpWord } from './client-help-word.js';
import { DwordBox } from './client-subscriber-globals.js';
import { ClockSynchronizationData } from './raw-clock-synchronization.js';
import { META_ZONE_BYTES } from './raw-meta-zone-cleanup.js';
import { MonitorTimeState } from './raw-monitor-time.js';
import { RawPacketCopies } from './raw-packet-copy.js';

// State 블록 한 개의 DWORD 수(0x90BYTE)와 이전 이름 버퍼/시험 전투 주소 버퍼 바이트, 플레이어 경고 표의 DWORD 수이다. (2026-11-12 WB04-E335)
const BLOCK_DWORDS = 36;
const HANDLE_BYTES = 64;
const WARNING_WORDS = 255;

// 불리언 한 칸을 `{ value }` 상자로 노출하는 클래스이다(쓰기는 Boolean 으로 정규화). 사용: box.value = true. 임의 DWORD 상위 비트를 불리언으로 가장하지 않는 칸에만 쓴다. (2026-11-12 WB04-E335)
export class BoolBox {
    // 현재 불리언 값이다.
    #value;

    // 초기값을 불리언으로 맞춰 둔다. (2026-11-12 WB04-E335)
    constructor(initial = false) {
        this.#value = Boolean(initial);
    }

    get value() { return this.#value; }

    set value(next) { this.#value = Boolean(next); }
}

// DWORD 상자들의 이름·최초 값이다(보호 PE 의 초기값 또는 최초 BSS 0). 같은 이름의 접근자가 같은 상자를 돌려준다. (2026-11-12 WB04-E335)
const DWORD_INITIALS = Object.freeze({
    debugBurst: 0, joinTestBattleCommand: 0x0200002b, viewMetaCommand: 0x02000013, firstMenuPending: 1, userUpdated: 0, tellTips: 1, tipNumber: 1,
    introAvailable: 0, autoDemo: 0, zonePending: 0, sourceType: 0, begin: 0, centralCreated: 0, loginProgressSerial: 0, restoreOwner: 0xffffffff,
    restoreSubscriber: 0xffffffff, noRestartSerial: 0, scriptBreak: 1, abortPending: 0, stateAuxCC: 0, metadataMode: 0, metaConnected: 0,
    connectionNotified: 0, cachedPeerIndex: 0xffffffff, cachedPeerValue: 0xffffffff, cachedPeerFieldA4: 0xffffffff, cachedPeerFieldA8: 0xffffffff,
    hudModeCommand: 0x0200003d, addIslandCommand: 0x02000022, syncConnection: 0, syncReplyMode: 0, gameTicks: 0, cdPresent: 0, metaRefresh: 0,
    metaRebuild: 0, metaCdPresent: 0, metaPending: 0, localServerToken: 0, battleTerritory: 0, metaFirstReset: 1
});
// double 상자들의 이름이다(모두 최초 BSS 0, 게임 정지 가능한 시계로 바꾸지 않는 벽시각 기한). (2026-11-12 WB04-E335)
const DOUBLE_NAMES = Object.freeze(['activityEnd', 'captureEnd', 'connectEnd', 'loginEnd', 'reconnectSaveEnd', 'localServerSearchEnd', 'localServerRetryEnd']);

// Client 수명 State/로그인 전역 소유자이다. 사용: const local = new ClientLocalStateGlobals(); local.begin().value = 27; local.state().current[0] = 3. 복사/이동하지 않고 한 객체를 모든 소비자가 빌린다. (2026-11-12 WB04-E335)
export class ClientLocalStateGlobals {
    // 이름 → DWORD/double/Int32/불리언 상자 장부이다(접근자가 같은 상자를 돌려줌).
    #dword = {};
    #double = {};
    #int32 = {};
    #bool = {};
    // 0054db38 helpWord 캐시, 이전 이름·시험 전투 주소 버퍼, State 다섯 블록, 플레이어 경고 표이다.
    #helpWord = new ClientHelpWord();
    #testBattleAddress = new Uint8Array(HANDLE_BYTES);
    #previousHandle = new Uint8Array(HANDLE_BYTES);
    #blocks = Array.from({ length: 5 }, () => new Uint32Array(BLOCK_DWORDS));
    #state;
    #playerNoMakeWarnings = new Uint32Array(WARNING_WORDS);
    // 가변 Zone token 장부, 시간 알림 상태, 70표본 시간 동기화 데이터이다.
    #zonePackets = new RawPacketCopies();
    #monitorTime = new MonitorTimeState();
    #clockSamples = new ClockSynchronizationData();
    // 005c86c0 의 Zone 8BYTE(DWORD 2칸)와 앞 6BYTE 뷰, 메타 구역 기록 7344BYTE 이다.
    #zoneWords = new Uint32Array(2);
    #zone = new Uint8Array(this.#zoneWords.buffer, 0, 6);
    #metaZoneRecords = new Uint8Array(META_ZONE_BYTES);
    // 다른 모듈이 속성으로 읽고 쓰는 묶음 뷰(주변음·시간 동기화·메타 구역·battle BYTE)이며 한 번 만들어 계속 같은 객체를 돌려준다.
    #ambientSound;
    #clockSynchronization;
    #metaZones;
    #battleByte;

    // 새 Client 에서만 최초 BSS 0 과 PE 초기값을 만든다. 파일/소켓/할당 패킷은 만들지 않는다. (2026-11-12 WB04-E335)
    constructor() {
        // 이름마다 PE 초기값의 DWORD 상자를 만든다.
        for (const [name, initial] of Object.entries(DWORD_INITIALS)) this.#dword[name] = new DwordBox(initial);
        // 벽시각 기한 double 상자를 만든다.
        for (const name of DOUBLE_NAMES) this.#double[name] = new DoubleBox(0);
        this.#int32.invalidTerritory = new Int32Box(127);
        this.#int32.ambientKind = new Int32Box(0);
        this.#int32.ambientForced = new Int32Box(0);
        this.#bool.savedGame = new BoolBox(false);
        this.#bool.simulconnect = new BoolBox(false);
        const [old, phase, immediate, current, next] = this.#blocks;
        this.#state = Object.freeze({ old, phase, immediate, current, next });
        const dword = this.#dword;
        const doubles = this.#double;
        const ints = this.#int32;
        const territory = dword.battleTerritory;
        this.#ambientSound = {
            get deadline() { return doubles.activityEnd.value; },
            set deadline(value) { doubles.activityEnd.value = value; },
            get kind() { return ints.ambientKind.value; },
            set kind(value) { ints.ambientKind.value = value; },
            get forced() { return ints.ambientForced.value; },
            set forced(value) { ints.ambientForced.value = value; }
        };
        this.#clockSynchronization = {
            get connection() { return dword.syncConnection.value; },
            set connection(value) { dword.syncConnection.value = value; },
            get replyMode() { return dword.syncReplyMode.value; },
            set replyMode(value) { dword.syncReplyMode.value = value; },
            get gameTicks() { return dword.gameTicks.value; },
            set gameTicks(value) { dword.gameTicks.value = value; }
        };
        const metaFirstReset = dword.metaFirstReset;
        const records = this.#metaZoneRecords;
        this.#metaZones = {
            get firstReset() { return metaFirstReset.value; },
            set firstReset(value) { metaFirstReset.value = value; },
            records
        };
        this.#battleByte = {
            get value() { return territory.value & 0xff; },
            set value(next) { territory.value = ((territory.value & 0xffffff00) | (next & 0xff)) >>> 0; }
        };
    }

    // 0054db38 helpWord 캐시를 옵션 해석과 모든 메뉴/도움말 소비자가 같은 객체로 공유한다(M7.1). (2026-11-12 WB04-E335)
    helpWord() { return this.#helpWord; }

    // 0054dbc4 의 최초 BSS 0 이며 로그인/옵션 재해석이 같은 상자를 공유한다. Session 반납/재생성이 재설정하지 않는다. (2026-11-12 WB04-E335)
    debugBurst() { return this.#dword.debugBurst; }

    // 005427a4 의 현재 개발자 2bh 명령(최초 0x0200002b)이며 일반 입력이 비교 순간 읽는다. (2026-11-12 WB04-E335)
    joinTestBattleCommand() { return this.#dword.joinTestBattleCommand; }

    // 0054dae4 의 현재 옵션 BYTE 64칸이다(호스트 물리이며 직접 caller 의 지역 32BYTE 와 구별). (2026-11-12 WB04-E335)
    testBattleAddress() { return this.#testBattleAddress; }

    // 00542744 의 현재 ViewMeta 명령(최초 0x02000013)이며 신전 Descend event4 가 큐 직전에 읽는다. (2026-11-12 WB04-E335)
    viewMetaCommand() { return this.#dword.viewMetaCommand; }

    // 00540a30 의 현재 127(signed)이며 004b3620 진단에서 별도 지역없음 전역과 구별한다. (2026-11-12 WB04-E335)
    invalidTerritory() { return this.#int32.invalidTerritory; }

    // old/phase/immediate/current/next 순서의 물리 배열 다섯 개이다(진단도 같은 720BYTE 를 읽음). (2026-11-12 WB04-E335)
    blocks() { return this.#blocks; }

    // `RawLocalMissionState` 에 같은 블록 참조를 제공하는 {old, phase, immediate, current, next} 이다. (2026-11-12 WB04-E335)
    state() { return this.#state; }

    // 같은 State+80 이 가리키는 가변 Zone 의 실제 소유 token 장부이다. (2026-11-12 WB04-E335)
    zonePackets() { return this.#zonePackets; }

    // 00542530 의 최초 1 이며 WelcomeGiven 비영 분기의 안내 종료 위치에서만 0 을 쓴다. (2026-11-12 WB04-E335)
    firstMenuPending() { return this.#dword.firstMenuPending; }

    // 0054da8c 의 BSS 0 이다. 종료된 서버의 security.dat/명령줄 검증 writer 는 제품에 연결하지 않는다(사용자 지시). (2026-11-12 WB04-E335)
    userUpdated() { return this.#dword.userUpdated; }

    // 0053190c 의 최초 1 이며 tellTips ReadInt writer 와 State3 가 같은 DWORD 를 공유한다. (2026-11-12 WB04-E335)
    tellTips() { return this.#dword.tellTips; }

    // 00531910 의 최초 1 이며 tipNumber ReadInt 와 실제 TellTip 순번 변경이 같은 DWORD 를 공유한다. (2026-11-12 WB04-E335)
    tipNumber() { return this.#dword.tipNumber; }

    // 0054db98 은 시작 detect intro 의 실제 파일 조회 결과이다. 옵션/State 교체로 재탐색하지 않는다. (2026-11-12 WB04-E335)
    introAvailable() { return this.#dword.introAvailable; }

    // 0054db90 의 현재 옵션을 메인 Idle 과 동기 옵션 해석이 공유한다. (2026-11-12 WB04-E335)
    autoDemo() { return this.#dword.autoDemo; }

    // 00544d68/60·0054db9c/dba0 의 시간 알림 상태이다(guard/기한/monitor/alert 를 Session 교체에서도 보존). (2026-11-12 WB04-E335)
    monitorTime() { return this.#monitorTime; }

    // 주변음 부모가 받는 { deadline, kind, forced } 뷰이다: 00544d58 기한과 00544d50/54 두 int32 를 값 복사 없이 빌린다. (2026-11-12 WB04-E335)
    ambientSound() { return this.#ambientSound; }

    // 기존 Login 과 Editor 가 00544d58 의 같은 벽시각 기한을 직접 쓴다. (2026-11-12 WB04-E335)
    activityEnd() { return this.#double.activityEnd; }

    // 005c85c0 이며 실제 Fort 요청 작성과 State 정리가 같은 DWORD 를 쓴다. (2026-11-12 WB04-E335)
    zonePending() { return this.#dword.zonePending; }

    // 0055a420 을 State 와 Reply 수신이 직접 공유한다. Session 밖에서도 마지막 값을 보존한다. (2026-11-12 WB04-E335)
    sourceType() { return this.#dword.sourceType; }

    // 005c85d4 의 현재 정상 0/1 모델(불리언)이며 로그인/State 가 함께 빌린다. (2026-11-12 WB04-E335)
    savedGame() { return this.#bool.savedGame; }

    // 005c7a74 의 Login/Ready/타이머가 같은 시작 DWORD 를 쓴다. 세션 반납이 암묵 초기화하지 않는다. (2026-11-12 WB04-E335)
    begin() { return this.#dword.begin; }

    // 005c85b0 의 실제 cf570 반환 serial 이며 005cab34 와 별개이다. (2026-11-12 WB04-E335)
    loginProgressSerial() { return this.#dword.loginProgressSerial; }

    // 00542528 의 실제 저장/복구 owner 이며 최초 0xffffffff 이다. (2026-11-12 WB04-E335)
    restoreOwner() { return this.#dword.restoreOwner; }

    // 005c893c 의 BSS 0 이며 자동 서버 선택 실패 안내의 현재 실제 serial 이다. (2026-11-12 WB04-E335)
    noRestartSerial() { return this.#dword.noRestartSerial; }

    // 0054252c 의 실제 저장/복구 subscriber 이며 최초 0xffffffff 이다. (2026-11-12 WB04-E335)
    restoreSubscriber() { return this.#dword.restoreSubscriber; }

    // 005c84c0 의 현재 이전 이름 버퍼(64BYTE)를 reader/writer 가 함께 빌린다. (2026-11-12 WB04-E335)
    previousHandle() { return this.#previousHandle; }

    // 005c85a0 의 실제 metadata writer 와 cleanup 이 공유하는 DWORD 이다(현재 중앙섬 상태를 복제하지 않음). (2026-11-12 WB04-E335)
    centralCreated() { return this.#dword.centralCreated; }

    // 005c8618 의 Reply/Zone/LoginBegin 벽시각 기한이다(연결/로그인/활동 기한과 별개). (2026-11-12 WB04-E335)
    captureEnd() { return this.#double.captureEnd; }

    // 005c85f8 의 State8/9 연결 만료 시각이다(시계 getter 나 프레임 시각으로 대체하지 않음). (2026-11-12 WB04-E335)
    connectEnd() { return this.#double.connectEnd; }

    // 005c85f0 의 로그인/Dais/충돌 효과가 같은 double 을 빌린다. (2026-11-12 WB04-E335)
    loginEnd() { return this.#double.loginEnd; }

    // 00542544 의 DWORD(최초 1)이며 Login 완료 0 과 Abort 1 을 같은 상자에 쓴다. (2026-11-12 WB04-E335)
    scriptBreak() { return this.#dword.scriptBreak; }

    // 005c85e0 은 004b6350 이 1 을 쓰고 004b8cb0 이 검사하는 별도 DWORD 이다(requestPhase 005c85ac 와 합치지 않음). (2026-11-12 WB04-E335)
    abortPending() { return this.#dword.abortPending; }

    // 005c85cc 의 정확한 4BYTE 만 소유한다(인접 005c85d0 과 구별, 확인된 직접 writer 는 004b2df0 의 0 store). (2026-11-12 WB04-E335)
    stateAuxCC() { return this.#dword.stateAuxCC; }

    // 005c85c8 은 004b6dd0 의 1 store 와 0047d8c0/0047e630 의 읽기를 보존한다(미복원 부모의 전체 역할은 확정하지 않음). (2026-11-12 WB04-E335)
    metadataMode() { return this.#dword.metadataMode; }

    // 005c85c4 의 실제 DWORD 이며 상태 29/30/31 이 현재 값을 쓴다. (2026-11-12 WB04-E335)
    metaConnected() { return this.#dword.metaConnected; }

    // 005c85b4 는 004b6dd0 의 별도 0 검사/1 쓰기 DWORD 이다. (2026-11-12 WB04-E335)
    connectionNotified() { return this.#dword.connectionNotified; }

    // 00542534 의 State 선택 인덱스 캐시이며 004b2cc0 도 0xffffffff 로 무효화한다. (2026-11-12 WB04-E335)
    cachedPeerIndex() { return this.#dword.cachedPeerIndex; }

    // 00542538 은 004b6dd0 에서 선택 레코드 DWORD 와 비교하는 캐시이다(00542528/2c 의 저장 복원 슬롯과 구별). (2026-11-12 WB04-E335)
    cachedPeerValue() { return this.#dword.cachedPeerValue; }

    // 0054253c 는 004b6dd0 에서 선택 대상 +a4 DWORD 와 비교한다. (2026-11-12 WB04-E335)
    cachedPeerFieldA4() { return this.#dword.cachedPeerFieldA4; }

    // 00542540 은 004b6dd0 에서 선택 대상 +a8 DWORD 와 비교한다. (2026-11-12 WB04-E335)
    cachedPeerFieldA8() { return this.#dword.cachedPeerFieldA8; }

    // 005c8610 의 8BYTE 이며 004b6730 이 별도로 검사하는 벽시각 기한이다(LoginEnd·ActivityEnd 를 대신하지 않음). (2026-11-12 WB04-E335)
    reconnectSaveEnd() { return this.#double.reconnectSaveEnd; }

    // 005427ec 의 현재 명령(최초 0x0200003d)을 호출 직전에 읽고 변경도 보존한다. (2026-11-12 WB04-E335)
    hudModeCommand() { return this.#dword.hudModeCommand; }

    // 00542780 의 현재 명령(최초 0x02000022)을 빈 섬 예약 직전에 읽는다. (2026-11-12 WB04-E335)
    addIslandCommand() { return this.#dword.addIslandCommand; }

    // 004c1f20 이 정확히 지우는 실제 세 DWORD(connection 005c8bf4·replyMode 005c8c00·gameTicks 0055b4ac)를 값 복사 없이 빌리는 접근자 묶음이다. (2026-11-12 WB04-E335)
    clockSynchronization() { return this.#clockSynchronization; }

    // 005c8bf0/8bf8/8bfc/8c08/70표본을 같은 Client 수명으로 빌린다. (2026-11-12 WB04-E335)
    clockSamples() { return this.#clockSamples; }

    // 005c8bf4 의 현재 동기화 token 상자이다(전체 22연결표와 구별). 같은 상자를 `clockSynchronization().connection` 이 읽고 쓴다. (2026-11-12 WB04-E335)
    synchronizationConnection() { return this.#dword.syncConnection; }

    // 005c8c00 은 인접 동기화 송수신 카운터와 별도인 응답 모드 상자이다. (2026-11-12 WB04-E335)
    synchronizationReplyMode() { return this.#dword.syncReplyMode; }

    // 0055b4ac 의 현재 DWORD 를 로그인 State 와 정리가 직접 공유한다(시계의 double·프레임 번호로 바꾸지 않고 원시 32비트 보존). (2026-11-12 WB04-E335)
    gameTicks() { return this.#dword.gameTicks; }

    // 005c85b8 의 현재 00440c60 결과이다(난수 시드가 아님, State7 의 실제 CD 확인 뒤에만 쓴다). (2026-11-12 WB04-E335)
    cdPresent() { return this.#dword.cdPresent; }

    // State19 의 005c85bc 이며 State7 의 005c85b8 과 서로 다른 현재 CD 결과이다. (2026-11-12 WB04-E335)
    metaCdPresent() { return this.#dword.metaCdPresent; }

    // 005c85e8 이며 State19 의 renderer 초기화 직후 0 을 쓰는 실제 장기 DWORD 이다(인접 metaRefresh/Rebuild 와 별도). (2026-11-12 WB04-E335)
    metaPending() { return this.#dword.metaPending; }

    // 005c85a8 의 현재 검색 토큰이며 같은 CRT 와 66h 송수신이 이 상자를 빌린다. (2026-11-12 WB04-E335)
    localServerToken() { return this.#dword.localServerToken; }

    // 005c8600 의 검색 기한이며 State35/경쟁 검색 수신이 같은 double 을 갱신한다(메타 전송 반납으로 재초기화하지 않음). (2026-11-12 WB04-E335)
    localServerSearchEnd() { return this.#double.localServerSearchEnd; }

    // 005c8608 의 별도 재송신 기한이다(검색 기한과 합치지 않음). (2026-11-12 WB04-E335)
    localServerRetryEnd() { return this.#double.localServerRetryEnd; }

    // 005c85d8 의 현재 메타 갱신 예약 DWORD 이다. (2026-11-12 WB04-E335)
    metaRefresh() { return this.#dword.metaRefresh; }

    // 005c85dc 의 별도 메타 재구성 예약 DWORD 이다. (2026-11-12 WB04-E335)
    metaRebuild() { return this.#dword.metaRebuild; }

    // 0054db50 의 정상 0/1 을 모든 State 소비자가 함께 읽는다(Session 교체가 초기화하지 않는 연결 모드). (2026-11-12 WB04-E335)
    simulconnect() { return this.#bool.simulconnect; }

    // 005c85d0 의 첫 BYTE 를 같은 DWORD 에서 빌리는 `{ value }` 이다. BYTE 쓰기는 나머지 3BYTE 를 보존한다. (2026-11-12 WB04-E335)
    battleByte() { return this.#battleByte; }

    // 004bf4a0 은 005c85d0 전체 DWORD 로 Terr%02d 를 만든다. battleByte 와 같은 실제 저장소이다. (2026-11-12 WB04-E335)
    battleTerritory() { return this.#dword.battleTerritory; }

    // 005c86c0 의 DWORD 와 +4 WORD 를 담은 DWORD 2칸이다(마지막 상위 WORD 는 기존 호스트 보존 칸). (2026-11-12 WB04-E335)
    zoneWords() { return this.#zoneWords; }

    // 로그인/metadata 와 정리가 빌리는 같은 앞 6BYTE 뷰이다(인접 보존 WORD 를 Zone 길이에 포함하지 않음). (2026-11-12 WB04-E335)
    zone() { return this.#zone; }

    // 00531760 과 00546908 의 51×144BYTE 를 같은 장기 주소로 제공하는 { firstReset, records } 이다. (2026-11-12 WB04-E335)
    metaZones() { return this.#metaZones; }

    // 00531760 의 최초 1 상자이다(Client 생성 한 번뿐이며 Session 생성/반납으로 되돌리지 않음). (2026-11-12 WB04-E335)
    metaFirstReset() { return this.#dword.metaFirstReset; }

    // 7344BYTE 기록 표이다(내용은 같은 `metaZones().records` 에서만 바뀜, 사본을 만들지 않음). (2026-11-12 WB04-E335)
    metaZoneRecords() { return this.#metaZoneRecords; }

    // 0054d4e8 의 타입별 경고 비트 표(255 DWORD)이며 SP 추적/TotalMade 와 별개이다. (2026-11-12 WB04-E335)
    playerNoMakeWarnings() { return this.#playerNoMakeWarnings; }
}
