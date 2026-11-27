// 고정 공통 수학이 native libm을 호출하지 않고 같은 비트/표준 인수 변환·권한을 유지하는지 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { createModMath, MOD_MATH_PROFILE } from '../www/modding/mod-math.js';
// 기대값은 공유 프로필의 고정 자료이며 테스트 실행 중 새로 생성하지 않는다.
const basic = JSON.parse(fs.readFileSync(new URL('../../shared/modding/v0/conformance/math-vectors.json', import.meta.url))), grid = JSON.parse(fs.readFileSync(new URL('../../shared/modding/v0/conformance/math-grid.json', import.meta.url)));
// IEEE754 binary64를 NaN 분류 또는 big endian16진수로 표시한다.
function bits(value) { const buffer = Buffer.alloc(8); buffer.writeDoubleBE(value); return Number.isNaN(value) ? 'NaN' : buffer.toString('hex'); }
test('공통 수학29+256벡터는 부호0/NaN/서브노멀/큰 인수에서 정확히 같다', () => {
    const math = createModMath(Math); assert.equal(MOD_MATH_PROFILE, basic.provenance.reference); assert.equal(grid.profile, MOD_MATH_PROFILE);
    // 원래 네 native 초월함수와 별개인 고정 JS 함수의 결과를 전 범위 입력으로 비교한다.
    for (const row of [...basic.vectors, ...grid.vectors]) assert.equal(bits(math[row.name](...row.args.map(Number))), row.expected, JSON.stringify(row));
});
test('표준 ToNumber/메서드 형태와 guest의 기존 random 차단을 보존한다', () => {
    const base = Object.create(null, Object.getOwnPropertyDescriptors(Math)), forbidden = () => { throw new Error('tamed random'); }; Object.defineProperty(base, 'random', { value: forbidden, configurable: true });
    const math = createModMath(base); assert.equal(Object.isFrozen(math), true); assert.equal(Object.isFrozen(math.cos), true); assert.equal(math.random, forbidden); assert.throws(() => math.random(), /tamed/u); assert.equal(math.cos('0.1'), math.cos(.1)); assert.equal(math.pow('2', '3'), 8); assert.ok(Number.isNaN(math.sin(undefined)));
    // BigInt/Symbol과 생성자 사용은 native Math처럼 거절한다.
    for (const name of ['sin', 'cos', 'atan2', 'pow']) { assert.throws(() => math[name](1n), TypeError); assert.throws(() => math[name](Symbol()), TypeError); assert.throws(() => Reflect.construct(math[name], [0, 1]), TypeError); }
});

test('C pow와 다른 ES의 NaN/0/±1/Infinity 규칙과 부호0을 보존한다', () => {
    const math = createModMath(Math);
    assert.equal(math.pow(NaN, 0), 1); assert.equal(math.pow(NaN, -0), 1);
    // 무한 지수의 절댓값1은 ES에서 NaN이며 C의 특별 반환값1을 사용하지 않는다.
    for (const base of [-1, 1]) for (const exponent of [-Infinity, Infinity, NaN]) assert.ok(Number.isNaN(math.pow(base, exponent)));
    assert.equal(math.pow(-0, -3), -Infinity); assert.ok(Object.is(math.pow(-0, 3), -0)); assert.ok(Object.is(math.pow(-0, .5), 0));
    assert.equal(math.pow(-Infinity, .5), Infinity); assert.ok(Object.is(math.pow(-Infinity, -.5), 0)); assert.ok(Object.is(math.pow(-Infinity, -3), -0));
    // 근사값의 허용 오차가 아닌 ES가 정확히 지정한 특별값 전체 조합을 native 의미와 대조한다.
    for (const base of [-Infinity, -1, -0, 0, 1, Infinity, NaN]) for (const exponent of [NaN, -Infinity, -3, -2, -.5, -0, 0, .5, 2, 3, Infinity]) assert.ok(Object.is(math.pow(base, exponent), Math.pow(base, exponent)), `${String(base)} ** ${String(exponent)}`);
});
