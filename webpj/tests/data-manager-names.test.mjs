// 원본 토큰 읽기(00487620)·SNART 이름표 선택(004a8800)·Template 구역 이름표(004bccb0)·DataManager 초기화(0044d870)를 원본 10.78 관측으로 대조한다(WB04-E19,
// cpppj ScriptCommandTests 의 Token 행·DataManagerInitializeTests 이식).
//  - script-command-x86.tsv 의 Token 128행: 소비 포인터와 토큰 바이트(인용·78 바이트 한도·구분자·뒤 공백의 첫 바이트 보존).
//  - template-section-names-x86.tsv 12행: 최초 512 할당·재사용·NULL 공개 후 실패의 전체 manager/두 표/호출 순서. data-manager-initialize-x86.tsv 64행: root 진단·이름표 1회 조회·이름별 Create·미래 이름 변경.
//  - 계약: SNART 변환의 제자리 불변식, 진단 장치 누락 거부.
// cpppj 의 Client/Storage 공유 이름표 수명 검사는 해당 Client 단위를 옮길 때 대조한다. (2026-11-08 WB04-E19: 데이터 매니저 이름표 묶음 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { readRawTextToken } from '../www/core/raw-text-token.js';
import { selectSnartName, readSnartName } from '../www/core/raw-snart.js';
import { getTemplateSectionNames } from '../www/core/raw-template-section-names.js';
import { initializeDataManager, DATA_MANAGER_INITIALIZE_ASSERTION } from '../www/core/raw-data-manager-initialize.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E19)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const scriptRows = fixture('script-command-x86.tsv');
const sectionRows = fixture('template-section-names-x86.tsv');
const initializeRows = fixture('data-manager-initialize-x86.tsv');
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본 바이트를 UTF-8 치환 없이 복원한다('-' 는 빈 문자열). (2026-11-08 WB04-E19)
const bytesOf = (encoded) => (encoded === '-' ? '' : encoded.match(/../g).map((pair) => String.fromCharCode(parseInt(pair, 16))).join(''));

// 기계어의 출력값을 읽으며 토큰 정답을 이 검사에서 다시 계산하지 않는다.
test('원본 토큰: 원본10.78 Token 128행(소비 포인터·토큰 바이트)', { skip: !scriptRows && '원본 fixture 없음' }, () => {
    const rows = scriptRows.filter((row) => row[0] === 'Token');
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 9);
        const source = bytesOf(row[1]);
        const delimiter = bytesOf(row[2]);
        const token = delimiter === 'DEFAULT' ? readRawTextToken(source) : readRawTextToken(source, 0, delimiter);
        assert.equal(token.text, bytesOf(row[4]), `행 ${count} 텍스트 ${row[1]}`);
        assert.equal(token.next, Number(row[5]), `행 ${count} 다음 위치`);
        count += 1;
    }
    assert.equal(count, 128);
});

// 한도·인용·구분자의 독립 경계 계약이다.
test('원본 토큰: 78 바이트 한도·인용·NUL·뒤 공백 보존', () => {
    // 쉼표와 뒤따르는 공백은 연속 구분자로 함께 건너뛴다.
    assert.deepEqual(readRawTextToken('  abc, def'), { text: 'abc', next: 7 });
    assert.deepEqual(readRawTextToken('"a b" c'), { text: 'a b', next: 6 });
    assert.deepEqual(readRawTextToken('x'.repeat(100)), { text: 'x'.repeat(78), next: 78 });
    assert.deepEqual(readRawTextToken('ab\0cd'), { text: 'ab', next: 2 });
    assert.deepEqual(readRawTextToken('" "'), { text: ' ', next: 3 });
    assert.throws(() => readRawTextToken('ab', 3), RangeError);
});

// 같은 SNART 표의 제자리 변환과 index 선택을 확인한다.
test('SNART 이름표: 제자리 변환과 이름 선택', () => {
    const names = Uint8Array.from(Buffer.from('/One/Two//\0'.slice(0, -1) + '\0', 'latin1'));
    let diagnostics = 0;
    assert.equal(readSnartName(names, 0, () => { diagnostics += 1; }), 'One');
    assert.equal(readSnartName(names, 1), 'Two');
    assert.equal(diagnostics, 0);
    // 변환은 첫 BYTE 와 마지막 FF 를 남긴다.
    assert.equal(names[0], 0);
    const bad = Uint8Array.from(Buffer.from('Xa/b/\0', 'latin1'));
    assert.throws(() => selectSnartName(bad, 0), /진단 미연결/);
    let called = 0;
    assert.equal(readSnartName(Uint8Array.from(Buffer.from('Xa/b//\0', 'latin1')), 0, () => { called += 1; }).length >= 0, true);
    assert.equal(called, 1);
    assert.throws(() => selectSnartName(new Uint8Array(0), 0), RangeError);
});

// Native 장치의 입력만 재생하며 초기화 판단은 제품 본문이 수행하는 장면이다(cpppj InitializeReplay). (2026-11-08 WB04-E19)
const INIT_MANAGER = 0x10001000;
const INIT_TABLE = 0x10002000;
const INIT_ALTERNATE = 0x10003000;
class InitializeReplay {
    constructor(kind, root, change, allocated = 1) {
        this.mutation = change;
        this.allocation = allocated;
        this.current = kind ? INIT_TABLE : 0;
        this.creates = 0;
        this.selected = 0;
        this.index = 0;
        this.trace = [];
        this.manager = new Uint8Array(12);
        this.table = new Uint8Array(512).fill(0xa5);
        this.alternate = new Uint8Array(512).fill(0xa5);
        put(this.manager, 0, 0x514690);
        put(this.manager, 4, root);
        put(this.manager, 8, 0xa5a5a5a5);
        const alt = '/Alt/Last//';
        for (let at = 0; at < alt.length; at += 1) this.alternate[at] = alt.charCodeAt(at);
        this.alternate[alt.length] = 0;
        if (kind === 1) {
            const raw = '/One/Two//';
            for (let at = 0; at < raw.length; at += 1) this.table[at] = raw.charCodeAt(at);
            this.table[raw.length] = 0;
        }
        if (kind === 2) this.table.set([0, 0x4f, 0x6e, 0x65, 0, 0x54, 0x77, 0x6f, 0, 0, 0xff]);
        if (kind === 3) { this.table[0] = 0x2f; this.table[1] = 0x2f; this.table[2] = 0; }
    }

    event(entry) { this.trace.push(entry); }

    // 읽기 조회 자체는 전역을 변경하지 않고 같은 물리 표를 반환한다.
    buffer(token) {
        if (token === INIT_TABLE) return this.table;
        if (token === INIT_ALTERNATE) return this.alternate;
        throw new RangeError('Native 이름표 NULL 접근');
    }

    // 실제 getter 진입과 선택된 할당/서식을 기록하며 부모 모듈을 그대로 실행한다.
    names() {
        this.event(`names:${this.current}`);
        const hooks = {
            allocate: (size) => { this.event(`allocate:${size}`); return this.allocation ? INIT_TABLE : 0; },
            buffer: (token) => this.buffer(token),
            format: (number) => { this.event(`format:${number}`); return Uint8Array.of(0x54, 0x65, 0x72, 0x72, 0x30 + Math.floor(number / 10), 0x30 + (number % 10), 0); },
        };
        return this.buffer(getTemplateSectionNames(this, hooks));
    }

    // 한 initialize 호출의 표 포인터/인덱스를 고정하되 전체 creates 는 반복 사이에 보존한다.
    initialize() {
        const hooks = {
            assertion: (kind) => { this.event(`assert:${kind}`); if (this.mutation === 1 && kind === DATA_MANAGER_INITIALIZE_ASSERTION.rootNotEmpty) this.current = INIT_ALTERNATE; },
            names: () => { const bytes = this.names(); this.selected = this.current; this.index = 0; return bytes; },
            create: (name) => {
                this.event(`at:${this.selected}:${this.index}`);
                this.index += 1;
                const base = this.buffer(this.selected);
                const address = this.selected + (name.byteOffset - base.byteOffset);
                const end = name.indexOf(0);
                assert.ok(end >= 0, '초기화 검사 실제 NUL 범위');
                this.event(`create:${INIT_MANAGER}:${address}:${hex(name.subarray(0, end))}`);
                this.creates += 1;
                put(this.manager, 4, 0x10004000);
                put(this.manager, 8, 0x10004000 + this.creates * 60);
                if (this.creates === 1 && this.mutation === 2) this.current = INIT_ALTERNATE;
                if (this.creates === 1 && this.mutation === 3) {
                    // 같은 표의 미래 항목만 명시 외부 입력으로 바꾼다.
                    for (const [before, after] of [['State\0', 'Other\0'], ['Two\0', 'Six\0']]) {
                        const found = Buffer.from(this.table).indexOf(Buffer.from(before, 'latin1'));
                        if (found >= 0) this.table.set(Buffer.from(after, 'latin1'), found);
                    }
                }
            },
        };
        initializeDataManager(this.manager, hooks);
        // 마지막 빈 항목은 Create 가 없으므로 반환 위치에서 같은 무효과 관측을 기록한다.
        this.event(`at:${this.selected}:${this.index}`);
    }

    // 실제 RET 만 실행하는 004437f0 은 효과가 없으므로 trace 의 그 한 항목만 제외한 기대 문자열이다.
    static withoutNoop(text) { return text.split(';').filter((entry) => entry !== 'baseNoop').join(';'); }
}

// 초기화 64행의 전체 manager/두 표/순서를 같은 외부 입력으로 재생한다.
test('DataManager 초기화: 원본10.78 64행(root 진단·이름표 1회 조회·Create·미래 이름 변경)', { skip: !initializeRows && '원본 fixture 없음' }, () => {
    assert.equal(initializeRows.length, 64);
    let count = 0;
    for (const row of initializeRows) {
        assert.equal(row.length, 11);
        const scene = new InitializeReplay(number(row[0]), number(row[1]), number(row[3]));
        // 각 행의 repeat 도 원래 같은 Client/manager 에서 계속한다.
        for (let call = 0; call < number(row[2]); call += 1) scene.initialize();
        const label = `행 ${count} ${row.slice(0, 4).join(',')}`;
        assert.deepEqual([row[4], row[5]], ['1', '0x0']);
        assert.equal(scene.current, number(row[6]), `${label} 현재 표`);
        assert.equal(hex(scene.manager), row[7], `${label} manager`);
        assert.equal(hex(scene.table), row[8], `${label} 표`);
        assert.equal(hex(scene.alternate), row[9], `${label} 대체 표`);
        assert.equal(scene.trace.join(';'), InitializeReplay.withoutNoop(row[10]), `${label} 호출`);
        count += 1;
    }
    assert.equal(count, 64);
});

// getter 12행의 최초 512 할당/재사용/NULL 공개 후 실패를 전체 BYTE 에 대조한다.
test('Template 구역 이름표: 원본10.78 12행(최초 512 할당·재사용·NULL 공개 후 실패)', { skip: !sectionRows && '원본 fixture 없음' }, () => {
    assert.equal(sectionRows.length, 12);
    let count = 0;
    for (const row of sectionRows) {
        assert.equal(row.length, 10);
        const scene = new InitializeReplay(number(row[0]), 0, 0, number(row[1]));
        let completed = true;
        // 기존표이면 할당 입력 0 에서도 실제 할당을 선택하지 않는다.
        try { for (let call = 0; call < number(row[2]); call += 1) scene.names(); } catch (error) { if (error instanceof RangeError) completed = false; else throw error; }
        const label = `행 ${count} ${row.slice(0, 3).join(',')}`;
        assert.equal(completed, row[3] === '1', `${label} 완료`);
        assert.equal(row[4], completed ? '0x0' : '0x4bcced');
        assert.equal(scene.current, number(row[5]), `${label} 현재 표`);
        assert.equal(hex(scene.manager), row[6], `${label} manager`);
        assert.equal(hex(scene.table), row[7], `${label} 표`);
        assert.equal(hex(scene.alternate), row[8], `${label} 대체 표`);
        assert.equal(scene.trace.join(';'), InitializeReplay.withoutNoop(row[9]), `${label} 호출`);
        count += 1;
    }
    assert.equal(count, 12);
});

// 선택하지 않은 하위는 기존 표 반환에서 요구하지 않는다.
test('Template 구역 이름표: 기존 표는 하위 없이 반환한다', () => {
    const globals = { current: 0x1234 };
    assert.equal(getTemplateSectionNames(globals, {}), 0x1234);
    assert.throws(() => getTemplateSectionNames({ current: 0 }, {}), /하위 미연결/);
});
