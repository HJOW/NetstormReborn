// GetTechnology 일반 입력 분배(dispatchGetTechnologyInput, 원문 542970 → 492a30)를 원본 10.78 x86 관측 48행과 대조한다. (2026-11-22 WB04-E1200)
// 열은 `이름 / 사건 코드 / 첫 값 / 둘째 값 / 편집기 / 미션 / 호출 흔적 / 완료 / 스택 변화`이다. 흔적 `technology:N` 은 기술 지급 부모가 둘째 값 N(DWORD)으로 한 번 불렸다는 뜻이고 `-` 는 불리지 않았다는 뜻이다.
// 코드는 수정키·뗌 비트가 달라도 번호가 GetTechnology(0x0200009f)이면 분배되고, 뗌(0x80000000)이면 효과 없이 소비된다. 편집기·미션 표식은 결과에 영향이 없다(원문은 새 조건을 만들지 않는다).
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { GET_TECHNOLOGY_COMMAND, dispatchGetTechnologyInput } from '../www/core/get-technology-input.js';

// 관측 파일을 읽는다(없는 환경에서는 건너뛴다). 첫 줄은 열 이름이다.
const file = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'get-technology-input-x86.tsv');
const rows = fs.existsSync(file) ? fs.readFileSync(file, 'utf8').split(/\r?\n/u).filter(Boolean).slice(1).map((line) => line.split('\t')) : null;

test('GetTechnology 입력 분배: 원본 48행의 지급 호출 흔적·완료·스택 변화가 모두 일치한다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 48);
    let called = 0;
    for (const [name, event, first, second, , , trace, completed, stackDelta] of rows) {
        const calls = [];
        const handled = dispatchGetTechnologyInput({ code: Number(event) >>> 0, x: Number(first) >>> 0, y: Number(second) >>> 0 }, (technology) => { calls.push(`technology:${technology}`); });
        assert.equal(calls.length ? calls.join('|') : '-', trace, name);
        // 번호가 같으면 항상 분배(true)이고 `completed` 열이 1, 스택은 변하지 않는다.
        assert.equal(handled, completed === '1', name); assert.equal(stackDelta, '0', name);
        if (calls.length) called += 1;
    }
    // 눌림 코드(뗌 없음) 24행만 지급을 부른다.
    assert.equal(called, 24);
});

test('GetTechnology 입력 분배: 다른 명령 번호는 분배하지 않고 지급 장치 없이 눌림이면 거부한다', () => {
    assert.equal(dispatchGetTechnologyInput({ code: GET_TECHNOLOGY_COMMAND + 1, x: 0, y: 5 }, () => { throw new Error('호출되면 안 됨'); }), false);
    assert.throws(() => dispatchGetTechnologyInput({ code: GET_TECHNOLOGY_COMMAND, x: 0, y: 5 }, null), /미연결/u);
    // 뗌은 장치 없이도 소비한다.
    assert.equal(dispatchGetTechnologyInput({ code: (GET_TECHNOLOGY_COMMAND | 0x80000000) >>> 0, x: 0, y: 5 }, null), true);
});
