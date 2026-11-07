// 대본이 예약하는 글꼴/음질 변경 명령(ChangeFontFace·ChangeSoundQuality)을 기존 글꼴 캐시·설정·음향 장치 수명에 전달하는 입력 분배 모듈이다(cpppj `client/PresentationCommandInput.cpp/.h`, 10.78 전용, 원문 004d7051/004d7295).
// Guide/Gump/뗌 뒤이며 Console/정지보다 앞이다. 수정키를 지운 코드가 두 명령이 아니면 false, 대상의 뗌은 효과 없이 true 이다. 호스트 UI 선택이나 정지 관문을 추가하지 않는다.
//  - ChangeFontFace(0x0200001b): fontFace(y) → rebuildFonts(face; 범위 밖은 null=Arial, 유효 빈 이름은 빈 이름) → height() 를 먼저 읽고 width() → invalidate(0,0,width,height,0) → currentFontFace() → set('fontFaceName', 현재 이름).
//  - ChangeSoundQuality(0x020000dd): closeSound() → config('soundQuality=' + signed y) → sound('priestmove.wav',0,0,0,0,0). Config 가 실패하면 되돌림이나 음향 합성을 하지 않는다.
// 선택된 분기의 누락은 그 위치에서 거부하고 앞선 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E25: 글꼴/음질 명령 이식으로 추가)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 두 명령의 초기 DWORD(00542764/00542a68)이다. (2026-11-08 WB04-E25)
export const CHANGE_FONT_FACE_COMMAND = 0x0200001b;
export const CHANGE_SOUND_QUALITY_COMMAND = 0x020000dd;
// 수정키 제거 마스크이다. (2026-11-08 WB04-E25)
const KEY_MASK = 0x03ffffff;

// 선택된 장치가 없으면 빈 성공으로 바꾸지 않고 거부한다. (2026-11-08 WB04-E25)
function need(device) {
    if (typeof device !== 'function') throw new Error('글꼴/음질 명령의 실제 경계 미연결');
    return device;
}

// 두 원문 이름만 ASCII 대소문자 무시로 조회하고 미등록이면 0xffffffff 이다. (2026-11-08 WB04-E25)
export function presentationCommandScriptCode(name) {
    const key = asciiLower(name);
    if (key === 'changefontface') return CHANGE_FONT_FACE_COMMAND;
    if (key === 'changesoundquality') return CHANGE_SOUND_QUALITY_COMMAND;
    return 0xffffffff;
}

// 수정키를 제거한 명령 번호가 두 명령 중 하나인지이다. (2026-11-08 WB04-E25)
export function isPresentationCommand(code) {
    const value = (code & KEY_MASK) >>> 0;
    return value === CHANGE_FONT_FACE_COMMAND || value === CHANGE_SOUND_QUALITY_COMMAND;
}

// event = { code, x, y }, hooks = { fontFace(index) → 이름|null, rebuildFonts(face|null), width(), height(), invalidate(x,y,w,h,flags), currentFontFace(), set(key,value), closeSound(), config(text), sound(name,0,0,0,0,0) } 이다.
// 소비하면 true 이다. (2026-11-08 WB04-E25)
export function dispatchPresentationCommandInput(event, hooks) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!isPresentationCommand(code)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    if (code === CHANGE_FONT_FACE_COMMAND) {
        const face = need(hooks.fontFace)(event.y);
        need(hooks.rebuildFonts)(face);
        // 재생성 뒤 높이를 먼저 읽고 폭을 읽는다.
        const height = need(hooks.height)();
        const width = need(hooks.width)();
        need(hooks.invalidate)(0, 0, width, height, 0);
        need(hooks.set)('fontFaceName', need(hooks.currentFontFace)());
        return true;
    }
    need(hooks.closeSound)();
    // signed %d 이며 장치 허용 0..3 으로의 낮춤은 Config 해석이 하고 원시 값은 보존한다.
    need(hooks.config)(`soundQuality=${event.y | 0}`);
    need(hooks.sound)('priestmove.wav', 0, 0, 0, 0, 0);
    return true;
}
