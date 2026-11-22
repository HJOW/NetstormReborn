// 선택한 원소의 기술을 현재 Session 의 기술 부모에 지급하는 연결(applyThemeTechnology, 원문 00492ae0 CheatAllTechnology)을 가짜 Session 으로 확인한다. (2026-11-22 WB04-E1189)
// 지급 규칙(group 10·theme 5 제외, 이미 가진 기술 건너뜀, 호출마다 개수·부모 재조회)은 RawThemeTechnology 가 맡고, 여기서는 Session 경계 연결과 입력 명령 번호를 본다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { CHEAT_ALL_TECHNOLOGY_COMMAND, GET_TECHNOLOGY_COMMAND, applyThemeTechnology } from '../www/core/get-technology-input.js';

// 타입 표·기술 부모·보유 기술·생성 기록을 가진 가짜 Session 이다. owned 는 이미 가진 타입 번호 집합이다.
function session({ types, owned = new Set(), parent = 7 }) {
    const log = [];
    return {
        log, owned,
        core: () => ({ types }), currentTechnologyParent: () => { log.push(['parent']); return parent; },
        priests: () => ({ carrier: { findContained: (at, mask, type, index) => { log.push(['find', at, mask, type, index]); return owned.has(type) ? 100 + type : 0; } } }),
        createContained: (at, type, argument, flags, attach, quality) => { log.push(['create', at, type, argument, flags, attach, quality]); owned.add(type); return 200 + type; },
    };
}

test('명령 번호: CheatAllTechnology 0x02000028, GetTechnology 0x0200009f', () => {
    assert.equal(CHEAT_ALL_TECHNOLOGY_COMMAND, 0x02000028); assert.equal(GET_TECHNOLOGY_COMMAND, 0x0200009f);
});

test('선택한 원소의 없는 기술만 타입 순서대로 만들고 group 10·theme 5·다른 원소는 건너뛴다', () => {
    const types = [{ group: 1, theme: 1 }, { group: 2, theme: 0 }, { group: 10, theme: 1 }, { group: 3, theme: 5 }, { group: 4, theme: 1 }, { group: 5, theme: 1 }];
    const fake = session({ types, owned: new Set([4]) });
    applyThemeTechnology(fake, 1);
    // 원소 1 의 타입 0·4·5 중 이미 가진 4 는 건너뛰고 0·5 만 (부모 7, 인자 0, 플래그 4, 부착 0x10, 품질 0)로 만든다.
    assert.deepEqual(fake.log.filter((entry) => entry[0] === 'create'), [['create', 7, 0, 0, 4, 0x10, 0], ['create', 7, 5, 0, 4, 0x10, 0]]);
    assert.deepEqual(fake.log.filter((entry) => entry[0] === 'find').map((entry) => entry.slice(1)), [[7, 4, 0, 0], [7, 4, 4, 0], [7, 4, 5, 0]]);
    assert.deepEqual([...fake.owned].sort(), [0, 4, 5]);
});

test('같은 원소를 다시 지급해도 새로 만들지 않는다', () => {
    const types = [{ group: 1, theme: 2 }, { group: 2, theme: 2 }];
    const fake = session({ types }); applyThemeTechnology(fake, 2); const first = fake.log.filter((entry) => entry[0] === 'create').length;
    applyThemeTechnology(fake, 2); assert.equal(first, 2); assert.equal(fake.log.filter((entry) => entry[0] === 'create').length, 2);
});

test('theme 는 signed 32비트로 읽고 해당 원소가 없으면 아무것도 만들지 않는다', () => {
    const fake = session({ types: [{ group: 1, theme: 0 }] });
    applyThemeTechnology(fake, 3); applyThemeTechnology(fake, -1); applyThemeTechnology(fake, 0x100000001); assert.equal(fake.log.filter((entry) => entry[0] === 'create').length, 0);
    applyThemeTechnology(fake, 0); assert.equal(fake.log.filter((entry) => entry[0] === 'create').length, 1);
});

test('실제 Session 경계가 하나라도 없으면 거부한다', () => {
    const base = session({ types: [] });
    for (const name of ['core', 'priests', 'createContained', 'currentTechnologyParent']) assert.throws(() => applyThemeTechnology({ ...base, [name]: undefined }, 0), /경계 필요/u, name);
    assert.throws(() => applyThemeTechnology(null, 0), /경계 필요/u);
});
