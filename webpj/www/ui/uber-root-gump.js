// 원본 Uber 루트의 실제 자식 필드와 현재 HUD 노드 뷰를 옮긴다(cpppj client/UberRootGump.cpp/.h, 10.78).
// 사용: const view = uberRootDockView(token, { node, base }); new HudDock(view, globals, color, hooks).
import { GumpCreateNode } from './gump-create.js';
import { HudDockPart } from './hud-dock.js';

// 원본 0xc8BYTE Uber 파생 노드의 10개 DWORD 자식·예약 필드이다.
export class UberRootNode extends GumpCreateNode {
    // 생성자가 보존하는 예약 필드와 현재 자식 토큰이다.
    vertical = 0; minimapDivider = 0; chatDivider = 0; minimap = 0; production = 0;
    reservedB4 = 0; chat = 0; reservedBC = 0; knowledge = 0; message = 0;
}

// 같은 실제 파생/기본 주소를 매 조회마다 확인하고 현재 필드를 직접 빌린다.
export function uberRootDockView(token, hooks) {
    token >>>= 0;
    const current = () => {
        if (!token || typeof hooks?.node !== 'function' || typeof hooks?.base !== 'function') throw new Error('Uber 루트 실제 저장소 누락');
        const derived = hooks.node(token), base = hooks.base(token);
        if (!(derived instanceof UberRootNode) || derived !== base) throw new Error('Uber 루트 파생 저장소 불일치');
        return derived;
    };
    current();
    return (part) => {
        const node = current();
        switch (part) {
        case HudDockPart.root: return token;
        case HudDockPart.vertical: return node.vertical;
        case HudDockPart.minimapDivider: return node.minimapDivider;
        case HudDockPart.chatDivider: return node.chatDivider;
        case HudDockPart.minimap: return node.minimap;
        case HudDockPart.production: return node.production;
        case HudDockPart.chat: return node.chat;
        case HudDockPart.knowledge: return node.knowledge;
        default: throw new RangeError('Uber 현재 HUD 필드 종류 오류');
        }
    };
}
