// 일시정지 메뉴가 쓰는 음소거 푸시/팝(원본 SoundMusic::PushMute/PopMute)을 효과음 재생기와 음악 음량 노드에서 검사한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1089)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SoundList } from '../www/audio/sound-list.js';
import { createSoundState, SoundPlayer } from '../www/audio/sound-player.js';
import { AudioSystem, centibelsToGain, volumeStepToCentibels } from '../www/audio/audio-system.js';

// 필수 장치만 채운 효과음 재생기다. 장치는 호출하지 않는 값만 쓴다.
function player() {
    const state = createSoundState(), hooks = Object.fromEntries(['status', 'play', 'setPosition', 'setVolume', 'setPan', 'stop', 'duplicate', 'load'].map((name) => [name, () => 0]));
    return { state, player: new SoundPlayer(new SoundList(() => {}), state, hooks) };
}

// 증폭률 노드만 흉내 내는 가짜 AudioContext 이다.
function fakeContext() {
    return { createGain: () => ({ gain: { value: 1 }, connect() {} }), destination: {}, state: 'running' };
}

test('효과음 음소거: 전체 음량을 최소로 낮추고 예약하며 깊이가 0 이 되면 복원한다', () => {
    const { state, player: sound } = player(); state.initialized = true; sound.setMasterVolume(-500);
    sound.pushMute(); assert.equal(state.masterVolume, -10000); assert.equal(state.pendingMasterVolume, -500); assert.equal(sound.muteDepth(), 1);
    // 보류 중의 음량 변경은 예약만 하고 두 번째 푸시는 예약값을 덮어쓰지 않는다.
    sound.setMasterVolume(-1000); assert.equal(state.masterVolume, -10000); assert.equal(state.pendingMasterVolume, -1000);
    sound.pushMute(); assert.equal(sound.muteDepth(), 2); assert.equal(state.pendingMasterVolume, -1000);
    sound.popMute(); assert.equal(state.masterVolume, -10000); assert.equal(sound.muteDepth(), 1);
    sound.popMute(); assert.equal(state.masterVolume, -1000); assert.equal(sound.muteDepth(), 0);
    // 깊이가 0 이하이면 팝은 아무 일도 하지 않는다.
    sound.popMute(); assert.equal(sound.muteDepth(), 0); assert.equal(state.masterVolume, -1000);
});

test('음악 음소거: 음량 노드를 0 으로 낮추고 보류 중 단계 변경은 해제 때 적용한다', () => {
    const audio = new AudioSystem(fakeContext(), { soundVolumeStep: 5, musicVolumeStep: 3 });
    const level = centibelsToGain(volumeStepToCentibels(3)); assert.equal(audio.musicGain.gain.value, level);
    audio.pushMute(); assert.equal(audio.musicGain.gain.value, 0); assert.equal(audio.musicMuteDepth, 1);
    audio.setMusicVolumeStep(5); assert.equal(audio.musicGain.gain.value, 0);
    audio.pushMute(); audio.popMute(); assert.equal(audio.musicGain.gain.value, 0);
    audio.popMute(); assert.equal(audio.musicGain.gain.value, centibelsToGain(volumeStepToCentibels(5))); assert.equal(audio.musicMuteDepth, 0);
    // 깊이 0 의 팝은 음량을 바꾸지 않는다.
    audio.popMute(); assert.equal(audio.musicGain.gain.value, centibelsToGain(volumeStepToCentibels(5))); assert.equal(audio.musicMuteDepth, 0);
    // 보류가 없을 때의 단계 변경은 바로 적용된다.
    audio.setMusicVolumeStep(1); assert.equal(audio.musicGain.gain.value, centibelsToGain(volumeStepToCentibels(1)));
});
