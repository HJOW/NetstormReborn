// 다이스·제단 수명 raw-dais-lifecycle.js(RawDaisLifecycle)를 원본 10.78 관측으로 대조하고, 상태/훅 공장의 계약을 확인한다.
//  - daislifecycle-current-x86.tsv 의 originals 964행(모드 dpp 다이스 최초 Pop·app 제단 최초 Pop·apre 제단 전처리·dpost 다이스 삭제 후처리·apost 제단 삭제 후처리):
//    효과 순서(F 프레임·unpop/repop·C/owner/P 제단 생성·N/REG 21 예약·SURFACE/HAS/ISLAND/OPTION/CHAT 경고·destroy·postpop/post/B 마지막 공통 경계)와
//    네 슬롯(5·7·8·9)의 경계 직전 raw 전체, 아홉 Player 의 획득 플래그/SID 쌍 네 칸이 같다. 독립 입력 배열과 실제 Player 기록 묶음을 각각 같은 964행으로 대조한다.
// cpppj DaisLifecycleTests 의 장면(합성 자산·실제 Frame/Host/Damageable/Carrier)을 같은 순서로 옮겼다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-13 WB04-E369: 7-P3 RawDaisLifecycle 이식으로 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get } from './raw-support.mjs';
import { joinEffects } from './dais-support.mjs';
import { lifecycleScene, LifecycleBodyEnd, STRIDE, GRANT_AT, PAIR_AT } from './dais-lifecycle-support.mjs';

// 원본 관측 10.78(originals) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E369)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'daislifecycle-current-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 한 행을 실행해 마지막 공통 경계에서 중단되는지 확인하고 효과·raw·Player 칸을 원본과 대조한다. bound 가 참이면 실제 Player 기록 묶음 경로이다. (2026-11-13 WB04-E369)
function replay(row, bound, checked) {
    assert.equal(row.length, 5);
    const input = JSON.parse(row[1]);
    const scene = lifecycleScene(input, bound);
    const { lifecycle, effects, boundaryImages, world, lifecycleState, records, beforeRecords } = scene;
    const run = {
        dpp: () => lifecycle.postPop(5, input.flags),
        app: () => lifecycle.postPop(7, input.flags),
        apre: () => lifecycle.altarPreDestroy(7, input.flags),
        dpost: () => lifecycle.postDestroy(5, input.flags),
        apost: () => lifecycle.postDestroy(7, input.flags),
    }[input.mode];
    assert.ok(run, `수명 fixture 모드 누락: ${input.mode}`);
    const where = `행 ${checked} (${row[1]}) ${bound ? '묶음' : '독립'}`;
    assert.throws(run, LifecycleBodyEnd, `${where} 경계 중단`);
    assert.equal(joinEffects(effects), row[2], `${where} 효과`);
    const images = row[3].split(';');
    assert.equal(images.length, 4);
    assert.equal(boundaryImages.length, 4);
    // 마지막 하위 호출 직전 raw 전체(가상 표 주입 DWORD 제외)를 비교한다.
    boundaryImages.forEach((image, index) => assert.equal(image.slice(8), images[index].slice(8), `${where} 슬롯 ${[5, 7, 8, 9][index]}`));
    const fields = [];
    // 아홉 Player 의 획득 플래그/SID 쌍 네 칸을 현재 저장소에서 읽는다.
    for (let owner = 0; owner < 9; owner += 1) {
        const player = lifecycleState.players[owner];
        const at = owner * STRIDE + PAIR_AT;
        fields.push(bound
            ? [world.state.grantFlag(owner), get(records, at), get(records, at + 4), get(records, at + 8), get(records, at + 12)].join(',')
            : [world.state.grantFlags[owner], player.first, player.firstMirror, player.second, player.secondMirror].join(','));
    }
    assert.equal(fields.join(';'), row[4], `${where} Player 칸`);
    // 묶음 경로는 출력 필드 이외 Player 바이트를 전수 보존해야 한다.
    if (bound) {
        records.forEach((value, index) => {
            const offset = index % STRIDE;
            if (offset !== GRANT_AT && !(offset >= PAIR_AT && offset < PAIR_AT + 16)) assert.equal(value, beforeRecords[index], `${where} 보존 바이트 ${index}`);
        });
    }
    world.kernel.clear();
}

test('DaisLifecycle: 원본 10.78 daislifecycle 964행의 효과 순서·경계 직전 raw·Player 칸이 독립 입력 배열에서 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 입력마다 새 풀/해시/Host 를 만들어 이전 행의 결과를 쓰지 않는다.
    for (const row of rows) {
        replay(row, false, checked);
        checked += 1;
    }
    assert.equal(checked, 964);
});

test('DaisLifecycle: 같은 964행이 실제 Player 기록 9칸을 묶은 경로에서도 같고 출력 필드 이외 바이트가 보존된다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 묶음 경로는 독립 배열을 0xdead 로 오염시켜 사본을 읽으면 반드시 실패하게 한다.
    for (const row of rows) {
        replay(row, true, checked);
        checked += 1;
    }
    assert.equal(checked, 964);
});
