// Kernel 진단 장치와 슬롯 0 계약을 검사한다(cpppj KernelDiagnosticCurrentTests 중 Client 접합을 뺀 다섯 장면의 이식).
//  - 빈 슬롯 삭제: 단언(`processes[pid] != NULL`, `\Ns\O\Kernel.cpp`, 0x82) 뒤 슬롯을 다시 읽고, 단언 중 등록된 현재 객체만 소멸시킨다.
//  - 소진 등록: 요약→상세→단언(`firstSlot != 0`, 0x7a)을 현재 장치로 부르고 정상 복귀하면 캡처한 슬롯 0 에 등록한다. 실패는 그 앞 효과만 남긴다.
//  - 슬롯 0: 프레임 실행·Clear 순회에서 제외, 덮어쓰기는 소멸 호출 없이 보관하고 최종 반납(drainOwnedForShutdown)에서 정리한다.
//  - 소멸 중 새로 등록한 슬롯 0 도 마지막 비우기 뒤에는 실행 표에 남지 않는다.
// 원본 단언 문자열·행은 cpppj 검사의 독립 상수이다. Client 의 실제 procSumm.txt 출력과 월드 PID 재작성 장면은 Client/World 이식(WB05)에서 옮긴다.
// (2026-11-08 WB04-D5c: Kernel 기준선 정렬과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { BaseProcess, Kernel } from '../www/core/kernel.js';

// 소멸/프레임 호출을 외부에서 기록하는 관찰용 프로세스이다. (2026-11-08 WB04-D5c)
class Observed extends BaseProcess {
    constructor(destroy = null, frame = null) {
        super();
        this.onDestroy = destroy;
        this.onFrame = frame;
    }

    runFrame() { this.onFrame?.(); }

    destroy() { this.onDestroy?.(); }
}

// 연산이 예외를 던지는지 본다. (2026-11-08 WB04-D5c)
function rejects(operation) {
    try {
        operation();
        return false;
    } catch {
        return true;
    }
}

// 소진 상세 문구(원문 그대로)이다. (2026-11-08 WB04-D5c)
const DETAIL = 'Out of processes. A process count trace has been dumped\nto procSumm.txt.  Please deliver this file with the bug report.\n';

// 빈 슬롯 삭제: 단언은 정확히 한 번, 모드별(정상 복귀/단언 중 슬롯 채움/단언 예외)로 현재 슬롯을 다시 읽는다. 물리 범위 밖은 단언 전에 거부한다.
test('Kernel 진단: 빈 슬롯 삭제는 단언 뒤 현재 슬롯을 다시 읽는다', () => {
    for (const mode of [0, 1, 2]) {
        const kernel = new Kernel(2);
        let assertions = 0;
        let destroyed = 0;
        const devices = {
            assertion: (expression, file, line) => {
                assertions += 1;
                assert.equal(expression, 'processes[pid] != NULL');
                assert.equal(file, '\\Ns\\O\\Kernel.cpp');
                assert.equal(line, 0x82);
                // 선택된 함수의 수명을 교체해도 현재 호출은 정상적으로 끝나야 한다.
                kernel.bindDiagnostics({});
                if (mode === 1) assert.equal(kernel.add(new Observed(() => { assert.ok(kernel.get(1) !== null && kernel.size === 1); destroyed += 1; })), 1);
                if (mode === 2) throw new Error('선택된 단언 실패');
            },
        };
        kernel.bindDiagnostics(devices);
        assert.equal(rejects(() => kernel.remove(1)), mode === 2, `모드 ${mode}`);
        assert.equal(assertions, 1);
        assert.equal(destroyed, mode === 1 ? 1 : 0);
        assert.equal(kernel.size, 0);
        assert.equal(kernel.get(1), null);
        kernel.bindDiagnostics(devices);
        assert.ok(rejects(() => kernel.remove(3)), '물리 범위 밖');
        assert.equal(assertions, 1, '범위 밖은 단언 전에 거부');
    }
});

// 소진 등록: 요약(S)→상세(D)→단언(A) 순서와 문구, 진단 중 빈 슬롯이 생겨도 캡처한 슬롯 0 에 등록한다.
test('Kernel 진단: 소진은 현재 장치를 차례로 부르고 캡처한 슬롯 0 에 등록한다', () => {
    for (const freeDuringAssertion of [false, true]) {
        let destroyed = 0;
        const kernel = new Kernel(1);
        assert.equal(kernel.add(new Observed(() => { destroyed += 1; })), 1);
        let calls = '';
        let detail = '';
        const devices = {};
        // 장치가 다음 장치를 연결하는 연쇄이다(각 사용 지점은 그 시점의 현재 장치를 읽는다).
        devices.summary = () => {
            calls += 'S';
            devices.detail = (text) => {
                calls += 'D';
                detail = text;
                devices.assertion = (expression, file, line) => {
                    calls += 'A';
                    assert.equal(expression, 'firstSlot != 0');
                    assert.equal(file, '\\Ns\\O\\Kernel.cpp');
                    assert.equal(line, 0x7a);
                    if (freeDuringAssertion) kernel.remove(1);
                };
                kernel.bindDiagnostics(devices);
            };
            kernel.bindDiagnostics(devices);
        };
        kernel.bindDiagnostics(devices);
        const added = new Observed(() => { destroyed += 1; });
        assert.equal(kernel.add(added), 0);
        assert.equal(calls, 'SDA');
        assert.equal(detail, DETAIL);
        assert.equal(kernel.get(0), added);
        assert.equal(kernel.size, freeDuringAssertion ? 1 : 2);
        assert.equal(destroyed, freeDuringAssertion ? 1 : 0);
        assert.equal(kernel.get(2), null);
        kernel.drainOwnedForShutdown();
        assert.equal(destroyed, 2);
        assert.equal(kernel.size, 0);
        assert.equal(kernel.get(0), null);
    }
});

// 소진 진단 실패: 요약/상세/단언 중 어디서 던지든 그 앞 효과만 남고 새 프로세스는 슬롯에 기록되지 않는다. 장치가 없으면 첫 장치 위치에서 거부한다.
test('Kernel 진단: 소진 진단 실패는 선택된 부분 효과만 남긴다', () => {
    for (const stop of [0, 1, 2]) {
        let destroyed = 0;
        const kernel = new Kernel(1);
        kernel.add(new Observed());
        let calls = '';
        kernel.bindDiagnostics({
            summary: () => { calls += 'S'; if (stop === 0) throw new Error('요약 실패'); },
            detail: () => { calls += 'D'; if (stop === 1) throw new Error('상세 실패'); },
            assertion: () => { calls += 'A'; throw new Error('단언 실패'); },
        });
        assert.ok(rejects(() => kernel.add(new Observed(() => { destroyed += 1; }))));
        assert.equal(calls, 'SDA'.slice(0, stop + 1));
        assert.equal(destroyed, 0, '등록되지 않은 객체는 소멸하지 않는다');
        assert.equal(kernel.size, 1);
        assert.equal(kernel.get(0), null);
        kernel.clear();
    }
    const unbound = new Kernel(1);
    unbound.add(new Observed());
    assert.ok(rejects(() => unbound.add(new Observed())));
    assert.equal(unbound.size, 1);
    assert.equal(unbound.get(0), null);
    // 요약만 연결하면 상세 위치에서, 상세까지 연결하면 단언 위치에서 거부한다.
    unbound.bindDiagnostics({ summary: () => {} });
    assert.throws(() => unbound.add(new Observed()), /상세 저장소 미연결/);
    unbound.bindDiagnostics({ summary: () => {}, detail: () => {} });
    assert.throws(() => unbound.add(new Observed()), /단언 미연결/);
});

// 슬롯 0 덮어쓰기: 소멸 호출을 추가하지 않고(밀려난 객체 보관), 프레임 실행·Clear 순회는 슬롯 0 을 건너뛰며, 선택한 소유자만 최종 반납한다.
test('Kernel 진단: 슬롯 0 덮어쓰기는 순회에서 빠지고 선택한 소유자만 최종 반납한다', () => {
    let destroyed = '';
    let frames = '';
    const kernel = new Kernel(1);
    kernel.bindDiagnostics({ summary: () => {}, detail: () => {}, assertion: () => {} });
    kernel.add(new Observed(() => { destroyed += 'N'; }, () => { frames += 'N'; }));
    const old = new Observed(() => { destroyed += 'A'; }, () => { frames += 'A'; });
    assert.equal(kernel.add(old), 0);
    const current = new Observed(() => { destroyed += 'B'; }, () => { frames += 'B'; });
    assert.equal(kernel.add(current), 0);
    assert.equal(destroyed, '');
    assert.equal(kernel.get(0), current);
    assert.equal(kernel.size, 2);
    kernel.runFrame();
    assert.equal(frames, 'N');
    kernel.clear();
    assert.equal(destroyed, 'N');
    assert.equal(kernel.size, 1);
    assert.equal(kernel.get(0), current);
    kernel.drainOwnedForShutdown((process) => process === old);
    assert.equal(destroyed, 'NA');
    assert.equal(kernel.get(0), current);
    assert.equal(kernel.size, 1);
    kernel.remove(0);
    assert.equal(destroyed, 'NAB');
    assert.equal(kernel.get(0), null);
    assert.equal(kernel.size, 0);
});

// 소멸 중 새로 등록한 슬롯 0 은 마지막 비우기 뒤에는 실행 표에 남지 않고, 소멸 효과는 최종 반납에서만 실행된다.
test('Kernel 진단: 슬롯 0 소멸 중 새 슬롯 0 도 마지막 비우기 뒤 남지 않는다', () => {
    let oldDestroyed = 0;
    let newDestroyed = 0;
    const kernel = new Kernel(1);
    kernel.bindDiagnostics({ summary: () => {}, detail: () => {}, assertion: () => {} });
    kernel.add(new Observed());
    kernel.add(new Observed(() => {
        oldDestroyed += 1;
        assert.ok(kernel.get(0) !== null);
        assert.equal(kernel.add(new Observed(() => { newDestroyed += 1; })), 0);
    }));
    kernel.remove(0);
    assert.equal(oldDestroyed, 1);
    assert.equal(newDestroyed, 0);
    assert.equal(kernel.get(0), null);
    assert.equal(kernel.size, 1);
    kernel.drainOwnedForShutdown();
    assert.equal(newDestroyed, 1);
    assert.equal(oldDestroyed, 1, '소멸 중이던 객체는 다시 소멸하지 않는다');
    assert.equal(kernel.size, 0);
    // 소멸 중 같은 슬롯의 재삭제는 거부한다.
    const reentrant = new Kernel(2);
    reentrant.add(new Observed(() => { assert.throws(() => reentrant.remove(1), /중복 삭제/); }));
    reentrant.remove(1);
    assert.equal(reentrant.size, 0);
});
