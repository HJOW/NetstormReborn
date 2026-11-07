// 썬 바리케이드 두 dispatcher(457890→457460 Rebuild, 457e20→4578f0 Refresh)의 현재 타입 진단과 복귀 뒤 재판정을 옮긴 모듈이다(cpppj `o/RawFenceTypeParent.cpp/.h`, 10.78 전용).
// 순서: 현재 checking DWORD 가 비0 이고 슬롯 타입 BYTE(+10)가 현재 타입 전역 DWORD 와 다르면 원문 진단('fence.getType() == sunFenceType', '\Ns\O\Fence.cpp', Rebuild 0x60f / Refresh 0x602)을 요청한다.
//   진단이 복귀하면 checking 을 다시 보지 않고 같은 슬롯 BYTE 와 현재 전역 DWORD 전체를 다시 읽어 같으면 하위 몸체 body(sid)를 실행한다(BYTE 는 zero-extend 하여 DWORD 전체와 비교, 캐시하지 않음).
// raw 는 진단 동안 유지되는 50 BYTE 슬롯, currentType()/checking() 은 호출 위치마다 현재 값을 읽는 getter 이다. 진단/하위는 선택될 때만 필수이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E14: 펜스 타입 부모 이식으로 추가)

// 두 부모의 작업 종류이다. (2026-11-08 WB04-E14)
export const FENCE_TYPE_OPERATION = Object.freeze({ rebuild: 'Rebuild', refresh: 'Refresh' });

// 진단의 원문 식과 파일, 작업별 줄 번호이다. (2026-11-08 WB04-E14)
const FENCE_EXPRESSION = 'fence.getType() == sunFenceType';
const FENCE_FILE = '\\Ns\\O\\Fence.cpp';
const FENCE_LINES = Object.freeze({ Rebuild: 0x60f, Refresh: 0x602 });

// hooks = { body(sid), assertion(식, 파일, 줄) }. (2026-11-08 WB04-E14)
export function runFenceTypeParent(raw, sid, currentType, checking, operation, hooks = {}) {
    if (raw.length !== 50 || (operation !== FENCE_TYPE_OPERATION.rebuild && operation !== FENCE_TYPE_OPERATION.refresh)) throw new TypeError('펜스 타입 부모의 물리 슬롯/작업 오류');
    if (checking() !== 0 && raw[10] !== (currentType() >>> 0)) {
        if (typeof hooks.assertion !== 'function') throw new Error('펜스 타입 원문 진단 장치 미연결');
        hooks.assertion(FENCE_EXPRESSION, FENCE_FILE, FENCE_LINES[operation]);
    }
    // 진단 복귀 뒤 현재 슬롯 BYTE 와 전역 DWORD 를 다시 읽는다.
    if (raw[10] === (currentType() >>> 0)) {
        if (typeof hooks.body !== 'function') throw new Error('펜스 타입 부모의 실제 하위 미연결');
        hooks.body(sid);
    }
}
