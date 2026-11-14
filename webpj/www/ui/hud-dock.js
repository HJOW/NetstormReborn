// 원본 HUD 구분선(divider, 0x11c BYTE 노드)의 생성·슬레이브 등록·재배치와 HUD 본체의 표시 모드·채팅·지도 영역 갱신을 같은 Gump 저장소에 옮긴다(cpppj `client/HudDock.cpp/.h`, 기준선 1f00a25a, 10.78 전용).
// HudDividerGump(004c6c50 생성·004c4790 등록·004c48f0 부착)는 Uber 와 생산 패널이 공유하는 몸체이고, HudDock 은 HUD 직계 자식 8개의 현재 토큰(매 소비 때 다시 읽음)과 실제 화면/지도 전역(live 객체)을 묶어
// 004c7160 표준 배치 등록·004c6530 표시 모드/채팅 토글/지도 갱신을 수행한다. 원문의 float32 저장 위치와 중간 배정도 합(저장하지 않는 `FST field→FMUL scale`)을 그대로 지킨다.
// 사용: const dock = new HudDock(nodes, globals, () => color, hooks); dock.construct(token, axis, width, height, backgroundFlags, type, frame); dock.registerSlave(master, slave, flags); dock.attach(token, x, y, parent, mode); dock.handle(event, first, second).
//   nodes = { root, vertical, minimapDivider, chatDivider, minimap, production, chat, knowledge }(토큰 객체) 또는 `HudDockPart` 를 받아 현재 토큰을 돌려주는 함수, globals = { screenWidth, screenHeight, showHud, mapLeft, mapTop, mapWidth, mapHeight,
//   mapFloatRect[4], mapRect{left,top,right,bottom}, leftBottomRect{…}, fullRefresh, savedChatY }(복사하지 않고 같은 객체에 쓴다), hooks = { node, divider, constructBackground, activate, baseAttach, push, attach, move, resolveSerial, bubble,
//   closePopups, invalidate, finishResize, assertion, baseHandle, chatVisible, chatInput }(도달한 경계만 요구). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E470: 5-B HudDock 이식, 기준선 뒤 cpppj 변경 없음)
import { GumpBackgroundNode } from './gump-background-create.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 실제 10.78 divider 가상 표와 HUD 명령의 원시 값이다(이동 0200003a·크기 변경 02000046·HUD 모드 0200003d·채팅 크기 02000005·채팅 토글 02000088).
const DIVIDER_VTABLE = 0x00514fd8; const MOVED = 0x0200003a; const RESIZED = 0x02000046; const HUD_MODE = 0x0200003d; const CHAT_SIZE = 0x02000005; const CHAT_TOGGLE = 0x02000088;
// 고정 슬레이브 슬롯 수(원본 20DWORD)와 RegisterSlave 가득 참 진단의 원본 줄 번호이다.
const SLAVE_SLOTS = 10; const SLAVE_FULL_LINE = 83;

// HUD 직계 자식 종류이다. 현재 실제 Uber this 의 필드를 가리키며 루트(전역 등록 0 루트)와 새로 생성 중인 this 를 구별한다.
export const HudDockPart = Object.freeze({ root: 0, vertical: 1, minimapDivider: 2, chatDivider: 3, minimap: 4, production: 5, chat: 6, knowledge: 7 });

// 원본 0x11c BYTE divider 노드이다. +b8 axis(크기 연동·드래그 제한·커서 모양 원시 플래그)·+bc/+c0/+c4/+c8 signed 화면 제한·+cc..118 슬레이브 10칸([serial, 관계 플래그])이다.
export class HudDividerNode extends GumpBackgroundNode {
    // 원시 플래그와 signed 제한 네 값이다.
    axis = 0; minX = 0; maxX = 0; minY = 0; maxY = 0;
    // 현재 serial 과 관계 플래그의 고정 10개 슬롯이다.
    slaves = Array.from({ length: SLAVE_SLOTS }, () => [0, 0]);
}

// 원본 DWORD 합산/뺄셈의 하위 32비트(signed)이다.
const add = (a, b) => (a + b) | 0;
const subtract = (a, b) => (a - b) | 0;
// 원본 signed64 _ftol 의 하위 DWORD 이다(indefinite 는 0).
const truncate = (value) => cameraFtolLow(value);
// FST field→FMUL scale 이므로 저장된 float32 를 다시 곱하지 않고 확장 값에 현재 scale 을 곱한 뒤 float32 로 저장한다.
function setWidth(node, value) { node.baseWidth = Math.fround(value); node.width = Math.fround(value * node.scale); }
// 세로도 FST 뒤 아직 확장 정밀도인 값을 현재 scale 에 곱한다.
function setHeight(node, value) { node.baseHeight = Math.fround(value); node.height = Math.fround(value * node.scale); }

// 표시 플래그를 켜거나 끈다(unsigned DWORD 로 유지).
const setFlag = (node, mask) => { node.flags = (node.flags | mask) >>> 0; };
const clearFlag = (node, mask) => { node.flags = (node.flags & ~mask) >>> 0; };

// 현재 divider 저장소와 외부 경계를 같은 hooks 에서 읽는다. 도달한 경계만 요구한다(C++ runtime_error → Error).
function need(callback, message) { if (typeof callback !== 'function') throw new Error(message); return callback; }

// Uber 와 생산 패널의 실제 구분선이 공유하는 생성/부착/슬레이브 몸체이다. globals = { screenWidth, screenHeight }(live), color = () => signed BYTE(00540452). (2026-11-15 WB04-E470)
export class HudDividerGump {
    // 실제 화면 두 전역(live 객체), 런타임 색 패치의 현재 값 getter, 같은 callback 묶음이다.
    #globals; #color; #hooks;
    // 호출보다 오래 사는 실제 구분선 종류/가상 분배를 사본 없이 빌린다.
    constructor(globals, color, hooks) { this.#globals = globals; this.#color = color; this.#hooks = hooks; }

    // 구분선과 공통 Gump 가 같은 현재 주소를 사용해야 한다.
    #node(token) {
        const node = token && this.#hooks.node ? this.#hooks.node(token) : null;
        if (!node) throw new Error('구분선의 실제 기본 저장소 없음');
        return node;
    }

    // 확장 주소를 별도 배경 사본으로 대신하지 않는다.
    #divider(token) {
        const node = token && this.#hooks.divider ? this.#hooks.divider(token) : null;
        if (!node || node !== this.#node(token)) throw new Error('구분선의 실제 파생 저장소 불일치');
        return node;
    }

    // 004c6c50: 기본 배경 생성(signed 정수→float32) 뒤 원본 확장 필드만 초기화하고 20DWORD 슬레이브 영역을 지운다. 기존 제한 4개는 보존한다. 활성 등록(flag 1)을 하고 token 을 돌려준다.
    construct(token, axis, width, height, backgroundFlags, type, frame) {
        if (!this.#hooks.constructBackground || !this.#hooks.activate) throw new Error('HUD divider 생성 경계 없음');
        this.#hooks.constructBackground(token, Math.fround(width | 0), Math.fround(height | 0), backgroundFlags >>> 0, type >>> 0, frame >>> 0);
        const node = this.#divider(token);
        node.vtable = DIVIDER_VTABLE; setFlag(node, 0x60200); node.command = ((this.#color() << 24) >> 24) >>> 0; node.axis = axis >>> 0;
        // 20DWORD 슬레이브 영역을 모두 지운다.
        for (const slave of node.slaves) { slave[0] = 0; slave[1] = 0; }
        this.#hooks.activate(token, 1);
        return token;
    }

    // 004c4790: 첫 빈 serial 슬롯에 등록한다(중복 허용). 가득 차면 진단(line 83)을 부르고 복귀해도 자식에 40000 비트를 남긴다. slave 0 은 아무것도 하지 않는다.
    registerSlave(master, slave, flags) {
        if (!slave) return;
        const divider = this.#divider(master);
        let index = 0;
        // serial 0 은 빈 슬롯이다.
        for (; index < divider.slaves.length; index += 1) if (!divider.slaves[index][0]) break;
        if (index === divider.slaves.length) { need(this.#hooks.assertion, 'HUD slave 한도 진단 경계 없음')(SLAVE_FULL_LINE); } else { divider.slaves[index] = [this.#node(slave).serial, flags >>> 0]; }
        setFlag(this.#node(slave), 0x40000);
    }

    // 004c48f0: 첫 popup 정리 뒤 oldY 를 저장하며 기본 부착과 방송 뒤에는 현재 좌표/관계를 다시 읽고 슬레이브를 관계 플래그 1/2/4/8 순서로 재배치한다.
    attach(token, x, y, parent, mode) {
        this.#hooks.closePopups();
        const master = this.#divider(token);
        const oldY = master.y;
        this.#hooks.baseAttach(token, x, y, parent, mode);
        if (!(master.flags & 1)) return;
        if (master.axis & 0x10) setWidth(master, this.#globals.screenWidth - x);
        this.#hooks.bubble(token, MOVED, master.serial, 0);
        // 고정 10칸을 순회하되 매번 현재 serial 을 읽으며 해결한 slave 는 네 관계 분기에서 유지한다.
        for (let index = 0; index < master.slaves.length; index += 1) {
            if (!master.slaves[index][0]) continue;
            const slave = this.#hooks.resolveSerial(master.slaves[index][0]);
            if (!slave) continue;
            const child = this.#node(slave);
            const preservedY = Math.fround(y - (oldY - child.y));
            const relation = () => master.slaves[index][1];
            if (relation() & 1) {
                this.#hooks.push(slave); setWidth(child, master.x - child.x); this.#hooks.attach(slave, child.x, child.y, 0, 1);
            }
            if (relation() & 2) {
                this.#hooks.push(slave);
                if (relation() & 0x10) setHeight(child, master.y - child.y);
                if (!(relation() & 0x20)) { setHeight(child, master.y - child.y); this.#hooks.attach(slave, child.x, child.y, 0, 1); } else this.#hooks.attach(slave, child.x, preservedY, 0, 1);
            }
            if (relation() & 4) {
                this.#hooks.push(slave);
                // 두 필드 합은 slave 폭 계산이 끝날 때까지 float32 로 줄이지 않는다.
                const right = master.baseWidth + master.x;
                setWidth(child, this.#globals.screenWidth - right); this.#hooks.attach(slave, Math.fround(right), child.y, 0, 1);
            }
            if (relation() & 8) {
                this.#hooks.push(slave);
                // 확장 합과 실제 Attach 의 float32 인자를 구별한다.
                const bottom = master.baseHeight + master.y;
                setHeight(child, (relation() & 0x40) ? (oldY - master.y) + child.baseHeight : this.#globals.screenHeight - bottom);
                this.#hooks.attach(slave, child.x, Math.fround(bottom), 0, 1);
            }
        }
    }
}

// HUD 본체이다. 상태와 노드는 처리기 및 모든 콜백보다 오래 살아 있어야 한다. 토큰은 같은 실제 부모의 현재 필드를 읽으며 값을 캐시하지 않는다. (2026-11-15 WB04-E470)
export class HudDock {
    // 현재 토큰 뷰(HudDockPart → 토큰), 실제 전역 live 객체, 색 getter, 실제 공통 Gump 와 가상 처리 callback 이다.
    #nodes; #globals; #color; #hooks; #divider;
    // nodes 는 HudDockNodes 객체 또는 같은 현재 필드를 읽는 함수이다. 노드/트리 처리기 아홉 개가 없으면 만들기 전에 거부한다(C++ invalid_argument → TypeError).
    constructor(nodes, globals, color, hooks) {
        const names = ['root', 'vertical', 'minimapDivider', 'chatDivider', 'minimap', 'production', 'chat', 'knowledge'];
        this.#nodes = typeof nodes === 'function' ? nodes : (part) => nodes[names[part]];
        this.#globals = globals; this.#color = color; this.#hooks = { ...hooks };
        if (['node', 'divider', 'baseAttach', 'push', 'attach', 'move', 'resolveSerial', 'bubble', 'closePopups'].some((name) => typeof this.#hooks[name] !== 'function')) throw new TypeError('HUD의 실제 노드/트리 처리기가 필요합니다');
        this.#divider = new HudDividerGump(globals, color, this.#hooks);
    }

    // 같은 실제 부모의 현재 필드를 읽는다(캐시하지 않는다).
    #token(part) { return this.#nodes(part) >>> 0; }

    // 원본 유효 포인터의 수명 계약을 토큰 경계에서 검사한다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node) throw new Error('HUD Gump 저장소 없음');
        return node;
    }

    // 제품 투영이 별도 복사본을 반환하지 않도록 같은 상속 주소를 요구한다.
    #dividerNode(token) {
        const node = token ? this.#hooks.divider(token) : null;
        if (!node || node !== this.#node(token)) throw new Error('HUD divider의 동일 저장소 필요');
        return node;
    }

    // 004c6c50: 배경 생성 뒤 divider 필드·고정 10슬롯을 초기화한다. 같은 공유 몸체를 호출한다.
    construct(token, axis, width, height, backgroundFlags, type, frame) { return this.#divider.construct(token, axis, width, height, backgroundFlags, type, frame); }

    // 004c4790: 첫 빈 serial 슬롯에 등록한다. 중복은 허용하며 가득 차도 child 의 40000 비트를 쓴다.
    registerSlave(master, slave, flags) { this.#divider.registerSlave(master, slave, flags); }

    // 004c47f0: 실제 기본 부착 뒤 현재 관계 플래그의 1/2/4/8 순서로 재배치한다.
    attach(token, x, y, parent, mode) { this.#divider.attach(token, x, y, parent, mode); }

    // 004c7160 의 실제 자식 생성 후 등록 10회와 세 divider 의 제한만 설정한다.
    registerNormalLayout() {
        const part = HudDockPart;
        // 원문 등록 순서이다(세로 divider 6회, 지도 divider 3회, 채팅 divider 1회).
        const registrations = [[part.vertical, part.minimapDivider, 1], [part.vertical, part.chatDivider, 4], [part.vertical, part.minimap, 1], [part.vertical, part.production, 1], [part.vertical, part.knowledge, 1], [part.vertical, part.chat, 4],
            [part.minimapDivider, part.minimap, 8], [part.minimapDivider, part.production, 2], [part.minimapDivider, part.knowledge, 2], [part.chatDivider, part.chat, 8]];
        for (const [master, slave, flags] of registrations) this.registerSlave(this.#token(master), this.#token(slave), flags);
        const vertical = this.#dividerNode(this.#token(part.vertical)); vertical.minX = 60; vertical.maxX = 130; vertical.axis = (vertical.axis | 4) >>> 0;
        const minimap = this.#dividerNode(this.#token(part.minimapDivider)); minimap.minY = subtract(this.#globals.screenHeight, 123); minimap.maxY = this.#globals.screenHeight | 0; minimap.axis = (minimap.axis | 8) >>> 0;
        const chat = this.#dividerNode(this.#token(part.chatDivider)); chat.minY = 25; chat.maxY = subtract(this.#globals.screenHeight, 25); chat.axis = (chat.axis | 8) >>> 0;
    }

    // 004c6530: HUD 모드 1/3/4·채팅 표시·divider 이동의 viewport 갱신이다. 사건에서 release 비트도 제거한 뒤 처리한다. 반환은 원본 EAX 이다.
    handle(event, first, second) {
        // 사건과 두 인자는 원본 DWORD 이다.
        first >>>= 0; second >>>= 0;
        const part = HudDockPart; const globals = this.#globals; const hooks = this.#hooks;
        const command = (event >>> 0) & 0x03ffffff;
        if (command === CHAT_TOGGLE) {
            if (!hooks.chatVisible || !hooks.chatInput) throw new Error('HUD의 실제 채팅 설정 경계 없음');
            if (second && (second !== 2 || globals.screenHeight <= this.#node(this.#token(part.chatDivider)).y)) {
                hooks.chatVisible(true);
                if (globals.screenHeight <= this.#node(this.#token(part.chatDivider)).y) {
                    hooks.push(this.#token(part.chatDivider)); const token = this.#token(part.chatDivider); hooks.attach(token, this.#node(token).x, globals.savedChatY, this.#token(part.root), 0);
                }
            } else {
                hooks.chatVisible(false);
                if (this.#node(this.#token(part.chatDivider)).y < globals.screenHeight) globals.savedChatY = this.#node(this.#token(part.chatDivider)).y;
                hooks.chatInput(false); hooks.push(this.#token(part.chatDivider)); const token = this.#token(part.chatDivider);
                hooks.attach(token, this.#node(token).x, Math.fround(globals.screenHeight), this.#token(part.root), 0);
            }
        }
        if (command === MOVED || command === RESIZED) {
            if (!hooks.invalidate) throw new Error('HUD dirty 경계 없음');
            if (this.#token(part.vertical) && first === this.#node(this.#token(part.vertical)).serial) {
                const node = this.#node(this.#token(part.vertical)); const left = truncate(node.x + node.baseWidth); const height = globals.screenHeight;
                globals.mapWidth = subtract(globals.screenWidth, left); globals.mapLeft = left;
                hooks.invalidate(0, 0, add(left, 1), height, 0);
                hooks.invalidate(globals.mapLeft, globals.mapHeight, globals.mapWidth, subtract(globals.screenHeight, globals.mapHeight), 0);
            }
            if (this.#token(part.chatDivider) && first === this.#node(this.#token(part.chatDivider)).serial) {
                const height = truncate(this.#node(this.#token(part.chatDivider)).y); globals.mapHeight = height;
                hooks.invalidate(globals.mapLeft, height, globals.mapWidth, subtract(globals.screenHeight, height), 0);
            }
            // 레지스터/스택에 먼저 고정한 정수 LTRB 이다.
            const left = globals.mapLeft; const top = globals.mapTop; const right = add(left, globals.mapWidth); const bottom = add(top, globals.mapHeight);
            Object.assign(globals.mapRect, { left, top, right, bottom });
            for (const [index, value] of [left, top, right, bottom].entries()) globals.mapFloatRect[index] = Math.fround(value);
            if (this.#token(part.minimapDivider)) {
                const node = this.#node(this.#token(part.minimapDivider));
                Object.assign(globals.leftBottomRect, { left: 0, top: truncate(node.y), right: truncate(node.baseWidth), bottom: globals.screenHeight | 0 });
            } else Object.assign(globals.leftBottomRect, { left: 0, top: 0, right: 0, bottom: 0 });
            return 1;
        }
        if (command !== HUD_MODE) {
            if (command === CHAT_SIZE) {
                const y = Math.fround(subtract(subtract(globals.screenHeight, second | 0), 5));
                hooks.push(this.#token(part.chatDivider)); const token = this.#token(part.chatDivider); hooks.attach(token, this.#node(token).x, y, this.#token(part.root), 0);
                return 1;
            }
            return need(hooks.baseHandle, 'HUD 기본 입력 경계 없음')(event, first, second);
        }
        if (first === 1 || first === 3) {
            // 표시 비트를 켜고 왼쪽 덱의 이동 위치를 정한다.
            setFlag(this.#node(this.#token(part.vertical)), 1); setFlag(this.#node(this.#token(part.minimapDivider)), 1); setFlag(this.#node(this.#token(part.minimap)), 1); setFlag(this.#node(this.#token(part.production)), 1);
            clearFlag(this.#node(this.#token(part.knowledge)), 1);
            hooks.move(this.#token(part.vertical), (first === 3 || globals.showHud) ? 80.0 : -this.#node(this.#token(part.vertical)).baseWidth, 0, 0);
            hooks.move(this.#token(part.minimapDivider), 0, Math.fround(subtract(globals.screenHeight, 80)), 0);
        } else if (first === 4) {
            clearFlag(this.#node(this.#token(part.production)), 1); setFlag(this.#node(this.#token(part.knowledge)), 1); clearFlag(this.#node(this.#token(part.minimap)), 1);
            hooks.move(this.#token(part.vertical), 170, 0, 0); hooks.move(this.#token(part.minimapDivider), 0, Math.fround(globals.screenHeight | 0), 0);
        }
        hooks.push(this.#token(part.chatDivider)); const token = this.#token(part.chatDivider); hooks.attach(token, this.#node(token).x, this.#node(token).y, 0, 0);
        globals.fullRefresh = (globals.fullRefresh + 1) >>> 0;
        need(hooks.finishResize, 'HUD 화면 후처리 경계 없음')();
        return 0;
    }
}
