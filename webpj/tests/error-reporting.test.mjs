// 브라우저 앱의 오류 보고가 예외의 원인과 기존 수명을 보존하는지 검사한다. (2026-11-19 WB04-E862)
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { reportGameError } from '../www/platform/error-reporting.js';

// 콘솔 출력을 수집하고 검사 뒤 복원한다.
async function captureErrors(action) {
    const original = console.error;
    const entries = [];
    console.error = (...args) => entries.push(args);
    try {
        await action(entries);
    } finally {
        console.error = original;
    }
}

// 진단 출력 자체가 실패하더라도 원래 처리 흐름을 방해하지 않는다.
test('오류 보고는 원본 객체와 문맥을 보존하고 콘솔 실패를 삼킨다', async () => {
    const error = new Error('고장');
    await captureErrors(async (entries) => {
        reportGameError('test', error, { state: 'loading' });
        assert.equal(entries.length, 1);
        assert.equal(entries[0][1].phase, 'test');
        assert.equal(entries[0][1].state, 'loading');
        assert.match(entries[0][1].timestamp, /^\d{4}-\d\d-\d\dT/);
        assert.equal(entries[0][2], error);
        console.error = () => { throw new Error('콘솔 고장'); };
        assert.doesNotThrow(() => reportGameError('test', error));
    });
});

// 루트 검증처럼 초기화 본문 앞부분의 예외도 콘솔에 남기고 같은 예외를 호출자에게 전달한다.
test('초기화 실패를 보고하고 다시 던진다', async () => {
    await captureErrors(async (entries) => {
        const app = new Netstorm();
        let thrown;
        try { app.init(null); } catch (error) { thrown = error; }
        assert.ok(thrown instanceof TypeError);
        assert.equal(entries.length, 1);
        assert.equal(entries[0][1].phase, 'initialization');
        assert.equal(entries[0][2], thrown);
    });
});

// 비동기 준비 실패는 오류 화면으로 전환하고 원인과 적재 세대를 기록한다.
test('현재 세대의 적재 실패를 보고하고 오류 상태를 유지한다', async () => {
    const error = new Error('준비 실패');
    await captureErrors(async (entries) => {
        const app = new Netstorm({ prepare: async () => { throw error; } });
        app._resources = {}; app._surfaceStorage = {};
        await app._load();
        assert.equal(app._state, 'error');
        assert.equal(app._loadError, error);
        assert.equal(entries.length, 1);
        assert.equal(entries[0][1].phase, 'asset-loading');
        assert.equal(entries[0][1].state, 'loading');
        assert.equal(entries[0][1].generation, 1);
        assert.equal(entries[0][2], error);
    });
});

// 취소된 이전 세대의 실패는 새 수명에 영향을 주지 않으며 오류로 보고하지 않는다.
test('이전 적재 세대의 실패는 무시한다', async () => {
    let rejectPreparation;
    await captureErrors(async (entries) => {
        const app = new Netstorm({ prepare: () => new Promise((resolve, reject) => { rejectPreparation = reject; }) });
        app._resources = {}; app._surfaceStorage = {};
        const loading = app._load();
        app._loadGeneration += 1;
        rejectPreparation(new Error('취소된 적재'));
        await loading;
        assert.equal(entries.length, 0);
        assert.equal(app._loadError, null);
    });
});

// 프레임 콜백의 오류에 시간과 상태를 붙이고 원본 오류를 다시 던진다.
test('프레임 오류를 보고하고 브라우저에 전달한다', async () => {
    const error = new Error('프레임 실패');
    await captureErrors(async (entries) => {
        const app = new Netstorm();
        let callback;
        app._root = { ownerDocument: { hidden: false, defaultView: { requestAnimationFrame: (fn) => { callback = fn; return 1; } } } };
        app._onFrame = () => { throw error; };
        app._startFrameLoop();
        assert.throws(() => callback(123), (actual) => actual === error);
        assert.equal(entries.length, 1);
        assert.equal(entries[0][1].phase, 'frame');
        assert.equal(entries[0][1].frameTime, 123);
        assert.equal(entries[0][2], error);
    });
});

// 입력 콜백은 수신 대상의 this 와 전달값을 유지하고 실패한 이벤트 종류를 기록한다.
test('이벤트 오류를 보고하면서 리스너 수명과 호출 문맥을 보존한다', async () => {
    const error = new Error('입력 실패');
    await captureErrors(async (entries) => {
        const app = new Netstorm();
        app._abort = new AbortController();
        const target = { addEventListener(type, fn, options) { this.type = type; this.fn = fn; this.options = options; } };
        const event = { type: 'click' };
        app._listen(target, 'click', function (received) {
            assert.equal(this, target);
            assert.equal(received, event);
            throw error;
        }, { passive: false });
        assert.equal(target.options.signal, app._abort.signal);
        assert.equal(target.options.passive, false);
        assert.throws(() => target.fn(event), (actual) => actual === error);
        assert.equal(entries.length, 1);
        assert.equal(entries[0][1].eventType, 'click');
        assert.equal(entries[0][1].phase, 'event');
        assert.equal(entries[0][2], error);
    });
});
