// 31h Subscriber 명령의 캠페인 관문과 원본 현재값 출력을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { executeDeveloperSubscriber } from '../www/core/raw-campaign-developer-subscriber.js';

// 현재 manager·소유자·이름이 별도 장기 저장소인 관찰 입력을 만든다.
function scene() {
    const headers = new Uint8Array(180), handle = new Uint8Array(64); handle.set([65, 0]);
    return { headers, owner: { playerState: { templateOwner: 1 }, templateStorage: { headers: () => headers }, core: { ownerMode: { fort: false } }, globals: { mySubscriber: 0xfffffffe }, handle } };
}

// 입력 두 값이 있어도 manager가 없거나 캠페인이면 기존 Fort는 변경하지 않는다.
test('개발자 Subscriber: 현재 이름·ID와 Fort 관문', () => {
    const { owner, headers } = scene(), calls = [], trace = (...args) => calls.push(args);
    assert.equal(executeDeveloperSubscriber('ssi 123 NewName', owner, trace), true);
    assert.deepEqual(calls.pop(), [29, 'subid = -2 handle = A\n']); assert.equal(owner.globals.mySubscriber, 0xfffffffe);
    new DataView(headers.buffer).setUint32(20, 7, true);
    assert.equal(executeDeveloperSubscriber('SETSUBID 4 Name', owner, trace), true);
    assert.deepEqual(calls.pop(), [29, 'subid = -2 handle = A\n']);
    owner.core.ownerMode.fort = true;
    assert.throws(() => executeDeveloperSubscriber('ssi 4 Name', owner, trace), /웹에서 제외/);
    assert.equal(executeDeveloperSubscriber('ssi 4', owner, trace), true);
    assert.equal(executeDeveloperSubscriber('other 4', owner, trace), false);
});

// manager 부재는 fort 상태를 조회하지 않고 NUL 부족은 추적 호출 전에 진단한다.
test('개발자 Subscriber: 선행 관문과 문자열 물리', () => {
    const { owner } = scene(), calls = []; Object.defineProperty(owner.core.ownerMode, 'fort', { get() { throw new Error('미선택 fort 조회'); } });
    executeDeveloperSubscriber('ssi 1 Name', owner, (...args) => calls.push(args)); assert.equal(calls.length, 1);
    owner.handle.fill(65); assert.throws(() => executeDeveloperSubscriber('ssi', owner, (...args) => calls.push(args)), /NUL 누락/);
    assert.equal(calls.length, 1);
    owner.handle[63] = 0; owner.playerState.templateOwner = 9;
    assert.throws(() => executeDeveloperSubscriber('ssi 2 Name', owner, () => {}), /Template9/);
});
