// 원본 10.78 소리 재생 관측을 실제 이름 표·재생기·상태표 장치에서 재생한다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, adler32 } from './raw-support.mjs';
import { SoundList, SOUND_FIELD, SOUND_LIST_BASE } from '../www/audio/sound-list.js';
import { createSoundState, SILENT_SOUND_BUFFER, SoundPlayer } from '../www/audio/sound-player.js';
import { makeSoundProcessHooks } from '../www/core/sound-process.js';

// 원본 관측 장치의 논리 버퍼 기준이다.
const BUFFER_BASE = 0x16001000;
// 부호 없는 DWORD 관측을 signed 정수로 바꾼다.
const signed = (text) => Number(text) | 0;
// 16진수 바이트 이름을 원본 좁은 문자열로 복원한다.
const nameFromHex = (text) => Buffer.from(text, 'hex').toString('latin1');

// 장치 상태·이벤트를 같은 관측 형식으로 기록한다.
function deviceFixture() {
    const status = [], events = [], queues = new Map();
    const device = { status, events, queues, loadKind: 'b', attenuation: 0, copyResult: 0, gainResult: 0, balanceResult: 0 };
    // 각 새 버퍼에 원본 관측과 같은 논리 번호를 부여한다.
    const create = () => { status.push(0); return BUFFER_BASE + 16 * (status.length - 1); };
    // 이미 할당한 장치 버퍼만 조회한다.
    const index = (buffer) => { const number = (buffer - BUFFER_BASE) / 16; if (!Number.isInteger(number) || number < 0 || number >= status.length) throw new Error('버퍼 번호 오류'); return number; };
    // 실제 논리 버퍼는 #번호, 그 밖의 표식은 숫자로 남긴다.
    const ordinal = (buffer) => { const number = (buffer - BUFFER_BASE) / 16; return Number.isInteger(number) && number >= 0 && number < status.length ? `#${number}` : String(buffer >>> 0); };
    device.hooks = {
        status: (buffer) => { const at = index(buffer), queue = queues.get(at); if (queue?.length) status[at] = queue.shift(); events.push(`Q:${ordinal(buffer)}`); return status[at]; },
        play: (buffer, flags) => { status[index(buffer)] = 1 | (flags ? 4 : 0); events.push(`P:${ordinal(buffer)}:0:0:${flags}`); },
        setPosition: (buffer, position) => { events.push(`C:${ordinal(buffer)}:${position}`); },
        setVolume: (buffer, value) => { events.push(`V:${ordinal(buffer)}:${value}`); return device.gainResult; },
        setPan: (buffer, value) => { events.push(`N:${ordinal(buffer)}:${value}`); return device.balanceResult; },
        stop: (buffer) => { status[index(buffer)] &= ~5; events.push(`X:${ordinal(buffer)}`); },
        duplicate: (buffer) => { events.push(`D:${ordinal(buffer)}`); return { result: device.copyResult, buffer: device.copyResult >= 0 ? create() : 0 }; },
        load: (name) => { const buffer = device.loadKind === 'b' ? create() : device.loadKind === 's' ? SILENT_SOUND_BUFFER : 0; events.push(`F:${name}`); return { buffer, attenuation: device.attenuation }; },
        log: (channel, text) => { events.push(`G:${channel}:${text.replace(/[\n ]+$/, '')}`); },
    };
    device.report = (_expression, line) => { events.push(`!${line}`); };
    device.take = () => events.splice(0).join(';') || '-';
    return device;
}

// 한 독립 관측의 모든 연산과 사건·반환·물리 표를 원본 순서로 비교한다.
function replay(row, caseNumber) {
    const device = deviceFixture(), state = createSoundState(), list = new SoundList(device.report), player = new SoundPlayer(list, state, device.hooks, device.report);
    const results = [], observed = [];
    // 선행 연산의 항목 참조는 같은 실제 표의 논리 주소다.
    const handle = (text) => text.startsWith('$') ? results[Number(text.slice(1))] : Number(text) >>> 0;
    // 한 연산 뒤마다 장치 사건을 분리해 원본 결과 형식으로 남긴다.
    for (const operation of row[2].split('|')) {
        const op = operation.split(':'); let result = 0, shown = '-', entry = false;
        if (op[0] === 'i') { list.initialize(); result = list.fallback; entry = true; }
        else if (op[0] === 'n') { result = list.lookup(nameFromHex(op[1])); entry = true; }
        else if (op[0] === 'g') {
            const key = { initialized: 'initialized', device: 'device', enabled: 'enabled', swap: 'swapSpeakers', max: 'maxPlaying', playing: 'playing', master: 'masterVolume', depth: 'volumeHoldDepth', pending: 'pendingMasterVolume' }[op[1]];
            if (!key) throw new Error('관측 전역 오류'); state[key] = ['initialized', 'device', 'enabled', 'swap'].includes(op[1]) ? Number(op[2]) !== 0 : signed(op[2]);
        } else if (op[0] === 'view') Object.assign(state.view, { left: signed(op[1]), top: signed(op[2]), right: signed(op[3]), bottom: signed(op[4]) });
        else if (op[0] === 'cam') Object.assign(state.view, { cameraX: signed(op[1]), cameraY: signed(op[2]) });
        else if (op[0] === 'st') device.status[Number(op[1])] = Number(op[2]);
        else if (op[0] === 'sq') device.queues.set(Number(op[1]), op[2].split(',').map(Number));
        else if (op[0] === 'hr') device[{ copy: 'copyResult', gain: 'gainResult', balance: 'balanceResult' }[op[1]]] = signed(op[2]);
        else if (op[0] === 'ld') { device.loadKind = op[1]; device.attenuation = signed(op[2]); }
        else if (op[0] === 'clr') list.setField(handle(op[1]), SOUND_FIELD.buffer, 0);
        else if (op[0] === 'play') { result = player.play(handle(op[1]), Number(op[2]), signed(op[3]), signed(op[4]), Number(op[5]), signed(op[6])); entry = true; }
        else if (op[0] === 'loop') { result = player.playLoopAt(bitsToFloat(op[1]), bitsToFloat(op[2]), handle(op[3]), handle(op[4])); entry = true; }
        else if (op[0] === 'once') { result = player.playOnceAt(bitsToFloat(op[1]), bitsToFloat(op[2]), handle(op[3]), Number(op[4]), Number(op[5])); entry = true; }
        else if (op[0] === 'stop') player.stop(handle(op[1]));
        else if (op[0] === 'isp') { result = player.isPlaying(handle(op[1])); shown = String(result); }
        else if (op[0] === 'name') { result = player.playByName(nameFromHex(op[1]), Number(op[2]), signed(op[3]), signed(op[4]), Number(op[5]), signed(op[6])); entry = true; }
        else if (op[0] === 'at') { result = player.playNameAt(bitsToFloat(op[1]), bitsToFloat(op[2]), nameFromHex(op[3]), Number(op[4])); entry = true; }
        else if (op[0] === 'recount') { result = player.recount(); shown = String(result | 0); }
        else if (op[0] === 'stopname') player.stopByName(nameFromHex(op[1]));
        else if (op[0] === 'isname') { result = player.isPlayingByName(nameFromHex(op[1])); shown = String(result); }
        else if (op[0] === 'switch') { result = player.setNamePlaying(signed(op[1]), op[2] === '-' ? null : nameFromHex(op[2]), Number(op[3]), signed(op[4])); shown = String(result | 0); }
        else if (op[0] === 'stoploops') player.stopLoops();
        else if (op[0] === 'master') player.setMasterVolume(signed(op[1]));
        else throw new Error(`관측 연산 오류: ${op[0]}`);
        if (entry) shown = result ? String((result - SOUND_LIST_BASE) | 0) : 'null';
        results.push(result); observed.push(`${device.take()}=${shown}`);
    }
    assert.equal(observed.join('|'), row[3], `사건 ${caseNumber}`);
    assert.deepEqual([list.firstFree - SOUND_LIST_BASE, adler32(list.raw()), state.playing, state.serial], [Number(row[4]), Number(row[5]), signed(row[6]), Number(row[7])], `표 ${caseNumber}`);
    if (row.length === 11) assert.deepEqual([state.masterVolume, state.volumeHoldDepth, state.pendingMasterVolume], [signed(row[8]), signed(row[9]), signed(row[10])], `음량 ${caseNumber}`);
}

// 상태·장치 분기를 포함한 모든 독립 스크립트를 빠짐없이 재생한다.
test('SoundPlayer: 원본 10.78 재생·복제·위치·정지 1065행', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/soundplay-x86.tsv')).filter((row) => row[0] === 'originals' && row[1] === 'Script');
    assert.equal(rows.length, 1065);
    // 행마다 새 원본 표와 장치 상태를 사용한다.
    for (let index = 0; index < rows.length; index += 1) replay(rows[index], index);
});

// 현재 상태의 재조회와 이름·반복·전체 음량 제어를 원본 독립 관측과 대조한다.
test('SoundPlayer: 원본 10.78 제어·재계산·반복 정지 1063행', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/soundcontrol-x86.tsv')).filter((row) => row[0] === 'originals' && row[1] === 'Script');
    assert.equal(rows.length, 1063);
    // 각 관측은 독립 장치와 이름 표에서 시작한다.
    for (let index = 0; index < rows.length; index += 1) replay(rows[index], index);
});

// 사전 적재는 재생 옵션과 무관하며 같은 실제 재생기가 프로세스 효과를 받는다.
test('SoundPlayer: 사전 적재와 SoundProcess 실제 재생 경계', () => {
    const device = deviceFixture(), state = createSoundState(), list = new SoundList(device.report), player = new SoundPlayer(list, state, device.hooks, device.report);
    list.initialize(); device.attenuation = 125;
    const sound = player.preload('thunderCrack.wav'); assert.equal(device.take(), 'F:thunderCrack.wav');
    assert.equal(list.field(sound, SOUND_FIELD.attenuation), 125); assert.equal(player.preload('thunderCrack.wav'), sound); assert.equal(device.take(), '-');
    state.initialized = state.device = state.enabled = true;
    const hooks = makeSoundProcessHooks(player, () => 7); assert.equal(hooks.wallSeconds(), 7);
    assert.equal(hooks.play(sound, 0, 0, 0, 0, 0), sound); assert.equal(hooks.isPlaying(sound), 1); hooks.stop(sound);
    assert.equal(hooks.isPlaying(sound), 0); assert.equal(state.playing, 0);
    assert.throws(() => new SoundPlayer(list, state, {}), /필수 장치/);
});

// 외부 효과로 보기가 바뀔 수 있으므로 투영·가시·좌우·음량은 현재 보기를 각각 읽는다.
test('SoundPlayer: 위치 재생의 현재 화면 재조회', () => {
    const device = deviceFixture(), state = createSoundState(), list = new SoundList(device.report), player = new SoundPlayer(list, state, device.hooks, device.report);
    list.initialize(); state.initialized = state.device = state.enabled = true; let reads = 0;
    state.currentView = () => { reads += 1; return { cameraX: 0, cameraY: 0, left: 0, top: 0, right: 640, bottom: 480 }; };
    const sound = list.lookup('priestFall.wav'); assert.equal(player.playOnceAt(20, 21, sound, 0, 0), sound); assert.equal(reads, 4);
});
