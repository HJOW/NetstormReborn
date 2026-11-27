// 엔진/플랫폼 libm에 의존하지 않는 공통 모드 수학이다. 네 JS 구현과 lock 버전을 DN도 동일하게 사용한다.
import sin from '@stdlib/math-base-special-sin';
import cos from '@stdlib/math-base-special-cos';
import atan2 from '@stdlib/math-base-special-atan2';
import pow from '@stdlib/math-base-special-pow';

// 계약/적합성 자료가 지칭하는 고정 알고리즘 프로필이다. 네 직접 패키지는 모두0.3.1이다.
export const MOD_MATH_PROFILE = 'stdlib-fdlibm-js-v0';

// 반드시 해당 guest의 tamed Math를 받는다. 호스트 Math.random을 guest에 복사하면 안 된다.
export function createModMath(base) {
    const descriptors = Object.getOwnPropertyDescriptors(base);
    // 단항+는 Math의 ToNumber처럼 BigInt/Symbol을 거절하고 사용자 변환도 연산 전에 끝낸다.
    // 간결 메서드는 native Math 함수처럼 생성자로 사용할 수 없다.
    const functions = {
        sin(value) { return sin(+value); }, cos(value) { return cos(+value); }, atan2(y, x) { return atan2(+y, +x); },
        // C계열 pow의 특별값/제곱근 최적화보다 ES의 NaN·±0·±Infinity 규칙을 먼저 적용한다.
        pow(x, y) {
            x = +x; y = +y;
            if (Number.isNaN(y)) return NaN;
            if (y === 0) return 1;
            if (Number.isNaN(x)) return NaN;
            const odd = Number.isInteger(y) && Math.abs(y % 2) === 1;
            if (!Number.isFinite(x)) return y > 0 ? (x < 0 && odd ? -Infinity : Infinity) : (x < 0 && odd ? -0 : 0);
            if (x === 0) return y > 0 ? (Object.is(x, -0) && odd ? -0 : 0) : (Object.is(x, -0) && odd ? -Infinity : Infinity);
            if (!Number.isFinite(y)) { const magnitude = Math.abs(x); if (magnitude === 1) return NaN; return (magnitude > 1) === (y > 0) ? Infinity : 0; }
            return pow(x, y);
        },
    };
    // 다른 상수/함수/random의 SES 권한 제한과 원래 descriptor는 그대로 보존한다.
    for (const [name, value] of Object.entries(functions)) descriptors[name] = { ...descriptors[name], value: Object.freeze(value) };
    return Object.freeze(Object.create(Object.getPrototypeOf(base), descriptors));
}
