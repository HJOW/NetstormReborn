// 스펠 도움말 그림 임시 제외 규칙 spell-frame-visibility.js 를 cpppj `client/SpellFrameVisibility.h` 의 조건식(폭탄 종류 && 도움말 프레임 ≥ 0 && 지금 프레임 == 도움말 프레임)으로 검사한다.
//  - cpppj 에는 이 규칙만을 직접 보는 시험이 없고(GameWorld·RawCampaignFactoryDisplay·RawCampaignSession 의 그리기 경로가 호출), 웹은 세 조건을 표로 모두 돌려 어느 하나라도 빠지면 잡는다.
//  - 실제 타입 표 연결: 실제 10.78 타입 정의에서 폭탄(bomb) 종류 타입은 도움말 프레임을 가지며(helpFrame ≥ 0), 그 프레임에서만 숨기고 이웃 프레임·비폭탄 타입은 그대로 그린다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E381)
import test from 'node:test';
import assert from 'node:assert/strict';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { TypeFlag2 } from '../www/assets/type-flags.js';
import { suppressSpellHelpFrame } from '../www/core/spell-frame-visibility.js';

// 폭탄 비트(0x100)는 원본 타입 플래그2 의 값이다(상수를 그대로 쓰면 변형이 가려지므로 리터럴로 확인한다). (2026-11-13 WB04-E381)
const ORIGINAL_BOMB_BIT = 0x100;

test('폭탄 비트는 원본 TypeFlag2.kBomb(0x100)이다', () => {
    assert.equal(TypeFlag2.bomb, ORIGINAL_BOMB_BIT);
});

test('도움말 그림 제외: 폭탄 종류·도움말 프레임 존재·같은 프레임의 세 조건이 모두 맞을 때만 숨긴다(조합표)', () => {
    // 비트 하나만 바뀌어도 결과가 갈리도록 폭탄 비트 외의 flags2 비트는 모두 켠 값과 모두 끈 값을 함께 쓴다.
    for (const others of [0, 0xfffffeff]) {
        for (const bomb of [false, true]) {
            const type = { flags2: (others | (bomb ? ORIGINAL_BOMB_BIT : 0)) >>> 0 };
            for (const helpFrame of [-1, 0, 5, 1000]) {
                for (const frame of [-1, 0, 4, 5, 6, 1000, 0xffffffff]) {
                    const expected = bomb && helpFrame >= 0 && frame === helpFrame;
                    assert.equal(suppressSpellHelpFrame(type, { helpFrame }, frame), expected, `others=${others} bomb=${bomb} help=${helpFrame} frame=${frame}`);
                }
            }
        }
    }
});

test('도움말 그림 제외: 도움말 프레임이 -1(없음)이면 어떤 프레임도 숨기지 않고 0 번 도움말은 0 번 프레임에서만 숨긴다', () => {
    const bomb = { flags2: ORIGINAL_BOMB_BIT };
    assert.equal(suppressSpellHelpFrame(bomb, { helpFrame: -1 }, -1), false);
    assert.equal(suppressSpellHelpFrame(bomb, { helpFrame: 0 }, 0), true);
    assert.equal(suppressSpellHelpFrame(bomb, { helpFrame: 0 }, 1), false);
    assert.equal(suppressSpellHelpFrame({ flags2: 0 }, { helpFrame: 0 }, 0), false);
});

test('도움말 그림 제외: 실제 10.78 타입 정의의 폭탄 타입은 도움말 프레임 한 장만 숨기고 비폭탄 타입은 아무 프레임도 숨기지 않는다', () => {
    const assets = realCaptureAssets();
    let bombTypes = 0;
    let hiddenTypes = 0;
    let plainWithHelp = 0;
    // 자산 TYPE 70 이상의 모든 타입을 돌아 폭탄 타입의 도움말 프레임에서만 true 인지 센다.
    for (let number = 70; number < assets.types.length; number += 1) {
        const definition = assets.table.definitions[number - 70];
        if (!definition) continue;
        const record = assets.types[number];
        const special = { helpFrame: definition.helpFrame };
        const bomb = (record.flags2 & ORIGINAL_BOMB_BIT) !== 0;
        let hidden = 0;
        for (let frame = -1; frame <= Math.max(special.helpFrame, 0) + 1; frame += 1) if (suppressSpellHelpFrame(record, special, frame)) hidden += 1;
        if (bomb) {
            bombTypes += 1;
            assert.equal(hidden, special.helpFrame >= 0 ? 1 : 0, `TYPE ${number}`);
            if (hidden) hiddenTypes += 1;
        } else {
            assert.equal(hidden, 0, `TYPE ${number}`);
            if (special.helpFrame >= 0) plainWithHelp += 1;
        }
    }
    // 폭탄 타입(스펠)이 있고 그중 도움말 프레임이 있는 것이 있다. 비폭탄 중에도 도움말 그림이 있는 타입이 있어도 숨기지 않는다.
    assert.ok(bombTypes > 0);
    assert.ok(hiddenTypes > 0);
    assert.ok(plainWithHelp >= 0);
});

test('도움말 그림 제외: 프레임 비교는 부호 있는 64비트 정수 그대로여서 32비트로 접히는 큰 값은 같다고 보지 않는다', () => {
    const bomb = { flags2: ORIGINAL_BOMB_BIT };
    assert.equal(suppressSpellHelpFrame(bomb, { helpFrame: 5 }, 5), true);
    assert.equal(suppressSpellHelpFrame(bomb, { helpFrame: 5 }, 2 ** 32 + 5), false);
    assert.equal(suppressSpellHelpFrame(bomb, { helpFrame: 5 }, -(2 ** 32) + 5), false);
});
