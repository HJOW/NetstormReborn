// 초기 지형 도장과 지역 도장이 공유하는 후보 선택(cpppj `o/RawFrameChoose` ChooseTerrainFrame, 원본 0049aa90)의 웹 대응 `assets/terrain-tiles.js` chooseTerrainFrame 을 검사한다(4-B, WB04-E294).
//  - 후보 검색의 네 단계(정확히 → 정규화 → (방향, 'A') → 방향만)와 후보 최대 256개 제한, 난수 소비(theme -1: next(후보 수) 한 번, 0..3: low + next(high - low) 한 번, 후보 없음: 소비 없음).
//  - 10차 NB23 교정(기준선 뒤 cpppj `bda77381e`, 3.6절 ② 반영): 후보가 8개 미만이라 원소 범위가 비는 코드는 오류 대신 전체 후보에서 고른다(후보가 충분하면 기존 분위 규칙 그대로).
//    cpppj `KnownBug_NB23_SparseTileCodeFallsBackToAllCandidates` 의 계약 문장(`B/A` 5개·`C/A` 16개 코드)을 같은 입력으로 옮겼다.
// 원본 관측 파일이 없는 단위 계약 검사이다(기준선의 후보 검색·분위 규칙은 terrain-tiles.test.mjs 가 원본 Fort 로 대조한다). 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E294)
import test from 'node:test';
import assert from 'node:assert/strict';
import { FrameCodeTable, chooseTerrainFrame } from '../www/assets/terrain-tiles.js';
import { GameRandom } from '../www/core/random.js';

// 방향·변형 글자 이름 목록(`'BA'` 등)으로 프레임 코드 표를 만든다. (2026-11-11 WB04-E294)
const tableOf = (names) => new FrameCodeTable(names.map((name, number) => ({ side: name.charCodeAt(0), variant: name.charCodeAt(1), number: number & 0xff, flags: 0 })));

// 호출 기록과 정해진 값을 돌려주는 난수 대역이다. (2026-11-11 WB04-E294)
function script(value) {
    const calls = [];
    return { calls, next(limit) { calls.push(limit); return value; } };
}

// NB23 교정의 계약: 후보가 부족한 코드는 모든 원소에서 예외 없이 그 코드의 후보 안에서 고르고, 충분하면 원소 분위를 지킨다. (2026-11-11 WB04-E294)
test('후보 선택: 후보가 8개 미만인 코드는 모든 원소에서 전체 후보 안에서 고른다(NB23)', () => {
    const names = [];
    // 'B'/'A' 코드 5개: 분위 크기 1 이라 원소 범위가 빈다.
    for (let index = 0; index < 5; index += 1) names.push('BA');
    // 'C'/'A' 코드 16개: 분위 크기 4 라 원소 범위가 있다.
    for (let index = 0; index < 16; index += 1) names.push('CA');
    const frames = tableOf(names);
    const random = new GameRandom(1);
    // 후보가 부족한 코드는 모든 원소(0..3)에서 그 코드의 후보(0..4) 안이다.
    for (let theme = 0; theme < 4; theme += 1) {
        for (let round = 0; round < 40; round += 1) {
            const chosen = chooseTerrainFrame(frames, 'B', 'A', theme, random);
            assert.ok(chosen >= 0 && chosen < 5, `theme ${theme}`);
        }
    }
    // 후보가 충분하면 원소 1 의 분위(인덱스 5 + 5 .. 5 + 7)를 지킨다.
    for (let round = 0; round < 40; round += 1) {
        const chosen = chooseTerrainFrame(frames, 'C', 'A', 1, random);
        assert.ok(chosen >= 5 + 5 && chosen < 5 + 8);
    }
});

// 난수 소비를 대역으로 고정한다: 빈 원소 범위는 theme -1 과 같이 후보 수 한계로 정확히 한 번, 경계(후보 8개)에서는 원소 범위를 쓴다. (2026-11-11 WB04-E294)
test('후보 선택: 빈 원소 범위는 후보 수로 한 번, 8개부터는 원소 범위로 한 번 난수를 쓴다', () => {
    for (const count of [1, 2, 3, 4, 5, 6, 7]) {
        const frames = tableOf(Array.from({ length: count }, () => 'BA'));
        for (let theme = 0; theme < 4; theme += 1) {
            const random = script(count - 1);
            assert.equal(chooseTerrainFrame(frames, 'B', 'A', theme, random), count - 1, `후보 ${count} 원소 ${theme}`);
            assert.deepEqual(random.calls, [count]);
        }
    }
    // 후보 8개: 분위 크기 2 → 원소 t 의 범위 [2t + 1, 2t + 2) 라 next(1) 하나이며 low 번째 후보이다.
    const eight = tableOf(Array.from({ length: 8 }, () => 'BA'));
    for (let theme = 0; theme < 4; theme += 1) {
        const random = script(0);
        assert.equal(chooseTerrainFrame(eight, 'B', 'A', theme, random), 2 * theme + 1, `원소 ${theme}`);
        assert.deepEqual(random.calls, [1]);
    }
    // theme -1 은 후보가 몇 개든 후보 수 한계로 한 번이다.
    const random = script(2);
    assert.equal(chooseTerrainFrame(tableOf(['BA', 'BA', 'BA']), 'B', 'A', -1, random), 2);
    assert.deepEqual(random.calls, [3]);
});

// 후보 검색 네 단계와 폴백: 정확히 → 정규화 → (방향, 'A') → 방향만, 어느 단계에서도 없으면 -1 이고 난수를 쓰지 않는다. (2026-11-11 WB04-E294)
test('후보 선택: 후보 검색 단계와 후보 없음', () => {
    // 정확히 일치.
    let random = script(0);
    assert.equal(chooseTerrainFrame(tableOf(['CD', 'BF', 'BA']), 'B', 'F', -1, random), 1);
    // 정규화 단계: ('A','F') 는 정확히 일치가 없고 정규화한 ('B','F') 가 하나뿐이어도 그 후보를 쓴다.
    random = script(0);
    assert.equal(chooseTerrainFrame(tableOf(['CD', 'BC', 'BF']), 'A', 'F', -1, random), 2);
    assert.deepEqual(random.calls, [1]);
    // (방향, 'A') 단계: 'B'/'F' 가 없고 정규화 결과도 없을 때 방향만 맞는 앞선 코드(BC)가 아니라 'B'/'A' 후보를 쓴다.
    random = script(0);
    assert.equal(chooseTerrainFrame(tableOf(['CD', 'BC', 'BA']), 'B', 'F', -1, random), 2);
    assert.deepEqual(random.calls, [1]);
    // 방향만 단계: 'B'/'F'·'B'/'A' 가 없고 'B' 로 시작하는 코드만 있을 때.
    random = script(0);
    assert.equal(chooseTerrainFrame(tableOf(['CD', 'BC', 'BD']), 'B', 'F', -1, random), 1);
    assert.deepEqual(random.calls, [2]);
    // 후보 없음.
    random = script(0);
    assert.equal(chooseTerrainFrame(tableOf(['CD', 'EF']), 'B', 'F', 2, random), -1);
    assert.deepEqual(random.calls, []);
    assert.equal(chooseTerrainFrame(tableOf([]), 'B', 'F', -1, random), -1);
    assert.deepEqual(random.calls, []);
});

// 후보는 저장 순서대로 최대 256개이다. (2026-11-11 WB04-E294)
test('후보 선택: 후보는 최대 256개로 제한된다', () => {
    const frames = tableOf(Array.from({ length: 300 }, () => 'BA'));
    const random = script(255);
    assert.equal(chooseTerrainFrame(frames, 'B', 'A', -1, random), 255);
    assert.deepEqual(random.calls, [256]);
    // 원소 3 의 범위는 256 개 기준(분위 64 → [193, 256))이다.
    const themed = script(62);
    assert.equal(chooseTerrainFrame(frames, 'B', 'A', 3, themed), 193 + 62);
    assert.deepEqual(themed.calls, [63]);
});

// 난수가 한계를 넘기는 잘못된 값은 후보 범위 오류이고, 정상 입력에서 원소 범위는 빈 범위 폴백과 구별된다. (2026-11-11 WB04-E294)
test('후보 선택: 원소 범위의 난수가 후보 수를 넘으면 오류이다', () => {
    const frames = tableOf(Array.from({ length: 24 }, () => 'BF'));
    assert.throws(() => chooseTerrainFrame(frames, 'B', 'F', 3, script(5)), /후보 범위/);
    assert.equal(chooseTerrainFrame(frames, 'B', 'F', 3, script(4)), 23);
});
