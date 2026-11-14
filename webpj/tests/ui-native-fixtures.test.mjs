// UI 부품 이식 모듈의 원본 10.78 독립 관측 fixture 대조: 예산(SP) 표시 재부착(StormPowerRefresh, 121행)·MetaImage 노드(326행)·MetaImage 목록(514행)·편집기 Image(1,570행)·텍스트 clipboard(880행). 입력 열만 읽고 관측 열은 비교에만 쓴다.
// cpppj `StormPowerRefreshTests`·`MetaImageGumpTests`·`MetaImageListTests` 와 같은 방식이다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E477)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { refreshStormPowerGump } from '../www/ui/storm-power-refresh.js';
import { MetaImageGump, MetaImageGumpNode } from '../www/ui/meta-image-gump.js';
import { MetaImageList, MetaImageListNode } from '../www/ui/meta-image-list.js';
import { RawImage } from '../www/ui/raw-image.js';
import { TextClipboard } from '../www/ui/text-clipboard.js';

// 탭 구분 fixture 의 머리글/주석을 건너뛴 행 목록이다(빈 열 보존).
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#') && !line.startsWith('op\t')).map((line) => line.split('\t')); }
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const floatValue = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const readWord = (bytes, at) => (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
const putWord = (bytes, at, value) => { for (let i = 0; i < 4; i += 1) bytes[at + i] = (value >>> (i * 8)) & 0xff; };
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예산(SP) 표시 재부착: 없는 대상/루트/첫째·마지막 슬롯/죽음/중복/두 정밀도와 중단을 모두 재생한다.
class BudgetStop extends Error {}
const BUDGET_NODES = 0x12000100;
test('StormPowerRefresh 원본 10.78 120행: 현재 serial 조회와 Push/Attach 순서·좌표·깊이 복원과 중단 prefix 가 전체 노드/등록표/호출 순서까지 일치한다', () => {
    const table = rows('budget-refresh-x86.tsv'); const failures = []; let stopped = 0;
    assert.equal(table.length, 120);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 12, `행 ${index}`);
        const bytes = Uint8Array.from(Buffer.from(row[5], 'hex')); const registry = Uint8Array.from(Buffer.from(row[6], 'hex')); const entries = new Uint32Array(1000); const nodes = [new GumpCreateNode(), new GumpCreateNode(), new GumpCreateNode()];
        for (let i = 0; i < entries.length; i += 1) entries[i] = readWord(registry, i * 4);
        // Native 에 해당하는 필드만 같은 host 노드에 입력한다.
        for (let i = 0; i < nodes.length; i += 1) { const at = i * 192; const node = nodes[i]; node.flags = readWord(bytes, at + 4); node.serial = readWord(bytes, at + 0x2c); node.depth = readWord(bytes, at + 0x30); node.x = floatValue(readWord(bytes, at + 0x4c)); node.y = floatValue(readWord(bytes, at + 0x50)); }
        let serial = Number(row[4]); const calls = []; let interrupted = false;
        const nodeOf = (token) => { if (token < BUDGET_NODES || (token - BUDGET_NODES) % 192 || (token - BUDGET_NODES) / 192 >= nodes.length) throw new RangeError('최초 예산 노드 토큰 범위'); return nodes[(token - BUDGET_NODES) / 192]; };
        const effect = (attach, token, x = 0, y = 0, parent = 0, mode = 0) => {
            calls.push(`${attach ? 'A:' : 'P:'}${token}${attach ? `:${x}:${y}:${parent}:${mode}` : ''}`); const node = nodeOf(token);
            if (Number(row[1])) { node.depth = attach ? 0xfedcba98 : 0x89abcdef; if (!attach) { node.x = floatValue(0xc2300000); node.y = floatValue(0x42b00000); serial = 0xfedcba98; } }
            if (row[2] === (attach ? 'attach' : 'push')) throw new BudgetStop();
        };
        const hooks = { node: nodeOf, push: (token) => effect(false, token), attach: (token, x, y, parent, mode) => effect(true, token, floatBits(x), floatBits(y), parent, mode) };
        try { refreshStormPowerGump(entries, serial, hooks); } catch (error) { if (!(error instanceof BudgetStop)) throw error; interrupted = true; stopped += 1; }
        // 원문 store 가 접근한 실제 표현만 전체 입력 버퍼에 되돌린다.
        for (let i = 0; i < nodes.length; i += 1) { const at = i * 192; putWord(bytes, at + 0x30, nodes[i].depth); putWord(bytes, at + 0x4c, floatBits(nodes[i].x)); putWord(bytes, at + 0x50, floatBits(nodes[i].y)); }
        const problems = []; if (hex(bytes) !== row[7]) problems.push('nodes'); if (serial !== Number(row[8])) problems.push('serial'); if ((calls.join(';') || '-') !== row[9]) problems.push('calls'); if (interrupted !== (row[10] === '1')) problems.push('stopped');
        for (let i = 0; i < entries.length; i += 1) if (entries[i] !== readWord(registry, i * 4)) { problems.push('registry'); break; }
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []); assert.equal(stopped, 48);
});

test('MetaImageGump 원본 10.78 326행: 기본 생성·크기·부착 축소·가상 출력의 기하 비트와 호출 인자/순서가 두 정밀도 모두 일치한다', () => {
    const table = rows('meta-image-x86.tsv'); const failures = [];
    assert.equal(table.length, 326);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 24, `행 ${index}`);
        const image = new RawImage(); const width = Number(row[1]) | 0; const height = Number(row[2]) | 0;
        if (Number(row[9]) && width > 0 && height > 0) image.convert({ width, height, pixels: new Uint8Array(width * height).fill(7) }, false);
        const node = new MetaImageGumpNode(); node.image = image; node.colorMode = Number(row[6]); node.scale = floatValue(Number(row[3])); node.baseWidth = floatValue(Number(row[4])); node.baseHeight = floatValue(Number(row[5]));
        const state = new GumpCreateState(); const factory = new GumpCreate(state, { node: (token) => (token === 1 ? node : null) }); const trace = [];
        const hooks = {
            node: (token) => (token === 1 ? node : null), position: () => { trace.push('position'); return { x: Number(row[7]) | 0, y: Number(row[8]) | 0 }; },
            attach: (token, x, y, parent, mode) => {
                trace.push(`attach:${floatBits(x)}:${floatBits(y)}:${parent}:${mode}`); node.baseWidth = floatValue(Number(row[4])); node.baseHeight = floatValue(Number(row[5]));
                if (Number(row[10])) image.convert({ width: 37, height: 19, pixels: new Uint8Array(37 * 19).fill(7) }, false);
            },
            color: (token) => { trace.push(`color:${token}`); },
            draw: (request) => { assert.equal(request.image, image); trace.push(`draw:${request.scaled ? 'scaled' : request.mapped ? 'mapped' : 'plain'}:${request.position.x >>> 0}:${request.position.y >>> 0}:${request.scale >>> 0}`); },
        };
        const body = new MetaImageGump(factory, hooks);
        if (row[0] === 'Construct') body.construct(1, image); else if (row[0] === 'Resize') body.resize(1); else if (row[0] === 'Attach') body.attach(1, 1.25, -2.5, 0xaabbccdd, 0x80000000); else body.draw(1);
        const problems = [];
        if (floatBits(node.scale) !== Number(row[12]) || floatBits(node.baseWidth) !== Number(row[13]) || floatBits(node.baseHeight) !== Number(row[14]) || floatBits(node.width) !== Number(row[15]) || floatBits(node.height) !== Number(row[16])) problems.push('geometry');
        if ((trace.join(';') || '-') !== row[22]) problems.push('trace'); if (node.command !== Number(row[17]) || node.vtable !== Number(row[18]) || node.flags !== Number(row[19])) problems.push('fields'); if (state.count !== Number(row[20]) || state.serial !== Number(row[21])) problems.push('state');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('MetaImageList 원본 10.78 514행: 목록 전체 tail/자식 기하/호출 순서와 실제 MetaImage 자식이 생성·부착·히트·입력·그리기·ID·이미지·삭제·추가·소멸에서 일치한다', () => {
    const table = rows('meta-image-list-x86.tsv'); const failures = [];
    assert.equal(table.length, 514);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 26, `행 ${index}`);
        const node = new MetaImageListNode(); node.count = 0x5a5a5a5a; node.ids.fill(0x5a5a5a5a); node.cellHeight = node.cellWidth = floatValue(0x5a5a5a5a);
        const children = Array.from({ length: 22 }, () => new MetaImageGumpNode()); const images = Array.from({ length: 22 }, () => new RawImage()); const trace = []; const state = new GumpCreateState();
        const base = (token) => { if (token === 1) return node; return token >= 100 && token < 122 ? children[token - 100] : null; };
        const factory = new GumpCreate(state, { node: base }); const life = new GumpLifetime(state, new GumpLifetimeState(), { node: base });
        const prepare = (i) => { const w = [224, 6, 112][i % 3]; const h = [112, 4, 224][i % 3]; if (images[i].width() === 0) images[i].convert({ width: w, height: h, pixels: new Uint8Array(w * h).fill(7) }, false); };
        const attachBase = (token, x, y, parent) => {
            trace.push(`attach:${token === 1 ? 'list' : token - 100}:${floatBits(x)}:${floatBits(y)}`); const current = base(token); current.x = x; current.y = y; current.parent = parent;
            if (token === 1 && Number(row[13]) === 1) { node.baseWidth = 131.75; node.baseHeight = 99.5; }
        };
        const meta = new MetaImageGump(factory, { node: (token) => (token >= 100 && token < 122 ? children[token - 100] : null), attach: attachBase }); let body;
        const hooks = {
            node: (token) => (token === 1 ? node : null), image: (token) => (token >= 100 && token < 122 ? children[token - 100] : null),
            createImage: (image) => { trace.push('allocate'); const token = 100 + node.count; return meta.construct(token, image); },
            baseAttach: attachBase, attach: (token, x, y, parent, mode) => { if (token === 1) body.attach(token, x, y, parent, mode); else meta.attach(token, x, y, parent, mode); },
            push: (token) => { trace.push(`push:${token === 1 ? 'list' : token - 100}`); base(token).flags = (base(token).flags | 8) >>> 0; if (token === 100 && Number(row[13]) === 2) { node.count = Math.max(1, node.count - 1); node.cellWidth = 17.25; node.cellHeight = 13.5; } },
            kill: (token) => { trace.push(`kill:${token - 100}`); }, destructBase: (token) => { trace.push('destruct'); life.destructBase(token); },
            bounds: () => { trace.push('bounds'); return { left: 10, top: 20, right: 210, bottom: 140 }; }, redraw: (token, erase) => { trace.push(`redraw:${erase ? 1 : 0}`); },
            notify: (token, event, first, second) => { trace.push(`notify:${event}:${first}:${second}`); }, green: () => 43,
            fill: (rect, color) => { trace.push(`fill:${rect.left >>> 0}:${rect.top >>> 0}:${rect.right >>> 0}:${rect.bottom >>> 0}:${color}`); },
        };
        body = new MetaImageList(factory, hooks); let returned = body.construct(1, Number(row[2]) | 0);
        if (row[0] !== 'Construct') {
            node.count = Number(row[1]); node.selection = Number(row[8]) | 0; node.baseWidth = floatValue(Number(row[3])); node.baseHeight = floatValue(Number(row[4])); node.scale = floatValue(Number(row[5]));
            node.cellWidth = floatValue(Number(row[6])); node.cellHeight = floatValue(Number(row[7])); node.x = 2.25; node.y = 3.5;
            // 이미 존재하는 실제 자식/ID 를 명시 입력으로 준비한다.
            for (let i = 0; i < node.count; i += 1) { prepare(i); meta.construct(100 + i, images[i]); node.images[i] = 100 + i; node.ids[i] = 4096 + i * 7; }
            if (row[0] === 'Destruct') node.flags = (node.flags | 0x40000) >>> 0;
            trace.length = 0; returned = 0;
            if (row[0] === 'Attach') body.attach(1, 2.25, 3.5, 0, 0); else if (row[0] === 'Hit') returned = body.hit(1, Number(row[9]) | 0, Number(row[10]) | 0) >>> 0;
            else if (row[0] === 'Handle') returned = body.handle(1, Number(row[11]), Number(row[9]) | 0 >>> 0, Number(row[10]) | 0 >>> 0) >>> 0; else if (row[0] === 'Draw') body.draw(1);
            else if (row[0] === 'Id') returned = body.id(1, Number(row[12]) | 0) >>> 0;
            else if (row[0] === 'Image') { const image = body.image(1, Number(row[12]) | 0); returned = image ? images.indexOf(image) + 1 : 0; }
            else if (row[0] === 'Remove') returned = body.remove(1, Number(row[12]) | 0) >>> 0; else if (row[0] === 'Add') { prepare(node.count); returned = body.add(1, images[node.count], 0x81234567) >>> 0; } else body.destruct(1);
        }
        const tail = new Uint8Array(0xbc); const geometry = new Uint8Array(22 * 28);
        putWord(tail, 0, node.count); putWord(tail, 0xac, node.selection >>> 0); putWord(tail, 0xb0, node.columns >>> 0); putWord(tail, 0xb4, floatBits(node.cellHeight)); putWord(tail, 0xb8, floatBits(node.cellWidth));
        // 실제 포인터만 Native 와 같은 논리 자식 순번으로 정규화한다.
        for (let i = 0; i < 21; i += 1) { putWord(tail, 4 + i * 4, node.images[i] ? node.images[i] - 99 : 0); putWord(tail, 0x58 + i * 4, node.ids[i]); }
        for (let i = 0; i < 22; i += 1) { const child = children[i]; [child.scale, child.baseWidth, child.baseHeight, child.width, child.height, child.x, child.y].forEach((value, field) => putWord(geometry, i * 28 + field * 4, floatBits(value))); }
        const problems = []; if (returned !== Number(row[15])) problems.push('returned'); if (hex(tail) !== row[22]) problems.push('tail'); if (hex(geometry) !== row[23]) problems.push('geometry'); if ((trace.join(';') || '-') !== row[24]) problems.push('trace');
        if (floatBits(node.baseWidth) !== Number(row[16]) || floatBits(node.baseHeight) !== Number(row[17]) || node.flags !== Number(row[18]) || node.vtable !== Number(row[19])) problems.push('node');
        if (state.count !== Number(row[20]) || state.serial !== Number(row[21])) problems.push('state');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 편집기 Image(RawImage): x87 두 모드·삼색 전 조합·긴 run·실제 지형 Bitmap 의 변환/인코딩/복사/소멸을 원본 shape 바이트에 대조한다(1,570행). 기존 RLE renderer 의 픽셀 대조는 렌더러 단위 시험이 맡는다. (2026-11-15 WB04-E477)
test('RawImage 원본 10.78 1,570행: 변환·crop·shape 인코딩·복사본·소멸의 전체 shape 바이트와 크기가 두 정밀도 모두 일치한다', () => {
    const lines = fs.readFileSync(new URL('../../cpppj/tests/fixtures/editor-image-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t')); const failures = [];
    assert.equal(lines.length, 1570);
    for (const [index, row] of lines.entries()) {
        assert.equal(row.length, 14, `행 ${index}`);
        const pixels = Uint8Array.from(Buffer.from(row[4], 'hex')); const width = Number(row[1]) | 0; const height = Number(row[2]) | 0; const image = new RawImage(); image.convert({ width, height, pixels }, Number(row[3]) !== 0);
        const problems = [];
        if (image.width() !== (Number(row[6]) | 0) || image.height() !== (Number(row[7]) | 0)) problems.push('size');
        if (hex(image.shape()) !== row[9] || image.shapeSize() !== Number(row[8]) || image.bits().length !== 0) problems.push('shape');
        const copy = new RawImage(); copy.copyFrom(image); if (copy.shape() === image.shape() || hex(copy.shape()) !== row[10]) problems.push('copy'); if (copy.width() !== image.width() || copy.height() !== image.height() || copy.shapeSize() !== image.shapeSize()) problems.push('copysize');
        image.destroy(); image.destroy(); copy.destroy(); copy.destroy();
        if (image.width() !== 0 || image.height() !== 0 || image.shape().length !== 0 || image.bits().length !== 0 || image.shapeSize() !== Number(row[8])) problems.push('destroy');
        if (copy.width() !== 0 || copy.height() !== 0 || copy.shape().length !== 0 || copy.shapeSize() !== Number(row[8])) problems.push('copydestroy');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 텍스트 clipboard 880행(read 240/write 640): CF_TEXT 읽기/쓰기와 GlobalAlloc/Lock 순서·용량·실패 순서를 명시 장치에서 대조한다.
test('TextClipboard 원본 10.78 880행: 읽기/쓰기의 용량·NUL 패딩·실패 장치별 호출 순서와 접근 실패(80행)가 일치한다', () => {
    const lines = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-clipboard-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t')); const failures = []; let faults = 0;
    assert.equal(lines.length, 880);
    for (const [index, row] of lines.entries()) {
        assert.equal(row.length, 8, `행 ${index}`);
        const flags = new Map(); for (const assignment of row[3].split(',')) { const at = assignment.indexOf('='); flags.set(assignment.slice(0, at), Number(assignment.slice(at + 1))); }
        const memory = new Uint8Array(1024).fill(0xa5); const trace = []; const flag = (name) => (flags.has(name) ? flags.get(name) : 1); const HANDLE = 0x12345678;
        const devices = {
            formatAvailable: (format) => { trace.push(`available:${format}`); return flag('available'); }, open: (window) => { trace.push(`open:${window}`); return flag('open'); },
            get: (format) => { trace.push(`get:${format}`); return flag('get') ? HANDLE : 0; }, lock: (handle) => { trace.push(`lock:${handle}`); return flag('lock') ? memory : null; },
            unlock: (handle) => { trace.push(`unlock:${handle}`); return flag('unlock'); }, empty: () => { trace.push('empty'); return flag('empty'); },
            allocate: (options, count) => { trace.push(`allocate:${options},${count}`); return flag('allocate') ? HANDLE : 0; }, set: (format, handle) => { trace.push(`set:${format},${handle}`); return flag('set') ? HANDLE : 0; },
            close: () => { trace.push('close'); return flag('close'); },
        };
        const source = Uint8Array.from(Buffer.from(row[1] === '-' ? '' : row[1], 'hex')); const clipboard = new TextClipboard(devices); const capacity = Number(row[2]); const output = new Uint8Array(capacity + 8).fill(0xa5); let completed = true;
        if (row[0] === 'read') { memory.set(source); memory[source.length] = 0; }
        try { if (row[0] === 'read') clipboard.read(output.subarray(0, capacity)); else clipboard.write(source); } catch (error) { if (!(error instanceof RangeError)) throw error; completed = false; faults += 1; }
        const problems = []; if (completed !== (Number(row[4]) !== 0)) problems.push('completed');
        if (hex(row[0] === 'read' ? output : memory.subarray(0, source.length + 9)) !== row[6]) problems.push('output'); if ((trace.join(';') || '-') !== row[7]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []); assert.equal(faults, 80);
});
