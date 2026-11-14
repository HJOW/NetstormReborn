// 월드 설치 전후의 UI 도 같은 현재 TYPE HP getter 를 사용할 수 있게 연결한다(cpppj client/CurrentTypeHitPoints.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 번호 선택과 같은 물리 TYPE 의 현재 getNum/진단 복귀를 공통 getter(0049fce0)에 연결한다. 현재 표는 생성 시 캐시하지 않고 실제 읽기 위치에서만 요구하며, 진단 뒤 현재 표 기준을 읽되 처음 선택한 TYPE this 는 보존한다.
// 원문 TYPE base/전역의 수명은 호출자가 반환 함수보다 오래 유지한다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E435: RawMenuControls 선행 현재 TYPE HP getter 이식)
import { currentRiftTypeNumber, readRiftHitPoints } from './squid-factory.js';

// TYPE 번호로 최대 HP 를 읽는 getter 를 만든다. 사용: const hp = makeCurrentTypeHitPoints(() => types, { checking, quarter, priest, dais }, assertion); hp(74)
// currentTypes() 는 호출 때마다 현재 TYPE 레코드 배열을, globals 는 { checking(), quarter(), priest(), dais() } 현재값 함수를, assertion(expression, file, line)은 원문 진단 장치다.
export function makeCurrentTypeHitPoints(currentTypes, globals, assertion) {
    return (number) => {
        if (typeof currentTypes !== 'function') throw new Error('현재 TYPE HP 의 실제 표 조회 미연결');
        const selectedBase = currentTypes(); number >>>= 0;
        if (number >= selectedBase.length) throw new RangeError('현재 TYPE HP 번호 범위');
        const selected = selectedBase[number];
        return readRiftHitPoints(selected, globals, { number: () => currentRiftTypeNumber(selected, selectedBase, currentTypes()), assertion });
    };
}
