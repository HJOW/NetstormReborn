// cpppj FortTypeConversionStorageTests의 같은 참조·할당·해제·소비자 수명을 대조한다. (WB04-E573)
import test from 'node:test';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { FortTypeConversionStorage } from '../www/core/fort-type-conversion-storage.js';

// 초기 미작성 바이트를 읽는 대신 모든 검사 블록을 명시 입력으로 먼저 작성한다.
function initialize(owner, token, value) { const bytes = owner.bytes(token); assert.equal(bytes.length, 255); bytes.fill(value); return bytes; }

test('FortTypeConversionStorage: 현재DWORD는 같은 참조이며 할당과 독립 owner가 바꾸지 않는다', () => {
    const first = new FortTypeConversionStorage(), second = new FortTypeConversionStorage(), current = first.current();
    assert.equal(current, first.current()); assert.notEqual(current, second.current()); assert.deepEqual([current.value, second.current().value], [0, 0]);
    const token = first.allocate(255); assert.ok(token); assert.equal(current.value, 0); current.value = token; second.current().value = 0x87654321;
    assert.equal(first.current().value, token); current.value = 0; assert.equal(first.bytes(token).length, 255); assert.equal(second.current().value, 0x87654321);
});
test('FortTypeConversionStorage: 두 할당은 독립이며 반복 bytes는 사본 없는 같은255BYTE다', () => {
    const owner = new FortTypeConversionStorage(); owner.current().value = 0xfedcba98;
    const first = owner.allocate(255), second = owner.allocate(255), a = initialize(owner, first, 0x31), b = initialize(owner, second, 0x72);
    assert.notEqual(first, second); assert.notEqual(a, b); assert.equal(owner.bytes(first), a); assert.equal(owner.bytes(second), b); assert.equal(owner.current().value, 0xfedcba98);
    a[0] = 0x19; owner.bytes(first)[254] = 0xe7; assert.equal(owner.bytes(first)[0], 0x19); assert.equal(a[254], 0xe7); assert.ok(b.every((x) => x === 0x72)); b[127] = 0xa5; assert.equal(a[127], 0x31);
});
test('FortTypeConversionStorage:0/없는/반납한 토큰을 다시 읽거나 반납하지 않는다', () => {
    const owner = new FortTypeConversionStorage(); assert.throws(() => owner.bytes(0), RangeError); assert.throws(() => owner.bytes(0xffffffff), RangeError); assert.throws(() => owner.release(0xffffffff), RangeError);
    const token = owner.allocate(255); initialize(owner, token, 0x4c); owner.release(token); assert.throws(() => owner.bytes(token), RangeError); assert.throws(() => owner.release(token), RangeError); assert.equal(owner.current().value, 0);
});
test('FortTypeConversionStorage: 비현재/현재 해제는 현재값을 바꾸지 않고 다른 실제 블록을 보존한다', () => {
    const owner = new FortTypeConversionStorage(); owner.release(0); const previous = owner.allocate(255), current = owner.allocate(255); initialize(owner, previous, 0x18); const live = initialize(owner, current, 0x59);
    owner.current().value = current; owner.release(0); owner.release(previous); assert.equal(owner.current().value, current); assert.equal(owner.bytes(current), live); assert.equal(live[254], 0x59); assert.throws(() => owner.bytes(previous));
    owner.release(current); assert.equal(owner.current().value, current); assert.throws(() => owner.bytes(current)); owner.release(0); assert.equal(owner.current().value, current); owner.current().value = 0; assert.equal(owner.current().value, 0);
});
test('FortTypeConversionStorage: 잘못된 크기와 할당 중단은 현재값·발급 순서·살아 있는 블록을 보존한다', () => {
    let reject = false; const owner = new FortTypeConversionStorage((bytes) => { if (reject) throw new Error('확보 중단'); bytes.fill(0xd3); }); const first = owner.allocate(255), bytes = owner.bytes(first); owner.current().value = first;
    // 각 실패가 실제 다음 토큰이나 기존 할당을 손상하지 않는지 확인한다.
    for (const size of [0, 1, 254, 256, 0xffffffff]) assert.throws(() => owner.allocate(size), RangeError);
    reject = true; assert.throws(() => owner.allocate(255), /확보 중단/); reject = false; const second = owner.allocate(255); assert.equal(second, first + 1); assert.equal(owner.current().value, first); assert.equal(owner.bytes(first), bytes); assert.ok(bytes.every((x) => x === 0xd3));
    const exhausted = new FortTypeConversionStorage(undefined, 0xfffffffe); assert.equal(exhausted.allocate(255), 0xffffffff); exhausted.release(0xffffffff); assert.throws(() => exhausted.allocate(255), /소진/); assert.equal(exhausted.current().value, 0);
});
test('FortTypeConversionStorage: 두 실제 소비자가 같은 전체 owner를 유지하고 마지막 반납 뒤 weak 수명이 끝난다', () => {
    // 검사와 같은 Node에서 실제 GC를 요청하며 소멸한 owner/뷰를 사용하지 않는다.
    const code = `import assert from 'node:assert/strict';
import { FortTypeConversionStorage } from ${JSON.stringify(new URL('../www/core/fort-type-conversion-storage.js', import.meta.url).href)};
let owner=new FortTypeConversionStorage(), first=owner, second=owner; const token=owner.allocate(255); owner.current().value=token; owner.bytes(token).fill(0x8b); const weak=new WeakRef(owner); owner=null;
await new Promise(setImmediate); global.gc(); assert.equal(first.current().value,token); first.bytes(token)[254]=0xc6; assert.equal(second.bytes(token)[254],0xc6); first=null;
await new Promise(setImmediate); global.gc(); assert.ok(weak.deref()); assert.equal(second.bytes(token)[0],0x8b); second=null;
// 실제 이벤트 루프 경계를 넘기며 마지막 소비자 반납 뒤 GC 수명을 관측한다.
let expired=false; for(let step=0;step<80;step+=1){ await new Promise(setImmediate); global.gc(); if(!weak.deref()){expired=true;break;} } assert.equal(expired,true);`;
    const result = spawnSync(process.execPath, ['--expose-gc', '--input-type=module', '-e', code], { encoding: 'utf8' }); assert.equal(result.status, 0, result.stderr);
});
