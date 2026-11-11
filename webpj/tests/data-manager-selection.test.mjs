// DataManager 섹션 검색/선택(cpppj `o/RawDataManagerSelection`, 원본 0044ce60 · 0044ceb0)의 웹 대응 `core/raw-data-manager-selection.js` 를 검사한다(4-B, WB04-E282).
//  - 고정 관측 `datamanager-selection-x86.tsv` 864행: 전체 256BYTE(manager 12 + 노드 3개 76BYTE 간격) 위에서 검색(F)과 선택(S)을 이름 3개(sTaTe·TERRITORY·missing)·쓰기 허용·4개 변이(비교 중 current 변경 · 비교 결과 강제 · 진단 뒤 수리)로 실행해
//    실행 뒤 전체 메모리·반환 핸들·비교/진단 호출의 순서와 그때의 전체 메모리를 대조한다.
//  - 손 시험(cpppj DataManagerSelection_PhysicalContractsAndAsciiNames): 이름 비교 규칙(대소문자 무시·NUL 이전·접두 길이), 12BYTE 미만 manager 거부(불변), 빈 목록 검색, 진단 장치 누락 거부, 비ASCII 거부.
//    CampaignTemplates_* 시험은 CampaignTemplateStorage(4-D)가 이 모듈을 쓸 때 옮긴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E282)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import {
    DATA_MANAGER_SELECTION_ASSERTION, compareDataManagerSectionNames, findDataManagerSection, selectDataManagerSection,
} from '../www/core/raw-data-manager-selection.js';

// 저장소 안의 고정 관측 파일이다. (2026-11-11 WB04-E282)
const FIXTURE = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures/datamanager-selection-x86.tsv');

// 최초 원문에 사용한 불투명 노드 주소와 한 입력의 노드 간격(60BYTE + 양쪽 보호 8BYTE)이다. (2026-11-11 WB04-E282)
const NODES = 0x12100100;
const STRIDE = 76;

// 소문자 16진수 문자열 ↔ 바이트 배열. (2026-11-11 WB04-E282)
const hexBytes = (hex) => Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
const bytesHex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 비정렬 DWORD 읽기/쓰기이다. (2026-11-11 WB04-E282)
const get = (bytes, at) => (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 예외가 나는지 본다. (2026-11-11 WB04-E282)
function throws(action, type = Error) {
    try {
        action();
        return false;
    } catch (error) {
        return error instanceof type;
    }
}

// 원본 864입력의 전체 256BYTE·반환·비교/진단 순서가 같다. 기대값은 fixture 에서만 읽는다. (2026-11-11 WB04-E282)
test('DataManager 섹션 검색/선택: 원본 10.78 864입력의 전체 256BYTE·반환 핸들·비교/진단 순서가 같다', () => {
    const rows = readFileSync(FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '').map((line) => line.split('\t'));
    assert.equal(rows.length, 865);
    assert.equal(rows[0][0], 'input');
    let count = 0;
    // 각 행은 독립적으로 관측한 원본 전체 메모리를 같은 저장소에 적용한다.
    for (const row of rows.slice(1)) {
        assert.equal(row.length, 5);
        const input = row[0].split(',');
        assert.equal(input.length, 7);
        const bytes = hexBytes(row[1]);
        assert.equal(bytes.length, 256);
        const manager = bytes.subarray(8, 20);
        const variant = Number(input[6]);
        const query = Number(input[3]);
        const trace = [];
        let comparisons = 0;
        const hooks = {
            // 같은 입력 노드의 실제 60BYTE 만 빌린다.
            stream: (handle) => {
                const index = (handle - NODES - 8) / STRIDE;
                if (!Number.isInteger(index) || index < 0 || index >= 3) throw new RangeError('검색 원문 노드 범위');
                return bytes.subarray(28 + index * STRIDE + 8, 28 + index * STRIDE + 8 + 60);
            },
            // 비교 결과는 공식 ASCII CRT 경계이며 원본 판단 본문을 대체하지 않는다.
            compare: (left, right) => {
                trace.push(`C,${bytesHex(Uint8Array.from(right, (char) => char.charCodeAt(0)))},${bytesHex(bytes)}`);
                comparisons += 1;
                if ((variant === 1 || variant === 2) && comparisons === 1) put(manager, 8, NODES + 2 * STRIDE + 8);
                return variant === 1 && comparisons === 1 ? 0 : compareDataManagerSectionNames(left, right);
            },
            // 원본 관측에 명시한 진단 수리 입력만 적용한다.
            assertion: (kind) => {
                const line = kind === DATA_MANAGER_SELECTION_ASSERTION.missingCurrent ? 0x68 : 0x6a;
                trace.push(`A,${line},${bytesHex(bytes)}`);
                if (line === 0x68 || variant === 3) put(manager, 8, NODES + 2 * STRIDE + 8);
            },
        };
        const names = ['sTaTe', 'TERRITORY', 'missing'];
        const result = input[0] === 'F'
            ? findDataManagerSection(manager, names[query], hooks)
            : selectDataManagerSection(manager, names[query], Number(input[5]) | 0, hooks);
        assert.equal(result, Number(row[3]), `반환 ${row[0]}`);
        assert.equal(bytesHex(bytes), row[2], `메모리 ${row[0]}`);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[4], `순서 ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 864);
});

// 이름 비교 규칙·물리 범위·누락 장치 거부(cpppj DataManagerSelection_PhysicalContractsAndAsciiNames). (2026-11-11 WB04-E282)
test('DataManager 섹션 이름 비교와 물리 계약: ASCII 대소문자 무시·NUL 이전·12BYTE 미만 거부·진단 장치 누락 거부', () => {
    assert.equal(compareDataManagerSectionNames('STATE', 'state'), 0);
    assert.equal(compareDataManagerSectionNames('State\0ignored', 'state'), 0);
    assert.ok(compareDataManagerSectionNames('State', 'State0') < 0);
    assert.ok(compareDataManagerSectionNames('Territory', 'State') > 0);
    assert.equal(compareDataManagerSectionNames('', ''), 0);
    assert.equal(compareDataManagerSectionNames('a', ''), 1);
    assert.equal(compareDataManagerSectionNames('', 'a'), -1);
    const manager = new Uint8Array(12).fill(0xa5);
    const before = manager.slice();
    assert.ok(throws(() => findDataManagerSection(manager.subarray(0, 11), 'State', {}), RangeError));
    assert.deepEqual(manager, before);
    put(manager, 4, 0);
    assert.equal(findDataManagerSection(manager, 'State', {}), 0);
    assert.equal(get(manager, 8), 0);
    // 현재 노드가 없으면 진단이 필요한데 장치가 없으므로 거부하고 current 는 0 으로 남는다.
    assert.ok(throws(() => selectDataManagerSection(manager, 'State', 1, {})));
    assert.equal(get(manager, 8), 0);
    // 128 이상의 글자는 CRT 로캘 의존이라 복원하지 않고 거부한다.
    assert.ok(throws(() => compareDataManagerSectionNames('\xc0', '\xe0'), RangeError));
    // 소문자 변환은 A~Z 만 바꾼다: '@'(0x40)·'['(0x5b)는 '`'(0x60)·'{'(0x7b)와 다르다.
    assert.ok(compareDataManagerSectionNames('@', '`') < 0);
    assert.ok(compareDataManagerSectionNames('[', '{') < 0);
    assert.equal(compareDataManagerSectionNames('Z', 'z'), 0);
    assert.equal(compareDataManagerSectionNames('A', 'a'), 0);
});

// 계약: 노드 조회 장치 누락·짧은 노드·이름 NUL 누락은 성공으로 바꾸지 않고 거부하며, 비교 장치가 없으면 공식 비교를 쓴다. (2026-11-11 WB04-E282)
test('DataManager 섹션 검색 계약: 노드 조회 장치/60BYTE/이름 NUL 누락 거부와 기본 비교', () => {
    // 노드 하나짜리 목록: head=h1, 이름 'State'.
    const make = () => {
        const manager = new Uint8Array(12);
        const node = new Uint8Array(60);
        node.set([0x53, 0x74, 0x61, 0x74, 0x65], 16);
        put(manager, 4, 1);
        return { manager, node };
    };
    const { manager, node } = make();
    assert.equal(findDataManagerSection(manager, 'STATE', { stream: () => node }), 1);
    assert.equal(get(manager, 8), 1);
    // 이름이 다르면 next(0)로 끝나 current 0.
    assert.equal(findDataManagerSection(manager, 'Other', { stream: () => node }), 0);
    assert.equal(get(manager, 8), 0);
    // 조회 장치가 없으면 거부한다.
    assert.ok(throws(() => findDataManagerSection(make().manager, 'State', {})));
    // 노드가 60BYTE 미만이면 거부한다.
    assert.ok(throws(() => findDataManagerSection(make().manager, 'State', { stream: () => new Uint8Array(59) }), RangeError));
    // 이름 40BYTE 안에 NUL 이 없으면 거부한다.
    const full = new Uint8Array(60).fill(0x41);
    assert.ok(throws(() => findDataManagerSection(make().manager, 'State', { stream: () => full }), RangeError));
    // 선택: 쓰기 허용(비영)에서는 버퍼/끝이 비어도 진단 없이 pos 만 0 으로 쓴다.
    const target = make();
    put(target.node, 4, 77);
    assert.equal(selectDataManagerSection(target.manager, 'State', 5, { stream: () => target.node }), 1);
    assert.equal(get(target.node, 4), 0);
    // 읽기 선택에서 버퍼나 끝이 0 이면 진단이 한 번 나가고, 그 뒤에도 pos 를 0 으로 쓴다.
    let diagnoses = 0;
    put(target.node, 4, 9);
    selectDataManagerSection(target.manager, 'State', 0, { stream: () => target.node, assertion: (kind) => { assert.equal(kind, DATA_MANAGER_SELECTION_ASSERTION.emptyRead); diagnoses += 1; } });
    assert.equal(diagnoses, 1);
    assert.equal(get(target.node, 4), 0);
    // 이름 40BYTE 중 마지막 BYTE(39번째)가 NUL 인 39글자 이름은 읽히고, 40글자 이름(NUL 없음)은 거부한다.
    const longest = make();
    longest.node.fill(0x42, 16, 55);
    longest.node[55] = 0;
    assert.equal(findDataManagerSection(longest.manager, 'B'.repeat(39), { stream: () => longest.node }), 1);
    longest.node[55] = 0x42;
    assert.ok(throws(() => findDataManagerSection(make().manager, 'B', { stream: () => longest.node }), RangeError));
    // 읽기 선택의 진단이 필요한데 장치가 없으면 거부하고 pos 를 쓰지 않는다.
    const unwired = make();
    put(unwired.node, 4, 9);
    assert.ok(throws(() => selectDataManagerSection(unwired.manager, 'State', 0, { stream: () => unwired.node })));
    assert.equal(get(unwired.node, 4), 9);
    // 버퍼와 끝이 모두 채워진 읽기 선택은 진단이 없다.
    put(target.node, 0, 0x1000);
    put(target.node, 8, 20);
    selectDataManagerSection(target.manager, 'State', 0, { stream: () => target.node, assertion: () => { throw new Error('예상 밖 진단'); } });
});
