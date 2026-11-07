// 이식 대응표(webpj/PORT_MAP.tsv)와 도구(tools/port-map.mjs)를 검사한다.
//  1) 형식·규칙: 열 이름, 행마다 분류가 도구 규칙과 같고 메모 접두가 규칙 ID 이며, 상태·웹 모듈·base 조건을 지킨다(git 없이 확인할 수 있는 부분).
//  2) 분류 표본: 단독 경로·경계 판정이 문서(docs/webpj-port-map-20261106.md)와 같은지 대표 단위로 고정한다.
//  3) 완전성: 기준선 cpppj 커밋의 o/·client/·zacket/ 단위와 행이 일대일이고 미분류가 0이며, 모든 웹 모듈이 대응되거나 웹 전용으로 표시된다(git 기록 필요).
//  4) 판정 보존: --write 의 재구성이 사람 칸(web·base·status·메모)을 보존한다.
// (2026-11-06 WB02-G: LEFT_JOBS.webpj.md 3.2절 순서1의 완료 기준을 검사로 고정하기 위해 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { REPO_ROOT } from './helpers.mjs';
import { BASELINE, CLASSES, COLUMNS, RULES, STATUSES, checkMap, classify, formatMap, listUnits, listWebModules, parseMap, readMap, rebuildRows, reviewedBase, splitNote, unitKey } from '../tools/port-map.mjs';

// 현재 대응표이다. (2026-11-06 WB02-G)
const map = readMap();

// 기준선 커밋을 git 기록에서 읽을 수 있는지 본다(얕은 복제 등에서는 완전성 검사를 건너뛴다). (2026-11-06 WB02-G)
function hasBaseline() {
    try {
        execFileSync('git', ['cat-file', '-e', `${BASELINE}^{commit}`], { cwd: REPO_ROOT, stdio: 'ignore' });
        return true;
    } catch {
        return false;
    }
}

// 형식·규칙: git 없이 확인할 수 있는 모든 조건이 지켜진다.
test('이식 대응표: 형식·분류 규칙·상태 조건', () => {
    assert.deepEqual(map.header, [...COLUMNS]);
    assert.ok(map.rows.length > 900, `행 수 ${map.rows.length}`);
    const { errors, counts } = checkMap(map, null);
    assert.deepEqual(errors, []);
    // 다섯 분류와 상태가 모두 정의 안의 값이다.
    assert.deepEqual(Object.keys(counts.class).filter((name) => !CLASSES.includes(name)), []);
    assert.deepEqual(Object.keys(counts.status).filter((name) => !STATUSES.includes(name)), []);
    // 규칙 ID 는 겹치지 않는다.
    const ids = RULES.map((rule) => rule.id);
    assert.equal(new Set(ids).size, ids.length);
});

// 분류 표본: 경계 판정을 대표 단위로 고정한다. 규칙을 바꾸면 이 표와 문서 3절을 함께 고친다.
test('이식 대응표: 단독 경로와 웹 제외 경계의 대표 분류', () => {
    const expected = {
        'client/LocalTcpTransport': 'exclude-mp', // LAN TCP
        'client/CompositeMissionTransport': 'port', // 단독도 InternalMission 역할로 바인딩(UberGump.cpp:852)
        'client/ClientCampaignTransport': 'port',
        'client/CampaignTransportServices': 'port',
        'zacket/LocalMissionTransport': 'port', // zlocal 양끝
        'client/RawCampaignSessionLogin': 'port', // .inc 도 별도 행
        'o/RawMissionLogin': 'port', // 단독 미션 로그인
        'o/PacketRegistrationTable': 'port',
        'o/RawMetaZoneCleanup': 'port', // State 정리에서 단독도 호출
        'o/RawOutpostLifecycle': 'exclude-mp',
        'o/RawSavedStateRead': 'exclude-mp',
        'client/HtmlZsend': 'exclude-mp',
        'client/UberEditor': 'exclude-edit', // UberGump.cpp 가 포함하는 .inc 지만 Edit 전용
        'client/UberHud': 'port',
        'o/RawFortDeckLoad': 'exclude-edit', // 기존 Editor Deck
        'o/RawFortWorldLoad': 'port', // 미션 적재
        'o/RawCampaignWorldLoad': 'port',
        'o/RawFortImage': 'port', // 로그인 sendFort 직렬화
        'client/TellMapMenus': 'exclude-edit',
        'client/GpuPresenter': 'exclude-host',
        'client/CdPresence': 'exclude-host',
        'client/ScreenModeTransition': 'exclude-host',
        'client/Screen': 'adapt',
        'client/ClientMain': 'adapt',
        'o/BaseFile': 'adapt',
        'client/ExternalHelpClient': 'adapt',
        'o/SidPool': 'port',
        'client/ChatWindow': 'port', // 단독 HUD Chat/Console
        'client/RawBattleLeave': 'port', // 단독 LeaveBattle
    };
    const actual = Object.fromEntries(Object.keys(expected).map((key) => [key, classify(key)?.class]));
    assert.deepEqual(actual, expected);
    // 범위 밖·비소스 경로는 단위가 아니다.
    assert.deepEqual([unitKey('cpppj/src/platform/Utf8Path.cpp'), unitKey('cpppj/src/o/CMakeLists.txt'), unitKey('cpppj\\src\\o\\SidPool.h')], [null, null, 'o/SidPool']);
});

// 기존 웹 모듈 대응: 이식을 마친 WB04 모듈과 3.6절 판정 행의 기록을 고정한다.
test('이식 대응표: 기존 웹 모듈 대응과 3.6절 판정 기록', () => {
    const row = (key) => map.rows.find((item) => unitKey(item.cpp) === key);
    // base 는 기준선이거나 그 뒤 변경을 판정한 커밋이다(2026-11-07 WB02-H 에서 9cb0547f 판정). (2026-11-06 WB02-G, 2026-11-07 WB02-H 수정)
    assert.deepEqual([row('o/SidPool').web, row('o/SidPool').status], ['www/core/sid-pool.js', 'port']);
    assert.match(row('o/SidPool').base, /^[0-9a-f]{8}$/);
    assert.equal(row('o/SquidFactory').status, 'port');
    // 감시 쌍 생성은 WB04-D5a 에서 옮겼고 Process VT 부착은 기준선도 DependFormHost 에서 명시 오류라 WB04-D5c 에서 port 로 올렸다.
    assert.equal(row('o/RawDependCreate').status, 'port');
    assert.equal(row('o/SquidProcess').status, 'port');
    // 3.6절 9곳은 판정(①/②/③)을 메모에 남겼다.
    const judged = ['o/Config', 'o/ConfigInterface', 'client/TellSection', 'o/RiftTypeTable', 'o/CanonDecoder', 'o/RawTerrainContext', 'o/RawIslandPostPop', 'o/SidPool', 'o/RawSquidContainedUnpop'];
    assert.deepEqual(judged.filter((key) => !/3\.6[①②③]/.test(splitNote(row(key).note).text)), []);
    // 대응표 base 가 fixture-manifest 의 재검토 기준으로 쓰인다.
    assert.equal(reviewedBase('cpppj/src/o/SidPool.h', map), row('o/SidPool').base);
    assert.equal(reviewedBase('cpppj/src/platform/Utf8Path.cpp', map), null);
});

// 완전성: 기준선 단위와 행이 일대일이고 모든 웹 모듈이 대응된다(git 기록이 있을 때).
test('이식 대응표: 기준선 단위 완전성과 웹 모듈 대응', { skip: !hasBaseline() && `git 기록에 기준선 ${BASELINE} 없음` }, () => {
    const units = listUnits();
    assert.ok(units && units.size === map.rows.length, `기준선 단위 ${units?.size}, 행 ${map.rows.length}`);
    assert.deepEqual(checkMap(map, units).errors, []);
    // 모든 단위가 규칙으로 분류된다(미분류 0).
    assert.deepEqual([...units.keys()].filter((key) => !classify(key)), []);
    assert.ok(listWebModules().length > 80);
});

// 판정 보존: 재구성은 사람 칸을 바꾸지 않고, 규칙 접두만 다시 쓴다.
test('이식 대응표: --write 재구성이 사람 칸을 보존한다', { skip: !hasBaseline() && `git 기록에 기준선 ${BASELINE} 없음` }, () => {
    const units = listUnits();
    const { rows, dropped } = rebuildRows(units, map.rows);
    assert.deepEqual(dropped, []);
    // 다시 만든 글이 현재 파일과 같다(현재 파일이 --write 결과 그대로).
    assert.equal(formatMap(rows), formatMap(map.rows));
    // 접두가 틀린 메모도 규칙 ID 로 고쳐지고 사람 메모는 남는다.
    const sample = { ...map.rows.find((item) => unitKey(item.cpp) === 'o/SidPool'), note: '[wrong] 사람 메모' };
    const rebuilt = rebuildRows(new Map([['o/SidPool', units.get('o/SidPool')]]), [sample]).rows[0];
    assert.deepEqual([rebuilt.note, rebuilt.web, rebuilt.status], ['[o-default] 사람 메모', sample.web, sample.status]);
    // 파서는 CRLF 와 머리말 주석을 받아들인다.
    assert.equal(parseMap(`# 주석\r\n${COLUMNS.join('\t')}\r\na\tport\t\t\ttodo\t[o-default]\r\n`).rows[0].cpp, 'a');
});
