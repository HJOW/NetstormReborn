// 원문 004b6dd0 의 State 3(메인 메뉴) 선택 부모를 같은 Native 블록/현재 장치로 실행하는 모듈이다(cpppj `client/NativeMainMenuState.cpp/.h`). 호스트 메뉴 표시와 Native State 공통 확정을 분리한다.
// 사용:
//   installNativeMainMenuState(globals, hooks) → LocalMissionParentResult
//     globals = { state: { old, phase, immediate, current, next }, sourceType, outcome, firstMenu(각각 `{ value }` 상자: 0055a420/005c85ac/00542530), battle() → boolean(00594fbc 현재값), editor(`{ value }`, 005c85a4),
//                 background(Uint8Array: 00565e70 의 현재 NUL 문자열 물리), currentName, pendingName(0055b490/494 의 실제 할당, pending NULL 은 빈 배열), userUpdated() → DWORD(0054da8c), tellTips() → DWORD(0053190c),
//                 referenceValidationNotice(원문 대조 장치만 true; 제품은 서비스 종료한 인터넷 검증을 비활성화하므로 기본 false) }.
//     hooks = { configInt(키) → signed 정수(intro·WelcomeGiven), request(상태), deleteSavedState(), requestMusic(이름), sourceBackground(출처) → 이름|null, leaveBattle(), prefix(), hudPrefix(), title(), closeGuide(), tell(섹션),
//               startup() → DWORD, editorPlayers(), tail(), changedState() → LocalMissionParentResult }.
//     순서: current 가 3 이 아니면 거부 → current/immediate/next 의 DWORD 33 을 0 → intro 설정이 0 이면 State 39 예약·immediate[35]=1 로 확정(같은 DWORD 이며 별도 사본 아님) → 공통 앞단(출처 0 정규화)·배경 첫 BYTE 0 →
//     승패(1 victory/fanfare·2 defeat/defeat) 또는 titleMenu 배경 → outcome 0 → 출처가 있으면 저장 삭제·fanfare 와 출처 배경 → 이전 Battle14 통지 → 공통 준비·HUD1 → pending/현재 Fort 첫 BYTE 0 → 제목 갱신 →
//     current 가 3 이 아니면 changedState 결과를 그대로(Guide/안내/tail 을 중복 실행하지 않음) → Guide 소멸 → Welcome 이면 Tell("Welcome"), 아니면 Tell("Blank")·startup 이 거짓이고 firstMenu 가 참일 때
//     (참조 모드만 UserNotUpdated)·Tips 안내 후 firstMenu 0 → 편집이면 editorPlayers → tail → Commit.
//   readNativeFortEntrySource(phase, sourceBox) — State 1/3 공통 앞단: signed 이전 phase ≤ 6 또는 11 이면 출처 0 을 쓰고 현재 출처를 돌려준다.
//   notifyNativeFortEntryLeave(state, battle(), leave) — 현재 battle 이고 이전 phase 가 14 일 때만 cleanup 전 같은 52h 알림 장치를 호출한다(앞선 callback 이 바꾼 현재 값도 호출 순간에 재조회).
//   prepareNativeEditorEntry(state, sourceBox, background, battle, leave) — 실제 current 1/2 의 출처/제목 첫 BYTE/통지만 실행한다(웹은 Edit 제외라 소비자가 없으나 cpppj 계약 보존을 위해 옮김).
// 원본 관측: `native-main-menu-state-x86.tsv` 82관측(41 입력 × 두 제어 워드)을 tests/native-main-menu-state.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E534: 5-C 메인 메뉴·타이틀 상태 NativeMainMenuState 이식)
import { LocalMissionParentResult } from './raw-local-mission-state.js';

// 선택된 하위만 도달 위치에서 요구한다. (2026-11-15 WB04-E534)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Native State3 선택 장치 미연결');
    return hook;
}

// 원문의 strcpy 처럼 NUL 까지만 쓰며 뒤의 기존 BYTE 를 지우지 않는다. (2026-11-15 WB04-E534)
function background(target, name) {
    if (name.length >= target.length) throw new RangeError('Native State3 배경 이름 물리 부족');
    for (let index = 0; index < name.length; index += 1) target[index] = name.charCodeAt(index) & 0xff;
    target[name.length] = 0;
}

// Native signed JLE 를 같은 현재 값으로 판정하고 출처의 모든 32비트를 보존한다. (2026-11-15 WB04-E534)
export function readNativeFortEntrySource(phase, sourceType) {
    if ((phase | 0) <= 6 || phase === 11) sourceType.value = 0;
    return sourceType.value >>> 0;
}

// 앞선 배경/음악 callback 뒤 바뀐 현재 모드와 이전 phase 도 호출 순간에 다시 조회한다. (2026-11-15 WB04-E534)
export function notifyNativeFortEntryLeave(state, battle, leave) {
    if (battle() && state.phase[0] === 14) need(leave)();
}

// State 1/2 의 Editor 진입은 공통 출처/첫 BYTE/통지만 실행하고 결과/음악/전체 배경은 그대로 둔다. (2026-11-15 WB04-E534)
export function prepareNativeEditorEntry(state, sourceType, backgroundBytes, battle, leave) {
    if (state.current[0] !== 1 && state.current[0] !== 2) throw new Error('Native Editor 앞단의 실제 current1/2 계약');
    readNativeFortEntrySource(state.phase[0], sourceType);
    if (backgroundBytes.length === 0) throw new RangeError('Native Editor 배경 첫BYTE 물리 부족');
    backgroundBytes[0] = 0;
    notifyNativeFortEntryLeave(state, battle, leave);
}

// Native State 공통 144BYTE 확정과 다음 Poll 은 이 부모 밖의 기존 RawLocalMissionState 가 수행한다. (2026-11-15 WB04-E534)
export function installNativeMainMenuState(g, h) {
    if (g.state.current[0] !== 3) throw new Error('Native State3 실제 current 계약');
    g.state.current[33] = 0;
    g.state.immediate[33] = 0;
    g.state.next[33] = 0;
    // 005c8754 는 별도 사본이 아니라 immediate+8c 의 같은 DWORD 이다.
    if (!(need(h.configInt)('intro') | 0)) {
        need(h.request)(39);
        g.state.immediate[35] = 1;
        return LocalMissionParentResult.Commit;
    }
    // 공통 앞단을 State 1 과 공유하며 배경 callback 뒤의 현재 출처 조회는 아래 원래 위치에 남긴다.
    const source = readNativeFortEntrySource(g.state.phase[0], g.sourceType);
    const outcome = g.outcome.value >>> 0;
    if (g.background.length === 0) throw new RangeError('Native State3 배경 첫BYTE 물리 부족');
    g.background[0] = 0;
    if (outcome === 1) {
        need(h.deleteSavedState)();
        background(g.background, 'victory');
        need(h.requestMusic)('fanfare.mus');
    } else if (outcome === 2) {
        need(h.deleteSavedState)();
        background(g.background, 'defeat');
        need(h.requestMusic)('defeat.mus');
    } else background(g.background, 'titleMenu');
    g.outcome.value = 0;
    if (source) {
        need(h.deleteSavedState)();
        need(h.requestMusic)('fanfare.mus');
        const name = need(h.sourceBackground)(g.sourceType.value >>> 0);
        if (name !== null && name !== undefined) background(g.background, name);
    }
    notifyNativeFortEntryLeave(g.state, g.battle, h.leaveBattle);
    need(h.prefix)();
    need(h.hudPrefix)();
    if (g.pendingName.length !== 0) g.pendingName[0] = 0;
    if (g.currentName.length === 0) throw new RangeError('Native State3 현재 Fort 첫BYTE 물리 부족');
    g.currentName[0] = 0;
    need(h.title)();
    // 다른 Fort 설치의 결과를 보존하고 메인 전용 Guide/안내/tail 을 중복 실행하지 않는다.
    if (g.state.current[0] !== 3) return need(h.changedState)();
    need(h.closeGuide)();
    if (!(need(h.configInt)('WelcomeGiven') | 0)) need(h.tell)('Welcome');
    else {
        need(h.tell)('Blank');
        // 원문과 달리 제품은 종료된 Auto Updater 서버의 검증/Not Validated 안내를 사용하지 않는다(AGENTS.md 사용자 지시). 참조 비교만 원래 분기를 선택한다.
        if (!need(h.startup)() && g.firstMenu.value) {
            if (g.referenceValidationNotice && !need(g.userUpdated)()) need(h.tell)('UserNotUpdated');
            if (need(g.tellTips)()) need(h.tell)('Tips');
        }
        g.firstMenu.value = 0;
    }
    if (g.editor.value) need(h.editorPlayers)();
    need(h.tail)();
    return LocalMissionParentResult.Commit;
}
