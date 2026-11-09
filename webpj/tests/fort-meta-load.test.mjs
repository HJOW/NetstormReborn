// Fort 메타 로더 raw-fort-meta-load.js(Badges 장부 setter/검색/읽기와 Technology wrapper)를 원본 10.78 관측과 cpppj `FortMetaLoadTests` 계약으로 대조한다.
//  - 원본 관측 fortmeta-x86.tsv 의 10.78(originals) 470행: 입력 12 DWORD(종류·type·count·장부 프로필·Template·slot·present·end·깊이·옛 변환표·새 변환표·변이)에서
//    결과(검색 비트)·stream pos/end·깊이·현재 변환표 유무·하위 효과 순서·28×220 BYTE 장부 전체(RLE)·보고뒤 계속한 진단 수가 원본과 한 바이트도 다르지 않다.
//  - 계약: 같은 풀 실제 Contents(Factory→ContainedCreate→ContentForm→FortObjectLoad)가 Technology 목록을 생성하고 pos/깊이/변환표/하위 효과 순서를 지킴,
//    부족한 장부/필수 효과/손상 물리 stream 거부, 진단·기술 부모 조회 뒤에만 섹션 선택, 선택 실패도 깊이 복구, E225 Badges 전역 장부가 같은 배열로 로더와 조회에 쓰임.
// CD 10.72 행(originalCD)과 CD 의 Badges 깊이 증감 분기는 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E226: Fort 메타 로더 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { INDEPENDENT_CONTAINED_STATE, RawContainedCreate } from '../www/core/contained-create.js';
import { FortObjectLoad, createFortObjectLoadState } from '../www/core/fort-object-load.js';
import { GameRandom } from '../www/core/random.js';
import { SECTION_NAMES } from '../www/assets/fort.js';
import { RawFortBadgesGlobals } from '../www/core/raw-fort-badges-globals.js';
import { FortMetaAssertion, RawFortMetaLoad, makeFortMetaContentsHooks } from '../www/core/raw-fort-meta-load.js';

// 저장된 독립 기계어 출력(입력과 관찰 행)을 한 번만 읽는다. originals 관찰만 쓴다. (2026-11-10 WB04-E226)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortmeta-x86.tsv'));
const observations = rows ? rows.filter((row) => row[0] === 'originals') : null;

// 입력 열의 고정 순서이며 기대 값은 fixture 에만 있다. (2026-11-10 WB04-E226)
const FIELD = Object.freeze({ kind: 0, type: 1, count: 2, profile: 3, template: 4, slot: 5, present: 6, end: 7, depth: 8, oldConversion: 9, newConversion: 10, mutation: 11 });
// 진단 종류의 fixture 이름 순서(MissingBadgesTemplate·BadgeCountRange·MissingTechnologyTemplate·TechnologyNotMine)이다. (2026-11-10 WB04-E226)
const ASSERTION_NAMES = ['B', 'C', 'T', 'M'];

// 패딩까지 명시한 28개 220 BYTE 입력 표이다(첫 중복·마지막 unsigned 타입을 원본 입력과 같게 준비한다). (2026-11-10 WB04-E226)
function badgeInput(profile) {
    const bytes = new Uint8Array(6160).fill(0xa5);
    for (let index = 0; index < 28; index += 1) {
        let type = 70 + index;
        if (profile && (index === 1 || index === 8)) type = 80;
        if (profile && index === 27) type = 0xffffffff;
        put(bytes, index * 220, type);
    }
    return bytes;
}

// 전체 raw 표를 원본 무손실 BYTE run(길이:값, 쉼표 구분)으로 직렬화한다. (2026-11-10 WB04-E226)
function badgeRle(bytes) {
    const parts = [];
    for (let start = 0; start < bytes.length;) { // 패딩이나 상위 수량 BYTE 도 하나도 생략하지 않는다.
        let end = start + 1;
        while (end < bytes.length && bytes[end] === bytes[start]) end += 1;
        parts.push(`${end - start}:${bytes[start].toString(16).padStart(2, '0')}`);
        start = end;
    }
    return parts.join(',');
}

// 비정렬 DWORD 를 읽는다. (2026-11-10 WB04-E226)
const dword = (bytes, at) => (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
// 슬롯 WORD 를 읽는다. (2026-11-10 WB04-E226)
const word = (pool, sid, offset) => pool.slot(sid)[offset] | (pool.slot(sid)[offset + 1] << 8);
// RawFortBadgesSave 의 읽기 규칙과 같은 현재 수량 조회이다(처음 일치한 기록의 count, 없으면 0). (2026-11-10 WB04-E226)
function readBadgeCount(records, type) {
    for (let index = 0; index < 28; index += 1) if (dword(records, index * 220) === type >>> 0) return dword(records, index * 220 + 216);
    return 0;
}
// 예외가 나는지 확인한다. (2026-11-10 WB04-E226)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 한 native 입력의 명시 하위 경계와 현재 장부/깊이를 갖는 장면이다. 기대 장부/분기를 계산하지 않고 원본 입력만 초기화한다. (2026-11-10 WB04-E226)
class ReplayScene {
    constructor(pool, fields) {
        this.pool = pool;
        this.input = fields.split(',').map(Number);
        assert.equal(this.input.length, 12);
        this.badges = badgeInput(this.input[FIELD.profile]);
        this.depth = { value: this.input[FIELD.depth] };
        this.state = { templatePresent: this.input[FIELD.template] !== 0, templateSlot: this.input[FIELD.slot] | 0, localOwner: 1, hasTypeConversion: this.input[FIELD.oldConversion] !== 0 };
        const count = this.input[FIELD.kind] === 0 ? this.input[FIELD.count] : 3;
        const data = [count & 0xff];
        // native 메모리의 타입/수량 BYTE 만 공급하며 setter 결과는 계산하지 않는다.
        for (let index = 0; index < count; index += 1) data.push(index % 7 === 6 ? 255 : 70 + (index % 28), (index * 13 + 1) & 0xff);
        this.stream = { bytes: Uint8Array.from(data), position: 19, end: this.input[FIELD.end], present: this.input[FIELD.present] !== 0 };
        this.events = [];
        this.assertions = 0;
    }

    // TypeNames/하위 Contents 실행 완료를 주장하지 않는 명시 관찰 경계이다. (2026-11-10 WB04-E226)
    hooks() {
        return {
            assertion: (reason) => {
                this.events.push(`A:${ASSERTION_NAMES[reason]}`);
                this.assertions += 1;
                if (this.input[FIELD.mutation] === 3) put(this.badges, 0, 999);
            },
            loadingDepthChanged: (value) => { assert.equal(value, this.depth.value); this.events.push(`L:${value}`); },
            technologyParent: () => { this.events.push('P:50'); return 50; },
            prepareTypeNames: () => { this.events.push(`N:${Number(this.state.hasTypeConversion)}`); this.state.hasTypeConversion = this.input[FIELD.newConversion] !== 0; },
            contents: (parent, current, flags, relaxed) => {
                assert.ok(parent === 50 && flags === 17 && relaxed);
                this.events.push(`C:50:17:1:${this.depth.value}:${current.position}`);
                current.position = 5;
                if (this.input[FIELD.mutation] === 2) this.state.hasTypeConversion = true;
            },
            releaseTypeConversion: () => { assert.ok(this.state.hasTypeConversion); this.events.push('F'); },
            refreshTechnologyUi: () => { this.events.push('U'); if (this.input[FIELD.mutation] === 1) this.depth.value += 5; },
        };
    }

    // 이벤트 기록만 연결하며 발생할 이벤트를 미리 계산하지 않는다. (2026-11-10 WB04-E226)
    eventText() { return this.events.length ? this.events.join(';') : '-'; }
}

test('Fort 메타 로더: 원본 10.78 470입력의 장부 전체·stream·깊이·변환표·효과 순서·진단이 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    // 판본별 풀은 장기 객체이며 각 입력의 raw 표와 stream 만 새로 구성한다.
    const pool = new SidPool(24000, { server: false });
    let checked = 0;
    let normal = 0;
    let diagnosed = 0;
    for (const row of observations) {
        assert.equal(row.length, 10);
        const scene = new ReplayScene(pool, row[1]);
        const loader = new RawFortMetaLoad(pool, scene.badges, scene.depth, scene.state, scene.hooks());
        let result = '-';
        const kind = scene.input[FIELD.kind];
        if (kind === 0) loader.loadBadges(scene.stream);
        else if (kind === 1) loader.setBadge(scene.input[FIELD.type], scene.input[FIELD.count] | 0);
        else if (kind === 2) loader.loadTechnology(scene.stream);
        else result = String(loader.findBadge(scene.input[FIELD.type]) >>> 0);
        const label = `행 ${checked} 입력 ${row[1]}`;
        assert.equal(result, row[2], label);
        assert.equal(String(scene.stream.position), row[3], `${label} position`);
        assert.equal(String(scene.stream.end), row[4], `${label} end`);
        assert.equal(String(scene.depth.value), row[5], `${label} depth`);
        assert.equal(String(Number(scene.state.hasTypeConversion)), row[6], `${label} conversion`);
        assert.equal(scene.eventText(), row[7], `${label} events`);
        assert.equal(badgeRle(scene.badges), row[8], `${label} badges`);
        assert.equal(String(scene.assertions), row[9], `${label} assertions`);
        assert.equal(loader.pool, pool);
        checked += 1;
        if (row[9] === '0') normal += 1; else diagnosed += 1;
    }
    assert.equal(checked, observations.length);
    assert.ok(checked === 470 && normal > 0 && diagnosed > 0);
});

test('Fort 메타 로더 계약: 선택한 Technology 내용물만 같은 풀의 실제 Factory→ContainedCreate→ContentForm 으로 생성하고 필수 경계 순서를 지킨다', () => {
    const unsupported = () => { throw new Error('선택 Technology 소비자의 미복원 경계 호출'); };
    const pool = new SidPool(24000, { server: false });
    // 내장 Form 네 생성자와 부모(89) 생성자 주소·내용물 TYPE 두 개를 갖는 188칸 TYPE 표이다.
    const types = Array.from({ length: 188 }, (_, number) => ([1, 2, 3, 6, 89].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}));
    Object.assign(types[89], { footX: 1, footY: 1 });
    Object.assign(types[110], { flags1: 0x1000, contentListFlags: 0x10 });
    Object.assign(types[111], { flags1: 0x2000, contentListFlags: 0x20 });
    const factory = new SquidFactory(pool, types);
    const parent = factory.create(89, 2);
    const children = [];
    let transmissions = 0;
    const forms = new ContentFormHost(pool, types, (owner, child, reason) => { assert.ok(owner === parent && reason === 1); children.push(child); }, { transmit: () => { transmissions += 1; } });
    const contained = new RawContainedCreate(pool, INDEPENDENT_CONTAINED_STATE, forms.bindCreate(factory, { notifySurface: unsupported }));
    const frames = types.map(() => ({ codes: [] }));
    const random = new GameRandom(12345);
    const objectState = createFortObjectLoadState({ value: 0 });
    const objects = new FortObjectLoad({
        pool, finder: { pool, begin: unsupported, next: unsupported }, types, frames, random, state: objectState,
        hooks: {
            create: unsupported, notifySurface: unsupported, setState: unsupported, setOwner: unsupported, hasHitPoints: unsupported, maxHitPoints: unsupported,
            virtualCount: unsupported, pop: unsupported, destroy: unsupported, chunkRegion: unsupported, territoryAllowed: unsupported, loadingDepthChanged: unsupported, finishOwner: unsupported,
            createContent: (owner, type, quantity, flags, attach, quality) => contained.create(owner, type, quantity, flags, attach, quality),
        },
    });
    // Technology 섹션 7 BYTE([2, 110, 9, 111, 254, 255, 0xee])와 마지막 1 BYTE 의 미소비 꼬리를 가진 parsed Fort 이다.
    const names = SECTION_NAMES;
    const fort = { sections: names.map(() => new Uint8Array(0)), technology: { items: [], complete: true }, technologyTrailingBytes: 1 };
    fort.sections[names.indexOf('Technology')] = Uint8Array.from([2, 110, 9, 111, 254, 255, 0xee]);
    const item = (type, quantity) => ({ type, storedType: type, quantity, listFlags: 0, listFlagsStored: false, contents: [], contentsComplete: true });
    fort.technology.items = [item(110, 9), item(111, -2)];
    const depth = { value: 3 };
    const state = { templatePresent: true, templateSlot: 1, localOwner: 1, hasTypeConversion: true };
    const effects = [];
    const hooks = {
        assertion: unsupported,
        loadingDepthChanged: (value) => { assert.equal(value, depth.value); objectState.loadingDepth = value; },
        technologyParent: () => { effects.push('parent'); return parent; },
        prepareTypeNames: () => { effects.push('prepare-boundary'); assert.ok(state.hasTypeConversion); },
        releaseTypeConversion: () => { effects.push('release-boundary'); assert.ok(state.hasTypeConversion); },
        refreshTechnologyUi: () => { effects.push('ui-boundary'); },
        contents: unsupported,
    };
    const bound = makeFortMetaContentsHooks(pool, objects, fort, hooks);
    const badges = badgeInput(0);
    const meta = new RawFortMetaLoad(pool, badges, depth, state, bound);
    const stream = { bytes: fort.sections[names.indexOf('Technology')], position: 19, end: 7, present: true };
    meta.loadTechnology(stream);
    assert.ok(children.length === 2 && transmissions === 0 && stream.position === 6 && stream.end === 7 && depth.value === 3 && objectState.loadingDepth === 3 && !state.hasTypeConversion);
    assert.deepEqual(effects, ['parent', 'prepare-boundary', 'release-boundary', 'ui-boundary']);
    const form = new SquidForms(pool);
    assert.ok(form.parent(children[0]) === parent && form.parent(children[1]) === parent);
    assert.ok(word(pool, children[0], 18) === 110 && word(pool, children[1], 18) === 111);
    assert.ok(word(pool, children[0], 12) === 9 && word(pool, children[1], 12) === 65534 && random.state === 12345);
    assert.ok(word(pool, parent, 6) === children[0] && word(pool, children[0], 4) === children[1]);
    // 다른 풀의 Contents 몸체와 현재 원본 섹션이 아닌 stream 은 거부한다. 느슨한 변환(false)은 미복원 진단 모드로 거부한다.
    assert.ok(throws(() => makeFortMetaContentsHooks(new SidPool(24000, { server: false }), objects, fort, hooks)));
    // 거부는 내용물을 하나도 만들기 전에 이루어져야 한다(자식 수·전송 수·stream 위치 불변).
    const before = { children: children.length, transmissions, position: stream.position };
    const unchanged = () => children.length === before.children && transmissions === before.transmissions && stream.position === before.position;
    assert.ok(throws(() => bound.contents(parent, { ...stream, bytes: new Uint8Array(0) }, 17, true)) && unchanged());
    assert.ok(throws(() => bound.contents(parent, { ...stream, bytes: Uint8Array.from(stream.bytes) }, 17, true)) && unchanged());
    // 같은 buffer 의 다른 위치/길이(부분 뷰)도 현재 원본 섹션이 아니다.
    assert.ok(throws(() => bound.contents(parent, { ...stream, bytes: stream.bytes.subarray(1) }, 17, true)) && unchanged());
    assert.ok(throws(() => bound.contents(parent, { ...stream, bytes: stream.bytes.subarray(0, 6) }, 17, true)) && unchanged());
    assert.ok(throws(() => bound.contents(parent, stream, 17, false)) && unchanged());
    // 꼬리 BYTE 수가 섹션보다 크면 소비 폭 오류로 거부한다.
    fort.technologyTrailingBytes = 8;
    assert.ok(throws(() => bound.contents(parent, stream, 17, true)) && unchanged());
    fort.technologyTrailingBytes = 1;
});

test('Fort 메타 로더 계약: 부족한 장부·필수 효과·손상된 물리 stream 을 기본 성공값으로 대체하지 않는다', () => {
    const pool = new SidPool(24000, { server: false });
    const scene = new ReplayScene(pool, '0,80,1,0,1,1,1,6,0,0,1,0');
    assert.ok(throws(() => new RawFortMetaLoad(pool, scene.badges, scene.depth, scene.state, { ...scene.hooks(), prepareTypeNames: undefined })));
    assert.ok(throws(() => new RawFortMetaLoad(pool, scene.badges.subarray(0, 6159), scene.depth, scene.state, scene.hooks())));
    assert.ok(throws(() => new RawFortMetaLoad({ edition: 'cd1072' }, scene.badges, scene.depth, scene.state, scene.hooks())));
    const loader = new RawFortMetaLoad(pool, scene.badges, scene.depth, scene.state, scene.hooks());
    scene.stream.bytes = new Uint8Array(0);
    assert.ok(throws(() => loader.loadBadges(scene.stream)));
});

test('Fort 메타 로더 계약: 두 wrapper 는 진단과 기술 부모가 현재 섹션을 바꾼 뒤에만 실제 선택을 실행한다', () => {
    const pool = new SidPool(24000, { server: false });
    const badges = badgeInput(0);
    const depth = { value: 3 };
    const state = { templatePresent: false, templateSlot: 8, localOwner: 1, hasTypeConversion: false };
    const bytes = Uint8Array.from([0]);
    const first = { bytes, position: 11, end: 1, present: true };
    const second = { bytes, position: 19, end: 1, present: true };
    let current = first;
    const effects = [];
    const hooks = {
        assertion: (reason) => { effects.push(`A${reason}`); if (reason === FortMetaAssertion.MissingBadgesTemplate) current = second; },
        loadingDepthChanged: (value) => { assert.equal(value, depth.value); effects.push(`L${value}`); },
        technologyParent: () => { assert.equal(depth.value, 4); effects.push('P'); current = second; return 50; },
        prepareTypeNames: () => { effects.push('N'); state.hasTypeConversion = true; },
        contents: (parent, stream, flags, relaxed) => {
            assert.ok(parent === 50 && stream === second && flags === 17 && relaxed && !stream.position && depth.value === 4);
            effects.push('C');
            stream.position = 1;
        },
        releaseTypeConversion: () => { effects.push('F'); },
        refreshTechnologyUi: () => { effects.push('U'); },
    };
    const meta = new RawFortMetaLoad(pool, badges, depth, state, hooks);
    meta.loadBadgesSelected(() => { assert.ok(depth.value === 3 && current === second); effects.push('S3'); current.position = 0; return current; });
    assert.deepEqual(effects, ['A0', 'S3']);
    assert.ok(first.position === 11 && second.position === 1 && depth.value === 3);
    effects.length = 0;
    current = first;
    meta.loadTechnologySelected(() => { assert.ok(depth.value === 4 && current === second && effects.at(-1) === 'P'); effects.push('S4'); current.position = 0; return current; });
    assert.deepEqual(effects, ['L4', 'A2', 'A3', 'P', 'S4', 'N', 'C', 'F', 'U', 'L3']);
    assert.ok(first.position === 11 && second.position === 1 && !state.hasTypeConversion && depth.value === 3);
});

test('Fort 메타 로더 계약: 기술 선택 실패도 이미 수행한 부모 조회는 유지하면서 깊이를 복구하고 빠진 선택 callback 은 쓰기 전에 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    const badges = badgeInput(0);
    const depth = { value: 3 };
    const state = { templatePresent: true, templateSlot: 1, localOwner: 1, hasTypeConversion: false };
    let parents = 0;
    const fail = (message) => () => { throw new Error(message); };
    const hooks = {
        assertion: fail('정상 Template 의 예상 밖 진단'),
        loadingDepthChanged: (value) => { assert.equal(value, depth.value); },
        technologyParent: () => { assert.equal(depth.value, 4); parents += 1; return 0; },
        prepareTypeNames: fail('선택 실패 뒤 추가 처리 없음'), releaseTypeConversion: fail('선택 실패 뒤 추가 처리 없음'), refreshTechnologyUi: fail('선택 실패 뒤 추가 처리 없음'),
        contents: fail('선택 실패 뒤 내용물 없음'),
    };
    const meta = new RawFortMetaLoad(pool, badges, depth, state, hooks);
    assert.ok(throws(() => meta.loadTechnologySelected(() => { assert.ok(depth.value === 4 && parents === 1); throw new Error('기술 파일 선택 실패 입력'); })));
    assert.ok(depth.value === 3 && parents === 1 && throws(() => meta.loadTechnologySelected(undefined)) && throws(() => meta.loadBadgesSelected(undefined)) && depth.value === 3 && parents === 1);
});

test('Fort 메타 로더 계약: E225 Badges 전역 장부의 같은 배열을 로더와 조회가 함께 쓰고 로더는 count 외 바이트를 바꾸지 않는다', () => {
    const owner = new RawFortBadgesGlobals();
    const records = owner.records();
    const original = Uint8Array.from(records);
    const pool = new SidPool(24000, { server: false });
    const depth = { value: 5 };
    const state = { templatePresent: true, templateSlot: 1, localOwner: 1, hasTypeConversion: false };
    let assertions = 0;
    const failDepth = () => { throw new Error('패치 Badges 가 로딩 깊이를 바꾸면 안 됩니다'); };
    const hooks = {
        assertion: (kind) => { assert.equal(kind, FortMetaAssertion.BadgeCountRange); assertions += 1; },
        loadingDepthChanged: failDepth,
        technologyParent: failDepth, prepareTypeNames: failDepth, contents: failDepth, releaseTypeConversion: failDepth, refreshTechnologyUi: failDepth,
    };
    const meta = new RawFortMetaLoad(pool, records, depth, state, hooks);
    meta.loadBadges({ bytes: Uint8Array.from([2, 71, 7, 0, 9]), position: 0, end: 5, present: true });
    assert.ok(depth.value === 5 && assertions === 0 && owner.records() === records);
    assert.ok(readBadgeCount(records, 71) === 7 && readBadgeCount(records, 0) === 9);
    meta.setBadge(72, 256);
    assert.ok(assertions === 1 && readBadgeCount(records, 72) === 256);
    meta.loadBadges({ bytes: new Uint8Array(0), position: 0, end: 0, present: false });
    meta.loadBadges({ bytes: Uint8Array.from([1, 71, 8]), position: 0, end: 3, present: true });
    assert.ok(readBadgeCount(records, 71) === 8 && readBadgeCount(records, 0) === 9 && readBadgeCount(records, 72) === 256);
    for (let index = 0; index < records.length; index += 1) if (index % 220 < 216) assert.equal(records[index], original[index], `index ${index}`);
    owner.resetCounts();
    assert.ok(readBadgeCount(records, 71) === 0 && readBadgeCount(records, 72) === 0 && readBadgeCount(records, 0) === 0);
});
