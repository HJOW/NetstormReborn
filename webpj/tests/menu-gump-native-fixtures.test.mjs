// 메뉴 Gump(MenuGump, 원본 크기/프레임/확장 idle/controlled 메뉴 스택)의 원본 10.78 독립 관측 154행(originals; CD 10.72 154행은 웹 범위 밖이라 제외) 대조: 입력 노드 55DWORD×3(메뉴·장식·제목)·controlled 표/idle 앞 4칸/루트·
// 콜백 응답(콜백 직전 상태를 남긴 뒤 입력이 지정한 필드만 쓰는 재조회 입력)을 같은 저장소에 공급하고, 콜백 당시 노드/표 상태를 포함한 호출 기록·최종 노드 55DWORD×3·표 상태·action 반환값·뒤쪽 idle 보호를
// cpppj `MenuGumpTests`(OriginalMachineObservations)와 같은 입력 구성으로 비교한다. 입력 열만 공급하고 관측 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E492)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreateState, gumpNodeFloatBits } from '../www/ui/gump-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { MenuGump, MenuGumpNode, MenuGumpState } from '../www/ui/menu-gump.js';

// 격리 x86 의 세 안정 토큰(메뉴·장식·제목)과 원본 offset 의 55DWORD 필드 목록이다.
const TOKENS = [0x18000010, 0x18001010, 0x18002010];
const FIELD_OFFSETS = [0, 4, 56, 60, 64, 68, 72, 184, 368, 372, 376, ...Array.from({ length: 40 }, (_, index) => 380 + index * 4), 544, 548, 2952, 2956];
// float32/DWORD 변환의 공용 보기이다.
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const bitsOf = (value) => { f32[0] = value; return u32[0]; }; const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
// 16진 문자열을 바이트/DWORD 열로 바꾼다.
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 한 행의 메뉴 장면이다: 세 노드·메뉴 표·idle 표·등록 루트·콜백 응답·호출 기록.
class MenuScene {
    constructor(row) {
        this.create = new GumpCreateState(); this.state = new MenuGumpState(); this.lifetimeState = new GumpLifetimeState(); this.nodes = new Map(TOKENS.map((token) => [token, new MenuGumpNode()]));
        this.responses = []; this.calls = '';
        this.readNodes(row[3]); this.readState(row[4]);
        if (row[5] !== '-') for (const text of row[5].split(';')) { const parts = text.split(','); this.responses.push({ kind: parts[0], token: number(parts[1]), target: number(parts[2]), offset: number(parts[3]), value: number(parts[4]) }); }
        this.build();
    }
    find(token) { return this.nodes.get(token) ?? null; }
    // 입력의 압축 DWORD 바이트를 55DWORD 로 풀어 각 노드에 쓴다(원문 offset 의 의미 필드만).
    readNodes(text) {
        const input = text.split(';');
        for (const [index, token] of TOKENS.entries()) {
            const packed = unhex(input[index]); assert.equal(packed.length, FIELD_OFFSETS.length * 4); const view = new DataView(packed.buffer);
            const words = FIELD_OFFSETS.map((_, i) => view.getUint32(i * 4, true)); this.writeWords(this.nodes.get(token), words);
        }
    }
    // 노드에 55DWORD 를 위치별로 쓴다(float 필드는 입력 비트를 유지).
    writeWords(node, w) {
        const setFloat = (field, bits) => { node[field] = floatOf(bits); node.floatBits[field] = bits >>> 0; };
        node.vtable = w[0]; node.flags = w[1]; setFloat('scale', w[2]); setFloat('baseWidth', w[3]); setFloat('baseHeight', w[4]); setFloat('width', w[5]); setFloat('height', w[6]);
        node.expanding = w[7]; node.itemWidth = floatOf(w[8]); node.targetWidth = floatOf(w[9]); node.targetHeight = floatOf(w[10]);
        const offsets = new Uint32Array(node.offsets.buffer, node.offsets.byteOffset, node.offsets.length); for (let i = 0; i < 40; i += 1) offsets[i] = w[11 + i];
        node.menuFlags = w[51]; node.itemCount = w[52]; node.fancy = w[53]; node.title = w[54];
    }
    // 현재 노드의 55DWORD 를 원문 offset 순서로 읽는다.
    wordsOf(node) {
        const offsets = new Uint32Array(node.offsets.buffer, node.offsets.byteOffset, node.offsets.length);
        return [node.vtable, node.flags, gumpNodeFloatBits(node, 'scale'), gumpNodeFloatBits(node, 'baseWidth'), gumpNodeFloatBits(node, 'baseHeight'), gumpNodeFloatBits(node, 'width'), gumpNodeFloatBits(node, 'height'),
            node.expanding, bitsOf(node.itemWidth), bitsOf(node.targetWidth), bitsOf(node.targetHeight), ...offsets, node.menuFlags, node.itemCount, node.fancy, node.title].map((value) => value >>> 0);
    }
    // 실제 현재 세 노드의 선택 필드를 원본 관찰과 같은 순서로 직렬화한다.
    nodesText() {
        return TOKENS.map((token) => { const bytes = Buffer.alloc(FIELD_OFFSETS.length * 4); this.wordsOf(this.nodes.get(token)).forEach((word, i) => bytes.writeUInt32LE(word, i * 4)); return bytes.toString('hex'); }).join(';');
    }
    // 전체 메뉴 표/앞 네 idle 칸/루트를 입력에서 읽는다.
    readState(text) {
        const parts = text.split('#'); this.state.index = number(parts[0]);
        const menus = parts[1].split(',').map(number); const idle = parts[2].split(',').map(number);
        assert.equal(menus.length, this.state.entries.length); assert.equal(idle.length, 4);
        menus.forEach((value, i) => { this.state.entries[i] = value; }); idle.forEach((value, i) => { this.lifetimeState.idle[i] = value; });
        this.create.entries[0] = number(parts[3]); this.create.collection.root = this.create.entries[0];
    }
    // 입력과 출력에 같은 현재 상태 표현을 사용한다.
    stateText() { return `${this.state.index >>> 0}#${Array.from(this.state.entries).join(',')}#${Array.from(this.lifetimeState.idle.subarray(0, 4)).join(',')}#${this.create.entries[0] >>> 0}`; }
    // 콜백 직전 상태를 남긴 뒤 명시 응답만 적용한다(원본 응답 입력은 필드 쓰기뿐).
    respond(kind, token, args = []) {
        this.calls += `${this.calls ? '|' : ''}${kind},${token}${args.map((value) => `,${value >>> 0}`).join('')}@${this.nodesText()}@${this.stateText()}`;
        for (const response of this.responses) {
            if (response.kind !== kind || response.token !== token) continue;
            if (!response.target) { this.state.index = response.value; continue; }
            const node = this.nodes.get(response.target); const setFloat = (field, bits) => { node[field] = floatOf(bits); node.floatBits[field] = bits >>> 0; };
            switch (response.offset) {
                case 60: setFloat('baseWidth', response.value); break; case 64: setFloat('baseHeight', response.value); break;
                case 372: node.targetWidth = floatOf(response.value); break; case 376: node.targetHeight = floatOf(response.value); break;
                case 2952: node.fancy = response.value; break; case 2956: node.title = response.value; break;
                default: throw new Error(`메뉴 응답 offset ${response.offset}`);
            }
        }
    }
    build() {
        this.lifetime = new GumpLifetime(this.create, this.lifetimeState, { node: (token) => this.find(token) });
        const r = (kind) => (token, ...rest) => this.respond(kind, token, rest);
        const hooks = {
            node: (token) => this.find(token), baseNode: (token) => this.find(token), push: (token) => this.respond('push', token), pop: (token) => this.respond('pop', token),
            redraw: (token, erase) => this.respond('redraw', token, [erase ? 1 : 0]), baseRedraw: (token, erase) => this.respond('base_redraw', token, [erase ? 1 : 0]),
            attach: (token, x, y, parent, mode) => this.respond('attach', token, [bitsOf(x), bitsOf(y), parent, mode]),
            attachObject: (token, object, x, y, parent, mode) => this.respond('object', token, [object, bitsOf(x), bitsOf(y), parent, mode]),
        };
        void r; this.menu = new MenuGump(this.create, this.state, this.lifetime, hooks);
    }
    // 저장한 action 을 구현에 전달하고 실제 반환값만 기록한다.
    execute(text) {
        const result = [];
        for (const action of text.split(';')) {
            const parts = action.split(','); const op = parts[0]; let value = '-'; const arg = (i) => number(parts[i]);
            if (op === 'Width') this.menu.setWidth(TOKENS[0], floatOf(arg(1)));
            else if (op === 'Height') this.menu.setHeight(TOKENS[0], floatOf(arg(1)));
            else if (op === 'Redraw') this.menu.redraw(TOKENS[0], arg(1) !== 0);
            else if (op === 'Expand') this.menu.startExpand(TOKENS[0], arg(1));
            else if (op === 'Idle') this.menu.idle(TOKENS[0]);
            else if (op === 'Item') { const point = this.menu.itemPosition(TOKENS[0], arg(1) | 0); value = `${bitsOf(point[0])},${bitsOf(point[1])}`; }
            else if (op === 'Take') value = String(this.menu.takeCurrent() >>> 0);
            else if (op === 'Current') value = String(this.menu.current() >>> 0);
            else if (op === 'Count') value = String(this.menu.itemCount() >>> 0);
            else if (op === 'Attach') this.menu.attachCurrent(arg(1) | 0, arg(2) | 0, arg(3));
            else if (op === 'Object') this.menu.attachObjectCurrent(arg(1), arg(2) | 0, arg(3) | 0);
            else throw new Error(`메뉴 fixture action 오류 ${op}`);
            result.push(value);
        }
        return result.join(';');
    }
}

test('MenuGump 원본 10.78 154행: 입력 노드·콜백 재조회·크기/프레임/확장 idle/controlled 스택의 콜백 당시 상태·최종 노드·표·반환·뒤쪽 idle 보호가 일치한다', () => {
    const all = fs.readFileSync(new URL('../../cpppj/tests/fixtures/menugump-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('edition\t')).map((line) => line.split('\t'));
    const table = all.filter((row) => row[0] === 'originals'); const failures = [];
    assert.equal(table.length, 154); assert.equal(all.length, 308);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 10, `행 ${index}`);
        const scene = new MenuScene(row); const result = scene.execute(row[2]); const problems = [];
        if (scene.nodesText() !== row[6]) problems.push('nodes');
        if (scene.stateText() !== row[7]) problems.push('state');
        if ((scene.calls || '-') !== row[8]) problems.push('calls');
        if (result !== row[9]) problems.push('result');
        if (!Array.from(scene.lifetimeState.idle.subarray(4)).every((token) => token === 0)) problems.push('idle-tail');
        if (problems.length) failures.push(`${index}/${row[1]}/${row[2].slice(0, 30)}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
