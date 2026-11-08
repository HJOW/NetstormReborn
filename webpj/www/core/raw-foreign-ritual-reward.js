// 비로컬 의식 보상의 SP 덧셈(004482b0)과 별도 Core 장부 기술 비트 쓰기(0048f380→0042b280)를 원문 순서대로 옮긴 모듈이다(cpppj `o/RawForeignRitualReward.cpp/.h`, 10.78 전용).
// UI 나 별도 Player 기술 목록을 소유하지 않는 원본 공용 게임 규칙이다.
//  - grantForeignRitualSp(owner, balance, set): 현재 SP 조회(balance(owner), float)에 5000 을 더하고 기본 x87 53비트 정밀도(JS 배정도)로 절삭한 signed64 하위 DWORD 를 SP 정책 setter 에 넘긴다(set(owner 의 signed32 비트, 값)).
//    가산용 AddSp 나 AI 지갑을 호출하지 않으며 getter→fadd→ftol→setter 순서이다. 비유한/범위 밖의 불확정값도 하위 DWORD 0 이다.
//  - grantForeignRitualType(records, owner, type, technologyBit, assertion): TYPE 의 기술 비트(technologyBit(type), signed)가 -1 이면 아무 것도 쓰지 않는다. 그 밖에는 범위(0..31) 밖일 때 진단(0x15a)을 부른 뒤 **정상 복귀하면** 계속하고,
//    별도 Core 장부의 owner 기록(+10 위치의 비정렬 DWORD)을 진단 뒤 현재 값으로 읽어 `1 << (bit & 31)` 비트만 켜서 쓴다(이웃 ID/랭크/padding 은 읽지도 쓰지도 않음, owner 는 32비트로 감기며 논리 owner 상한을 새로 추가하지 않는다).
//    물리 장부 끝을 넘으면 부분 BYTE 쓰기 없이 거부한다. records = { bytes, origin }(Core 기록의 시작 위치와 16바이트 간격).
// 실제 분산 SP 저장소/정책(ReadPlayerSpAmount·SetPlayerSpAmount)은 아직 이식 전이라 SP 의 180행 실제 저장 대조는 해당 단위에서 한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E106: 비로컬 의식 보상 이식으로 추가)

// 의식 SP 보상 상수, ftol 의 signed64 불확정값 경계, Core 기록 간격과 기술 DWORD 위치, 범위 진단 번호이다. (2026-11-08 WB04-E106)
const SP_REWARD = 5000;
const INT64_BOUNDARY = 2 ** 63;
const CORE_STRIDE = 16;
const CORE_TECH_AT = 10;
const ASSERT_BIT = 0x15a;

// 실제로 도달한 callback 만 요구하며 선택되지 않은 분기는 생략한다. (2026-11-08 WB04-E106)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('비로컬 의식 보상의 실제 하위 미연결');
    return hook;
}

// 원문 _ftol 의 signed64 절삭 뒤 하위 DWORD(signed32)를 취하며 비유한/범위 밖의 불확정값도 하위 0 이다. (2026-11-08 WB04-E106)
function integer(value) {
    if (!Number.isFinite(value) || value >= INT64_BOUNDARY || value < -INT64_BOUNDARY) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 가산용 AddSp 나 AI 지갑을 호출하지 않고 현재 getter → fadd → ftol → 정책 setter 를 실행한다. (2026-11-08 WB04-E106)
export function grantForeignRitualSp(owner, balance, set) {
    const value = need(balance)(owner) + SP_REWARD;
    need(set)(owner | 0, integer(value));
}

// TYPE getter 와 진단 효과 뒤 같은 장부의 현재 DWORD 를 읽으며 다른 Core 필드는 보존한다. (2026-11-08 WB04-E106)
export function grantForeignRitualType(records, owner, type, technologyBit, assertion) {
    const bit = need(technologyBit)(type);
    if (bit === -1) return;
    if (bit < 0 || bit >= 32) need(assertion)(ASSERT_BIT);
    const bytes = records.bytes;
    const offset = ((records.origin >>> 0) + Math.imul(owner, CORE_STRIDE) + CORE_TECH_AT) >>> 0;
    if (offset > bytes.length || 4 > bytes.length - offset) throw new RangeError('비로컬 기술 Core DWORD의 실제 물리 범위');
    // 진단 뒤 현재 장부에서 읽는 비정렬 기술 DWORD 이다(이웃 ID/랭크/padding 을 읽지 않는다).
    let value = (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
    value = (value | (1 << (bit & 31))) >>> 0;
    // 실제 선택 DWORD 만 쓴다.
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}
