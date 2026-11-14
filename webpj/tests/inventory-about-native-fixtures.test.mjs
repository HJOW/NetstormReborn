// 생산 내용물 정보 메뉴(InventoryAboutMenu, 원본 43d330)의 원본 10.78 독립 관측 대조: 이전 1,296행(클래스 0 고정)과 새 클래스 2,592행(내용물 클래스 6·회수 제외 122 포함)의 실제 비용·분기·호출 순서를
// cpppj `InventoryAboutMenuTests` 와 같은 입력 구성으로 비교한다. 내용물 슬롯(SID 50: +10 클래스·+18 종류 WORD·+20 플래그 WORD)과 기존 4af010(Affordable) 입력만 공급하고 새 About 계산의 기대값은 주입하지 않는다.
// 입력 열만 장치에 공급하고 효과 열은 비교에만 쓴다. 호출 뒤 내용물 풀 슬롯과 항목 노드가 변하지 않는지도 확인한다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E490)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { SidPool } from '../www/core/sid-pool.js';
import { InventoryItemGump } from '../www/ui/inventory-item-gump.js';
import { InventoryAboutMenu, InventoryAboutState } from '../www/ui/inventory-about-menu.js';

// 독립 원본의 명시 메뉴/부모 경계 토큰과 고정 입력 SID 이다.
const MENU = 0x12402000; const CONTENT_SID = 50;
// 탭 구분 fixture 의 머리글/주석 줄을 건너뛴 행 목록이다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t')); }
// 십진 입력의 하위 32비트와 float32 비트 보기이다.
const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const bitsOf = (value) => { f32[0] = value; return u32[0]; }; const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
// 바이트 문자열의 소문자 16진이다.
const textHex = (value) => Buffer.from(value, 'latin1').toString('hex');
// 비정렬 WORD 를 쓴다.
const putWord = (raw, offset, value) => { raw[offset] = value & 0xff; raw[offset + 1] = (value >>> 8) & 0xff; };

// 비용/SP 입력과 기존 4af010 만 공급하는 장면이다(C++ AboutScene). 입력 열(클래스 선택 시 앞에 클래스 하나)을 같은 내용물 슬롯과 정수 비용/SP 로 설치한다.
class AboutScene {
    constructor(input, withClass) {
        const shift = withClass ? 1 : 0; this.pool = new SidPool(24000, { server: true }); this.state = new InventoryAboutState(); this.trace = [];
        const raw = this.pool.physical(CONTENT_SID); raw.fill(0); raw[10] = withClass ? number(input[0]) & 0xff : 0; putWord(raw, 18, number(input[shift])); putWord(raw, 20, number(input[shift + 1]));
        this.editor = number(input[shift + 2]) !== 0; this.cost = number(input[shift + 3]) | 0; this.money = floatOf(number(input[shift + 4])); this.helpKind = number(input[shift + 5]);
        this.item = { content: CONTENT_SID, object: 314159 };
        // 기존 132행에서 대조된 실제 비교 몸체이며 비용/SP 장치만 이 장면에서 공급한다.
        this.inventory = new InventoryItemGump(this.pool, { localOwner: 1, editor: this.editor, stock: 0, disabled: 0, last: 0, bridge: 82, x: 16, y: 11, background: 0, backgroundFrame: 0 },
            { costUnits: () => this.cost, money: (who) => { this.add(`Money,${who >>> 0}`); return this.money; } });
    }
    add(text) { this.trace.push(text); }
    // 명시 메뉴/서식 경계만 기록하고 판단은 제품 코드에 맡긴다.
    hooks() {
        return {
            begin: () => { this.add('Begin'); return MENU; }, root: () => MENU, templateOwner: () => 2, editor: () => this.editor, cost: () => this.cost, affordable: (sid) => this.inventory.affordable(sid) !== 0,
            helpWord: () => (!this.helpKind ? null : this.helpKind === 2 ? 'Help' : ''),
            idle: (menu, on) => { assert.equal(menu, MENU); this.add(`Idle,${on ? 1 : 0}`); }, reference: (menu, object) => { assert.equal(menu, MENU); this.add(`Object,${object >>> 0}`); },
            describe: (kind, count, who, sid) => this.add(`Describe,${kind >>> 0},${count >>> 0},${who >>> 0},${sid >>> 0}`),
            translate: (text) => { this.add(`Translate,${textHex(text)}`); return text; },
            print: (pattern, value) => { const at = pattern.indexOf('%d'); assert.ok(at >= 0 && pattern.indexOf('%d', at + 2) < 0); return pattern.replace('%d', String(value | 0)); },
            header: (text) => this.add(`Header,${textHex(text)}`),
            entry: (label, command, parameter, include, enabled, force) => this.add(`Entry,${label === null || label === undefined ? '-' : textHex(label)},${command >>> 0},${parameter >>> 0},${include ? 1 : 0},${enabled ? 1 : 0},${force ? 1 : 0}`),
            attach: (point, parent) => this.add(`Attach,${point.x >>> 0},${point.y >>> 0},${parent >>> 0}`), sound: (name) => this.add(`Sound,${textHex(name)},0,0,0,0,0`),
        };
    }
}

// 한 fixture 의 모든 행을 재생하고 불일치 행 이름을 모은다.
function replay(name, withClass, expected) {
    const table = rows(name); assert.equal(table.length, expected); const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 2, `행 ${index}`);
        const scene = new AboutScene(row[0].split(','), withClass); const menu = new InventoryAboutMenu(scene.pool, scene.state, scene.hooks());
        const before = Uint8Array.from(scene.pool.physical(CONTENT_SID)); const item = { ...scene.item }; const problems = [];
        if (menu.open(scene.item, { x: 17, y: -9 }) !== 1) problems.push('result');
        if (scene.trace.join(';') !== row[1]) problems.push('trace');
        if (Buffer.compare(Buffer.from(before), Buffer.from(scene.pool.physical(CONTENT_SID))) !== 0) problems.push('pool');
        assert.deepEqual(scene.item, item);
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
}

test('InventoryAboutMenu 원본 10.78 이전 1,296행(클래스 0): 설명/비용/부족 머리/회수/Help/부착/음향의 호출 순서와 풀·노드 불변이 일치한다', () => { replay('inventory-about-x86.tsv', false, 1296); });
test('InventoryAboutMenu 원본 10.78 새 클래스 2,592행(내용물 클래스 6·회수 제외 122): 같은 항목의 호출 순서와 풀·노드 불변이 일치한다', () => { replay('inventory-about-class-x86.tsv', true, 2592); });
void bitsOf;
