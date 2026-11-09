// PATCH State.cpp 004b2df0 의 1256 BYTE 세계 정리 부모(전송/정지 → 저장/Player → UI/Kernel → Core 0 선택 → Pool → Player 재구성 → 모드/기한/전역)를 옮긴 모듈이다(cpppj `o/RawStateWorldCleanup.cpp/.h`, 10.78 전용).
//  - cleanupStateWorld(reason, globals, hooks): 실제 State 부모의 현재 값 재조회·직접 필드 쓰기·선택 하위 호출 순서를 그대로 보존한다. 부모는 전역 DWORD 를 사용 위치마다 `globals.read32` 로 다시 읽고
//    `write32`/`write16`/`write64Bits` 로 같은 실제 owner 필드에 직접 쓰며, 실제 UI/Kernel/Pool/Player/Template 자식은 `hooks.effect(StateWorldCleanupEffect)` 등 선택한 필수 경계로 호출한다.
//    빈 성공/가짜 미션/새 난수로 큰 계산을 대신하지 않는다. 도달한 장치가 없으면 그 위치에서 TypeError 로 거부해 앞선 효과를 남긴다.
//  - resetStatePlayerNoMakeWarnings(warnings)(004328a0): PlayerNoMake 경고 표의 첫 255 DWORD 만 0 으로 만든다(이웃 SP 추적값·추가 물리 칸은 보존).
//  - resetStateSpotMap(spots)(004ab940): 같은 256×256 BYTE spot 만 0 으로 만든다(다른 Pool/Hash/Graph 는 초기화하지 않음).
// PATCH 10.78 몸체만 구현한다. CD 10.72 대응(0042f5c0)은 같은 몸체로 가장하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E228: State 세계 정리 이식으로 추가)

// 원문의 무인자/고정 this 자식을 식별한다. 값은 Native 주소이며 host 함수 포인터로 역참조하지 않는다. (2026-11-10 WB04-E228)
export const StateWorldCleanupEffect = Object.freeze({
    CloseAllConnections: 0x4e29c0, // reason 0 의 첫 전송 정리이다.
    EndConnections: 0x4e2a10, // reason 0 의 두 번째 전송 정리이다.
    ResumeGame: 0x460e30, // 현재 pauseDepth 가 비영일 때 게임 정지 한 단계를 푼다.
    ResumeAudio: 0x4aa970, // 같은 반복의 음향 정지 한 단계를 푼다.
    Clear446e70: 0x446e70, // 정지 반복 뒤 원문 첫 자식을 실행한다.
    Clear4c1f20: 0x4c1f20, // 명령/현재 표의 기존 몸체를 재사용한다.
    ResetClock: 0x460e70, // 원문 시계 정리 몸체이다.
    Clear42b1b0: 0x42b1b0, // 22 DWORD 직접 초기화 앞의 기존 자식이다.
    RefreshSp: 0x43dad0, // 실제 SP 작성 뒤 현재 표시 소비자이다.
    ClearTemplates: 0x4bfb00, // Player 8 순회 뒤 Template 정리이다.
    LeaveMetamap: 0x4795a0, // 그때 현재 metamap 비영에서만 선택한다.
    Clear445140: 0x445140, // 현재 Template 정리 뒤의 기존 자식이다.
    ClearCursor: 0x4d5c60, // 선택/커서 해제 몸체를 재사용한다.
    ClearPlayerLinks: 0x48fcd0, // Pool 보다 먼저 Player 동적 필드/기술 링크를 정리한다.
    ClearKernel: 0x471af0, // 실제 Kernel 표의 현재 슬롯을 가상 소멸한다.
    KillGumps: 0x4666d0, // 기존 제외 타입 필터로 현재 Gump 들을 Kill 한다.
    CleanupGumps: 0x4662b0, // 현재 kill 큐의 실제 정리를 수행한다.
    ReplaceUber: 0x4c77c0, // 기존 루트 자식 이동/새 Uber 루트 교체 몸체이다.
    Clear43b540: 0x43b540, // Core 선택 해제 직전의 기존 자식이다.
    Reset422490: 0x422490, // Pool 초기화 뒤 원문 상태 자식이다.
    Clear4c2450: 0x4c2450, // 기존 세 표 정리 몸체이다.
    MapMode: 0x4437f0, // 이름은 기존 API 호환용이다. 실제 본문은 C3 빈 RET 이며 지도/모드 계산이 없다. 두 호출 위치를 보존한다.
    SceneMusic: 0x469fc0, // 실제 음악 부모를 그 시점에 호출한다.
    Clear472830: 0x472830, // 등록 20 포인터의 현재 SID 목록 +0c 개수만 0 으로 만든다.
    Clear4b2b50: 0x4b2b50, // 원문의 현재 State 하위 몸체이다.
    Clear4328a0: 0x4328a0, // 기존 해당 정리 몸체를 필수로 호출한다.
    Clear4ab940: 0x4ab940, // 기존 해시/목록 정리 몸체를 재사용한다.
    Reset568b60: 0x46edf0, // 원문 고정 this 00568b60 에 실행한다.
    Reset54c8a0: 0x432360, // 원문 고정 this 0054c8a0 에 실행한 뒤 그 첫 DWORD 를 1 로 쓴다.
    ResetGraph: 0x462b20, // UI/Kernel/Pool 정리 뒤 실제 Graph 표를 초기화한다.
    Clear4a9e50: 0x4a9e50, // 실제 Player 재할당 직전의 자식이다.
    InitializePlayers: 0x4910a0, // Core 0 선택 뒤 이전 Player 해제/새 1620 BYTE 구성을 실행한다.
    ResetZone: 0x4bfd60, // 원문 고정 this 005c86c0 의 실제 Zone 몸체이다.
    InterpretOptions: 0x435220, // 후반 모드 정리/조건부 Core 선택 뒤 현재 설정을 재해석한다.
});

// 사용: 상수는 Native 주소이며 이름 없이 숫자로 비교하지 않고 위 표의 이름을 쓴다. 아래 주소들은 State 전역의 Native 주소이다. (2026-11-10 WB04-E228)
// 한 Player 기록의 폭과 8개 전투 관련 필드 범위(0x7c..0x98 의 DWORD 8개)이다. (2026-11-10 WB04-E228)
const PLAYER_STRIDE = 180;
const FIELD_FIRST = 0x7c;
const FIELD_LAST = 0x98;
// 실수 상수: SP 에디터 값(0x483d9c80)과 마지막 기한에 더하는 10.0 초이다. (2026-11-10 WB04-E228)
const EDITOR_SP_BITS = 0x483d9c80;
const DEADLINE_SECONDS = 10.0;

// 실수 비트 변환용 공유 뷰이다. (2026-11-10 WB04-E228)
const floatView = new DataView(new ArrayBuffer(8));
function floatFromBits(bits) {
    floatView.setUint32(0, bits >>> 0, true);
    return floatView.getFloat32(0, true);
}
function doubleBits(value) {
    floatView.setFloat64(0, value, true);
    return floatView.getBigUint64(0, true);
}

// 004328a0: 두 고정 크기는 실제 rep stosd 의 ECX 이며 짧은 호스트 물리는 쓰기 전에 거부한다. warnings 는 Uint32Array 이다. (2026-11-10 WB04-E228)
export function resetStatePlayerNoMakeWarnings(warnings) {
    if (!warnings || warnings.length < 255) throw new RangeError('State PlayerNoMake 의 실제 255DWORD 누락');
    warnings.fill(0, 0, 255);
}

// 004ab940: 현재 spot 포인터의 65536 BYTE 만 쓰고 이웃 버퍼·그 크기·주소는 바꾸지 않는다. spots 는 Uint8Array 이다. (2026-11-10 WB04-E228)
export function resetStateSpotMap(spots) {
    if (!spots || spots.length < 65536) throw new RangeError('State spot 의 실제 65536BYTE 누락');
    spots.fill(0, 0, 65536);
}

// 원문이 실제로 도달한 장치만 그 위치에서 요구한다. 누락은 기본 성공값 대신 TypeError 이다. (2026-11-10 WB04-E228)
function need(fn) {
    if (typeof fn !== 'function') throw new TypeError('State 정리의 실제 선택 장치 누락');
    return fn;
}

// 현재 실제 Player 물리에 한 DWORD 만 쓴다. 다른 필드/기록을 대신 초기화하지 않는다. 각 원문 store 가 현재 base 를 다시 읽는다. (2026-11-10 WB04-E228)
function clearPlayerField(globals, player, field) {
    const bytes = need(globals.players)();
    const offset = player * PLAYER_STRIDE + field;
    if (offset > bytes.length || 4 > bytes.length - offset) throw new RangeError('State 정리 현재 Player 필드 물리 부족');
    for (let byte = 0; byte < 4; byte += 1) bytes[offset + byte] = 0; // 같은 원문 DWORD 의 인접 필드 밖은 보존한다.
}

// 004b2df0: 전송/정지 → 저장/Player → UI/Kernel → Core 0 → Pool → Player 재구성 → 모드/기한/전역 순서이다.
//  globals = { read32(address), write32(address, value), write16(address, value)(005c89b4 의 하위 WORD 만), write64Bits(address, bits: BigInt), players() → Uint8Array(현재 59534c 의 같은 물리 뷰),
//              templateManager(slot) → 정수(지정 slot 의 20 BYTE header 첫 DWORD), typeDefault(type) → { value }(0049a840 의 선택된 현재 TYPE+c4 실제 float 상자) }.
//  hooks = { effect(selected), disconnect(번호), assertion(line), pauseDepth(), saveFort(slot), saveBattle(slot), resetAi(player), resetPriest(player), resetOtherPlayer(player, value), setSp(owner, float),
//            selectCore(owner, selected), resetPool(size), wallSeconds() → double }. (2026-11-10 WB04-E228)
export function cleanupStateWorld(reason, globals, hooks) {
    const read = (address) => need(globals.read32)(address) >>> 0; // 각 사용 위치의 현재 DWORD 이다.
    const write = (address, value) => { need(globals.write32)(address, value >>> 0); }; // 실제 전역에 즉시 쓴다.
    const wide = (address, bits = 0n) => { need(globals.write64Bits)(address, bits); }; // 원문 double 0 또는 마지막 기한을 쓴다.
    const effect = (selected) => { need(hooks.effect)(selected); }; // 해당 실제 자식만 필수이다.
    const E = StateWorldCleanupEffect;
    if (reason === 0) { effect(E.CloseAllConnections); effect(E.EndConnections); }
    else if (read(0x5c85c0) === 0) { for (let player = 1; player < 9; player += 1) need(hooks.disconnect)(player); } // 진입 분기는 한 번만 판정한다.
    else need(hooks.disconnect)(9);
    need(hooks.disconnect)(10);
    need(hooks.disconnect)(11);
    const fort = read(0x594fb8); // PE 는 두 전역 0 쓰기보다 먼저 fort 를 캡처한다.
    write(0x5c85c0, 0);
    write(0x54db14, 0);
    // 단일 모드만 허용하며 읽기 short-circuit 순서도 원문과 같다.
    const invalid = fort !== 0 ? (read(0x594fbc) !== 0 || read(0x594fc8) !== 0) : (read(0x594fbc) !== 0 && read(0x594fc8) !== 0);
    if (invalid) need(hooks.assertion)(0x77a);
    while (need(hooks.pauseDepth)() !== 0) { effect(E.ResumeGame); effect(E.ResumeAudio); } // 정지 깊이를 매 반복 복귀 뒤 재조회한다.
    effect(E.Clear446e70); effect(E.Clear4c1f20); effect(E.ResetClock); effect(E.Clear42b1b0);
    for (let word = 0; word < 22; word += 1) write(0x5c84e8 + word * 4, 0); // 실제 22 DWORD 범위만 rep stosd 순서로 0 을 쓴다.
    let selected = read(0x540cac);
    const manager = need(globals.templateManager)(selected); // 이 두 읽기는 다음 8개 전역 쓰기보다 앞이다.
    write(0x5467b8, 0); write(0x542544, 1); write(0x5950ac, 0); write(0x5950b0, 0); write(0x595090, 0); write(0x5c85e0, 0); write(0x5c85cc, 0); write(0x5c85c8, 0);
    if (manager !== 0) {
        if (read(0x594fb8) !== 0 && read(0x5c85a4) === 0) { need(hooks.saveFort)(selected); selected = read(0x540cac); } // 실제 Fort 저장 복귀일 때만 owner 를 다시 읽는다.
        if (read(0x594fbc) !== 0 && read(0x594fa4) === 0 && read(0x5c8758) === 14) need(hooks.saveBattle)(selected);
    }
    for (let player = 1; player < 9; player += 1) { // Player 0 은 생략하고 각 실제 DWORD store 마다 현재 base 를 읽는다.
        for (let field = FIELD_FIRST; field <= FIELD_LAST; field += 4) clearPlayerField(globals, player, field); // 8개 전투 관련 필드만 0 을 쓴다.
        need(hooks.resetAi)(player);
        need(hooks.resetPriest)(player);
        if (read(0x595344) === 0) need(hooks.resetOtherPlayer)(player, 0);
        else {
            const owner = read(0x540c70); // PE 가 SP 호출까지 보관한 현재 owner 이다.
            if (player !== owner) need(hooks.resetOtherPlayer)(player, 0);
            else { const bits = read(0x5c85a4) !== 0 ? EDITOR_SP_BITS : 0; need(hooks.setSp)(owner, floatFromBits(bits)); effect(E.RefreshSp); }
        }
    }
    effect(E.ClearTemplates);
    const metamap = read(0x594fc8);
    write(0x54c8a0, 0); // PE 는 metamap 을 0 쓰기보다 먼저 캡처한다.
    if (metamap !== 0) effect(E.LeaveMetamap);
    effect(E.Clear445140); effect(E.ClearCursor);
    const defaultSp = Math.fround(read(0x540bdc) | 0); // 원문은 double 이 아닌 FILD signed DWORD → float32 저장이다.
    const type = read(0x541240);
    write(0x5467bc, 0); write(0x59508c, 0); write(0x5424bc, 25);
    need(globals.typeDefault)(type).value = defaultSp;
    wide(0x5950b8);
    write(0x5453e4, 0);
    wide(0x5950c8); wide(0x5950c0); wide(0x565e68); wide(0x5c8618); wide(0x595358);
    effect(E.ClearPlayerLinks); effect(E.ClearKernel); effect(E.KillGumps); effect(E.CleanupGumps); effect(E.ReplaceUber); effect(E.Clear43b540);
    need(globals.write16)(0x5c89b4, 0);
    write(0x595344, 0);
    need(hooks.selectCore)(0, 1); // 현재 Player 해제보다 먼저 실제 Core 10 BYTE 정리/NULL 선택을 완료한다.
    write(0x5ca8f0, 0); write(0x5ca938, 0);
    need(hooks.resetPool)(0);
    effect(E.Reset422490); effect(E.Clear4c2450); effect(E.MapMode); effect(E.SceneMusic); effect(E.Clear472830); effect(E.Clear4b2b50); effect(E.Clear4328a0); effect(E.Clear4ab940);
    effect(E.Reset568b60); effect(E.Reset54c8a0);
    write(0x54c8a0, 1); write(0x5c8bb0, 0);
    effect(E.ResetGraph);
    write(0x59a8ac, 0); write(0x59a8b0, 1);
    effect(E.Clear4a9e50); effect(E.InitializePlayers); effect(E.ResetZone);
    const editor = read(0x5c85a4); // 원문 cmp 결과를 다음 직접 전역 쓰기 동안 보관한다.
    write(0x5c85a0, 0); write(0x55a420, 0); write(0x5c8490, 0xc61c4000); write(0x5c848c, 0xc61c4000);
    if (read(0x595344) !== 0) need(hooks.setSp)(read(0x540c70), floatFromBits(editor !== 0 ? EDITOR_SP_BITS : 0));
    effect(E.RefreshSp);
    const state = read(0x5c8758);
    const mission = read(0x594fa0); // State/mission 은 모드 0 쓰기 앞에서 캡처한다.
    write(0x595088, 0); write(0x5c85d4, 0); write(0x594fa4, mission); write(0x594fb8, 0); write(0x594fbc, 0); write(0x594fc8, 0); write(0x5c85a4, 0);
    if (state !== 29 && state !== 33) need(hooks.selectCore)(1, 1);
    effect(E.InterpretOptions);
    write(0x5949d4, read(0x5949d4) + 1);
    effect(E.MapMode);
    const now = need(hooks.wallSeconds)();
    wide(0x5c8610, doubleBits(now + DEADLINE_SECONDS)); // PE 의 00501450 은 double 10 이며 프레임 시각을 재사용하지 않는다.
    write(0x5c7a74, 0); wide(0x5c85f0); write(0x5c85b4, 0); write(0x540bc0, 0); write(0x540bc4, 0); wide(0x5c85f8);
    write(0x542534, 0xffffffff); write(0x542538, 0xffffffff); write(0x54253c, 0xffffffff); write(0x542540, 0xffffffff);
}
