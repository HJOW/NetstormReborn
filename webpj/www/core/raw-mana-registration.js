// 마나 공급원 목록 등록(00472f70)의 현재 checking·중복 진단 복귀·생산 카운터를 옮긴 모듈이다(cpppj `o/RawManaRegistration.cpp/.h`, 10.78 전용).
// 순서: 최초 개수(signed)를 한 번 저장 → 양수면 첫 원소부터 같은 값을 찾는다(첫 검사는 개수 재조회보다 먼저, 이후 반복 조건은 저장한 개수) → 중복이고 현재 checking DWORD 가 비0 이면 원문 진단
//   ('!alreadyThere', '\Ns\O\Mana.cpp', 0x44)을 요청 → 진단이 정상 복귀해도 저장한 중복 판단은 다시 검색하지 않는다 → 중복이 아니고 저장한 개수가 현재 용량(signed 비교)보다 작으면
//   그 위치에 값을 쓰고 현재 개수를 읽어 하나 더한 값을 개수에 쓴다(꼬리 값은 지우지 않음) → 중복/가득 참/음수 용량이어도 productionDirty 를 32비트 감김으로 한 번 증가한다.
// value 는 원문 DWORD 전체이며 실제 postPop 호출자만 유효 SID 번호를 전달한다. 포인터 범위 밖 접근은 RangeError(호스트 장치 오류)이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E10: 마나 공급원 등록 이식으로 추가)

// 진단 접두의 원문 파일 이름과 줄 번호, 식이다. (2026-11-08 WB04-E10)
const ALREADY_THERE = '!alreadyThere';
const MANA_FILE = '\\Ns\\O\\Mana.cpp';
const MANA_LINE = 0x44;

// 공급원 목록(SquidPostPopList 형: readCount/capacity/storage/setCount)에 value 를 등록한다.
//  - state: productionDirty 속성을 가진 객체(SquidPostPopState). 이 DWORD 만 읽고 쓴다.
//  - diagnostics: { checking() → 현재 005e4794 DWORD (선택), assertion(식, 파일, 줄) (중복 진단이 선택될 때만 필수) }
// (2026-11-08 WB04-E10)
export function registerManaProvider(list, state, value, diagnostics = {}) {
    const target = value >>> 0;
    // 최초 개수를 signed 로 한 번만 저장한다.
    const count = list.readCount() | 0;
    let already = false;
    if (count > 0) {
        const storage = list.storage();
        let index = 0;
        // 첫 양수 count 로 진입한 원문은 현재 개수 재조회 전에 첫 원소를 검사한다.
        do {
            if (index >= storage.length) throw new RangeError('마나 공급원 검색의 실제 물리 범위 오류');
            if ((storage[index] >>> 0) === target) {
                already = true;
                break;
            }
            index += 1;
        } while (index < count);
    }
    // 현재 checking DWORD 는 진단 비교 위치에서 읽는다.
    if (already && (diagnostics.checking?.() ?? 0) !== 0) {
        if (typeof diagnostics.assertion !== 'function') throw new Error('마나 공급원 중복 원문 진단 장치 미연결');
        diagnostics.assertion(ALREADY_THERE, MANA_FILE, MANA_LINE);
    }
    if (!already) {
        // 진단이 선택된 중복 경로는 이 분기에 다시 진입하지 않는다.
        const capacity = list.capacity();
        if (capacity > 0xffffffff) throw new RangeError('마나 공급원 용량 DWORD 범위 오류');
        if (count < (capacity | 0)) {
            if (count < 0) throw new RangeError('마나 공급원 음수 저장 인덱스 오류');
            const storage = list.storage();
            // 원문 범위 밖 쓰기를 더 큰 배열로 숨기지 않는다.
            if (count >= storage.length) throw new RangeError('마나 공급원 저장의 실제 물리 범위 오류');
            storage[count] = target;
            list.setCount((list.readCount() + 1) >>> 0);
        }
    }
    // 중복/가득 참/음수 용량에도 현재 DWORD 를 한 번 증가한다.
    state.productionDirty = (state.productionDirty + 1) >>> 0;
}
