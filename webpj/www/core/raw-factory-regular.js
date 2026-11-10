// Factory 정기 처리 004549c0의 현재 레벨/프레임/난수 조회와 선택된 큰 업그레이드 효과를 한 본문으로 옮긴 모듈이다(cpppj `o/RawFactoryRegular.cpp/.h`, 10.78 전용).
// 사건 분배: 2 = 현재 레벨의 첫 프레임 + 난수(현재 레벨의 프레임 수)를 Frame.set 으로 지정, 1 = 프레임 한 칸 진행, 3 = count 가 있으면 완료(finish) 반환/없으면 업그레이드 그림(begin) 뒤 프레임 진행,
//   4(count 있음) = 0 반환, 5(레벨 > 2) = 상위 레벨 플레어(2, 1). 그 밖과 위 분기를 지난 모든 경우는 공통 Damageable 본문(damageable)의 결과를 돌려준다.
// 레벨 조회(level)는 첫 프레임 조회 전과 프레임 수 조회 전에 각각 한 번씩 부르며 합치지 않는다(조회 사이에 부모 TYPE 이 바뀔 수 있다). 선택되지 않은 분기의 장치는 요구하지 않고,
// 선택된 분기에서 장치가 없으면 그 위치에서 `Error` 로 거부한다(조용한 성공 없음). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E255: 자산 정기 처리 분배 RawAssetRegular 이식 중 cpppj 기준선 뒤 3c5f5092 가 분리한 새 파일을 같은 단위에서 옮겨 추가)

// 선택된 분기가 요구하는 장치가 연결되지 않았으면 그 위치에서 거부하고 연결된 함수를 그대로 돌려준다(호출 도중 교체돼도 현재 호출은 같은 함수를 쓴다). (2026-11-10 WB04-E255)
function need(callback, message) {
    if (typeof callback !== 'function') throw new Error(message);
    return callback;
}

// 정수를 부호 있는 32비트(int32)로, 부호 없는 32비트(uint32)로 읽는다. (2026-11-10 WB04-E255)
const toInt32 = (value) => value | 0;
const toUint32 = (value) => value >>> 0;

// Factory 정기 사건 번호이다: 1 프레임 진행, 2 난수 프레임 지정, 3 업그레이드, 4 보류 반환, 5 상위 레벨 플레어. (2026-11-10 WB04-E255)
const EVENT_ADVANCE = 1;
const EVENT_RANDOM_FRAME = 2;
const EVENT_UPGRADE = 3;
const EVENT_HOLD = 4;
const EVENT_FLARE = 5;

// 한 부모의 정기 사건을 실행한다. pool 은 10.78 SidPool, parent 는 부모 SID, event/count 는 사건 번호/횟수, payload 는 float32 값이다.
// hooks = { level(parent) → int, firstFrame(parent, level) → int, frameCount(parent, level) → int, random(limit) → uint32, frame(parent, value, flags), advance(parent, delta, flags),
//           begin(parent, payload), finish(parent) → float, flare(parent, count, code), damageable(parent, event, count, payload) → float } 이다.
// 반환은 다음 예약 값(float32)이다. 10.78 이 아닌 풀은 `TypeError` 로 거부한다. (2026-11-10 WB04-E255)
export function runFactoryRegular(pool, parent, event, count, payload, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('Factory4549c0은 PATCH10.78 본문입니다');
    if (event === EVENT_RANDOM_FRAME) {
        // 현재 레벨로 첫 프레임을 읽고, 프레임 수를 읽기 전에 레벨을 한 번 더 조회한다.
        const firstLevel = need(hooks.level, 'Factory Regular의 선택된 실제 하위 미연결')(parent);
        const first = need(hooks.firstFrame, 'Factory Regular의 선택된 실제 하위 미연결')(parent, firstLevel);
        const currentLevel = need(hooks.level, 'Factory Regular의 선택된 실제 하위 미연결')(parent);
        const size = need(hooks.frameCount, 'Factory Regular의 선택된 실제 하위 미연결')(parent, currentLevel);
        const offset = need(hooks.random, 'Factory Regular의 선택된 실제 하위 미연결')(toUint32(size));
        // 첫 프레임과 난수의 합은 DWORD 로 감긴다.
        need(hooks.frame, 'Factory Regular의 선택된 실제 하위 미연결')(parent, toInt32(toUint32(first) + toUint32(offset)), 0);
    } else if (event === EVENT_ADVANCE) {
        need(hooks.advance, 'Factory Regular의 선택된 실제 하위 미연결')(parent, 1, 0);
    } else if (event === EVENT_UPGRADE) {
        // count 가 있으면 완료 몸체의 반환이 그대로 다음 예약이다.
        if (count) return need(hooks.finish, 'Factory Regular의 선택된 실제 하위 미연결')(parent);
        need(hooks.begin, 'Factory Regular의 선택된 실제 하위 미연결')(parent, payload);
        need(hooks.advance, 'Factory Regular의 선택된 실제 하위 미연결')(parent, 1, 0);
    } else if (event === EVENT_HOLD && count) {
        return 0;
    } else if (event === EVENT_FLARE && need(hooks.level, 'Factory Regular의 선택된 실제 하위 미연결')(parent) > 2) {
        need(hooks.flare, 'Factory Regular의 선택된 실제 하위 미연결')(parent, 2, 1);
    }
    return need(hooks.damageable, 'Factory Regular의 선택된 실제 하위 미연결')(parent, event, count, payload);
}
