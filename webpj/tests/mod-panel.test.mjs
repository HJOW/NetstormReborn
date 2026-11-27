// 실제 메뉴 구성과 DOM 모달 동안의 원래 입력 큐 보류를 확인한다. DOM 동작은 브라우저 검사에서 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { buildOptionsItems } from '../www/ui/option-menus.js';
import { MOD_PANEL_TARGET, modPanelTexts } from '../www/ui/mod-panel.js';
import { Settings } from '../www/platform/settings.js';
import { Netstorm } from '../www/netstorm.js';

test('웹 Options만 모드 관리를 추가하고 ResetAll은 모든 언어에서 마지막이다', () => {
    // 원본 includeAll 목록을 변경하지 않고 세 언어의 추가 행만 검사한다.
    for (const [language, label] of [['english', 'Mods'], ['korean', '모드 관리'], ['japanese', 'モッド']]) {
        const items = buildOptionsItems(new Settings(), { language }), item = items.at(-2);
        assert.equal(item.label, label); assert.deepEqual(item.command, { name: 'Tell', argument: 'Patch1101.Mods' }); assert.equal(items.at(-1).command.argument, 'Patch1101.ResetAll');
        assert.equal(buildOptionsItems(new Settings(), { language, includeAll: true }).some((entry) => entry.command?.argument === MOD_PANEL_TARGET), false);
    }
    assert.equal(modPanelTexts('__proto__').menu, 'Mods');
});

test('DOM 모드 패널은 Native 모달 토큰을 만들지 않고 기존 입력 큐를 그대로 보류한다', () => {
    const app = new Netstorm(), queue = app._clientInputQueue; queue.push(0x70, 11, 12); queue.push(0x6e, 13, 14);
    const first = { ...queue.peek() }; app._modPanel = {};
    // 실제 World 장치가 없는 상태에서도 DOM 모달의 관문이 먼저 적용돼야 한다.
    assert.doesNotThrow(() => app._runCampaignInputFrame()); assert.equal(queue.size, 2); assert.deepEqual(queue.peek(), first);
});
