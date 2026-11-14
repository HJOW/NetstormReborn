// Ctrl+special 지점 진단(SpecialPointerTrace, 원본 4da263..2e6)의 원본 10.78 독립 관측 36행 대조: 조건 단락·빈 후보 목록·중복 후보의 현재 진행(역투영 → 첫 후보 → 직접 가상 70 → 다음 후보)을
// cpppj `PointerMenuParentTests`(ReplaysNativeSpecialTrace36)와 같은 입력 구성으로 비교한다. 투영과 finder 는 관측과 같은 명시 외부 입력이다.
// 입력 열(special·control·후보 목록 번호·좌표)만 장치에 공급하고 trace 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다.
// (같은 cpppj 시험 파일의 `pointer-object-menu-parent` 8행은 RawObjectMenuOpen 이 PORT_MAP todo 라 이 단위에서 제외한다.) (2026-11-15 WB04-E487)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { traceSpecialPointer } from '../www/ui/special-pointer-trace.js';

// 좌클릭 입력 코드(InputCode.kLeftButton)와 float32 비트 보기이다.
const LEFT_BUTTON = 0x1; const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const bitsOf = (value) => { f32[0] = value; return u32[0]; };

test('SpecialPointerTrace 원본 10.78 36행: 조건 단락·빈 후보·중복 후보의 역투영/첫 후보/가상 70/다음 후보 순서가 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/pointer-special-trace-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(table.length, 36);
    // 후보 목록 세 가지: 비어 있음/하나/중복 포함 셋이다.
    const candidates = [[], [50], [50, 51, 50]]; const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 6, `행 ${index}`);
        const trace = []; let position = 0; const list = candidates[Number(row[2])];
        const hooks = {
            project: (x, y) => { trace.push(`P,${x},${y}`); return [123.25, -9.5]; },
            begin: (point) => { trace.push(`B,${bitsOf(point[0])},${bitsOf(point[1])},0`); return list.length === 0 ? 0 : list[0]; },
            trace: (sid) => trace.push(`V,${sid}`),
            next: () => { trace.push('N'); position += 1; return position < list.length ? list[position] : 0; },
        };
        traceSpecialPointer(Number(row[1]) !== 0, Number(row[0]), { code: LEFT_BUTTON, x: Number(row[3]) | 0, y: Number(row[4]) | 0 }, hooks);
        if ((trace.join(';') || '-') !== row[5]) failures.push(`${index}: ${trace.join(';')} != ${row[5]}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
