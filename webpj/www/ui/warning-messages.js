// WarningMessages.cpp/.h의 같은 Gump/StyleText 저장소·20칸 표·가상 Draw/Idle/소멸을 조립한다(기준선 1f00a25a, 10.78 전용).
// common={create,life,collection,tree,lifetime,token(),node(token),changed()}, menus={textBackend(),styleBackend(),setPalette(),setExternalGumps()}의 실제 장치를 빌린다.
// 부모가 공통 Gump를 먼저 정리한 뒤 destroy()로 빌린 Draw 공급자를 끊는다. 별도 표시/측정 월드를 만들지 않는다. (2026-11-14 WB04-E454)
import { MessageGump, MessageGumpNode } from './message-gump.js';
import { MessageBoard, MessageBoardState, warningExpired } from './message-board.js';
import { GUMP_REGISTRY_CAPACITY } from './gump-create.js';
import { styleTextField } from './style-text-fragments.js';

// 같은 공통 Draw 공급자 2번과 원문 dead 비트다. 생산/미니맵 슬롯과 겹치지 않는다.
const DRAW_SLOT = 2; const DEAD = 0x10000;
// cpppj Warning 공장이 실제로 연결한 종류다. MessageGump가 지원해도 이 공장이 아직 연결하지 않은 6/8/10은 명시 오류다.
const SUPPORTED = new Set([1, 2, 3, 4, 5, 7, 9, 11, 16, 17, 18]);

// std::ostream defaultfloat/setprecision(17)처럼 IEEE 비트의 정확한 유리수를 17자리 짝수 동률 반올림으로 출력한다.
// JS toPrecision은 동률에서 다른 쪽을 고르므로 사용하지 않는다. 이 함수는 게임 계산/시계/난수를 읽거나 바꾸지 않는다.
function number17(value) {
    const raw = new DataView(new ArrayBuffer(8)); raw.setFloat64(0, value, true); const bits = raw.getBigUint64(0, true);
    const negative = (bits >> 63n) !== 0n; const fraction = bits & 0xfffffffffffffn; const exponent = Number((bits >> 52n) & 0x7ffn); const sign = negative ? '-' : '';
    if (exponent === 0x7ff) {
        if (!fraction) return `${sign}inf`;
        if (!(fraction & 0x8000000000000n)) return `${sign}nan(snan)`;
        return negative ? '-nan(ind)' : 'nan';
    }
    if (!exponent && !fraction) return `${sign}0`;
    let numerator = exponent ? fraction | 0x10000000000000n : fraction; let denominator = 1n; const binary = exponent ? exponent - 1075 : -1074;
    if (binary >= 0) numerator <<= BigInt(binary); else denominator <<= BigInt(-binary);
    const compare = (decimal) => decimal >= 0 ? numerator - denominator * 10n ** BigInt(decimal) : numerator * 10n ** BigInt(-decimal) - denominator;
    let decimal = Math.floor(Math.log10(Math.abs(value)));
    // log10 추정이 정확한 10의 거듭제곱 경계에서 한 칸 어긋나도 실제 정수 비교로 고친다.
    while (compare(decimal) < 0n) decimal -= 1;
    while (compare(decimal + 1) >= 0n) decimal += 1;
    const shift = 16 - decimal; let top = numerator; let bottom = denominator;
    if (shift >= 0) top *= 10n ** BigInt(shift); else bottom *= 10n ** BigInt(-shift);
    let rounded = top / bottom; const remainder = top % bottom;
    if (remainder * 2n > bottom || (remainder * 2n === bottom && (rounded & 1n))) rounded += 1n;
    if (rounded === 100000000000000000n) { rounded /= 10n; decimal += 1; }
    const digits = rounded.toString();
    if (decimal < -4 || decimal >= 17) { const tail = digits.slice(1).replace(/0+$/, ''); return `${sign}${digits[0]}${tail ? `.${tail}` : ''}e${decimal < 0 ? '-' : '+'}${String(Math.abs(decimal)).padStart(2, '0')}`; }
    if (decimal < 0) return `${sign}0.${'0'.repeat(-decimal - 1)}${digits}`.replace(/0+$/, '');
    const whole = digits.slice(0, decimal + 1); const tail = digits.slice(decimal + 1).replace(/0+$/, ''); return `${sign}${whole}${tail ? `.${tail}` : ''}`;
}

// 실제 MessageGump 노드에 보고용 원문과 부분 생성 표식만 덧붙인다. 출력/측정은 실제 head 체인이다.
class WarningNode extends MessageGumpNode {
    // 최종 Style 입력이며 별도 표시 캐시로 사용하지 않는다.
    source = '';
    // Text 생성 완료 뒤의 예외만 공통 scalar 소멸자에 보낸다.
    textConstructed = false;
}

// 등록표/루트/수명/글자 장치는 빌리고 20칸 serial과 파생 노드만 소유한다.
export class WarningMessages {
    // C++ RawMenuTree 값 복사의 참조들과 실제 menus/벽시각을 보관한다.
    #common; #menus; #now;
    // 같은 안정된 파생 저장소와 원문 표/생성자다.
    #messages = new Map(); #state = new MessageBoardState(); #board; #message;
    // 현재 기본/파생 노드가 같은 저장소를 가리키는 공통 장치만 연결한다.
    constructor(common, menus, now, sound, pauseContext = {}, suppressed = null) {
        if (typeof now !== 'function' || typeof sound !== 'function') throw new TypeError('Warning 실제 벽 시각/음향 연결 누락');
        this.#common = { ...common }; this.#menus = menus; this.#now = now; const c = this.#common;
        this.#message = new MessageGump(c.create, { ...pauseContext, node: (token) => this.node(token),
            constructText: (token) => { menus.textBackend().construct(token, 0, 0, 0, 0); this.#at(token).textConstructed = true; },
            style: (token, value) => { const node = this.#at(token); node.source = value; menus.styleBackend().parse(styleTextField(node, 'head'), value, node.textContext); },
            warningSound: sound, wallSeconds: now, idle: (token, enabled) => c.lifetime.setIdle(token, enabled), fit: (token, x, y) => menus.textBackend().fit(token, x, y) });
        this.#board = new MessageBoard(this.#state, { suppressed, lookup: (serial) => this.lookup(serial), kill: (token) => { c.lifetime.kill(token); c.changed(); },
            createText: (kind, player, parameter, source) => {
                if (!SUPPORTED.has(kind)) throw new Error('다른 원본 메시지 생성자 미연결');
                const token = c.token(); const node = new WarningNode(); this.#messages.set(token, node);
                try { this.#message.construct(token, kind, player, parameter, source); }
                catch (error) { if (node.textConstructed) c.lifetime.destroy(token); else this.#messages.delete(token); throw error; }
                return { token, serial: node.serial, kind, player, parameter };
            },
            width: (token) => { const width = c.node(token).baseWidth; if (!Number.isFinite(width) || width < 0 || width > 0x7fffffff) throw new RangeError('Warning 원본 ftol 폭 범위'); return Math.trunc(width); },
            attach: (token, x, y) => c.tree.attach(token, Math.fround(x), Math.fround(y), c.create.entries[0], 0) });
        menus.setExternalGumps(DRAW_SLOT, { text: (token) => this.node(token), owns: (token) => this.node(token) !== null, draw: (token) => menus.textBackend().draw(token) });
    }
    // at()의 원문 물리 부재를 명시 RangeError로 구별한다.
    #at(token) { const node = this.#messages.get(token); if (!node) throw new RangeError('Warning 현재 파생 노드 없음'); return node; }
    // 공통 기본/파생 분배가 같은 안정된 주소를 빌린다. 미등록이면 null이다.
    node(token) { return this.#messages.get(token) ?? null; }
    // 부모가 공통 Gump를 먼저 정리한 뒤 Draw 대여와 파생 저장소만 반납한다.
    destroy() { this.#menus.setExternalGumps(DRAW_SLOT, {}); this.#messages.clear(); }
    // 실제 팔레트/현재 화면과 같은 root의 크기만 갱신한다. 이미 저장된 글자 문맥은 바꾸지 않는다.
    setView(width, height, palette) {
        width |= 0; height |= 0; if (width <= 0 || height <= 0) throw new TypeError('Warning 화면 크기 오류');
        this.#menus.setPalette(palette); this.#state.screenWidth = width; this.#common.create.screenWidth = width; this.#common.create.screenHeight = height;
        const root = this.#common.create.entries[0]; if (root) { const node = this.#common.node(root); node.baseWidth = node.width = Math.fround(width); node.baseHeight = node.height = Math.fround(height); }
    }
    // 원문 Warning(kind17/player0/enabled1/parameter0)을 같은 표에 요청한다.
    add(text) { this.#board.show(17, 0, true, 0, text); }
    // 원문 NULL을 빈 문자열과 구별해 전달한다.
    addNull() { this.#board.showText(17, 0, true, 0, null); }
    // 같은 표의 영구 Pause를 만들거나 지연 삭제한다.
    pauseStatus(enabled) { this.#board.show(11, 0, enabled, 0, ''); }
    // 같은 표의 시간(player1)을 요청하며 중복 생성하지 않는다.
    gameTimeStatus() { this.#board.show(16, 1, true, 0, ''); }
    // 후속 공통 전투 안내도 같은 생성기와 표를 사용한다. 공장 미연결 종류는 명시 오류다.
    battleStatus(kind, player, parameter) { this.#board.show(kind | 0, player | 0, true, parameter | 0, ''); }
    // 현재 직계 자식을 검색해 같은 공통 Kill을 요청한다.
    remove(kind, player, parameter) { this.#board.show(kind | 0, player | 0, false, parameter | 0, ''); }
    // 기한 0이면 벽 시각을 읽지 않으며 캡처한 기한으로 현재 벽 시각을 판정한다.
    idle(token) { const deadline = this.#at(token).deadline; if (deadline !== 0 && warningExpired(deadline, this.#now())) { this.#common.lifetime.kill(token); this.#common.changed(); } }
    // 같은 TextGump의 실제 head/기본 scalar 소멸 순서다.
    deleteScalar(token, flags) { this.#menus.textBackend().deleteScalar(token, flags); }
    // 공통 수명 관리자의 실제 free 지점에서만 파생 저장소를 지운다.
    release(token) { this.#messages.delete(token); }
    // 현재 root의 비dead 직계 자식만 방문한다. 메뉴/다른 파생을 메시지로 잘못 읽지 않는다.
    lookup(serial) {
        const c = this.#common; const root = c.create.entries[0]; if (!root) return null; let visited = 0;
        // 원문처럼 1001번째 방문에서 실패하며 형제 next는 현재 노드에서 읽는다.
        for (let token = c.node(root).child; token; token = c.node(token).next) {
            visited += 1; if (visited > GUMP_REGISTRY_CAPACITY) throw new Error('Warning 직계 자식 순환');
            const base = c.node(token); if (base.serial === (serial >>> 0) && !(base.flags & DEAD)) { const node = this.node(token); if (node) return { token, serial: serial >>> 0, kind: node.kind, player: node.player, parameter: node.parameter }; }
        }
        return null;
    }
    // 별도 표시 상태 없이 같은 표/현재 root에서 생존 안내를 확인한다.
    hasMessages() {
        // 원문 20칸 순서를 유지한다.
        for (const serial of this.#state.serials) if (this.lookup(serial)) return true;
        return false;
    }
    // 같은 현재 표/노드/위치/등록표/삭제 큐를 원문 TSV로 보고한다. 게임 상태를 갱신하지 않는다.
    report() {
        const lines = []; let count = 0;
        // 실제 표의 현재 직계 생존 메시지만 출력하며 source의 탭/개행/NUL을 임의로 이스케이프하지 않는다.
        for (let index = 0; index < this.#state.serials.length; index += 1) {
            const record = this.lookup(this.#state.serials[index]); if (!record) continue;
            const node = this.#at(record.token); const position = this.#common.collection.position(record.token); count += 1;
            lines.push(`warning\t${index}\t${node.serial}\t${position.x | 0}\t${position.y | 0}\t${number17(node.width)}\t${number17(node.height)}\t${number17(node.deadline)}\t${node.source}\n`);
        }
        lines.push(`warning_count\t${count}\nwarning_registry\t${this.#common.create.entries[0]}\t${this.#common.create.count}\t${this.#common.life.killCount}\n`); return lines.join('');
    }
}
