// 보호 10.78의 장면 음악 독립 관측을 같은 현재 상태와 효과 순서로 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GameRandom } from '../www/core/random.js';
import { SceneMusic, SceneMusicState } from '../www/audio/scene-music.js';

// fixture의 64비트 double 비트와 곡 이름을 손실 없이 옮긴다.
function number(hex) { return Buffer.from(hex, 'hex').readDoubleBE(); }
function bits(value) { const data = Buffer.alloc(8); data.writeDoubleBE(value); return data.toString('hex'); }
function text(hex) { return hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'); }
function hex(value) { return value ? Buffer.from(value, 'latin1').toString('hex') : '-'; }

// 각 행은 새로운 음악·난수 수명과 원본에 입력한 장치 값만 사용한다.
function run(script) {
    const state = new SceneMusicState(), random = new GameRandom(0), events = [], results = [];
    const record = (value) => { results.push(`${events.splice(0).join(';') || '-'}=${value}`); };
    state.tints.set([0x1111, 0x2222, 0x3333, 0x4444]);
    let current = '', duration = 0, clocks = [], durations = [], sacrifices = [], waits = [], options = [];
    const next = (values, fallback) => values.length > 1 ? values.shift() : values.length ? values[0] : fallback;
    const hooks = {
        wallSeconds: () => { events.push('T'); return next(clocks, 0); }, currentName: () => current,
        select: (name) => { events.push(`S:${hex(name)}`); current = name; const value = next(durations, null); if (value !== null) duration = value; }, duration: () => duration,
        sacrificing: (owner) => { events.push(`C:${owner}`); return Boolean(next(sacrifices, 0)); },
        waitingRoom: () => { events.push('W'); return Boolean(next(waits, 0)); }, refresh: () => events.push('R'),
        ascendancyPalette: () => { events.push('O'); return Boolean(next(options, 0)); },
        loadPalette: (name) => { events.push(`J:${hex(name)}`); events.push('A'); }, thunderFlash: () => events.push('K'),
        weatherSound: (name) => events.push(`N:${hex(name)}`),
    };
    const music = new SceneMusic(state, random, hooks), field = { index: 'index', result: 'resultState', battle: 'battle', players: 'playersReady', local: 'localPlayer', tint: 'tint', dirty: 'paletteDirty' };
    // 한 스크립트의 입력·호출을 원본 순서로 실행한다.
    for (const operation of script.split('|')) {
        const [kind, first, second] = operation.split(':');
        if (kind === 'i') state[field[first]] = Number(second);
        else if (kind === 't') state.tints[Number(first)] = Number(second);
        else if (kind === 'f') state[{ fanfare: 'fanfareEnd', defeat: 'defeatEnd', song: 'songEnd' }[first]] = number(second);
        else if (kind === 'cur') current = text(first);
        else if (kind === 'rng') random.setState(Number(first));
        else if (kind === 'clk') clocks = first.split(';').map(number);
        else if (kind === 'durs') durations = first.split(';').map((value) => value === 'keep' ? null : number(value));
        else if (kind === 'sac') sacrifices = first.split(';').map(Number);
        else if (kind === 'wait') waits = first.split(';').map(Number);
        else if (kind === 'opt') options = first.split(';').map(Number);
        else if (kind === 'lock') record(music.resultMusicLocked() ? '1' : '0');
        else if (kind === 'req') { music.request(text(first)); record('-'); }
        else if (kind === 'next') { music.next(); record('-'); }
        else if (kind === 'frame') { music.frame(); record('-'); }
        else if (kind === 'start') { music.start(); record('-'); }
        else if (kind === 'weather') { music.applyWeather(); record('-'); }
        else throw new Error(`미등록 장면 음악 fixture 연산: ${kind}`);
    }
    return [results.join('|'), state.index, bits(state.fanfareEnd), bits(state.defeatEnd), bits(state.songEnd), state.tint, state.paletteDirty, random.state, hex(current), bits(duration), state.resultState].map(String);
}

// 원본이 남긴 모든 호출 순서·반환·전역·공유 난수와 곡 상태를 비교한다.
test('장면 음악: 원본 10.78 독립 관측 전체 대조', () => {
    const fixture = fs.readFileSync(new URL('../../cpppj/tests/fixtures/scenemusic-x86.tsv', import.meta.url), 'utf8');
    const rows = fixture.split(/\r?\n/).filter((line) => line.startsWith('originals\t')).map((line) => line.split('\t'));
    assert.equal(rows.length, 3113);
    for (const row of rows) {
        const actual = run(row[2]);
        assert.deepEqual(actual, row.slice(3), row[2]);
    }
});

// 현재 전투 조회는 시각 관문을 지난 위치에서만 실행하며 부수 효과 뒤에 다시 읽는다.
test('장면 음악: 현재 전투 조회와 갱신된 모드를 사용한다', () => {
    const state = new SceneMusicState(), random = new GameRandom(1), calls = []; let battle = 0, name = ''; const times = [10, 20, 200, 201];
    state.songEnd = 20; state.currentBattle = () => { calls.push('battle'); return battle; }; state.tints.set([1, 2, 3, 4]);
    const hooks = { wallSeconds: () => times.shift(), currentName: () => name, select: (value) => { name = value; battle = 1; }, duration: () => 0,
        sacrificing: () => false, waitingRoom: () => false, refresh: () => {}, ascendancyPalette: () => false,
        loadPalette: () => {}, thunderFlash: () => {}, weatherSound: () => {} };
    const music = new SceneMusic(state, random, hooks); music.frame(); assert.deepEqual(calls, []);
    music.start(); assert.deepEqual(calls, ['battle', 'battle']); assert.equal(state.index, 0); assert.equal(name, 'wind22.mus');
});
