// 문자 편집 Gump(TextEntry/Console)의 원본 10.78 독립 관측 2,292행(1,146입력 × 두 정밀도) 대조: 13개 진입(construct/consoleConstruct/attach/scroll/relative/handle/console/draw/idle/setText/append/excluded/horizontal/destroy)의
// 48필드 노드·두 Slider 11필드·편집 버퍼 10DWORD·할당 블록 전체·clipboard 출력·외부 호출 순서·반환·스택 정리량을 cpppj `TextEntryGumpTests` 와 같은 입력 구성으로 비교한다.
// 앞 13열(입력)만 장치에 공급하고 뒤 10열(관측)은 비교에만 쓴다. 호출 장치는 원문 격리 주소 토큰(노드 0x01009000·버퍼 0x01001000·Slider 0x0100a000/0x0100b000)과 `size + 16` 바이트 블록 할당기를 쓴다.
// 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E483)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { EntryScene, ENTRY_NODE, ROW_COLUMNS, entryNumber, floatOf, hexOf, signed, packed } from './text-entry-gump-native-support.mjs';

test('TextEntryGump/Console 원본 10.78 2,292행: 13개 진입의 노드 48필드·Slider·편집 버퍼·할당 전체·clipboard·호출 순서·반환·스택 정리량이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-entry-gump-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    assert.equal(table.length, 2292);
    const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, ROW_COLUMNS, `행 ${index}`);
        const scene = new EntryScene(row); const args = row[5] === '-' ? [] : row[5].split(':').map(entryNumber); const operation = row[1]; let result = 0;
        // 본문(NUL 제외)을 바이트 문자열로 만든 초기 입력과 NUL 포함 입력이다.
        const textString = Buffer.from(scene.text.subarray(0, scene.text.length - 1)).toString('latin1');
        if (operation === 'construct' || operation === 'consoleConstruct') {
            const initial = args[2] ? textString : null;
            if (operation === 'construct') scene.gump.construct(ENTRY_NODE, signed(args[0]), signed(args[1]), initial, args[3], args[4], args[5]);
            else scene.gump.constructConsole(ENTRY_NODE, signed(args[0]), signed(args[1]), initial, args[3], args[4], args[5]);
        } else if (operation === 'attach') scene.gump.attach(ENTRY_NODE, floatOf(args[0]), floatOf(args[1]), args[2], args[3]);
        else if (operation === 'scroll') scene.gump.setScroll(ENTRY_NODE, signed(args[0]), signed(args[1]));
        else if (operation === 'relative') scene.gump.relativeScroll(ENTRY_NODE, signed(args[0]), signed(args[1]));
        else if (operation === 'handle') result = scene.gump.handle(ENTRY_NODE, args[0], args[1], args[2]) >>> 0;
        else if (operation === 'console') result = scene.gump.handleConsole(ENTRY_NODE, args[0], args[1], args[2]) >>> 0;
        else if (operation === 'draw') scene.gump.draw(ENTRY_NODE);
        else if (operation === 'idle') scene.gump.idle(ENTRY_NODE);
        else if (operation === 'setText') scene.gump.setText(ENTRY_NODE, args[0] ? scene.text : null);
        else if (operation === 'append') scene.gump.appendText(ENTRY_NODE, args[0] ? scene.text : null, args[1] !== 0);
        else if (operation === 'excluded') scene.gump.setExcluded(ENTRY_NODE, args[0] ? textString : null);
        else if (operation === 'horizontal') scene.gump.setHorizontal(ENTRY_NODE, args[0]);
        else if (operation === 'destroy') scene.gump.destruct(ENTRY_NODE);
        else throw new Error(`TextEntry 관측 진입 이름 ${operation}`);
        const problems = [];
        if (packed(scene.nodeFields()) !== row[13]) problems.push('node');
        if (packed(scene.childFields(scene.vertical)) !== row[14]) problems.push('vertical');
        if (packed(scene.childFields(scene.horizontal)) !== row[15]) problems.push('horizontal');
        if (packed(scene.bufferFields()) !== row[16]) problems.push('buffer');
        if (scene.blocksOutput() !== row[17]) problems.push('blocks');
        if ((scene.clipboardOutput.length === 0 ? '-' : hexOf(scene.clipboardOutput)) !== row[18]) problems.push('clipboard');
        if ((scene.trace === '' ? '-' : scene.trace) !== row[19]) problems.push('trace');
        if (result !== entryNumber(row[20])) problems.push('result');
        if (entryNumber(row[21]) !== 4 + args.length * 4) problems.push('stack');
        if (!(entryNumber(row[22]) > 0)) problems.push('instructions');
        if (problems.length) failures.push(`${index}/${row[0]}/${operation}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 원본 관측 fixture 에는 Tab/ESC/위/아래 키가 없어 기준선 소스(TextEntryGump.cpp Handle)의 계약으로 독립 확인한다: 이 네 키는 기본 입력기가 처리하지 않아 반환 0, 변경 통지(changeEvent) 없음, 편집 버퍼/노드 변경 없음이다.
// 입력 전달(forwardInput) 통지는 처리 여부와 무관하게 공통 Redraw 뒤에 나가므로 이 계약에 넣지 않는다. 같은 계약이 기본 분기(코드 < 0x20 또는 >= 0x100)에도 적용되어 네 `case` 문은 동등 변형 대상이다. (2026-11-15 WB04-E483)
test('TextEntryGump handle: Tab/ESC/위/아래 키는 처리하지 않고 반환 0·통지 없음·본문 불변이다(독립 확인)', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-entry-gump-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    // 본문이 있고(length>0) 읽기 전용이 아닌 handle 행의 입력 열을 틀로 쓴다(관측 열은 쓰지 않는다).
    const template = table.find((row) => row[1] === 'handle' && Number(row[3].split(':')[7]) > 0 && Number(row[2].split(':')[27]) === 0);
    assert.ok(template);
    for (const code of [9, 27, 0x260000, 0x280000]) {
        const row = [...template]; row[5] = `${code}:0:0`;
        const scene = new EntryScene(row); const before = scene.bufferFields().join(':'); const nodeBefore = scene.nodeFields().join(':');
        assert.equal(scene.gump.handle(ENTRY_NODE, code, 0, 0) >>> 0, 0, `코드 ${code}`);
        assert.equal(scene.bufferFields().join(':'), before, `버퍼 ${code}`); assert.equal(scene.nodeFields().join(':'), nodeBefore, `노드 ${code}`);
        assert.ok(!scene.trace.includes(`bubble:${ENTRY_NODE}:${scene.globals.changeEvent}:0:0`), `변경 통지 ${code}`);
    }
});

// 초기/추가/교체 본문이 null 이면 빈 문자열과 같은 효과다(원문 null 문자열 포인터). fixture 의 null 입력 행이 드물어 독립 확인한다. (2026-11-15 WB04-E483)
test('TextEntryGump 구성/교체/추가: null 본문은 빈 본문과 같은 효과이다(독립 확인)', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-entry-gump-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    const template = table.find((row) => row[1] === 'setText'); assert.ok(template);
    const run = (arg) => { const row = [...template]; row[5] = '0'; const scene = new EntryScene(row); scene.gump.setText(ENTRY_NODE, arg); return [scene.bufferFields().join(':'), scene.blocksOutput(), scene.nodeFields().join(':')]; };
    assert.deepEqual(run(null), run(Uint8Array.of(0)));
    assert.deepEqual(run(undefined), run(Uint8Array.of(0)));
});
