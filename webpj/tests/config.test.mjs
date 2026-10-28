// 설정 객체 계층(www/assets/config.js)을 원본 x86 기계어에서 만든 기대값과 대조해 검사한다.
// 기대값 파일: cpppj/tests/fixtures/config-x86.tsv (cpppj 의 tools/decomp_config_oracle.py 가 두 판본 실행 결과가 같은 입력만 기록, 읽기 전용으로만 쓴다).
// 이 검사는 조회(Get)·치환(Subst)·섹션 검색(FindSection)·섹션 본문(Section) 행을 쓰고, 값 쓰기(Set)·줄 삭제(Remove) 행은 웹 판본이 아직 옮기지 않았으므로 건너뛴다.
// 행 수를 고정해 두어 cpppj 쪽 fixture 가 바뀌면 이 검사가 실패해 다시 확인하게 한다. 그 밖에 실제 원본 설정 자료(config.english·setup.cfg)의 치환도 확인한다.
// (2026-10-28 WB02-D: 설정 계층 이식 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { Config, ConfigRegistry, asciiLower, findSection, sectionRange } from '../www/assets/config.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';

// 원본 x86 기대값 파일의 경로이다. (2026-10-28 WB02-D)
const FIXTURE_PATH = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'config-x86.tsv');

// 이 검사가 쓰는 행 종류별 고정 개수이다. 파일이 바뀌어 개수가 달라지면 실패한다. (2026-10-28 WB02-D)
const EXPECTED_ROWS = Object.freeze({ Get: 77, Subst: 495, FindSection: 1045, Section: 408 });

// 기대값 생성 도구의 getenv 대체와 같은 고정 환경 변수 표이다(이름은 ASCII 대소문자 무시). (2026-10-28 WB02-D)
const ORACLE_ENVIRONMENT = new Map([['nsenv', 'env{A}val'], ['nsempty', '']]);

// 16진수 칸을 바이트 하나가 문자 하나인 문자열로 바꾼다. '-' 는 빈 문자열이다. (2026-10-28 WB02-D)
function unhex(field) {
    return field === '-' ? '' : Buffer.from(field, 'hex').toString('latin1');
}

// 한 입력의 설정 객체 묶음을 읽어 만든다. 칸 순서는 `개수 {이름 버퍼 환경}×개수`이며 이름 '~' 는 이름 없음, 버퍼 '~' 는 없음, '@n' 은 Def 줄의 버퍼다.
// 반환: { registry, objects, next }(next 는 다음에 읽을 칸 위치). (2026-10-28 WB02-D)
function readStack(tokens, start, defined) {
    const registry = new ConfigRegistry({ environment: (name) => ORACLE_ENVIRONMENT.get(asciiLower(name)) ?? null });
    const count = Number(tokens[start]);
    const objects = [];
    let cursor = start + 1;
    // 객체를 등록 순서대로 만든다.
    for (let i = 0; i < count; i += 1) {
        const [name, text, environment] = tokens.slice(cursor, cursor + 3);
        cursor += 3;
        const config = new Config(registry, name === '~' ? null : unhex(name));
        if (text !== '~') config.assign(text[0] === '@' ? defined.get(text.slice(1)) : unhex(text));
        config.environmentEnabled = Number(environment) !== 0;
        objects.push(config);
    }
    return { registry, objects, next: cursor };
}

// 원본 x86 기대값 파일을 읽어 한 줄씩 대조한다. 실패한 행은 번호와 입력을 모아 한 번에 보고한다.
test('설정 객체 층·치환·섹션 검색이 원본 x86 기대값 2,025행과 같다', (t) => {
    if (!fs.existsSync(FIXTURE_PATH)) {
        t.skip('cpppj/tests/fixtures/config-x86.tsv 가 없는 환경');
        return;
    }
    const defined = new Map();
    const counts = { Get: 0, Subst: 0, FindSection: 0, Section: 0 };
    const failures = [];
    // 줄마다 한 입력을 읽어 실행한다. 주석(#)과 빈 줄은 건너뛴다.
    for (const [index, line] of fs.readFileSync(FIXTURE_PATH, 'utf8').split('\n').entries()) {
        if (line.length === 0 || line[0] === '#') continue;
        const tokens = line.trim().split(/\s+/);
        const kind = tokens[0];
        if (kind === 'Def') {
            defined.set(tokens[1], unhex(tokens[2]));
            continue;
        }
        if (kind === 'Remove' || kind === 'Set') continue;
        const row = `행 ${index + 1} ${kind}`;
        if (kind === 'Subst' || kind === 'Get') {
            const self = Number(tokens[1]);
            const stack = readStack(tokens, 2, defined);
            if (kind === 'Subst') {
                const actual = stack.objects[self].substitute(unhex(tokens[stack.next]));
                const expected = unhex(tokens[stack.next + 1]);
                if (actual !== expected) failures.push(`${row}: 입력 ${JSON.stringify(unhex(tokens[stack.next]))} 기대 ${JSON.stringify(expected)} 실제 ${JSON.stringify(actual)}`);
            } else {
                const key = unhex(tokens[stack.next]);
                const found = Number(tokens[stack.next + 1]) !== 0;
                const expected = unhex(tokens[stack.next + 2]);
                const value = stack.objects[self].get(key);
                if ((value !== null) !== found || (value ?? '') !== expected) failures.push(`${row}: 키 ${JSON.stringify(key)} 기대 ${found}/${JSON.stringify(expected)} 실제 ${JSON.stringify(value)}`);
            }
        } else if (kind === 'FindSection') {
            const [, textField, headerField, wantName, expectedField, expectedName] = tokens;
            const matched = { value: '' };
            const found = findSection(unhex(textField), 0, unhex(headerField), Number(wantName) !== 0 ? matched : null);
            if (found !== Number(expectedField)) failures.push(`${row}: 기대 위치 ${expectedField} 실제 ${found}`);
            else if (found >= 0 && Number(wantName) !== 0 && matched.value !== unhex(expectedName)) failures.push(`${row}: 기대 이름 ${JSON.stringify(unhex(expectedName))} 실제 ${JSON.stringify(matched.value)}`);
        } else if (kind === 'Section') {
            const [, textField, nameField, expectedStart, expectedLength] = tokens;
            // 버퍼는 첫 NUL 에서 끝나는 C 문자열처럼 다루므로 객체에 넣어 같은 경로로 조회한다.
            const registry = new ConfigRegistry();
            const config = new Config(registry);
            config.assign(unhex(textField));
            const range = sectionRange(config.text, unhex(nameField));
            const actualStart = range?.start ?? -1;
            const actualLength = range?.length ?? 0;
            if (actualStart !== Number(expectedStart) || actualLength !== Number(expectedLength)) failures.push(`${row}: 기대 ${expectedStart}/${expectedLength} 실제 ${actualStart}/${actualLength}`);
        } else {
            failures.push(`${row}: 알 수 없는 행 종류`);
            continue;
        }
        counts[kind] += 1;
    }
    assert.deepEqual(failures.slice(0, 10), []);
    assert.deepEqual(counts, EXPECTED_ROWS);
});

// 실제 원본 자료(TAFF 의 config.english 와 setup.cfg)로 용어 치환과 기본값·찾지 못함 표시·이스케이프를 확인한다.
test('원본 config.english 용어 치환과 설정 층 우선순위', () => {
    const archive = new TaffArchive(readData('netstorm.tarc'));
    const registry = new ConfigRegistry();
    // 정적 초기화 순서: 용어표가 먼저, 전역 설정이 나중에 등록되어 전역 설정이 우선한다.
    const terms = new Config(registry);
    terms.loadBytes(archive.read('\\d\\config.english'), decodeOriginalText);
    const global = new Config(registry);
    global.loadBytes(readData('d/setup.cfg'), decodeOriginalText);
    assert.equal(global.substitute('Build {a factory} and a {FACTORY}.'), 'Build a Workshop and a Workshop.');
    assert.equal(global.substitute('{vortex}|{vortices}|{technology}|{priest}'), 'Temple|Temples|Knowledge|High Priest');
    // 설정에 없는 키는 `{키}`(대문자)로 남고, 기본값이 있으면 기본값이 쓰인다.
    assert.equal(global.substitute('{nothing here}'), '{NOTHING HERE}');
    assert.equal(global.substitute('{nothing|fallback {factory}}'), 'fallback Workshop');
    // 이스케이프: `n 은 줄바꿈, `{ 는 글자 그대로다.
    assert.equal(global.substitute('a`nb`{c`}'), 'a\nb{c}');
    // setup.cfg 의 스타일 코드와 경로 지정값(local 객체의 인자 치환).
    assert.equal(global.substitute('{H2}|{Text}|{CText}'), '~w|~w|~i');
    const local = new Config(registry, 'local');
    local.append('1="tell"');
    assert.equal(global.substitute('{missionSpec}'), '\\D\\tell.english');
    assert.equal(global.substitute('{guideSpec}'), '\\D\\offical*.english');
    local.dispose();
    assert.equal(global.substitute('{missionSpec}'), '\\D\\{LOCAL.1}.english');
    // 인코딩된 설정 파일의 첫 줄은 서명(mQdsT) 때문에 죽은 줄이다. setup.cfg 는 서명 뒤가 줄바꿈이라 영향이 없다.
    assert.equal(global.get('gamemaster'), '10');
    assert.equal(global.get('gameminor'), '78');
});
