// 자산 소유자 분배 raw-asset-owner.js(makeAssetOwnerDispatch)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: asset-owner-current-local-x86.tsv 10.78(originals) 행(파생 소유자 로컬 입력: 종류(고정 중립 / 생산)·extra·소유자 payload·생산 카운터·공통 부모 중단) → 종료 원인(returned/base), 공통 부모 호출 기록(`B소유자,카운터`),
//    효과 뒤 생산 카운터, 슬롯 전체 raw(가상 표는 비교 전에 입력 기록값으로 복원). CD 10.72 행은 웹 범위(10.78) 밖이다.
//  - 계약: AssetOwnerCurrent_LiveTableAndFailureKeepPrefix(생산 접두는 현재 가상 표/extra 를 호출마다 읽고 공통 부모 실패 뒤에도 앞선 증가를 유지).
//    실제 World 생성자와 생산 장부 합성(AssetOwnerCurrent_ActualWorldConstructorsAndProductionStorage)은 RawSurfaceWorld 이식 때 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E71: 자산 소유자 분배 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { makeAssetOwnerDispatch } from '../www/core/raw-asset-owner.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E71)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'asset-owner-current-local-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 슬롯 SID, 판본당 독립 입력 수(전체 128행 중 10.78 절반)이다. (2026-11-08 WB04-E71)
const SOURCE = 5;
const ROWS = 64;

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E71)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

test('AssetOwnerCurrent: 원본 10.78 행의 생산 접두/중립 몸체와 전체 payload/감김/공통 부모 중단 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 두 판본이 아닌 10.78 의 생산 접두/중립 몸체만 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const args = row[1].split(',').map(Number);
        assert.equal(args.length, 5);
        const [kind, extra, owner, start, stopFlag] = args;
        const dirty = { value: start >>> 0 };
        const stop = stopFlag !== 0;
        const pool = new SidPool(32768);
        const raw = pool.physical(SOURCE);
        // 원문은 free/dead 를 선행 검사하지 않는다. 명시 입력 payload 만 만든다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 29 + 7) & 0xff;
        raw[34] = 3;
        raw[40] = extra;
        // 실제 가상 분배를 위해 원문 입력의 첫 DWORD 만 기록한다. 결과 비교 때 같은 입력 DWORD 를 복원한다.
        const originalTable = get(raw, 0);
        put(raw, 0, kind ? 0x50a8e8 : 0x509148);
        let events = '';
        let cause = 'returned';
        const dispatch = makeAssetOwnerDispatch(pool, new BridgeConnectState(), dirty, (sid, player) => {
            assert.equal(sid, SOURCE);
            events = `B${player},${dirty.value}`;
            if (stop) throw new Error('base');
            raw[34] = player & 0xff;
        });
        try {
            dispatch(SOURCE, owner);
        } catch (error) {
            cause = error.message;
        }
        put(raw, 0, originalTable);
        assert.equal(cause, row[2], `행 ${count}: ${row[1]}`);
        assert.equal(events === '' ? '-' : events, row[3], `행 ${count}: ${row[1]}`);
        assert.equal(String(dirty.value), row[4], `행 ${count}: ${row[1]}`);
        assert.equal(hex(raw), row[5], `행 ${count}: ${row[1]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('AssetOwnerCurrent: 생산 접두는 현재 가상 표/extra 를 호출마다 읽고 공통 부모 실패 뒤에도 앞선 증가를 유지한다', () => {
    const pool = new SidPool(32768);
    const raw = pool.physical(SOURCE);
    const dirty = { value: 7 };
    let calls = 0;
    const dispatch = makeAssetOwnerDispatch(pool, new BridgeConnectState(), dirty, () => { calls += 1; throw new Error('base'); });
    const attempt = (owner) => { try { dispatch(SOURCE, owner); } catch { /* 공통 부모의 중단은 시험 입력이다 */ } };
    put(raw, 0, 0x509148);
    raw[40] = 0;
    attempt(2);
    assert.ok(dirty.value === 8 && calls === 1);
    raw[40] = 8;
    attempt(2);
    assert.ok(dirty.value === 8 && calls === 2);
    put(raw, 0, 0x50a8e8);
    raw[34] = 3;
    dispatch(SOURCE, 0xffffffff);
    assert.ok(dirty.value === 8 && calls === 2 && raw[34] === 0);
    // 둘째 생산 자산 표(0x509210)도 같은 접두를 쓴다.
    put(raw, 0, 0x509210);
    raw[40] = 0;
    attempt(1);
    assert.ok(dirty.value === 9 && calls === 3);
    assert.throws(() => makeAssetOwnerDispatch(pool, new BridgeConnectState(), dirty, null));
});
