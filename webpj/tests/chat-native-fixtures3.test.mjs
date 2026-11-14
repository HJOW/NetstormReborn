// 채팅 창 계열의 세 번째 원본 관측 연결: trace→Chat 전달(TraceChat, 37행)과 ChatWindow 로그 추가 부모, ChatWindow 기하(HudChatGeometry attach/helper/input, 두 정밀도 3,960행)를 원본 10.78 독립 관측 fixture 에 대조한다.
// 입력 열만 읽고 관측 열은 비교에만 쓴다(cpppj `TraceChatTests`·`HudChatGeometryTests` 와 같은 방식). 원본게임을 실행하지 않는다. (2026-11-15 WB04-E476)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { ChatWindow, ChatWindowState } from '../www/ui/chat-window.js';
import { forwardTraceToChat } from '../www/ui/trace-chat.js';
import { HudChatNode, HudChatGeometry } from '../www/ui/hud-chat-geometry.js';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { ScrollingTextGumpNode } from '../www/ui/text-gump.js';

// 탭 구분 fixture 의 주석/머리글을 건너뛴 행 목록이다. 빈 열도 보존한다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#') && !line.startsWith('name\t')).map((line) => line.split('\t')); }
const hexBytes = (bytes) => Buffer.from(bytes).toString('hex');
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const floatValue = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
// 8 DWORD 문맥을 원문 little endian 32 BYTE 의 hex 로 바꾼다.
function contextHex(context) { const bytes = new Uint8Array(32); for (let i = 0; i < 8; i += 1) for (let b = 0; b < 4; b += 1) bytes[i * 4 + b] = (context[i] >>> (b * 8)) & 0xff; return hexBytes(bytes); }
class ChatAppendStop extends Error {}

test('TraceChat 원본 10.78 37행: 현재 색/포인터 관문과 부모/두 자식의 문맥·alias·변경·부분 중단이 호출 순서까지 일치한다', () => {
    const table = rows('trace-chat-x86.tsv'); const failures = [];
    assert.equal(table.length, 37);
    for (const [index, fields] of table.entries()) {
        assert.equal(fields.length, 14, `행 ${index}`);
        const root = new HudChatNode(); const first = new ScrollingTextGumpNode(); const second = new ScrollingTextGumpNode(); const globals = new ChatWindowState(); const supplied = new Uint32Array(8);
        // 서로 다른 부모/두자식/전달 문맥의 실제 물리이다.
        for (let i = 0; i < 8; i += 1) { root.textContext[i] = 100 + i; first.textContext[i] = 200 + i; second.textContext[i] = 300 + i; supplied[i] = 400 + i; }
        root.text = 10; let status = 'normal'; const events = [];
        const token = (value) => { assert.ok(value === 10 || value === 20); return value === 10 ? '1' : '2'; };
        const event = (kind, child, text = '') => {
            const node = child === 10 ? first : second; let row = `${kind}:${token(child)}:${contextHex(root.textContext)}:${contextHex(node.textContext)}`;
            if (kind === 'parse') { const nul = text.indexOf('\0'); row += `:${Buffer.from(nul < 0 ? text : text.slice(0, nul), 'latin1').toString('hex')}`; }
            events.push(row);
            if (fields[6] === `stop-${kind}`) { status = 'stop'; throw new ChatAppendStop(); }
            if (fields[6] === `${kind}-child`) root.text = 20;
            if (fields[6] === `${kind}-context`) for (let i = 0; i < 8; i += 1) { root.textContext[i] = 700 + i; first.textContext[i] = 800 + i; }
        };
        const hooks = {
            chat: (value) => (value === 1 ? root : null), base: (value) => (value === 1 ? root : value === 10 ? first : value === 20 ? second : null),
            atBottom: (child) => { event('at', child); return fields[5] !== '0'; }, appendLog: (child, text) => { event('parse', child, text); }, requestBottom: (child) => { event('bottom', child); },
        };
        const window = new ChatWindow(globals, hooks); const text = Buffer.from(fields[7], 'hex').toString('latin1');
        try {
            if (fields[0] === 'append') { const context = fields[4] === 'supplied' ? supplied : fields[4] === 'root' ? root.textContext : fields[4] === 'child' ? first.textContext : []; window.appendLog(1, text, context); }
            else {
                const trace = { color: () => { events.push('color'); return Number(fields[2]); }, currentChat: () => { events.push('current'); return fields[3] === '0' ? 0 : 1; }, append: (current, body, context) => { window.appendLog(current, body, context); } };
                forwardTraceToChat(Number(fields[1]), text, trace);
            }
        } catch (error) { if (!(error instanceof ChatAppendStop)) throw error; }
        const problems = [];
        if (status !== fields[8]) problems.push('status'); if (contextHex(root.textContext) !== fields[9]) problems.push('root'); if (contextHex(first.textContext) !== fields[10]) problems.push('first'); if (contextHex(second.textContext) !== fields[11]) problems.push('second');
        if (token(root.text) !== fields[12]) problems.push('child'); if ((events.join(';') || '-') !== fields[13]) problems.push('trace');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 실제 ChatWindow 기하 관측을 같은 노드/명시 가상 경계로 재생한다: root(1)·Console/Text/helper/background(2..5) 다섯 노드의 크기/표시와 공개 가상/텍스트 요청 순서만 비교한다.
class GeometryScene {
    // 기하 모듈과 기본 노드 조회가 공유하는 실제 root 와 같은 네 자식 저장소, 명시 텍스트 조회/현재 모드 입력, 호출 순서이다.
    root = new HudChatNode(); children = [new GumpCreateNode(), new GumpCreateNode(), new GumpCreateNode(), new GumpCreateNode()]; bottom = 0; challenge = 0; trace = [];
    // 앞 14열 입력만 상태에 제공한다. 기대 기하/호출 순서는 뒤 관측 열에서만 읽는다.
    constructor(row) {
        const root = this.root; root.console = 2; root.text = 3; root.helper = 4; root.background = 5; root.leftPadding = Number(row[4]) | 0; root.helperVisible = Number(row[5]) & 0xff;
        const scales = row[7].split(':'); const positions = row[9].split(':'); assert.equal(scales.length, 4); assert.equal(positions.length, 6);
        // 실제 초기값은 oracle 과 같은 명시 표식이며 출력 수식을 쓰지 않는다.
        for (let token = 1; token <= 5; token += 1) {
            const node = this.node(token); node.flags = token === 1 ? 0xe01 : token === 2 ? Number(row[6]) : 0x601; node.scale = token === 1 ? 1.0 : floatValue(Number(scales[token - 2]));
            node.baseWidth = token === 1 ? floatValue(Number(row[2])) : [60, 40, 1, 20][token - 2]; node.baseHeight = token === 1 ? floatValue(Number(row[3])) : token === 2 ? floatValue(Number(row[8])) : [0, 50, 9, 30][token - 2];
            node.width = 123; node.height = -456;
        }
        root.x = floatValue(Number(positions[0])); root.y = floatValue(Number(positions[1])); this.children[0].x = floatValue(Number(positions[2])); this.children[0].y = floatValue(Number(positions[3]));
        this.bottom = Number(row[13]); this.challenge = Number(row[11]);
        const packed = (values) => values.map((value) => value >>> 0).join(':');
        this.geometry = new HudChatGeometry({
            chat: (token) => (token === 1 ? this.root : null), node: (token) => this.node(token),
            baseAttach: (token, x, y, parent, mode) => { this.trace.push(`baseAttach:${packed([token, floatBits(x), floatBits(y), parent, mode])}`); }, push: (token) => { this.trace.push(`push:${token}`); },
            attach: (token, x, y, parent, mode) => { this.trace.push(`attach:${packed([token, floatBits(x), floatBits(y), parent, mode])}`); },
            bottomIndex: (token) => { this.trace.push(`bottomIndex:${token}`); return this.bottom; }, requestBottom: (token, extra) => { this.trace.push(`bottom:${packed([token, extra])}`); },
            challenge: () => { this.trace.push(`challenge:${this.challenge}`); return this.challenge; },
        });
    }
    // 원문 포인터 정규화 1..5 와 같은 검사 토큰의 노드이다.
    node(token) { return token === 1 ? this.root : token >= 2 && token <= 5 ? this.children[token - 2] : null; }
    // 제품이 실제로 기록한 다섯 노드의 크기/표시를 원시 비트로 읽는다(untouched Console 높이와 root 크기 포함).
    state() { const parts = []; for (let token = 1; token <= 5; token += 1) { const n = this.node(token); parts.push([token, n.flags >>> 0, floatBits(n.baseWidth), floatBits(n.baseHeight), floatBits(n.width), floatBits(n.height)].join(':')); } return parts.join(';'); }
}

test('HudChatGeometry 원본 10.78 3,960행: 부착·도우미 표시·입력줄 표시의 다섯 노드 크기/표시와 공개 가상/텍스트 요청 순서가 두 정밀도 모두 일치한다', () => {
    const table = rows('hud-chat-geometry-x86.tsv'); const failures = [];
    assert.equal(table.length, 3960);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 20, `행 ${index}`); const problems = [];
        if (!(row[14] === '1' && row[18] === '0' && Number(row[19]) > 0)) problems.push('range');
        const scene = new GeometryScene(row);
        if (row[1] === 'attach') { const positions = row[9].split(':'); scene.geometry.attach(1, floatValue(Number(positions[4])), floatValue(Number(positions[5])), 17, 3); }
        else if (row[1] === 'helper') scene.geometry.setHelperVisible(1, Number(row[10]));
        else { assert.equal(row[1], 'input'); scene.geometry.setInputVisible(Number(row[12]) ? 1 : 0, Number(row[10])); }
        // 실제 4ba3d0 내부 Redraw 는 제품 기하 모듈의 외부 요청 경계 안쪽이므로 직접 trace 비교에서 제외한다.
        const expected = row[17] === '-' ? '' : row[17].split(';').filter((item) => !item.startsWith('redraw:')).join(';');
        if (scene.state() !== row[15]) problems.push('state'); if (scene.root.helperVisible !== Number(row[16])) problems.push('helper'); if (scene.trace.join(';') !== expected) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
