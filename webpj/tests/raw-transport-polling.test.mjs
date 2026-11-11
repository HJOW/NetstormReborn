// 전송 조회 도구(cpppj `o/RawTransportPolling.h`)의 웹 대응 `core/raw-transport-polling.js` 를 검사한다(4-C, WB04-E310).
// 원본 관측 파일이 없는 단위 계약 검사이다(cpppj 는 이 헤더를 쓰는 전송 owner 시험 안에서 간접 확인): 결과 구조의 기본값, 현재 ID 의 pending/정상 칸 선택과 범위 경계, 매 호출 현재 표 값을 읽음.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E310)
import assert from 'node:assert/strict';
import test from 'node:test';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { RawConnectionRegistry } from '../www/core/raw-connection-registry.js';
import { currentTransportToken, transportPollResult } from '../www/core/raw-transport-polling.js';

// 결과는 기본이 둘 다 거짓이고 값을 불리언으로 정규화한다. (2026-11-11 WB04-E310)
test('전송 조회: 결과 구조의 기본값과 정규화', () => {
    assert.deepEqual(transportPollResult(), { consumed: false, again: false });
    assert.deepEqual(transportPollResult(true), { consumed: true, again: false });
    assert.deepEqual(transportPollResult(false, true), { consumed: false, again: true });
    assert.deepEqual(transportPollResult(1, 'x'), { consumed: true, again: true });
    assert.deepEqual(transportPollResult(0, ''), { consumed: false, again: false });
});

// 음수 ID 는 pending 표, 0 이상은 정상 연결 표의 현재 토큰을 읽고 빈 칸은 0 이다. (2026-11-11 WB04-E310)
test('전송 조회: pending/정상 칸의 현재 토큰과 범위', () => {
    const registry = new RawConnectionRegistry();
    const view = registry.cleanup();
    view.transceivers[0] = 100;
    view.transceivers[21] = 121;
    view.pending[1] = 201;
    view.pending[31] = 231;
    // 경계 -31..21 의 양 끝과 안쪽을 읽는다.
    assert.deepEqual([currentTransportToken(registry, 0), currentTransportToken(registry, 21), currentTransportToken(registry, -1), currentTransportToken(registry, -31)], [100, 121, 201, 231]);
    // 빈 칸은 0 이다(pending 0 칸은 id 0 이 정상 칸을 가리키므로 읽히지 않는다).
    view.pending[0] = 999;
    assert.deepEqual([currentTransportToken(registry, 5), currentTransportToken(registry, -5), currentTransportToken(registry, 0)], [0, 0, 100]);
    // 범위 밖과 비정수는 거부한다: -32 는 pending 표의 마지막 합법 번호(-31)를 넘는다.
    for (const id of [-32, -33, 22, 100, -2147483648, 2147483647, 1.5, NaN, Infinity, '3', null, undefined]) {
        assert.throws(() => currentTransportToken(registry, id), RangeError, `id ${String(id)}`);
    }
});

// 매 호출 현재 표를 읽는다: 앞 조회 뒤 칸이 바뀌어도 이전 값을 재사용하지 않고, 실제 등록소 토큰과 이어진다. (2026-11-11 WB04-E310)
test('전송 조회: 매 호출 현재 표 값을 읽음', () => {
    const registry = new RawConnectionRegistry();
    const node = new PacketConnection();
    const token = registry.track(node, () => {}, () => {});
    assert.equal(currentTransportToken(registry, -1), 0);
    assert.equal(registry.accept(token), -1);
    assert.equal(currentTransportToken(registry, -1), token);
    registry.assign(-1, 3, 1, () => {});
    assert.deepEqual([currentTransportToken(registry, -1), currentTransportToken(registry, 3)], [0, token]);
    registry.forget(token);
    assert.equal(currentTransportToken(registry, 3), 0);
});
