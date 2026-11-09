// 10.78 번역 장부(raw-xlat.js: RawXlat·initializeRawXlat)를 원본 관측과 대조한다(WB04-E242).
//  - xlat-runtime-x86.tsv 2,031행: 사례 43개의 언어별 적재 45회(원문 파일 바이트 → 정규화·NUL 쓰기를 마친 전체 버퍼와 고유 키 수), 조회 1,935회(언어·키 바이트 → 결과 바이트), 시작 적재 8가지(존재 마스크 → 존재 확인/열기/닫기/9번 진단 순서).
//  - xlat-assets-x86.tsv 44행: 실제 보호 TARC 의 xlat.german/english/portuguese 97,526바이트 전체를 원문이 수정한 동일 크기 버퍼·고유 키 수와 대조하고 조회 40회를 확인한다(원본 바이트는 fixture 에 들어 있어 게임 파일을 읽지 않는다).
//  - cpppj XlatRuntimeTests 의 세 번째까지를 옮긴다. 네 번째(Client 의 언어 변경)는 client 이식, xlat-file-read fixture 는 XlatFileRead(adapt) 단위 몫이다.
//  - 웹 계약: 개별 설치/정규화/병합 단계, 재적재·길이 0·칸 범위 거부, 언어 범위와 폴백 순서, 불완전한 레코드, CR 이동.
// 입력만 설치하고 결과는 모듈이 계산한다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E242: 10.78 다중 번역 장부 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { RawXlat, initializeRawXlat, xlatBinaryString } from '../www/core/raw-xlat.js';

// 원본 관측 파일 행이다(10.78 전용). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E242)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const runtimeRows = readRows(path.join(FIXTURES, 'xlat-runtime-x86.tsv'));
const assetRows = readRows(path.join(FIXTURES, 'xlat-assets-x86.tsv'));

// 소문자 16진 문자열을 바이트열로 바꾼다("-" 는 빈 값). (2026-11-10 WB04-E242)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(Buffer.from(text, 'hex')));
// 바이트열을 원문 바이트 문자열로 바꾼 뒤 첫 NUL 앞까지 자른다(원본 시험의 `substr(0, find('\0'))`). (2026-11-10 WB04-E242)
const upToNul = (text) => (text.includes('\0') ? text.slice(0, text.indexOf('\0')) : text);
// 경로 문자열의 소문자 16진 표현이다(시작 파일 경계의 실제 인자 관측). (2026-11-10 WB04-E242)
const pathHex = (text) => Buffer.from(text, 'latin1').toString('hex');

// 사례 43개: 언어별 적재 뒤 전체 파일 버퍼·고유 키 수와 조회, 시작 적재의 존재/열기/닫기/진단 순서를 재생한다.
test('RawXlat: 원본10.78 적재 45회·조회 1935회·시작 8가지', { skip: !runtimeRows && '원본 fixture 없음' }, () => {
    let xlat = null;
    let cases = 0;
    let loads = 0;
    let queries = 0;
    let startup = 0;
    for (const row of runtimeRows) {
        const kind = row[0];
        if (kind === 'C') {
            assert.equal(Number(row[1]), cases);
            cases += 1;
            xlat = new RawXlat();
        } else if (kind === 'L') {
            assert.equal(row.length, 6);
            const which = Number(row[1]);
            assert.equal(Number(row[3]), 1);
            xlat.load(which, decode(row[2]));
            assert.equal(Buffer.from(xlat.buffer(which)).toString('hex'), row[4] === '-' ? '' : row[4], `적재 ${loads + 1} 버퍼`);
            assert.equal(xlat.size(), Number(row[5]), `적재 ${loads + 1} 고유 키 수`);
            loads += 1;
        } else if (kind === 'Q') {
            assert.equal(row.length, 4);
            const actual = upToNul(xlat.translate(xlatBinaryString(decode(row[2])), Number(row[1])));
            assert.equal(actual, xlatBinaryString(decode(row[3])), `조회 ${queries + 1} 언어 ${row[1]} 키 ${row[2]}`);
            queries += 1;
        } else {
            assert.equal(kind, 'I');
            assert.equal(row.length, 3);
            const mask = Number(row[1]);
            let trace = '';
            const table = new RawXlat();
            initializeRawXlat(table, {
                exists: (file) => {
                    if (trace) trace += ',';
                    trace += `exists:${pathHex(file)}`;
                    const bit = file === 'd/xlat.german' ? 0 : file === 'd/xlat.english' ? 1 : 2;
                    return (mask & (1 << bit)) !== 0;
                },
                read: (file) => {
                    trace += `,open:${pathHex(file)},close`;
                    return Uint8Array.of(0);
                },
                missing: () => { trace += ',log:9'; },
            });
            assert.equal(trace, row[2], `시작 마스크 ${mask}`);
            startup += 1;
        }
    }
    assert.deepEqual([cases, loads, queries, startup], [43, 45, 1935, 8]);
});

// 실제 보호 TARC 세 파일(97,526바이트)을 원문이 수정한 동일 크기 버퍼와 고유 키 수로 대조한다.
test('RawXlat: 원본10.78 실제 TARC 세 파일 97526바이트와 조회 40회', { skip: !assetRows && '원본 fixture 없음' }, () => {
    const xlat = new RawXlat();
    let loads = 0;
    let queries = 0;
    let total = 0;
    for (const row of assetRows) {
        if (row[0] === 'C') {
            assert.equal(Number(row[1]), 0);
        } else if (row[0] === 'L') {
            assert.equal(row.length, 6);
            const which = Number(row[1]);
            const source = decode(row[2]);
            assert.equal(Number(row[3]), 1);
            xlat.load(which, source);
            assert.equal(Buffer.from(xlat.buffer(which)).toString('hex'), row[4]);
            assert.equal(xlat.size(), Number(row[5]));
            total += source.length;
            loads += 1;
        } else {
            assert.equal(row[0], 'Q');
            assert.equal(upToNul(xlat.translate(xlatBinaryString(decode(row[2])), Number(row[1]))), xlatBinaryString(decode(row[3])), `언어 ${row[1]} 키 ${row[2]}`);
            queries += 1;
        }
    }
    assert.deepEqual([loads, queries, total], [3, 40, 97526]);
});

// 파일 부재 뒤 다음 언어로 진행하고 읽기 실패 전 완료한 언어 표/원문 버퍼를 보존한다.
test('RawXlat: 시작 읽기 실패는 앞서 끝난 언어를 보존한다', () => {
    const table = new RawXlat();
    const paths = [];
    assert.throws(() => initializeRawXlat(table, {
        exists: () => true,
        read: (file) => {
            paths.push(file);
            if (file === 'd/xlat.english') throw new Error('read');
            return Uint8Array.from(Buffer.from('*\nKey\n-\nGerman\n=\n'));
        },
    }), /read/);
    assert.deepEqual(paths, ['d/xlat.german', 'd/xlat.english']);
    assert.equal(table.translate('Key', 3), 'German');
    assert.equal(table.translate('Key', 6), 'Key');
    assert.ok(table.buffer(3).length > 0);
    assert.equal(table.buffer(1).length, 0);
    assert.equal(table.hasBuffer(3), true);
    assert.equal(table.hasBuffer(1), false);
});

// 개별 단계 계약: 정확 길이 설치(영으로 채움)→CR/NUL 정규화→병합을 따로 부를 수 있고, 중복 설치·길이 0 정규화/병합·칸 범위는 거부한다.
test('RawXlat: 설치·정규화·병합 단계와 거부 계약', () => {
    const table = new RawXlat();
    const installed = table.allocateFileBuffer(2, 12);
    assert.equal(installed.length, 12);
    assert.deepEqual([...installed], new Array(12).fill(0));
    assert.equal(table.buffer(2), installed); // 빌린 같은 배열이다.
    assert.throws(() => table.allocateFileBuffer(2, 4), /미정 포인터/);
    assert.throws(() => table.load(2, Uint8Array.of(1)), /원문 진단/); // 이미 설치한 칸의 재적재.
    installed.set(Buffer.from('*\r\nK\r\n-\r\nV\r\n=\r\n'.slice(0, 12)));
    // 마지막 BYTE 는 NUL 이 되고, NUL 전까지의 CR 은 하나씩 당겨져 사라지며 뒤는 NUL 로 남는다(크기는 그대로).
    table.normalizeFileBuffer(2);
    assert.equal(Buffer.from(installed).toString('latin1'), `*\nK\n-\nV${'\0'.repeat(5)}`);
    table.parseFileBuffer(2);
    assert.equal(table.size(), 0); // 끝 줄 '=' 이 없어 불완전한 레코드는 저장하지 않는다.
    const empty = new RawXlat();
    assert.equal(empty.allocateFileBuffer(4, 0).length, 0);
    assert.equal(empty.hasBuffer(4), true); // 길이 0 설치도 미설치와 구별한다.
    assert.throws(() => empty.normalizeFileBuffer(4), RangeError);
    assert.throws(() => empty.parseFileBuffer(4), RangeError);
    assert.throws(() => empty.normalizeFileBuffer(5), RangeError); // 미설치.
    assert.throws(() => empty.parseFileBuffer(5), RangeError);
    for (const which of [-1, 7, 1.5, NaN]) {
        assert.throws(() => empty.hasBuffer(which), RangeError, `칸 ${which}`);
        assert.throws(() => empty.buffer(which), RangeError);
        assert.throws(() => empty.allocateFileBuffer(which, 1), RangeError);
        assert.throws(() => empty.load(which, Uint8Array.of(0)), RangeError);
    }
    const legacy = new RawXlat();
    assert.throws(() => legacy.load(1, new Uint8Array(0)), RangeError); // 길이 0 은 마지막 BYTE 물리 범위 밖이며 설치하지 않는다.
    assert.equal(legacy.hasBuffer(1), false);
});

// 병합 계약: 같은 키의 다른 언어 값은 보존하고, 빈 번역은 등록된 값이며, 값이 없는 칸·없는 키는 원문으로 돌아가고, 언어 0·1 은 항상 원문이다.
test('RawXlat: 언어별 병합과 조회 폴백', () => {
    const table = new RawXlat();
    const german = '*\r\nKey\r\n-\r\nSchlüssel\r\n=\r\n*\r\nEmpty\r\n-\r\n\r\n=\r\n*\r\nOnlyGerman\r\n-\r\nNur\r\n=\r\n*\r\nBroken\r\n-\r\nHalf\r\n';
    table.load(3, Uint8Array.from(Buffer.from(german, 'latin1')));
    table.load(6, Uint8Array.from(Buffer.from('*\nKey\n-\nChave\n=\n', 'latin1')));
    assert.equal(table.size(), 3); // Key·Empty·OnlyGerman, 끝 줄이 없는 Broken 은 저장하지 않는다.
    assert.equal(table.translate('Key', 3), 'Schl\xfcssel'); // 원문 바이트(1252) 그대로이다.
    assert.equal(table.translate('Key', 6), 'Chave');
    assert.equal(table.translate('Key', 2), 'Key'); // 값이 없는 언어 칸.
    assert.equal(table.translate('Empty', 3), ''); // 빈 번역은 원문 대신 빈 문자열이다.
    assert.equal(table.translate('Empty', 6), 'Empty');
    assert.equal(table.translate('OnlyGerman', 6), 'OnlyGerman');
    assert.equal(table.translate('Broken', 3), 'Broken');
    assert.equal(table.translate('Missing', 3), 'Missing');
    assert.equal(table.translate('Key', 0), 'Key');
    assert.equal(table.translate('Key', 1), 'Key');
    assert.equal(table.translate('Key\0tail', 3), 'Schl\xfcssel'); // 첫 NUL 앞만 키이다.
    assert.equal(table.translate('Missing\0tail', 3), 'Missing\0tail'); // 없는 키는 입력 전체를 그대로 돌려준다.
    assert.equal(table.translate('', 3), '');
    assert.throws(() => table.translate('Key', 7), RangeError); // 키가 있으면 언어 칸 범위를 검사한다.
    assert.throws(() => table.translate('Key', -1), RangeError);
    assert.equal(table.translate('Missing', 7), 'Missing'); // 키가 없으면 범위를 보기 전에 원문을 돌려준다.
    assert.throws(() => table.translate(42, 3), TypeError);
    // 같은 언어를 다시 적재할 수는 없지만 다른 언어 값은 원 레코드에 병합된다.
    table.load(2, Uint8Array.from(Buffer.from('*\nKey\n-\nCle\n=\n', 'latin1')));
    assert.equal(table.translate('Key', 2), 'Cle');
    assert.equal(table.translate('Key', 3), 'Schl\xfcssel');
    assert.equal(table.size(), 3);
});

// 시작 적재 계약: 필요한 장치가 없으면 그 위치에서 거부하고, 부재 로그만 없을 때는 존재하는 파일을 읽은 뒤에 거부한다.
test('initializeRawXlat: 선택된 장치만 요구한다', () => {
    assert.throws(() => initializeRawXlat(new RawXlat(), {}), /존재 장치/);
    assert.throws(() => initializeRawXlat(new RawXlat(), null), /존재 장치/);
    assert.throws(() => initializeRawXlat(new RawXlat(), { exists: () => false }), /부재 로그/);
    assert.throws(() => initializeRawXlat(new RawXlat(), { exists: () => true }), /파일 읽기/);
    const table = new RawXlat();
    const seen = [];
    initializeRawXlat(table, {
        exists: (file) => { seen.push(file); return file !== 'd/xlat.english'; },
        read: () => Uint8Array.from(Buffer.from('*\nA\n-\nB\n=\n')),
        missing: (file) => seen.push(`missing:${file}`),
    });
    assert.deepEqual(seen, ['d/xlat.german', 'd/xlat.english', 'missing:d/xlat.english', 'd/xlat.portuguese']);
    assert.deepEqual([table.hasBuffer(3), table.hasBuffer(1), table.hasBuffer(6)], [true, false, true]);
    assert.equal(table.translate('A', 3), 'B');
    assert.equal(xlatBinaryString(Uint8Array.of(0, 0x80, 0x9f, 0xff)), '\0\x80\x9f\xff'); // 0x80~0x9f 도 바꾸지 않는다.
    assert.equal(xlatBinaryString(new Uint8Array(10000).fill(0x41)).length, 10000); // 묶음 경계를 넘는 길이.
});

// C 문자열 규칙: 내장 NUL 뒤는 읽지 않고 CR 도 당기지 않으며(strchr 가 NUL 에서 멈춤), 입력 배열은 복사해 설치해 호출자의 배열을 바꾸지 않는다.
test('RawXlat: 내장 NUL 과 입력 배열 복사', () => {
    const table = new RawXlat();
    const source = Uint8Array.from(Buffer.from('*\0\r\n*\nK\n-\nV\n=\n', 'latin1'));
    table.load(3, source);
    assert.equal(table.size(), 0); // 첫 줄의 키 줄에 LF 가 없고 NUL 이 먼저라 이후 레코드를 읽지 않는다.
    assert.equal(Buffer.from(table.buffer(3)).toString('latin1'), '*\0\r\n*\nK\n-\nV\n=\0'); // NUL 뒤 CR 은 그대로, 마지막 BYTE 만 NUL.
    assert.equal(Buffer.from(source).toString('latin1'), '*\0\r\n*\nK\n-\nV\n=\n'); // 호출자 배열은 정규화되지 않는다.
    source[0] = 0x41;
    assert.equal(table.buffer(3)[0], 0x2a); // 설치 뒤 호출자 배열의 변경이 보이지 않는다.
    // 종료 NUL 이 없는 버퍼를 정규화 없이 병합하면 문자열 끝을 찾지 못해 거부한다.
    const raw = new RawXlat();
    raw.allocateFileBuffer(5, 4).set(Buffer.from('AB\nC', 'latin1'));
    assert.throws(() => raw.parseFileBuffer(5), /NUL 범위/);
});

// 구분자 상태: `*` 는 처음 한 번만 키를 정하고, `-`/`=` 는 직전 상태가 맞을 때만 받아들이며, 키/번역이 여러 줄이어도 마지막 구분자 앞 NUL 까지가 본문이다.
test('RawXlat: 구분자 줄의 상태 순서와 여러 줄 키', () => {
    const load = (text) => {
        const table = new RawXlat();
        table.load(3, Uint8Array.from(Buffer.from(text, 'latin1')));
        return table;
    };
    const multi = load('*\nK\n*\nK2\n-\nV\n=\n'); // 키 상태의 두 번째 `*` 는 무시되어 본문이 이어진다.
    assert.equal(multi.size(), 1);
    assert.equal(multi.translate('K\n*\nK2', 3), 'V');
    assert.equal(multi.translate('K2', 3), 'K2');
    const early = load('x\n-\n*\nK\n-\nV\n=\n'); // 키 앞의 `-` 는 무시한다.
    assert.equal(early.size(), 1);
    assert.equal(early.translate('K', 3), 'V');
    const stray = load('=\n*\nK\n=\n-\nV\n=\n'); // 키 상태 이전/중의 `=` 는 무시한다.
    assert.equal(stray.size(), 1);
    assert.equal(stray.translate('K\n=', 3), 'V');
    const twoValues = load('*\nK\n-\nA\n-\nB\n=\n'); // 값 상태의 두 번째 `-` 도 무시한다.
    assert.equal(twoValues.translate('K', 3), 'A\n-\nB');
    // 언어 0 칸에 적재해도 조회는 항상 원문을 돌려주고, 다른 칸의 키 병합은 건드리지 않는다.
    const zero = new RawXlat();
    zero.load(0, Uint8Array.from(Buffer.from('*\nKey\n-\nX\n=\n', 'latin1')));
    assert.equal(zero.translate('Key', 0), 'Key');
    assert.equal(zero.size(), 1);
    // 문자열이 아닌 키는 문구와 함께 거부한다.
    for (const bad of [42, null, undefined, Uint8Array.of(65)]) assert.throws(() => zero.translate(bad, 3), /원문 바이트 문자열/);
});
