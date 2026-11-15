// 사제 정기 raw-priest-regular.js(runPriestRegular)를 원본 10.78 관측 384입력과 cpppj `ClientPriestOwner_OriginalDispatch384` 계약으로 대조한다(WB04-E528).
//  - 원본 관측 priest-regular-dispatch-x86.tsv 384행: 사건·count·payload 비트·외부 후단 반환 비트에서 반환 비트와 호출한 후단(601 가상 삭제 `destroy:<flags 4BYTE>`·602~605 `fall:`/`ritual:`·그 밖 `carrier:<사건/count/payload 12BYTE>`)의
//    인자를 대조한다. 기대 반환이나 사건을 재계산하지 않는다.
//  - 계약: 선택하지 않은 후단은 읽지 않고 도달한 후단의 미연결은 명시 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E528)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { runPriestRegular } from '../www/core/raw-priest-regular.js';

// 원본 관측 384행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E528)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priest-regular-dispatch-x86.tsv'));

// 단정도 float 와 비트 변환, 리틀엔디언 DWORD 16진 출력이다. (2026-11-15 WB04-E528)
const view = new DataView(new ArrayBuffer(4));
const bitsToFloat = (bits) => { view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const floatToBits = (value) => { view.setFloat32(0, value, true); return view.getUint32(0, true); };
const dwordHex = (...values) => values.map((value) => { const bytes = new Uint8Array(4); new DataView(bytes.buffer).setUint32(0, value >>> 0, true); return Buffer.from(bytes).toString('hex'); }).join('');

test('사제 정기: 원본 10.78 384입력의 반환 비트와 가상 삭제/고유/직접 부모 호출 인수가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 384);
    let cases = 0;
    // 고정 원본 관찰만 소비하며 기대 반환이나 사건을 재계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const [event, count, payload, effect] = row.slice(0, 4).map(Number);
        let trace = '';
        const hooks = {
            destroy: (sid, flags) => { assert.equal(sid, 60); trace = `destroy:${dwordHex(flags)}`; },
            unique: (sid, code, number, value) => {
                assert.ok(sid === 60 && code === event && number === count && floatToBits(value) === payload);
                trace = code === 603 ? 'fall:' : 'ritual:';
                return bitsToFloat(effect);
            },
            carrier: (sid, code, number, value) => {
                assert.equal(sid, 60);
                trace = `carrier:${dwordHex(code, number, floatToBits(value))}`;
                return bitsToFloat(effect);
            },
        };
        const result = runPriestRegular(60, event, count, bitsToFloat(payload), hooks);
        assert.equal(floatToBits(result), Number(row[4]) >>> 0, `행 ${cases}: ${row.slice(0, 4).join(',')}`);
        assert.equal(trace, row[5], `행 ${cases}`);
        cases += 1;
    }
    assert.equal(cases, 384);
});

test('사제 정기 계약: 601 은 count 0 이면 후단 없이 3 을 반환하고 선택하지 않은 후단은 읽지 않으며 미연결은 도달한 위치에서 거부한다', () => {
    // count 0 의 601 은 삭제 후단을 요구하지 않는다.
    assert.equal(runPriestRegular(5, 601, 0, 1, {}), 3);
    assert.throws(() => runPriestRegular(5, 601, 1, 1, {}), /사제601 실제 삭제 미연결/);
    assert.throws(() => runPriestRegular(5, 603, 0, 1, { destroy: () => {} }), /고유 정기 실제 후단 미연결/);
    assert.throws(() => runPriestRegular(5, 7, 0, 1, { destroy: () => {}, unique: () => 0 }), /Carrier 부모 미연결/);
    // 경계 사건: 601 아래/위, 602..605, 606.
    const seen = [];
    const hooks = { destroy: () => { seen.push('d'); }, unique: () => { seen.push('u'); return 1; }, carrier: () => { seen.push('c'); return 2; } };
    for (const event of [600, 601, 602, 605, 606]) runPriestRegular(5, event, 1, 0, hooks);
    assert.deepEqual(seen, ['c', 'd', 'u', 'u', 'c']);
});
