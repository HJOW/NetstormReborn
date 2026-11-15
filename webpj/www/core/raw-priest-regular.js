// 사제 494580 의 사건 선택과 Carrier 부모 순서를 typed/Native 에서 공유하는 정기 몸체를 옮긴 모듈이다(cpppj `o/RawPriestRegular.cpp/.h`, 10.78 전용).
// 사용: const next = runPriestRegular(sid, event, count, payload, { destroy(sid, flags), unique(sid, event, count, payload) → float, carrier(sid, event, count, payload) → float });
//   event 601: count 가 0 이면 3.0 을 반환하고, 양수이면 실제 가상 +10 삭제(flags 0x100000)를 요청한 뒤 −1.0 을 반환한다(count 는 부호 없는 비트로 전달하며 601 은 0 인지 여부만 읽는다).
//   event 602(회복)·603(추락)·604(의식 프레임)·605(기도): 기존 실제 고유 후단(unique)의 반환을 그대로 전달한다.
//   그 밖의 사건: 직접 42a750 Carrier 부모(carrier)의 반환을 그대로 전달한다.
// 선택한 후단만 도달한 위치에서 요구하고 미연결이면 명시 거부한다(선택되지 않은 경계는 읽지 않는다). 원본 494580 의 601~605 선택을 재사용하며 큰 고유 후단은 기존 몸체에 위임한다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E528: 4-D 구획 RawPriestRegular 이식)

// 사건 번호와 601 의 삭제 flags 상수이다. (2026-11-15 WB04-E528)
const EVENT_DESTROY = 601;
const EVENT_UNIQUE_FIRST = 602;
const EVENT_UNIQUE_LAST = 605;
const DESTROY_FLAGS = 0x100000;

// 601 의 선택 후 삭제만 실행하고 602~605 는 각각 현재 후단에 전달한다. 원문 가상 진입의 분기와 반환 상수를 보관한다. (2026-11-15 WB04-E528)
export function runPriestRegular(sid, event, count, payload, hooks) {
    if (event === EVENT_DESTROY) {
        if (!count) return 3.0;
        if (typeof hooks?.destroy !== 'function') throw new Error('사제601 실제 삭제 미연결');
        hooks.destroy(sid, DESTROY_FLAGS);
        return -1.0;
    }
    if (event >= EVENT_UNIQUE_FIRST && event <= EVENT_UNIQUE_LAST) {
        if (typeof hooks?.unique !== 'function') throw new Error('사제 고유 정기 실제 후단 미연결');
        return hooks.unique(sid, event, count, payload);
    }
    if (typeof hooks?.carrier !== 'function') throw new Error('사제 정기 실제 Carrier 부모 미연결');
    return hooks.carrier(sid, event, count, payload);
}
