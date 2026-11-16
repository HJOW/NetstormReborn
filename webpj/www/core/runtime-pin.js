// 실제 GameUse가 발급한 전체 Runtime 보유 핸들과 기존 일반 강한 참조를 구별한다(WB04-E626).
// 자동 소멸자가 없는 JS에서 C++ 지역 shared_ptr의 범위 종료를 명시 반납한다.
const issuedPins = new WeakSet();

// 사용: 실제 owner와 호출 수 감소 함수를 전달한다. owner/release의 기존 공개 형태와 멱등 반납을 유지한다.
export function createRuntimePin(owner, onRelease) {
    if (!owner || typeof onRelease !== 'function') throw new TypeError('Runtime pin의 실제 owner/반납 장치 필요');
    let held = owner;
    const pin = {
        // 정상/예외 복귀까지 실제 전체 owner를 보관하고 반납 뒤에는 null이다.
        get owner() { return held; },
        // 호출 수를 먼저 반납한 뒤 실제 owner 참조를 놓는다. 중복 반납은 무동작이다.
        release() { if (held) { onRelease(); held = null; } },
    };
    issuedPins.add(pin); return pin;
}

// 실제 발급 핸들만 범위 종료에서 반납한다. 기존 pin이 돌려준 일반 owner의 release 메서드는 호출하지 않는다.
export function releaseRuntimePin(pin) { if (pin && issuedPins.has(pin)) pin.release(); }
