// 원본 10.78 공통 Squid 소유자 지정(004adf00)을 옮긴 모듈이다(cpppj `o/RawCommonOwner.cpp/.h`). 가상 소유자 재정의의 base 몸체이며 Form 부착과 다른 ret4 계약이다.
// 순서: 소유자 번호 범위 진단(challenge 모드면 1..39 아니면 Squid.cpp 0x9ff, 일반 모드면 0..8 아니면 0xa02, 진단은 정상 복귀하면 계속) →
//       요새/전투 모드이고 현재 TYPE 의 genus 가 vortex·factory(0x4200)면 이전 소유자의 작업장 목록에서 제거 → 소유자 BYTE(+34)를 입력의 하위 BYTE 로 저장 →
//       현재 모드를 다시 읽어 활성이고 genus 가 맞으며 void·buried(extra 비트 8)가 아니면 새 소유자의 목록 끝에 추가.
// 소유자 쓰기 앞의 전체 TYPE/목록 사전 검사를 추가하지 않는다: 진단 뒤에는 물리 슬롯만 요구하며 TYPE/free 상태를 선행 거부하지 않는다. 선택된 장치가 연결되지 않았으면 그 위치에서 오류이다.
// 근거: cpppj 기준선 1f00a25a. 원본 관측 owner-x86.tsv(10.78 행, tests/squid-owner.test.mjs). 10.78 전용이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E2: 소유자 지정 이식으로 추가)

// 원문 진단 파일 이름이다(10.78). (2026-11-08 WB04-E2)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
// 소유자 BYTE·extra BYTE·TYPE·상태 위치이다. (2026-11-08 WB04-E2)
const OWNER = 34, EXTRA = 40, TYPE = 10, STATE = 11;
// 작업장 목록을 고치는 genus(vortex·factory) 마스크이다. (2026-11-08 WB04-E2)
const WORKSHOP_GENUS = 0x4200;

// 선택된 장치를 꺼낸다. 없으면 그 위치에서 오류이다. (2026-11-08 WB04-E2)
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`공통 소유자의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 004adf00: 객체의 소유자 BYTE 를 바꾼다. hooks: { fort(), battle(), challenge() → boolean, genus(typeByte) → flags2, remove(ownerByte, sid), append(ownerByte, sid), assertion(expression, file, line) }.
// 사용: setCommonSquidOwner(pool, sid, 3, hooks). (2026-11-08 WB04-E2)
export function setCommonSquidOwner(pool, sid, player, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('공통 소유자는 10.78 전용');
    const diagnose = (expression, line) => need(hooks.assertion, 'assertion')(expression, SQUID_CPP, line);
    const value = player >>> 0;
    if (need(hooks.challenge, 'challenge')()) {
        if (!value || value >= 0x28) diagnose('0 < newPlayerId && newPlayerId < CL_NUM_CHAL_PLAYERS', 0x9ff);
    } else if (value >= 9) diagnose('newPlayerId == INVALID_PLAYER_ID ||( FIRST_REAL_PLAYER <= newPlayerId && newPlayerId < MAX_PLAYERS)', 0xa02);
    // 진단 뒤 물리 슬롯만 요구한다.
    const raw = pool.physical(sid);
    if ((need(hooks.fort, 'fort')() || need(hooks.battle, 'battle')()) && (need(hooks.genus, 'genus')(raw[TYPE]) & WORKSHOP_GENUS)) need(hooks.remove, 'remove')(raw[OWNER], sid);
    raw[OWNER] = value & 0xff;
    const active = need(hooks.fort, 'fort')() || need(hooks.battle, 'battle')();
    if (active && (need(hooks.genus, 'genus')(raw[TYPE]) & WORKSHOP_GENUS) && !(raw[STATE] & 4) && !(raw[EXTRA] & 8)) need(hooks.append, 'append')(raw[OWNER], sid);
}
