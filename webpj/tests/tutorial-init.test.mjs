// 미션 Init(004c3290)의 설정 조회·이름 정규화·전역 쓰기·요새 로드 요청·기술 Header 순서를 원본 10.78 관측으로 대조한다(WB04-E38, cpppj TutorialInitTests 이식).
//  - tutorial-init-x86.tsv 152행: 전체 필드(flags/demo/이름 접두/튜토리얼·미션 번호/구독자), 지역 80바이트, 원래 이름, 옵션 87바이트, 큰 효과 호출 순서(C/G/O/S/L/H)와 효과가 현재 이름·구독자·flags 를 바꾸는 변형 4종.
//  - 계약: 80/256바이트 범위 거부와 누락된 효과의 거부. 실제 보호 Script 11개와 State 큐의 다음 루프 로드(MissionScript/MissionFortRequest/MissionTechnology)는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E38: Tutorial Init 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { resetMissionBattleOptions } from '../www/core/raw-mission-battle-options.js';
import { initializeTutorialMission } from '../www/core/tutorial-init.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E38)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'tutorial-init-x86.tsv'));
// 현재 C 문자열의 바이트만 원문 trace 용 16진 문자열로 옮긴다. (2026-11-08 WB04-E38)
const hexOf = (text) => Buffer.from(text, 'latin1').toString('hex');
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 명시 큰 경계의 같은 입력/변이와 원본이 남긴 현재 필드를 보유한다(cpppj InitScene). (2026-11-08 WB04-E38)
class InitScene {
    constructor(text) {
        // 입력 JSON 배열: 숫자는 문자열로 읽는다(원문 평면 입력과 같음). \u00xx 는 한 원본 바이트이다.
        this.input = JSON.parse(text).map((value) => String(value));
        assert.equal(this.input.length, 11);
        this.state = { name: this.input[0], namePrefix: 0, scratch: new Uint8Array(80).fill(0xa5) };
        this.demo = { value: number(this.input[5]) };
        this.flags = { value: number(this.input[6]) };
        this.seen = { value: number(this.input[7]) };
        this.subscriber = { value: number(this.input[8]) };
        this.source = number(this.input[9]);
        this.tutorial = { value: 0x22222222 };
        this.mission = { value: 0x11111111 };
        this.options = new Uint8Array(87);
        this.trace = [];
        // 독립 관측이 사용한 실제 87바이트 전 입력을 공급한다.
        for (let index = 0; index < this.options.length; index += 1) this.options[index] = (index * 11 + 17) & 0xff;
    }

    // 동일 Init 몸체와 실제 옵션 초기화에 입력을 연결하고 전체 출력을 직렬화한다.
    run() {
        const variant = number(this.input[10]);
        const add = (text) => this.trace.push(text);
        const hooks = {
            clearTemplates: () => { add('C'); if (variant === 1) this.state.name = 'AFTER_CLEAR'; },
            get: (key, fallback) => {
                const value = this.input[key === 'tutorialNumber' ? 1 : key === 'missionNumber' ? 2 : key === 'demo' ? 3 : 4];
                const result = value === '' && fallback !== null ? fallback : value;
                add(`G,${key},${hexOf(fallback ?? '')},${hexOf(result)}`);
                return result;
            },
            resetOptions: () => {
                add('O');
                if (variant === 2) { this.state.name = 'AFTER_OPTIONS'; this.source = 7; }
                resetMissionBattleOptions(this.options);
            },
            set: (key, value) => { add(`S,${hexOf(key)},${hexOf(value)}`); if (variant === 3) { this.state.name = 'AFTER_SET'; this.source = 0x76543210; } },
            requestFort: (value) => { add(`L,${hexOf(value)}`); if (variant === 4) this.flags.value = 0xfffffffe; },
            header: () => { add('H'); if (variant === 4) this.flags.value = (this.flags.value ^ 0x102) >>> 0; },
        };
        initializeTutorialMission(this.state, this.flags, this.demo, {
            tutorialNumber: this.tutorial, missionNumber: this.mission, tutorialSeen: this.seen, loginSubscriber: this.subscriber, initialSubscriber: () => this.source,
        }, hooks);
        const fields = [this.flags.value, this.demo.value, this.state.namePrefix, this.tutorial.value, this.mission.value, this.seen.value, this.subscriber.value].join(',');
        return [fields, Buffer.from(this.state.scratch).toString('hex'), hexOf(this.state.name), Buffer.from(this.options).toString('hex'), this.trace.join(';')];
    }
}

// 152개 첫 관측의 전체 필드/지역 80바이트/원래 이름/옵션/큰 경계 순서를 전수 비교한다.
test('Tutorial Init: 원본10.78 152행(전역·정규화 지역·원래 이름·옵션 87바이트·큰 효과 순서)', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 152);
    rows.forEach((row, rowIndex) => {
        assert.equal(row.length, 6);
        const result = new InitScene(row[0]).run();
        // 모든 별도 출력 영역은 같은 원문 열과 비교한다.
        result.forEach((value, index) => assert.equal(value, row[index + 1], `행 ${rowIndex + 1} 출력 ${index}`));
    });
});

// 범위 밖 오버런과 누락 효과를 선택 위치에서 거부하며 앞선 효과는 남긴다.
test('Tutorial Init: 이름 80바이트·로드 256바이트 초과와 효과 누락 거부', () => {
    const make = (name, load = name) => {
        const effects = [];
        const hooks = {
            clearTemplates: () => effects.push('C'),
            get: (key, fallback) => (key === 'loadFort' ? load : (fallback ?? '')),
            resetOptions: () => effects.push('O'),
            set: () => effects.push('S'),
            requestFort: () => effects.push('L'),
            header: () => effects.push('H'),
        };
        const state = { name, namePrefix: 0, scratch: new Uint8Array(80) };
        const globals = { tutorialNumber: { value: 0 }, missionNumber: { value: 0 }, tutorialSeen: { value: 0 }, loginSubscriber: { value: 0 }, initialSubscriber: () => 5 };
        return { effects, hooks, state, globals };
    };
    const flags = { value: 0 };
    const demo = { value: 0 };
    const long = make('x'.repeat(80));
    assert.throws(() => initializeTutorialMission(long.state, flags, demo, long.globals, long.hooks), RangeError);
    assert.deepEqual(long.effects, ['C', 'O']);
    const loadLong = make('Name', 'y'.repeat(256));
    assert.throws(() => initializeTutorialMission(loadLong.state, flags, demo, loadLong.globals, loadLong.hooks), RangeError);
    assert.deepEqual(loadLong.effects, ['C', 'O', 'S']);
    assert.equal(flags.value, 0);
    // 정상 경로: 첫 글자 대문자화와 구독자 복사, flags 비트.
    const ok = make('tutorial1');
    initializeTutorialMission(ok.state, flags, demo, ok.globals, ok.hooks);
    assert.deepEqual([flags.value, ok.globals.loginSubscriber.value, Buffer.from(ok.state.scratch.subarray(0, 9)).toString('latin1')], [1, 5, 'Tutorial1']);
    assert.deepEqual(ok.effects, ['C', 'O', 'S', 'L', 'H']);
    const missing = make('Name');
    delete missing.hooks.header;
    assert.throws(() => initializeTutorialMission(missing.state, { value: 0 }, demo, missing.globals, missing.hooks), /효과 누락/);
    assert.deepEqual(missing.effects, ['C', 'O', 'S', 'L']);
});
