// Player.cpp 의 현재 CoreData 조회(0048f0d0)·선택/정리(0048f3b0)·복원(0048fb30)을 실제 같은 물리에 옮긴 모듈이다(cpppj `o/RawPlayerCoreData.cpp/.h`).
// globals 는 원문 현재 전역을 직접 빌리는 가변 객체이며 Player/캐시의 별도 10 BYTE 사본을 만들지 않는다:
//   templateOwner(00540cac, 선택 인자 2를 가장 먼저 받는 별도 Template owner)·battleOwner(00540c70, 그 다음 인자 1을 받으며 두 owner 를 합치지 않음)·playerKnown(00595344, 비영 owner 선택 때 1)·
//   active(00595348, owner 0 의 마지막에 0)·metamap(00594fc8, getter 진단/선택 분기를 판정하는 현재 값)·current(00595350, 현재 포인터 역할: 실제 Player+6c/캐시/외부 물리의 Uint8Array 뷰 또는 null)·
//   cached(005953f0 의 같은 고정 10 BYTE)·players(0059534c 의 현재 Player 9×180 BYTE 물리, 비영 선택 위치에서만 읽음). 포인터 동일성은 같은 buffer 의 같은 byteOffset 으로 판정한다.
//  - readPlayerCoreData: metamap 이 비영이고 현재 포인터가 캐시가 아니면 진단(Player.cpp 0x55, `_myCoreData == &lastKnownCoreData`) 뒤 **현재 포인터를 다시 읽어** 같은 가변 뷰를 돌려준다.
//  - selectPlayerCoreData: 두 owner 를 쓰고, owner 0 이면 이전 현재 10 BYTE 를 4/4/2 로 캐시에 복사(앞선 store 효과를 다음 load 에도 유지)한 뒤 현재 NULL·active 0, 비영이면 known 1 →
//    metamap 이 켜져 있으면 캐시, 아니면 실제 Player 9×180 안의 유효 10 BYTE(owner×0xb4+0x6c)를 가리킨다. 범위 밖 가상 주소를 host 뷰로 가장하지 않는다(RangeError).
//  - restorePlayerCoreData: 현재 9기록 Core 를 Player 에 4/4/2 로 복원하고 선택된 active/current·저장 BYTE 를 갱신한다(저장 게임이면 Chat 재초기화). 원문 DWORD 감김과 선택된 load/store 의
//    부분 효과를 보존하며 전체 Player/기록 선행 검사를 추가하지 않는다. cpppj 에 이 복원의 직접 원본 관측이 없어 웹 검사는 소스 계약 검사뿐이다.
// 근거: cpppj 기준선 1f00a25a, 원본 관측 player-core-data-x86.tsv(804행, tests/player-core-data.test.mjs). 10.78 전용이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E3: Player CoreData 이식으로 추가)

// Player 한 기록의 폭, Core 위치, 저장 BYTE 위치, 구독자 ID 위치이다(10.78). (2026-11-08 WB04-E3)
const PLAYER_STRIDE = 0xb4, CORE_OFFSET = 0x6c, SAVED_OFFSET = CORE_OFFSET + 5, SUBSCRIBER_OFFSET = 0x2c;
// 원문 진단 파일 이름이다. (2026-11-08 WB04-E3)
const PLAYER_CPP = '\\Ns\\O\\Player.cpp';

// 두 뷰가 같은 원문 포인터(같은 buffer 의 같은 byteOffset)인지 본다. null 은 NULL 포인터이다. (2026-11-08 WB04-E3)
export function samePointer(first, second) {
    return first !== null && second !== null && first.buffer === second.buffer && first.byteOffset === second.byteOffset;
}

// 한 원문 load 의 전체 폭을 확인한 뒤 같은 폭을 캐시에 써서 다음 load 와 겹치는 효과도 보존한다. (2026-11-08 WB04-E3)
function copyCorePiece(destination, source, offset, width) {
    if (offset > source.length || width > source.length - offset) throw new RangeError('현재 CoreData 원문 load 물리 범위');
    // 원문 DWORD/WORD 레지스터에 캡처한 실제 바이트이며 대상 store 보다 먼저 전체 source 를 읽는다.
    const word = Array.from({ length: width }, (_, byte) => source[offset + byte]);
    if (offset > destination.length || width > destination.length - offset) throw new RangeError('CoreData cache 원문 store 물리 범위');
    // 앞선 store 의 효과를 다음 source load 에도 유지한다.
    for (let byte = 0; byte < width; byte += 1) destination[offset + byte] = word[byte];
}

// 원문 포인터에 해당하는 같은 물리 뒤쪽 뷰를 빌린다. 필요한 전체 10 BYTE 를 미리 요구하지 않는다. (2026-11-08 WB04-E3)
function coreAt(bytes, offset) {
    if (offset > bytes.length) throw new RangeError('CoreData 복원의 실제 시작 물리 범위');
    return bytes.subarray(offset);
}

// 선택된 원문 BYTE/DWORD load 만 검사하고 현재 little endian 값을 읽는다. (2026-11-08 WB04-E3)
function restoreRead(bytes, offset, width) {
    if (offset > bytes.length || width > bytes.length - offset) throw new RangeError('CoreData 복원의 실제 load 물리 범위');
    let value = 0;
    for (let byte = 0; byte < width; byte += 1) value |= bytes[offset + byte] << (byte * 8);
    return value >>> 0;
}

// 원문 현재 포인터 쓰기를 host 에서 표현할 수 있는 Player 10 BYTE 의 같은 alias 로 저장한다. (2026-11-08 WB04-E3)
function restoreCurrent(globals, players, offset) {
    if (offset > players.length || 10 > players.length - offset) throw new RangeError('CoreData 복원 뒤 현재 포인터의 실제 물리 범위');
    globals.current = players.subarray(offset, offset + 10);
}

// 0048f0d0: metamap 의 실제 포인터 동일성 진단 뒤 현재 포인터를 재조회해 같은 가변 뷰를 반환한다. hooks.assertion(line) 이 없으면 진단 분기에서 거부한다(단순 반환으로 치환하지 않음).
// 사용: const core = readPlayerCoreData(globals, { assertion: (line) => … }). (2026-11-08 WB04-E3)
export function readPlayerCoreData(globals, hooks = {}) {
    if (globals.metamap !== 0 && !samePointer(globals.current, globals.cached)) {
        if (typeof hooks.assertion !== 'function') throw new Error('현재 CoreData 실제 Player.cpp85 진단 장치 누락');
        hooks.assertion(0x55);
    }
    return globals.current;
}

// 0048f3b0: 선택/정리를 실제 native store 순서대로 수행한다(Session 생성/파괴 자체에 암묵 호출하지 않는다).
// 사용: selectPlayerCoreData(globals, owner, templateOwner). (2026-11-08 WB04-E3)
export function selectPlayerCoreData(globals, owner, templateOwner) {
    const ownerValue = owner >>> 0;
    globals.templateOwner = templateOwner >>> 0;
    globals.battleOwner = ownerValue;
    if (ownerValue === 0) {
        // 원문의 EAX 에 캡처한 포인터는 세 load 동안 변하지 않는다.
        const source = globals.current;
        if (source !== null) {
            copyCorePiece(globals.cached, source, 0, 4);
            copyCorePiece(globals.cached, source, 4, 4);
            copyCorePiece(globals.cached, source, 8, 2);
        }
        globals.current = null;
        globals.active = 0;
        return;
    }
    globals.playerKnown = 1;
    if (globals.metamap !== 0) {
        globals.current = globals.cached;
        return;
    }
    // 원문 32비트 imul/주소 덧셈이며 signed 산술 UB 를 만들지 않는다.
    const offset = (Math.imul(ownerValue, PLAYER_STRIDE) + CORE_OFFSET) >>> 0;
    // 비영/비 metamap 선택의 실제 위치에서 현재 base 를 한 번 읽는다.
    const players = globals.players;
    if (offset > players.length || 10 > players.length - offset) throw new RangeError('CoreData 선택의 실제 Player 물리 부족');
    globals.current = players.subarray(offset, offset + 10);
}

// 0048fb30: 현재 9기록 Core 를 Player 에 4/4/2 로 복원한다. records 는 { bytes, origin }(raw-player-core-records.js), state 는 { missionPost(), savedGame(), checking() }(각 원문 DWORD 의 현재 값),
// hooks 는 { assertion(expression, file, line), initializeChat() }. 패치 진단 뒤 Player base 를 다시 읽고 이미 선택한 local/remote 경로는 재판정하지 않는다.
// 사용: restorePlayerCoreData(globals, records, state, owner, hooks). (2026-11-08 WB04-E3)
export function restorePlayerCoreData(globals, records, state, owner, hooks) {
    if (state.missionPost() !== 0) return;
    const ownerValue = owner >>> 0;
    const playerOffset = Math.imul(ownerValue, PLAYER_STRIDE) >>> 0;
    const recordOffset = ((records.origin >>> 0) + Math.imul(ownerValue, 16)) >>> 0;
    // PATCH 는 Player base/보존 BYTE 보다 먼저 known 을 캡처한다.
    const initialKnown = globals.playerKnown;
    let players = globals.players;
    const saved = restoreRead(players, (playerOffset + SAVED_OFFSET) >>> 0, 1) & 0xff;
    const local = initialKnown === 0 || ownerValue === (globals.battleOwner >>> 0);
    // PATCH local 경로의 active 1 쓰기보다 먼저 읽은 checking 이다.
    const checking = state.checking() !== 0;
    if (local) globals.active = 1;
    if (checking) {
        const subscriber = restoreRead(players, (playerOffset + SUBSCRIBER_OFFSET) >>> 0, 4);
        const cachedSubscriber = restoreRead(records.bytes, recordOffset, 4);
        if (subscriber !== cachedSubscriber) {
            if (typeof hooks.assertion !== 'function') throw new Error('CoreData 복원의 현재 원문 진단 장치 누락');
            hooks.assertion('getPlayer(playerId)->subscriberId == cachedCoreData[playerId].subId', PLAYER_CPP, local ? 0x224 : 0x21f);
            players = globals.players;
        }
    }
    const destination = coreAt(players, (playerOffset + CORE_OFFSET) >>> 0);
    const source = coreAt(records.bytes, (recordOffset + 4) >>> 0);
    copyCorePiece(destination, source, 0, 4);
    copyCorePiece(destination, source, 4, 4);
    copyCorePiece(destination, source, 8, 2);
    if (local) restoreCurrent(globals, globals.players, (Math.imul(globals.battleOwner >>> 0, PLAYER_STRIDE) + CORE_OFFSET) >>> 0);
    if (state.savedGame() === 0) return;
    const restored = globals.players;
    const offset = (playerOffset + SAVED_OFFSET) >>> 0;
    if (offset >= restored.length) throw new RangeError('CoreData 저장BYTE 복원의 실제 store 물리 범위');
    restored[offset] = saved;
    if (typeof hooks.initializeChat !== 'function') throw new Error('CoreData 복원의 실제 Chat 재초기화 장치 누락');
    hooks.initializeChat();
}
