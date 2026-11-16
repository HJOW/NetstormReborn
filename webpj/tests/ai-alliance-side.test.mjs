// AiAllianceSide의 cpppj CampaignTestPlay_EnemyClassification 6조건과 물리 경계를 대조한다. 게임 실행이 아니다(WB04-E623).
import test from 'node:test';
import assert from 'node:assert/strict';
import { isAiEnemyOfLocal } from '../www/core/ai-alliance-side.js';
import { put } from './raw-support.mjs';

test('AI 생성 전 동맹: cpppj 원문6조건과 비정렬 입력·최소 길이·번호 범위', () => {
    const settings = new Uint8Array(600);
    assert.equal(isAiEnemyOfLocal(1, 1, 2, settings), false);
    assert.equal(isAiEnemyOfLocal(2, 1, 2, settings), true);
    assert.equal(isAiEnemyOfLocal(2, 1, 6, settings), false);
    put(settings, 0x23c, 2); assert.equal(isAiEnemyOfLocal(2, 1, 2, settings), false);
    put(settings, 0x23c, 24); assert.equal(isAiEnemyOfLocal(2, 1, 2, settings), true);
    assert.throws(() => isAiEnemyOfLocal(2, 1, 0, settings.subarray(0, 16)), /동맹 목록/);
    const unaligned = new Uint8Array(581).subarray(1); put(unaligned, 0x23c, 256);
    assert.equal(isAiEnemyOfLocal(2, 8, 0, unaligned), false);
    assert.throws(() => isAiEnemyOfLocal(9, 1, 0, settings), /owner 범위/);
    assert.throws(() => isAiEnemyOfLocal(1, -1, 0, settings), /owner 범위/);
});
