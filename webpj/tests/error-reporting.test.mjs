// 브라우저 앱의 오류 보고가 예외의 원인과 기존 수명을 보존하는지 검사한다. (2026-11-19 WB04-E862)
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { reportGameError } from '../www/platform/error-reporting.js';

// 선택한 콘솔 수준의 출력을 수집하고 검사 뒤 복원한다. (2026-11-19 WB04-E945)
async function captureErrors(action, level = 'error') {
    const original = console[level];
    const entries = [];
    console[level] = (...args) => entries.push(args);
    try {
        await action(entries);
    } finally {
        console[level] = original;
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
        // 준비 훅의 오류만 검사하도록 앞선 자산·팔레트 단계가 끝난 상태를 둔다.
        app._resources = {}; app._surfaceStorage = {}; app._clientGuide = {}; app._clientPaletteScreen = {}; app._clientColorSort = { mapsReady: true }; app._clientGump = {};
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
        // 준비 훅이 실제로 대기하도록 앞선 적재 단계의 완료 상태를 둔다.
        app._resources = {}; app._surfaceStorage = {}; app._clientGuide = {}; app._clientPaletteScreen = {}; app._clientColorSort = { mapsReady: true }; app._clientGump = {};
        const loading = app._load();
        app._loadGeneration += 1;
        rejectPreparation(new Error('취소된 적재'));
        await loading;
        assert.equal(entries.length, 0);
        assert.equal(app._loadError, null);
    });
});

// 프레임 콜백 오류를 기록한 뒤 다음 프레임을 예약한다. 실패한 프레임을 재시도하지 않는다.
test('프레임 오류를 보고하고 다음 프레임을 진행한다', async () => {
    const error = new Error('프레임 실패');
    await captureErrors(async (entries) => {
        const app = new Netstorm();
        // 예약 큐로 중복 예약·실패한 프레임 재실행 여부를 함께 확인한다.
        const callbacks = [], stamps = [];
        app._root = { ownerDocument: { hidden: false, defaultView: { requestAnimationFrame: (fn) => { callbacks.push(fn); return callbacks.length; } } } };
        app._lastCampaignMission = { name: 'Tutorial1' };
        app._onFrame = (now) => { stamps.push(now); if (stamps.length <= 2) throw error; app._startFrameLoop(); };
        app._startFrameLoop();
        assert.doesNotThrow(() => callbacks[0](123));
        assert.doesNotThrow(() => callbacks[1](148));
        assert.doesNotThrow(() => callbacks[2](173));
        assert.deepEqual(stamps, [123, 148, 173]);
        assert.equal(callbacks.length, 4);
        assert.equal(entries.length, 2);
        assert.equal(entries[0][1].phase, 'frame');
        assert.equal(entries[0][1].frameTime, 123);
        assert.equal(entries[0][1].mission, 'Tutorial1');
        assert.equal(entries[0][2], error);
        assert.equal(app._displayDirty, true);
    });
});

// 숨김·앱 해제 상태에서는 실패 뒤에도 새 프레임을 만들지 않는다.
test('프레임 실패 뒤 숨김 또는 해제된 앱은 재예약하지 않는다', async () => {
    await captureErrors(async (entries) => {
        // 두 수명 종료 조건을 각각 실제 예약 콜백에서 적용한다.
        for (const mode of ['hidden', 'released']) {
            const app = new Netstorm(), callbacks = [];
            const doc = { hidden: false, defaultView: { requestAnimationFrame: (fn) => { callbacks.push(fn); return 1; } } };
            app._root = { ownerDocument: doc };
            app._onFrame = () => { if (mode === 'hidden') doc.hidden = true; else app._root = null; throw new Error(mode); };
            app._startFrameLoop();
            callbacks[0](25);
            assert.equal(callbacks.length, 1);
            assert.equal(app._frameHandle, 0);
        }
        assert.equal(entries.length, 2);
    });
});

// 고정 시각에서도 같은 오류 정책을 적용하고 시각과 프레임 수를 앞으로 진행한다.
test('고정 시각 프레임 실패를 기록하고 다음 시각을 처리한다', async () => {
    // Node 26의 첫 localStorage 접근 경고는 게임 진단이 아니므로 타이머 실행 전에 출력되게 한다.
    await new Promise((resolve) => setImmediate(resolve));
    await captureErrors(async (entries) => {
        const app = new Netstorm({ fixedFrameMs: 25 }), stamps = [];
        app._root = { ownerDocument: { defaultView: { setTimeout } } };
        app._onFrame = (now) => { stamps.push(now); if (stamps.length === 1) throw new Error('고정 시각 실패'); };
        assert.equal(await app.stepFixedFrames(3), 3);
        assert.deepEqual(stamps, [25, 50, 75]);
        assert.equal(entries.length, 1);
        assert.equal(entries[0][1].fixedFrame, null);
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

// 영어 대체 경로의 현재 오류와 정리된 세대의 실패를 구별한다. (2026-11-19 WB04-E944)
test('한국어 자료 실패는 현재 적재 세대에서만 진행률과 진단을 남긴다', async () => {
    const error = new Error('글꼴 다운로드 실패');
    await captureErrors(async (entries) => {
        const app = new Netstorm();
        app._loadGeneration = 2;
        const loader = { async loadBundle(bundle, onProgress) {
            onProgress(1);
            throw error;
        } };
        const previousProgress = app._loadProgress;
        assert.equal(await app._loadKorean(loader, 10, 4, 1), null);
        assert.equal(app._loadProgress, previousProgress);
        assert.equal(entries.length, 0);
        assert.equal(await app._loadKorean(loader, 10, 4, 2), null);
        assert.deepEqual(app._loadProgress, { loaded: 11, total: 14 });
        assert.equal(entries[0][1].phase, 'korean-assets');
        assert.equal(entries[0][1].generation, 2);
        assert.equal(entries[0][2], error);
    }, 'warn');
});

// 정책에 의한 오디오 활성화 실패가 앱을 중단하지 않고 진단에 남는지 확인한다. (2026-11-19 WB04-E944)
test('오디오 활성화 거부를 현재 장치의 오류로 보고한다', async () => {
    const error = new Error('audio resume rejected');
    await captureErrors(async (entries) => {
        const app = new Netstorm();
        app._audioContext = { state: 'suspended', resume: () => Promise.reject(error) };
        app._unlockAudio();
        // 활성화 약속 뒤의 오류 처리까지 기다린다.
        await new Promise((resolve) => setImmediate(resolve));
        assert.equal(app._audioStatus, 'failed');
        assert.equal(entries[0][1].phase, 'audio-unlock');
        assert.equal(entries[0][2], error);
    });
});
