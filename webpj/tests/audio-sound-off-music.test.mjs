// 옵션 "Sound On" 을 끄면 음악도 무음이 되고 다시 켜면 현재 곡을 처음부터 재생하는 원본 동작을 AudioSystem 으로 확인한다. (2026-11-23 WB04-E1239)
// 원본 ReinterpretClientAudioOptions(4355df..435680, cpppj `ClientAudio.cpp`)는 소리 옵션이 꺼지면 장치를 닫고(CloseDevice) 음량을 쓴 뒤 현재 곡을 Select(현재 곡, 음악 옵션)로 다시 요청한다.
// 장치가 닫힌 동안 Select 는 곡 이름만 기억하고(SoundMusicSelection::Assign) 소리가 나지 않으며, 다시 켜면 열린 장치에서 같은 곡이 처음부터 재생된다.
// 실제 브라우저·Web Audio 를 쓰지 않고 가짜 문맥으로 재생·정지 호출을 센다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { AudioSystem } from '../www/audio/audio-system.js';

// 가짜 Web Audio 문맥이다. 시작한 소스와 정지 횟수를 기록한다. (2026-11-23 WB04-E1239)
function fakeContext() {
    const log = { started: [], stopped: 0 };
    const gain = () => ({ gain: { value: 1 }, connect() {} });
    return {
        log, destination: {}, createGain: gain, decodeAudioData: async () => ({}),
        createBufferSource() {
            const source = { loop: false, connect() {}, disconnect() {}, start() { log.started.push(source); }, stop() { log.stopped += 1; } };
            return source;
        },
    };
}

// 음악 곡을 적재한 재생기를 만든다. (2026-11-23 WB04-E1239)
async function makeAudio(names = ['ser22.mus', 'wind22.mus', 'fanfare.mus']) {
    const context = fakeContext(), audio = new AudioSystem(context);
    for (const name of names) await audio.loadMusic(name, new Uint8Array(4));
    return { context, audio };
}

test('소리 끄기: 재생 중인 음악도 멈추고 곡 이름은 기억하며, 켜면 같은 곡을 처음부터 다시 재생한다', async () => {
    const { context, audio } = await makeAudio();
    assert.equal(audio.playMusic('wind22.mus'), true);
    assert.equal(context.log.started.length, 1);
    audio.setSoundEnabled(false);
    // 장치를 닫은 것처럼 음악이 멈추고 현재 곡은 없지만 요청한 곡은 남는다.
    assert.equal(context.log.stopped, 1); assert.equal(audio.musicSource, null); assert.equal(audio.currentMusic, null); assert.equal(audio.wantedMusic, 'wind22.mus');
    // 소리가 꺼진 동안 곡 요청은 기억만 하고 소리를 내지 않는다(장면 음악이 곡을 바꿔도 마찬가지).
    assert.equal(audio.playMusic('ser22.mus'), false);
    assert.equal(context.log.started.length, 1); assert.equal(audio.wantedMusic, 'ser22.mus');
    // 켜면 마지막으로 선택한 곡이 처음부터 재생된다.
    audio.setSoundEnabled(true);
    assert.equal(context.log.started.length, 2); assert.equal(audio.currentMusic, 'ser22.mus'); assert.equal(audio.musicSource.loop, true);
    audio.close();
});

test('같은 소리 옵션을 다시 적용하면 재생 중인 음악을 건드리지 않는다(원본 장치 열기는 이미 열려 있으면 그대로)', async () => {
    const { context, audio } = await makeAudio();
    audio.playMusic('ser22.mus');
    const source = audio.musicSource;
    // 옵션 갱신(음량 변경 등)마다 setSoundEnabled 가 같은 값으로 불린다.
    audio.setSoundEnabled(true); audio.setSoundEnabled(true);
    assert.equal(context.log.started.length, 1); assert.equal(context.log.stopped, 0); assert.equal(audio.musicSource, source);
    audio.setSoundEnabled(false); audio.setSoundEnabled(false);
    assert.equal(context.log.stopped, 1);
    audio.close();
});

test('음악 옵션이 꺼져 있으면 소리를 다시 켜도 음악은 나지 않고, 소리가 꺼진 채 음악을 켜면 소리를 켤 때 재생된다', async () => {
    const { context, audio } = await makeAudio();
    audio.playMusic('ser22.mus');
    audio.setMusicEnabled(false); audio.setSoundEnabled(false); audio.setSoundEnabled(true);
    assert.equal(context.log.started.length, 1); assert.equal(audio.currentMusic, null);
    // 소리가 꺼진 채 음악을 켜도 소리는 없다.
    audio.setSoundEnabled(false); audio.setMusicEnabled(true);
    assert.equal(context.log.started.length, 1); assert.equal(audio.currentMusic, null);
    audio.setSoundEnabled(true);
    assert.equal(context.log.started.length, 2); assert.equal(audio.currentMusic, 'ser22.mus');
    audio.close();
});

test('결과 곡(fanfare)은 소리를 켠 뒤에도 한 번만 재생하고 곡을 요청한 적 없으면 아무 소리도 시작하지 않는다', async () => {
    const { context, audio } = await makeAudio();
    // 요청한 곡이 없으면 켜도 시작하지 않는다.
    audio.setSoundEnabled(false); audio.setSoundEnabled(true);
    assert.equal(context.log.started.length, 0);
    audio.setSoundEnabled(false); audio.playMusic('fanfare.mus'); audio.setSoundEnabled(true);
    assert.equal(context.log.started.length, 1); assert.equal(audio.musicSource.loop, false);
    audio.close();
    // 닫은 뒤에는 켜도 시작하지 않는다.
    audio.setSoundEnabled(false); audio.setSoundEnabled(true);
    assert.equal(context.log.started.length, 1);
});
