// 앱의 월드 설치 장치가 현재 번역과 설정을 사용하고 손상된 바이트를 거부하는지 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { displayTextCodec } from '../www/assets/display-text.js';

// 장치 생성 후 바뀐 현재 번역·설정 소유자도 호출 시점에 조회한다.
test('Netstorm 설치 장치: 현재 번역·NUL 설정·제목', () => {
    const app = new Netstorm(), writes = [];
    const install = app._campaignInstallServices();
    assert.throws(() => install.translateYou(), /번역 자원/);
    app._gameData = { set: (...args) => writes.push(args) }; app._resources = { translate: () => 'You' };
    assert.deepEqual([...install.translateYou()], [89, 111, 117, 0]);
    app._resources.translate = () => '당신';
    const text = install.translateYou();
    assert.equal(text.at(-1), 0); assert.equal(displayTextCodec.decode(text.subarray(0, -1)), '당신');
    install.setConfig('fort.mySubHandle', Uint8Array.from([...text, 65]));
    assert.deepEqual(writes, [['fort.mySubHandle', '당신']]);
    assert.throws(() => install.setConfig('x', Uint8Array.of(65)), /NUL/);
    assert.throws(() => install.setConfig('x', [65, 0]), /바이트/);
    let refreshed = false; app._refreshFortWindowTitle = () => { refreshed = true; };
    install.refreshWindowTitle(); assert.equal(refreshed, true);
});
