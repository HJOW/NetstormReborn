// 기술 획득 부모(00492a30)·테마 기술 지급(00492ae0)·소유자 색 변경(0043ca70)을 원본 10.78 관측으로 대조한다(WB04-E13, cpppj NewTechnologyTests·ThemeTechnologyTests·PlayerColorTests 이식).
//  - new-technology-x86.tsv 45행: 현재 기술 부모 재조회·중복 기술도 저장/안내 실행·각 중단 뒤 두 Config 부분 효과를 전체 호출 순서로 대조한다.
//  - theme-technology-x86.tsv 1200행: 타입 필터(group 10·theme 5)·현재 개수/기술 부모 재조회·생성 기록을 전체 호출 순서로 대조한다.
//  - player-color-x86.tsv 216행: 9 소유자·4색·전투 2조건·3 배치에서 색 표 36 BYTE, 풀 800 BYTE 창, 프레임 요청 순서를 대조한다.
//  - 계약: 선택 장치 누락은 앞선 효과 보존, 필수 경계 누락 거부, 프레임 소비자 실패는 색 표 쓰기를 되돌리지 않음.
// (2026-11-08 WB04-E13: 기술 획득·테마 기술·소유자 색 변경 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { acquireNewTechnology } from '../www/core/raw-new-technology.js';
import { RawThemeTechnology } from '../www/core/raw-theme-technology.js';
import { changePlayerColor } from '../www/core/raw-player-color.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E13)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const newRows = fixture('new-technology-x86.tsv');
const themeRows = fixture('theme-technology-x86.tsv');
const colorRows = fixture('player-color-x86.tsv');

// 원시 바이트(이진 문자열)를 소문자 hex 로 표현하거나 hex 를 이진 문자열로 읽는다('-' 는 빈 문자열). (2026-11-08 WB04-E13)
const hexOf = (value) => Array.from(value, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('');
const bytesOf = (text) => (text === '-' ? '' : text.match(/../g)?.map((pair) => String.fromCharCode(parseInt(pair, 16))).join('') ?? '');
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 원문 경계의 독립 응답만 주고 제품 함수가 순서·조건을 선택하게 하는 기술 획득 장면이다(cpppj Probe). (2026-11-08 WB04-E13)
class NewTechnologyProbe {
    constructor(found, caption, stop) {
        this.found = found; this.caption = caption; this.stop = stop;
        this.parents = 0; this.events = 0; this.trace = []; this.config = new Map();
    }

    // 효과보다 먼저 callback 진입을 기록하고 지정한 번째 진입에서 중단한다.
    event(value, Stop) { this.trace.push(value); this.events += 1; if (this.stop && this.events === this.stop) throw new Stop(); }

    hooks(Stop) {
        return {
            technologyParent: () => { const value = [10, 20, 30][this.parents < 3 ? this.parents : 2]; this.parents += 1; this.event(`G,${value}`, Stop); return value; },
            find: (parent, mask, kind, index) => { this.event(`Q,${parent},${mask},${kind},${index}`, Stop); return this.found; },
            create: (parent, kind, argument, flags, attach, quality) => this.event(`I,${parent},${kind},${argument},${flags},${attach},${quality}`, Stop),
            snapshot: () => this.event('S', Stop),
            type: (type) => { this.event(`T,${type}`, Stop); return 0x12345678; },
            name: (record) => { this.event(`N,${record}`, Stop); return this.caption; },
            configure: (key, value) => { this.event(`C,${hexOf(key)},${hexOf(value)}`, Stop); this.config.set(key, value); },
            // 256 BYTE 지역 버퍼의 서식이다(첫 NUL 에서 끝).
            format: (pattern, value) => { this.event(`F,${hexOf(pattern)},${value}`, Stop); return pattern.replace('%d', String(value)).slice(0, 255); },
            tell: (key) => this.event(`L,${hexOf(key)}`, Stop),
        };
    }
}

// 같은 최초 입력과 명시 중단으로 원문에서 동결한 45개 전체 순서/두 Config 결과를 비교한다.
test('기술 획득: 원본10.78 45행(현재 부모·중복 기술·중단 뒤 두 Config 부분 효과)', { skip: !newRows && '원본 fixture 없음' }, () => {
    class Stop extends Error {}
    let count = 0;
    for (const fields of newRows) {
        assert.equal(fields.length, 8);
        // 각 원문 관측에 새 장치를 제공해 앞 사례의 상태를 섞지 않는다.
        const probe = new NewTechnologyProbe(number(fields[1]), bytesOf(fields[2]), number(fields[3]));
        let completed = true;
        try { acquireNewTechnology(number(fields[0]), probe.hooks(Stop)); } catch (error) { if (error instanceof Stop) completed = false; else throw error; }
        const label = `행 ${count} ${fields.slice(0, 4).join(',')}`;
        assert.equal(completed, fields[4] === '1', `${label} 완료`);
        assert.equal(probe.trace.join(';'), fields[5], `${label} 호출 순서`);
        // 미작성과 작성한 빈 값을 별개로 비교한다.
        for (const [key, column] of [['global.newTechName', 6], ['global.newTechPic', 7]]) {
            assert.equal(probe.config.has(key), fields[column] !== '-', `${label} ${key} 작성 여부`);
            if (probe.config.has(key)) assert.equal(hexOf(probe.config.get(key)), fields[column], `${label} ${key} 값`);
        }
        count += 1;
    }
    assert.equal(count, 45);
});

// 선택된 저장 장치 누락은 기술 추가까지의 부분 효과를 유지하며 설정/안내 성공으로 바꾸지 않는다.
test('기술 획득: snapshot 누락은 앞선 효과를 남긴다', () => {
    class Stop extends Error {}
    const probe = new NewTechnologyProbe(0, 'Sun Cannon', 0);
    const hooks = probe.hooks(Stop);
    delete hooks.snapshot;
    assert.throws(() => acquireNewTechnology(83, hooks), /선택 장치 미연결/);
    assert.equal(probe.trace.join(';'), 'G,10;G,20;Q,20,4,83,0;G,30;I,30,83,0,4,16,0');
    assert.equal(probe.config.size, 0);
});

// 입력 타입 개수·기록·존재 여부를 명시하고 외부 반응만 공급하는 테마 기술 장면이다(cpppj ThemeScene). (2026-11-08 WB04-E13)
class ThemeScene {
    constructor(text) {
        const values = text.split(',');
        assert.equal(values.length, 6);
        this.input = values.map((value) => Number(value) | 0);
        this.count = this.input[1];
        this.parent = 50;
        this.countChanged = false;
        this.trace = [];
        this.born = [];
        this.contained = new Map();
        // 번호 0 을 포함하는 독립 타입 32개의 두 필드 입력이다.
        this.types = Array.from({ length: 32 }, (_, index) => ThemeScene.typeInput(index, this.input[2]));
        // 명시 하위 존재 여부는 본체의 필터와 무관하게 모든 부모/타입에서 준비한다.
        for (let owner = 50; owner < 53; owner += 1) {
            for (let type = 0; type < 32; type += 1) if (this.input[3] && (owner + type) % 3 === 0) this.contained.set(`${owner},${type}`, 1000 + owner * 32 + type);
        }
    }

    // 선택된 독립 TYPE 입력이다.
    static typeInput(index, variant) {
        if (variant === 0) return { group: index % 5 === 0 ? 10 : index % 6, theme: (index % 8) - 1 };
        if (variant === 1) return { group: index % 7 === 6 ? 10 : (index * 3) % 11, theme: ((index + 3) % 8) - 1 };
        const themes = [0, 1, 2, 3, 4, 5, -1, 6];
        return { group: index % 4 === 2 ? 10 : index % 3, theme: themes[index % 8] };
    }

    add(value) { this.trace.push(value); }

    // 현재 부모를 세 명시 부모 사이에서 한 칸 전진한다.
    advanceParent() { this.parent = 50 + ((this.parent - 49) % 3); }

    // 정해진 하위 경계에서만 현재 count 를 한 번 수정한다.
    changeCount(boundary, type) {
        if (this.countChanged) return;
        if (this.input[5] === 1 && boundary === 'find') this.count = type + 1;
        else if ((this.input[5] === 2 && boundary === 'create') || (this.input[5] === 3 && boundary === 'record')) this.count = 12;
        else return;
        this.countChanged = true;
    }

    hooks() {
        return {
            typeCount: () => { this.add(`N,${this.count}`); return this.count; },
            typeAt: (type) => {
                const record = this.types[type];
                assert.ok(record, '타입 범위');
                this.add(`R,${type},${record.group},${record.theme}`);
                this.changeCount('record', type);
                return record;
            },
            currentTechnology: () => { const current = this.parent; this.add(`G,${this.parent}`); if (this.input[4] === 4) this.advanceParent(); return current; },
            findContained: (owner, mask, type, index) => {
                const result = this.contained.get(`${owner},${type}`) ?? 0;
                this.add(`F,${owner},${mask},${type},${index},${result}`);
                this.changeCount('find', type);
                if (this.input[4] === 1 || this.input[4] === 3) this.advanceParent();
                return result;
            },
            createContained: (owner, type, argument, flags, attach, quality) => {
                const result = 700 + this.born.length;
                this.add(`C,${owner},${type},${argument},${flags},${attach},${quality},${result}`);
                this.born.push([owner, type, result]);
                this.contained.set(`${owner},${type}`, result);
                this.changeCount('create', type);
                if (this.input[4] === 2 || this.input[4] === 3) this.advanceParent();
                return result;
            },
        };
    }

    // 경계에서 실제 생성된 기록만 원본과 같은 형식으로 내보낸다.
    bornText() { return this.born.length ? this.born.map((row) => row.join(',')).join(';') : '-'; }
}

// 실제 원본 1200개 고유 입력의 전체 호출 순서와 현재 count/부모 및 생성 기록을 대조한다.
test('테마 기술: 원본10.78 1200행(필터·현재 개수/부모 재조회·생성 기록)', { skip: !themeRows && '원본 fixture 없음' }, () => {
    const unique = new Set();
    let count = 0;
    for (const fields of themeRows) {
        assert.equal(fields.length, 5);
        assert.ok(!unique.has(fields[0]), '고유 입력');
        unique.add(fields[0]);
        const scene = new ThemeScene(fields[0]);
        new RawThemeTechnology(scene.hooks()).apply(scene.input[0]);
        const label = `행 ${count} ${fields[0]}`;
        assert.equal(scene.trace.join(';'), fields[1], `${label} 호출 순서`);
        assert.equal(scene.count, Number(fields[2]), `${label} count`);
        assert.equal(scene.parent, Number(fields[3]), `${label} 부모`);
        assert.equal(scene.bornText(), fields[4], `${label} 생성 기록`);
        count += 1;
    }
    assert.equal(count, 1200);
});

// 실제 하위 경계를 누락했을 때 아무 동작도 하지 않은 성공으로 숨기지 않는다.
test('테마 기술: 다섯 경계 모두 필수이다', () => {
    for (const name of ['typeCount', 'typeAt', 'currentTechnology', 'findContained', 'createContained']) {
        const hooks = new ThemeScene('0,1,2,0,0,0').hooks();
        delete hooks[name];
        assert.throws(() => new RawThemeTechnology(hooks), TypeError, name);
    }
});

// 9소유자·4색·전투 2조건·3배치의 216입력을 같은 24000슬롯 풀에서 검사한다.
test('소유자 색 변경: 원본10.78 216행(색 표 36 BYTE·풀 800 BYTE 창·프레임 요청 순서)', { skip: !colorRows && '원본 fixture 없음' }, () => {
    assert.equal(colorRows[0][0], 'input');
    let count = 0;
    for (const row of colorRows.slice(1)) {
        assert.equal(row.length, 4);
        const input = row[0].split(',');
        assert.equal(input.length, 4);
        const [owner, color, battle, layout] = [number(input[0]), number(input[1]), number(input[2]), number(input[3])];
        // 원본 출력으로 다음 입력을 만들지 않고 매번 새 풀을 소유한다.
        const pool = new SidPool(24000, { server: false });
        const state = { islandType: 94, stalagType: 95, battle: battle !== 0, ownerColors: Int32Array.from([0, 1, 2, 3, 4, 5, 6, 7, 8]) };
        const events = [];
        // 모든 입력 슬롯을 먼저 확보하고 원본 관찰기와 같은 바이트를 채운다.
        for (let index = 5; index <= 20; index += 1) {
            const sid = pool.allocate(2);
            assert.equal(sid, index);
            const raw = pool.allocated(sid);
            for (let byte = 0; byte < raw.length; byte += 1) raw[byte] = (index * 13 + byte * 29 + layout * 17) & 0xff;
            raw[10] = [94, 95, 82, 96][(index + layout) % 4];
            raw[11] = [0, 1, 2, 4, 128, 34][(index + layout) % 6];
            raw[34] = (index * 3 + layout) % 9;
        }
        changePlayerColor(pool, state, owner, color, (sid, frame, flags) => {
            events.push(`${sid},${frame},${flags}`);
            put(pool.allocated(sid), 36, frame >>> 0);
        });
        const label = `행 ${count} ${row[0]}`;
        assert.equal(Buffer.from(state.ownerColors.buffer, state.ownerColors.byteOffset, 36).toString('hex'), row[1], `${label} 색 표`);
        assert.equal(Buffer.from(pool.bytes().subarray(250, 1050)).toString('hex'), row[2], `${label} 풀`);
        assert.equal(events.length ? events.join(';') : '-', row[3], `${label} 프레임 요청`);
        count += 1;
    }
    assert.equal(count, 216);
});

// 선택된 프레임 소비자의 실패는 원본처럼 먼저 쓴 색 표를 되돌리지 않는다.
test('소유자 색 변경: 프레임 소비자 누락은 색 쓰기를 남기고 범위 밖 소유자는 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    const sid = pool.allocate(2);
    const raw = pool.allocated(sid);
    raw[10] = 94;
    raw[34] = 2;
    const state = { islandType: 94, stalagType: 95, battle: false, ownerColors: Int32Array.from([0, 1, 2, 3, 4, 5, 6, 7, 8]) };
    assert.throws(() => changePlayerColor(pool, state, 2, 8, null), /프레임 소비자 누락/);
    assert.equal(state.ownerColors[2], 8);
    assert.throws(() => changePlayerColor(pool, state, 9, 1, null), RangeError);
});
