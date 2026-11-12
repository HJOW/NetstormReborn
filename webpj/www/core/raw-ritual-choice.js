// 의식 보상 선택 6b 패킷 수신(00448e90)의 10.78 몸체이다. cpppj RawRitualChoice 를 옮겼으며 소유자·보낸 쪽·TYPE 검사를 원문에 없는 조건으로 덧붙이지 않는다.
// 부모가 free 가 아니면 부모의 현재 종속 목록에서 mask 4 가 선 Form 을 모두 삭제 표식(+22=1)과 함께 가상 삭제하고, 그 뒤에 패킷의 종류를 읽어 같은 부모에 새 선택 Form 을 만든다.
// 웹 범위: 6b 는 서로 다른 클라이언트 사이의 전송이 아니라 단독 진행에서도 같은 세션이 자기 자신에게 보내는 로컬 패킷이라 단독 의식 보상 선택에서 도달한다.
// 10.72(CD)는 범위 밖이다. DOM 을 쓰지 않으며 실제 삭제(world.Destroy)는 호출자가 주는 훅이다. (2026-11-13 WB04-E366)

import { RawContainedFinder } from './raw-contained-finder.js';

// 6b 패킷 최소 길이이다: [0..3] 헤더, [4..5] 부모 SID, [6..9] 선택 종류 DWORD 이다. (2026-11-13 WB04-E366)
export const RITUAL_CHOICE_MIN_PACKET = 10;
// 부모 슬롯의 상태 바이트 위치(+11)와 free 비트이다. (2026-11-13 WB04-E366)
const PARENT_STATE_OFFSET = 11;
const FREE_BIT = 1;
// 후보 Form 의 목록 플래그 위치(+20)와 교체 대상 표시 비트(mask 4)이다. (2026-11-13 WB04-E366)
const CANDIDATE_FLAGS_OFFSET = 20;
const CHOICE_MASK = 4;
// 삭제 직전에 후보에 쓰는 삭제 표식 바이트 위치(+22)와 값이다. (2026-11-13 WB04-E366)
const DELETE_MARK_OFFSET = 22;
const DELETE_MARK = 1;
// 새 선택 Form 의 생성 인자이다: argument 0, flags 4(교체 대상 표시), attachFlags 0, quality 1. (2026-11-13 WB04-E366)
const CREATE_ARGUMENT = 0;
const CREATE_FLAGS = 4;
const CREATE_ATTACH_FLAGS = 0;
const CREATE_QUALITY = 1;

// 00448e90 의 수신 몸체이다. pool 은 10.78 SidPool, finderState 는 ContainedFinderState, create 는 같은 풀의 RawContainedCreate,
// packet 은 최소 10바이트 Uint8Array, destroy(form, flags) 는 실제 가상 삭제 경계이다(삭제할 후보가 있을 때만 필요하다).
// 순서: ① 판본·동일 풀·길이 검사 ② 부모 SID(LE 16비트)를 읽고 free 면 아무것도 하지 않고 반환 ③ finder 로 부모의 현재 종속 목록을 돌며
// 후보 +20 에 mask 4 가 선 것만 +22=1 을 쓰고 destroy(후보, 0) ④ 삭제 효과가 끝난 뒤 패킷에서 종류 DWORD 를 읽어 create(부모, 종류, 0, 4, 0, 1).
// 종류는 삭제 효과 뒤에 읽으므로 destroy 가 packet 을 바꾸면 바뀐 값이 쓰인다(원문 순서). 새 Form 의 SID 는 호출자가 쓰지 않아 돌려주지 않는다. (2026-11-13 WB04-E366)
export function receiveRitualChoice(pool, finderState, create, packet, destroy) {
    if (pool?.edition !== 'patch1078' || create?.pool !== pool || !(packet?.length >= RITUAL_CHOICE_MIN_PACKET)) {
        throw new TypeError('의식 선택 수신의 판본/동일Pool/10BYTE 계약');
    }
    const parent = packet[4] | (packet[5] << 8);
    // 부모가 free 이면 선택을 무시한다. 보낸 쪽·소유자·TYPE 은 원문이 보지 않는다.
    if (pool.slot(parent)[PARENT_STATE_OFFSET] & FREE_BIT) return;
    const finder = new RawContainedFinder(pool, finderState);
    // 부모의 현재 종속 목록 전체를 순서대로 돌며 교체 대상 Form 을 지운다(finder 가 후보의 다음 링크를 삭제 전에 저장한다).
    for (let current = finder.begin(parent); current; current = finder.next()) {
        if (!(pool.slot(current)[CANDIDATE_FLAGS_OFFSET] & CHOICE_MASK)) continue;
        pool.allocated(current)[DELETE_MARK_OFFSET] = DELETE_MARK;
        if (typeof destroy !== 'function') throw new Error('의식 선택의 실제 가상 삭제 미연결');
        destroy(current, 0);
    }
    // 삭제 효과가 끝난 뒤에 종류를 읽는다(부호 없는 32비트).
    const kind = (packet[6] | (packet[7] << 8) | (packet[8] << 16) | (packet[9] << 24)) >>> 0;
    create.create(parent, kind, CREATE_ARGUMENT, CREATE_FLAGS, CREATE_ATTACH_FLAGS, CREATE_QUALITY);
}
