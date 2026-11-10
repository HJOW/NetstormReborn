// 설정 층(cpppj Config.cpp·ConfigInterface.cpp)의 4-A 마감 검사(WB04-E268).
//  - original-x86.tsv 의 Config 270행(원본 기계어의 키 조회 findKey·원시 값 getRaw): 오프셋과 값이 같다. 같은 파일의 FrameFind 1536행은 rift-type-frames.test.mjs.
//  - ConfigInterface 의 순수 규칙: ConfigParseLong(CRT atol 감김)·ConfigListItem(`;` 목록)·타입 접근자 GetInt/ReadInt/ReadString/IntEquals 의 원본 quirk.
// 웹의 ConfigInterface 대응은 GameData(game-data.js)이며 설정 파일 위치 탐색·FILE 읽기·`*.tarc` 등록·로그는 브라우저 자료 조회로 대체한 adapt 경계다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E268)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { configParseLong, findKey, getRaw } from '../www/assets/config.js';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { configListItem } from '../www/core/raw-mission-ai-settings.js';

const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'original-x86.tsv'));
const configRows = rows ? rows.filter((row) => row[0] === 'Config') : null;

// 같은 버퍼·키에서 키 오프셋과 원시 값이 원본 관측과 같다(missing 은 값이 없음, hex: 는 원시 바이트).
test('Config: 원본 x86 키 조회·원시 값 270행이 같다', { skip: !configRows && '원본 fixture 없음' }, () => {
    let count = 0;
    let missing = 0;
    // 행마다 버퍼(16진), 키(16진), 기대 오프셋, 기대 값을 읽는다.
    for (const row of configRows) {
        assert.equal(row.length, 5);
        const text = Buffer.from(row[1], 'hex').toString('latin1');
        const key = Buffer.from(row[2], 'hex').toString('latin1');
        const expectedOffset = Number(row[3]);
        const value = getRaw(text, key);
        assert.equal(findKey(text, key), expectedOffset, row.join(' ').slice(0, 120));
        if (row[4] === 'missing') {
            assert.equal(value, null);
            missing += 1;
        } else {
            assert.ok(row[4].startsWith('hex:'));
            assert.equal(Buffer.from(value, 'latin1').toString('hex'), row[4].slice(4));
        }
        count += 1;
    }
    assert.equal(count, 270);
    assert.ok(missing > 0 && missing < count);
});

// 원본 CRT atol 과 `;` 목록 함수의 경계(ConfigInterface_ParseLongAndListItem_FollowOriginalRules).
test('ConfigInterface: ConfigParseLong 과 ConfigListItem 은 원본 규칙을 따른다', () => {
    assert.equal(configParseLong(''), 0);
    assert.equal(configParseLong('  \t-12abc'), -12);
    assert.equal(configParseLong('+7'), 7);
    assert.equal(configParseLong('x1'), 0);
    // 넘침 검사 없이 32비트로 감긴다.
    assert.equal(configParseLong('4294967297'), 1);
    assert.equal(configListItem('a;b;;d', 0), 'a');
    assert.equal(configListItem('a;b;;d', 1), 'b');
    assert.equal(configListItem('a;b;;d', 2), '');
    assert.equal(configListItem('a;b;;d', 3), 'd');
    assert.equal(configListItem('a;b;;d', 4), '');
    assert.equal(configListItem('a;b', -1), '');
});

// 타입 접근자의 원본 quirk(ConfigInterface_TypedAccessors_KeepOriginalQuirks): 같은 값이면 거짓, 키 없음은 거짓, 0 과의 비교는 항상 거짓.
test('ConfigInterface: GetInt·ReadInt·ReadString·IntEquals 의 원본 quirk', () => {
    const files = new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]);
    const data = GameData.build({ files, language: 'english' });
    data.set('SCREENW', '1024');
    data.set('currentLanguage', 'german');
    data.set('seven', ' 7x');
    data.set('zero', '0');
    // ReadInt: 찾았고 지금 값과 다르면 새 값, 같은 값이면 거짓, 키 없음은 거짓이며 값은 그대로이다.
    assert.deepEqual(data.readInt('SCREENW', 640), { changed: true, value: 1024 });
    assert.deepEqual(data.readInt('SCREENW', 1024), { changed: false, value: 1024 });
    assert.deepEqual(data.readInt('nokey', 1024), { changed: false, value: 1024 });
    // ReadString: 대소문자를 구분하는 바이트 비교이다.
    assert.deepEqual(data.readString('currentLanguage', 'english'), { changed: true, value: 'german' });
    assert.deepEqual(data.readString('currentLanguage', 'german'), { changed: false, value: 'german' });
    assert.deepEqual(data.readString('currentLanguage', 'German'), { changed: true, value: 'german' });
    // IntEquals: 지역 변수 0 에서 시작해 ReadInt 가 값을 바꿨고 그 값이 같을 때만 참이다(그래서 0 과의 비교는 항상 거짓).
    assert.equal(data.intEquals('seven', 7), true);
    assert.equal(data.intEquals('seven', 8), false);
    assert.equal(data.intEquals('zero', 0), false);
    assert.equal(data.intEquals('nokey', 0), false);
    // GetInt: 값이 없으면 빈 문자열을 읽어 0 이다.
    assert.equal(data.getInt('nokey'), 0);
    assert.equal(data.getInt('SCREENW'), 1024);
    assert.equal(data.getInt('seven'), 7);
});
