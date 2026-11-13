// 팔레트 읽기 뒤 지우기 관문 screen-palette-apply.js(applyScreenPaletteClear)를 원본 10.78 PE 접두 166관측과 계약으로 대조한다.
//  - screen-palette-clear-x86.tsv 166행(clear/flags/depth/작은 clip/하위 변경 × 두 x87 제어): 중단 원인(checkpoint·directdraw-vblank·directdraw-flip·lock·fill·unlock·update), 13개 현재 필드, 64바이트 물리 전체, 호출 trace 가 같다.
//    Device 는 cpppj ScreenPaletteApplyTests 의 명시 장치(Lock/Fill/Unlock/Update 만 모델링)를 그대로 옮긴 것이다. 실제 GDI/DirectDraw 표면은 실행하지 않는다.
//  - 계약: clear 0 또는 flags 0 은 loaded 만 쓰고 장치를 요구하지 않으며, 도달한 필수 하위 장치의 누락은 부분 효과와 함께 오류를 낸다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E376: 5-A 팔레트 계열 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { applyScreenPaletteClear } from '../www/core/screen-palette-apply.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E376)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'screen-palette-clear-x86.tsv'));

// 격리 원문과 동일한 명시 장치 토큰이다. (2026-11-13 WB04-E376)
const SURFACE = 0x12001000;
const OTHER = 0x12002000;
const PIXELS = 0x12005000;

// 콜백 중단을 부모의 정상 checkpoint 와 구별한다. (2026-11-13 WB04-E376)
class Stop extends Error {
    constructor(cause) {
        super(cause);
        this.cause_ = cause;
    }
}

// 원문에서 명시 장치였던 Lock/Fill/Surface.Unlock/Update 만 모델링한다. 순서는 제품 함수가 소유한다. (2026-11-13 WB04-E376)
class Device {
    constructor(fields) {
        assert.equal(fields.length, 5);
        this.clear = Number(fields[0]);
        this.flags = Number(fields[1]);
        this.depth = Number(fields[2]);
        this.view = [0, 0, 8, 4];
        this.clip = fields[3] === '1' ? [1, 1, 7, 3] : [...this.view];
        this.mutation = fields[4];
        this.draw = SURFACE;
        this.buffer = 0;
        this.pitch = 0;
        this.vfx = 0;
        this.loaded = 77;
        this.width = 8;
        this.height = 4;
        this.pixels = Uint8Array.from({ length: 64 }, (_, index) => index);
        this.trace = [];
        if (this.depth !== 0) { this.buffer = PIXELS; this.vfx = PIXELS; this.pitch = 8; }
    }

    // 원문 장치의 현재 값 변경/중단만 반영한다.
    mutate(boundary) {
        if (this.mutation === `${boundary}-stop`) throw new Stop(boundary);
        if (this.mutation === 'lock-size' && boundary === 'lock') { this.width = 6; this.height = 3; } else if (this.mutation === 'lock-flags0' && boundary === 'lock') this.flags = 0;
        else if (this.mutation === 'fill-draw' && boundary === 'fill') this.draw = OTHER;
        else if (this.mutation === 'fill-flags1' && boundary === 'fill') this.flags = 0x81;
        else if (this.mutation === 'unlock-buffer' && boundary === 'unlock') { this.buffer = PIXELS + 16; this.vfx = PIXELS + 16; this.pitch = 99; } else if (this.mutation === 'update-clip' && boundary === 'update') this.clip = [2, 2, 3, 3];
        else if (this.mutation === 'update-flags4' && boundary === 'update') this.flags = 0x84;
    }

    // 현재 13개 raw 필드를 쉼표로 표시한다.
    values() { return [this.flags, this.depth, this.draw, this.buffer, this.pitch, this.vfx, this.loaded, this.width, this.height, ...this.clip].join(','); }

    // 지우기 전후 모든 픽셀을 생략 없이 표시한다.
    raw() { return Buffer.from(this.pixels).toString('hex'); }

    // 실제 호출자와 같은 clear 관문을 제품 함수에 맡기고 하위만 명시한다.
    run() {
        const loaded = { value: this.loaded };
        const hooks = {
            lock: () => {
                const before = this.depth;
                this.depth += 1;
                this.trace.push(`LOCK:${before}`);
                if (before === 0) { this.buffer = PIXELS; this.vfx = PIXELS; this.pitch = 8; }
                this.mutate('lock');
            },
            fillBlack: () => {
                const rect = [0, 0, this.width, this.height];
                this.trace.push(`FILL:${rect.join(',')}:0:${this.clip.join(',')}`);
                // lock 뒤의 현재 크기와 clip 이다.
                if ((this.flags & 0x80) !== 0 && this.buffer !== 0) {
                    const left = Math.max(this.clip[0], Math.min(rect[0], this.clip[2]));
                    const top = Math.max(this.clip[1], Math.min(rect[1], this.clip[3]));
                    const right = Math.max(this.clip[0], Math.min(rect[2], this.clip[2]));
                    const bottom = Math.max(this.clip[1], Math.min(rect[3], this.clip[3]));
                    // 끝 좌표를 포함하지 않는다.
                    for (let y = top; y < bottom; y += 1) for (let x = left; x < right; x += 1) this.pixels[this.buffer - PIXELS + y * this.pitch + x] = 0;
                }
                this.mutate('fill');
            },
            unlock: () => {
                this.depth -= 1;
                if (this.depth === 0) {
                    this.trace.push(`UNLOCK:${this.draw}`);
                    this.mutate('unlock');
                    this.buffer = 0;
                    this.pitch = 0;
                    this.vfx = 0;
                }
            },
            waitVerticalBlank: () => {
                this.trace.push(`WAIT:${this.flags}`);
                if ((this.flags & 1) !== 0) throw new Stop('directdraw-vblank');
            },
            presentPreservingClip: () => {
                this.trace.push(`PRESERVE:${this.clip.join(',')}`);
                const previous = this.clip;
                this.trace.push(`UPDATE:${this.view.join(',')}`);
                this.clip = [...this.view];
                this.mutate('update');
                this.clip = previous;
            },
            flip: () => {
                this.trace.push(`FLIP:${this.flags}`);
                if ((this.flags & 4) !== 0) throw new Stop('directdraw-flip');
            },
        };
        try {
            applyScreenPaletteClear(this.clear, this.flags, loaded, hooks);
            this.loaded = loaded.value;
            this.trace.push('APPLY:0,256,0,1');
        } catch (error) {
            this.loaded = loaded.value;
            if (error instanceof Stop) return error.cause_;
            throw error;
        }
        return 'checkpoint';
    }
}

test('ScreenPaletteApply: 원본 10.78 PE 접두 166관측의 중단 원인·13개 필드·64바이트 물리·호출 trace 가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let compared = 0;
    let checkpoint = 0;
    // 두 정밀도도 별개 장치로 다시 초기화한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const device = new Device(row[1].split(','));
        const cause = device.run();
        const where = `행 ${compared} ${row[1]}`;
        assert.equal(cause, row[2], `${where} 원인`);
        assert.equal(device.values(), row[3], `${where} 필드`);
        assert.equal(device.raw(), row[4], `${where} 물리`);
        assert.equal(device.trace.join(';'), row[5], `${where} trace`);
        compared += 1;
        if (cause === 'checkpoint') checkpoint += 1;
    }
    assert.equal(compared, 166);
    assert.equal(checkpoint, 122);
});

test('ScreenPaletteApply: clear 0 또는 flags 0 은 loaded 만 쓰고 장치를 요구하지 않으며 도달한 필수 장치의 누락은 오류이다', () => {
    const loaded = { value: 77 };
    applyScreenPaletteClear(1, 0, loaded, {});
    assert.equal(loaded.value, 1);
    loaded.value = 77;
    applyScreenPaletteClear(0, 0x8a, loaded, {});
    assert.equal(loaded.value, 1);
    // 도달했는데 장치가 없으면 loaded 쓰기 뒤에 오류이며 첫 장치가 없으므로 아무 부분 효과도 없다.
    loaded.value = 77;
    assert.throws(() => applyScreenPaletteClear(1, 0x8a, loaded, {}), /필수 하위 장치 미연결/u);
    assert.equal(loaded.value, 1);
    // 호출 순서의 부분 효과: 뒤 장치가 빠지면 앞 장치까지만 실행된다.
    const order = [];
    const partial = { lock: () => order.push('lock'), fillBlack: () => order.push('fill'), unlock: () => order.push('unlock') };
    assert.throws(() => applyScreenPaletteClear(5, 1, loaded, partial), /미연결/u);
    assert.deepEqual(order, ['lock', 'fill', 'unlock']);
    // 모든 장치가 있으면 원문 순서 여섯 호출이다.
    const full = [];
    const hooks = Object.fromEntries(['lock', 'fillBlack', 'unlock', 'waitVerticalBlank', 'presentPreservingClip', 'flip'].map((name) => [name, () => full.push(name)]));
    applyScreenPaletteClear(1, 0xffffffff, loaded, hooks);
    assert.deepEqual(full, ['lock', 'fillBlack', 'unlock', 'waitVerticalBlank', 'presentPreservingClip', 'flip']);
});
