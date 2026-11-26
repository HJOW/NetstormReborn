// 초기 미션의 실제 조건표와 번역 자산을 독립 CP 카탈로그에 대조한다. 안내 조회가 진행 상태를 변경하면 안 된다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { TUTORIAL_OBJECTIVES, TUTORIAL_WAITING, tutorialObjectiveKey } from '../www/core/tutorial-objective.js';
import { createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { XlatTable } from '../www/assets/xlat.js';

// CP1371 카탈로그는 웹 표의 누락·오타를 검출하는 별도 기준 자료다.
const catalog = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/src/client/TutorialObjectiveCatalog.inc'), 'utf8');
const expected = [...catalog.matchAll(/\{(\d),'(.)',"([^"]+)"\}/g)].map((match) => ({ tutorial: Number(match[1]), stage: match[2].charCodeAt(0), english: match[3] }));

// 완결된40단계와 공유 문구2개를 포함한39키를 모두 비교한다.
test('튜토리얼 안내: CP1371의40단계·39키와 불변 목록이 일치한다', () => {
    assert.equal(expected.length, 40); assert.deepEqual(TUTORIAL_OBJECTIVES, expected);
    assert.equal(new Set(TUTORIAL_OBJECTIVES.map((entry) => `${entry.tutorial}:${entry.stage}`)).size, 40);
    assert.equal(new Set([...TUTORIAL_OBJECTIVES.map((entry) => entry.english), TUTORIAL_WAITING]).size, 39);
    assert.equal(TUTORIAL_WAITING, 'Wait for the next tutorial message.');
    assert.ok(Object.isFrozen(TUTORIAL_OBJECTIVES));
    // 같은 실제 gate 필드를 읽고 새 진행 상태를 만들지 않는지 각 단계마다 검사한다.
    for (const entry of expected) {
        assert.ok(Object.isFrozen(TUTORIAL_OBJECTIVES.find((value) => value.tutorial === entry.tutorial && value.stage === entry.stage)));
        const gate = Object.freeze({ ...createMissionFrameGateState(), started: 1, tutorialNumber: entry.tutorial, stage: entry.stage, lockFrames: 0 });
        assert.equal(tutorialObjectiveKey(`Tutorial${entry.tutorial}`, gate), entry.english);
        assert.equal(tutorialObjectiveKey(`tUTORIAL${entry.tutorial}`, gate), entry.english);
        assert.equal(tutorialObjectiveKey(`Tutorial${entry.tutorial}`, Object.freeze({ ...gate, lockFrames: 1 })), TUTORIAL_WAITING);
    }
});

// 타이머는 Tutorial2 F/H와 Tutorial5 C의 예약만 뜻하며 다른 단계는 같은 값에도 행동 문구를 유지한다.
test('튜토리얼 안내: 실제 예약·타이머와 범위 제외가 진행 값을 바꾸지 않는다', () => {
    //40단계의 양수/음수/NaN 타이머는 CP의 !=0 분기를 그대로 따라야 한다.
    for (const entry of expected) {
        const timed = (entry.tutorial === 2 && ['F', 'H'].includes(String.fromCharCode(entry.stage))) || (entry.tutorial === 5 && entry.stage === 67);
        for (const timerSeconds of [0, -0, 4.5, -1, NaN]) {
            const gate = Object.freeze({ ...createMissionFrameGateState(), started: 1, tutorialNumber: entry.tutorial, stage: entry.stage, lockFrames: 0, timerSeconds });
            assert.equal(tutorialObjectiveKey(`Tutorial${entry.tutorial}`, gate), timed && timerSeconds !== 0 ? TUTORIAL_WAITING : entry.english);
            assert.equal(gate.timerSeconds, timerSeconds);
        }
    }
    const gate = Object.freeze({ ...createMissionFrameGateState(), started: 1, tutorialNumber: 1, lockFrames: 0 });
    // 공식 이름과 번호가 맞지 않거나 데모·편집·완료 상태면 문구를 내지 않는다.
    for (const mission of ['Campaign1', 'Tutorial2', 'Tutorial01', 'Tutorial1 ', 'Skirmish']) assert.equal(tutorialObjectiveKey(mission, gate), '');
    for (const patch of [{ started: 0 }, { demo: 1 }, { tutorialNumber: 0 }, { tutorialNumber: 7 }, { stage: 71 }, { stage: 64 }]) assert.equal(tutorialObjectiveKey('Tutorial1', Object.freeze({ ...gate, ...patch })), '');
    assert.equal(tutorialObjectiveKey('Tutorial1', gate, 1), '');
    assert.throws(() => tutorialObjectiveKey('Tutorial1', null), /실제 gate/);
});

// 배포되는 한국어 표에도 각 영어 키가 한 번씩 있고 공용 번역 문구와 정확히 대응해야 한다.
test('튜토리얼 안내:39개 한국어 키가 공개 자산까지 누락·중복 없이 대응한다', () => {
    const source = fs.readFileSync(path.join(REPO_ROOT, 'webpj/translations/xlat.korean'), 'utf8');
    const published = fs.readFileSync(path.join(REPO_ROOT, 'webpj/www/data/d/xlat.korean'), 'utf8');
    const canonical = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'assets/lang/korean/xlat.korean'), 'utf8'));
    const translated = new XlatTable(source), english = new XlatTable('');
    assert.equal(published, source);
    const keys = new Set([...expected.map((entry) => entry.english), TUTORIAL_WAITING]);
    // Xlat의 중복 덮어쓰기에 가려지지 않도록 원문 블록의 출현 횟수도 확인한다.
    for (const key of keys) {
        assert.equal(source.replaceAll('\r', '').split(`\n${key}\n`).length - 1, 1, key);
        assert.ok(translated.has(key), key); assert.ok(canonical.has(key), key);
        assert.equal(translated.translate(key), canonical.translate(key));
        assert.notEqual(translated.translate(key), key); assert.equal(english.translate(key), key);
    }
});
