// 커서의 현재 Gump 조회·수신·부모와 팝업 종료 장치를 같은 컨트롤에 연결한다.
// 사용: bindCampaignCursorGumps(effects, controls). 두 객체는 커서 효과보다 오래 살아야 한다.
import { InputCode } from './input-queue.js';
import { makeClientCursorPrepareHooks } from './client-cursor-state-cleanup.js';
import { GumpText } from '../render/gump-text.js';
import { decodeGumpTextContext } from '../render/gump-text-draw.js';

// 원본 Gump의 조각 허용 조회와 조각 수신 명령이다.
const CURSOR_GUI_FLAGS = 0x0200005f;
const CURSOR_GUI_RECEIVE = 0x0200005e;

// 동일 컨트롤의 실제 트리와 가상 명령에 기존 커서 효과를 연결한다.
export function bindCampaignCursorGumps(effects, controls) {
    // 현재 화면점의 최상위 Gump 토큰을 조회한다.
    effects.placement.gumpAt = (x, y) => controls.pickGump({ x, y });
    // 가상 수신 반환과 재귀 분배 토큰을 구분해 원본 인자를 전달한다.
    effects.placement.accept = (token, event, first, second, hidden) => controls.dispatchGump(token, event, first, second, hidden !== 0);
    // 배치와 내려놓기가 같은 현재 Gump 부모 SID를 조회한다.
    const parent = (token) => controls.gumpParentSid(token);
    effects.placement.parent = parent; effects.drop.guiParent = parent;
    // Gump의 원본 조각 허용 플래그를 조회한다.
    effects.drop.guiFlags = (token) => controls.handleGump(token, CURSOR_GUI_FLAGS, 0, 0);
    // 선택된 조각의 원본 Gump 수신 명령을 실행한다.
    effects.drop.guiReceive = (token, form) => { controls.handleGump(token, CURSOR_GUI_RECEIVE, form, 0); };
    // 기존 종료 훅이 없을 때만 팝업의 지연 Kill을 요청한다.
    if (!effects.lifecycle.closePopups) effects.lifecycle.closePopups = () => { controls.requestClosePopups(); };
}

// 현재 Client의 화면·번역·글꼴·마우스와 선택된 진단 장치를 값형 효과에 연결한다.
export function makeCampaignCursorClientEffects(client, controls, effects = {}) {
    const result = { ...effects, placement: { ...effects.placement }, drop: { ...effects.drop }, lifecycle: { ...effects.lifecycle } };
    bindCampaignCursorGumps(result, controls);
    // 실제 건설 기록이 선택된 경우 같은 Client 진단 장치를 호출한다.
    result.drop.record = (...args) => {
        if (typeof client.recordDeveloperBuildPlacement !== 'function') throw new Error('커서의 실제 건설 기록 장치 미연결');
        client.recordDeveloperBuildPlacement(...args);
    };
    // 비용 표시 대상은 호출 시점의 메뉴를 읽는다.
    result.drop.showSp = (cost, x, y, reference) => {
        const menus = controls.menus(); if (!menus) throw new Error('커서 비용 표시의 실제 Gump 트리 미연결');
        menus.showSp(cost, x, y, reference);
    };
    // 원본 DWORD 커서 번호를 signed 값으로 전달한다.
    result.lifecycle.selectCursor = (number) => client.getCursor().set(number | 0);
    // 현재 언어 번역을 웹 글꼴이 소비할 문자열로 돌려준다.
    result.lifecycle.translate = (key) => client.translate(key);
    // 현재 FontSet의 인코딩과 8DWORD 문맥으로 NUL 종료 텍스트를 측정한다.
    result.lifecycle.measure = (text, words) => {
        const fonts = client.fonts(), encoded = fonts.encode(String(text).split('\0', 1)[0]);
        const terminated = new encoded.constructor(encoded.length + 1); terminated.set(encoded);
        const size = new GumpText(fonts).measure(terminated, decodeGumpTextContext(words));
        return [size.width, size.height];
    };
    // 현재 마우스 위치를 같은 Client 입력 장치에서 읽는다.
    result.lifecycle.mouse = () => { const point = client.poll(InputCode.leftButton); return { x: point.x, y: point.y }; };
    result.placement.invalidate = makeClientCursorPrepareHooks(client).invalidate;
    return result;
}
