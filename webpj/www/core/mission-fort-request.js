// 원본 004b51e0 의 미션 Fort 요청 작성과 다음 루프까지 보유하는 실제 Zone 이미지를 옮긴 모듈이다(cpppj `client/MissionFortRequest.cpp/.h`, 10.78).
//  - writeMissionFortRequest(name, globals, hooks): Init 이 State7(미션 시작)을 요청하는 몸체이다. 순서는 ① 큰 Fort 선택 `hooks.selectFort(이름의 첫 NUL 앞)` → ② Zone 512BYTE 패킷 작성(`makeLocalMissionZone`, 선언 길이 4)
//    → ③ 선택 뒤의 현재 로컬 주소 9BYTE 를 요청 +0x54/+0x58 의 DWORD 두 개와 +0x5c 의 낮은 BYTE 로 복사(+0x5c 의 윗 3BYTE 는 stale 그대로) → ④ +0 = 7, +0x7c = 현재 포트 → ⑤ `hooks.duplicate(packet)` 이 돌려준 Zone 핸들을 +0x80 에 쓰고
//    → ⑥ 복제 효과 뒤의 현재 순번을 1 증가(32비트 감김). 요청 144BYTE(36DWORD) 중 이 필드 밖은 그대로 보존한다. 주소·포트는 큰 효과 뒤의 현재 값을 읽는다.
//  - captureMissionFortZone(request, packet): 원본 0040a1e0(1) 의 길이 확정(15 + 첫 슬롯 수 × 71, 하위 WORD 로 선언 길이 기록)과 004dece0 의 소유 복제를 같은 버퍼에 수행한다. 복제 이미지는 요청이 단독 소유하며 핸들은 1 이다.
//    이미 소비했거나 복제가 남아 있거나 패킷이 512BYTE 가 아니면 거부한다(중복 캡처 금지).
//  - releaseMissionFortLoad(pending, requestBox): Guide 교체/전체 종료에서 이전 미션의 지연 로드(`LoadMissionFort`·`LoadMissionWorld`)만 취소하고 요청을 놓는다. State 의 이전 World 반납에서는 호출하지 않는다.
//  - createMissionFortLoadRequest(): 원본 `MissionFortLoadRequest`(요청 캡처 144BYTE·순번·Fort 이름·Zone 복제본·소비 표식·State7 앞단 캡처)의 빈 값이다.
//  기준선 뒤 cpppj 변경: `GeneralBattleStartRequest`(일반 LAN State7 요청, 헤더만 +11줄)는 LAN 이라 옮기지 않는다(3.6절 ③ LAN 제외). `WriteCurrentGuideFortRequest`/`GuideFortRequestHooks` 는 호출자가 `UberEditor.inc` 한 곳뿐인
//  Edit 전용이라 옮기지 않는다(PORT_MAP 메모, 편집기는 웹 제외).
//  references 대응(C++ → JS): `uint32_t&` 순번 = `{ value }` 상자, 살아 있는 `const uint32_t&` 포트 = 매번 읽는 함수 또는 `{ value }` 상자, `span<const BYTE>` 주소 = 같은 저장소를 보는 `Uint8Array`(큰 효과가 제자리에서 바꾸면 반영됨),
//  `LocalMissionStateBlock&` = 36DWORD `Uint32Array`. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E337: 4-D B 구획 MissionFortRequest 이식)
import { makeLocalMissionZone } from './local-mission-transport.js';

// 요청 블록의 DWORD 수(0x90BYTE)와 쓰는 필드의 DWORD 위치이다: +0 상태, +0x54/+0x58/+0x5c 로컬 주소 9BYTE, +0x7c 포트, +0x80 Zone 핸들. (2026-11-12 WB04-E337)
const BLOCK_DWORDS = 36;
const STATE = 0;
const ADDRESS_LOW = 0x54 / 4;
const ADDRESS_MID = 0x58 / 4;
const ADDRESS_HIGH = 0x5c / 4;
const PORT = 0x7c / 4;
const ZONE_HANDLE = 0x80 / 4;
// 로컬 주소는 NUL 포함 9BYTE 이며 DWORD 두 개 + 마지막 BYTE 한 개만 읽는다. (2026-11-12 WB04-E337)
const ADDRESS_BYTES = 9;
// 요청이 쓰는 미션 시작 상태 번호이다. (2026-11-12 WB04-E337)
const MISSION_START_STATE = 7;
// Zone 패킷 전체 예약 크기와 선언 길이 계산식(15 + 슬롯 수 × 71)의 상수이다. (2026-11-12 WB04-E337)
const ZONE_RESERVE = 512;
const ZONE_HEAD = 15;
const ZONE_CLIENT = 71;
// 요청 작성 중 observer 확정 전의 선언 길이이다. (2026-11-12 WB04-E337)
const PROVISIONAL_LENGTH = 4;

// 외부 효과가 누락된 요청을 성공으로 반환하지 않는다. 도달한 위치에서만 요구한다. (2026-11-12 WB04-E337)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('Fort 요청 실제 효과 누락');
    return effect;
}

// 원본 주소의 비정렬 DWORD 를 낮은 바이트부터 읽는다. 지정한 4BYTE 만 읽고 인접 요청 필드는 해석하지 않는다. (2026-11-12 WB04-E337)
function addressWord(address, start) {
    let value = 0;
    // 낮은 바이트부터 4개를 조합한다.
    for (let index = 0; index < 4; index += 1) value += address[start + index] * 2 ** (index * 8);
    return value;
}

// 현재 포트를 읽는다: 함수면 호출, `{ value }` 상자면 값, 숫자면 그대로이다(큰 효과 뒤에도 매번 현재 값을 읽는 쪽을 권장). (2026-11-12 WB04-E337)
function currentPort(port) {
    const value = typeof port === 'function' ? port() : port && typeof port === 'object' && 'value' in port ? port.value : port;
    return Number(value) >>> 0;
}

// 요청 작성에 필요한 현재 전역 묶음의 형식을 확인한다: next(36DWORD)·sequence({ value })·localAddress(9BYTE 이상)·port. (2026-11-12 WB04-E337)
function checkGlobals(globals) {
    if (!(globals?.next instanceof Uint32Array) || globals.next.length !== BLOCK_DWORDS) throw new TypeError('Fort 요청 블록은 36DWORD Uint32Array 여야 합니다');
    if (typeof globals.sequence !== 'object' || globals.sequence === null || !('value' in globals.sequence)) throw new TypeError('Fort 요청 순번은 { value } 상자여야 합니다');
    if (!(globals.localAddress instanceof Uint8Array) || globals.localAddress.length < ADDRESS_BYTES) throw new RangeError('Fort 요청 현재9BYTE 주소 누락');
}

// 원본 `MissionFortLoadRequest` 의 빈 값이다. next 는 Init 이 만든 144BYTE 요청 캡처이고, zone 은 관찰기 확정 뒤의 소유 복사본이며, starting 은 State7 앞단이 캡처한 정리 전 지역 값(없으면 null)이다. (2026-11-12 WB04-E337)
export function createMissionFortLoadRequest() {
    return {
        next: new Uint32Array(BLOCK_DWORDS),
        sequence: 0,
        fort: '',
        zone: new Uint8Array(0),
        consumed: false,
        starting: null,
    };
}

// 004b51e0 의 전체 요청 작성이다. State7 의 전투 정리/설치/상태 실행을 앞당기지 않는다. (2026-11-12 WB04-E337)
//  name: 논리 Fort 이름(첫 NUL 앞만 선택에 전달), globals: { next, sequence, localAddress, port }, hooks: { selectFort(name), duplicate(packet) → Zone 핸들 }.
export function writeMissionFortRequest(name, globals, hooks) {
    checkGlobals(globals);
    const text = String(name);
    const nul = text.indexOf('\0');
    // 큰 Fort 선택을 먼저 수행한다. 이 효과가 주소·포트·순번을 바꿔도 아래에서 현재 값을 읽는다.
    need(hooks?.selectFort)(nul < 0 ? text : text.slice(0, nul));
    const packet = makeLocalMissionZone();
    // 원본 관찰기는 뒤의 duplicate 진입 때 길이를 확정하므로 이 시점의 선언 길이는 4 이다.
    packet.storage[2] = PROVISIONAL_LENGTH;
    packet.storage[3] = 0;
    const { next, localAddress } = globals;
    next[ADDRESS_LOW] = addressWord(localAddress, 0);
    next[ADDRESS_MID] = addressWord(localAddress, 4);
    next[ADDRESS_HIGH] = ((next[ADDRESS_HIGH] & 0xffffff00) | localAddress[8]) >>> 0;
    next[STATE] = MISSION_START_STATE;
    next[PORT] = currentPort(globals.port);
    next[ZONE_HANDLE] = Number(need(hooks.duplicate)(packet)) >>> 0;
    // 복제 효과가 바꾼 현재 순번에 1 을 더한다(32비트 감김).
    globals.sequence.value = (globals.sequence.value + 1) >>> 0;
}

// 0040a1e0(1) 의 길이 확정과 004dece0 의 실제 소유 복제를 같은 버퍼에 수행한다. 호스트 포인터를 DWORD 로 자르지 않고 이 요청이 단독 소유한 이미지의 핸들 1 을 쓴다. (2026-11-12 WB04-E337)
export function captureMissionFortZone(request, packet) {
    if (request.consumed || request.zone.length !== 0 || packet.storage.length !== ZONE_RESERVE) throw new Error('Fort 요청 Zone 중복/예약 크기 오류');
    const length = ZONE_HEAD + packet.storage[14] * ZONE_CLIENT;
    if (length > packet.storage.length) throw new RangeError('Fort 요청 현재 Zone client 범위');
    packet.storage[2] = length & 0xff;
    packet.storage[3] = length >>> 8;
    // 선언 길이만큼을 요청 소유의 새 버퍼로 복사한다(스택 패킷 수명과 분리).
    request.zone = Uint8Array.from(packet.bytes());
    return 1;
}

// Guide 교체/전체 종료에서 이전 미션의 지연 로드만 취소한다. Session 이 아직 빌리는 이전 이미지의 실제 반납은 그 공유 소유자가 끝나는 시점에 맡기고 요청 참조만 놓는다. (2026-11-12 WB04-E337)
//  pending: client State 계약(`cancel(이름)`), requestBox: 요청을 담은 `{ value }` 상자(C++ shared_ptr& 대응).
export function releaseMissionFortLoad(pending, requestBox) {
    if (typeof pending?.cancel !== 'function') throw new TypeError('Fort 로드 취소는 State.cancel 이 필요합니다');
    pending.cancel('LoadMissionFort');
    pending.cancel('LoadMissionWorld');
    requestBox.value = null;
}
