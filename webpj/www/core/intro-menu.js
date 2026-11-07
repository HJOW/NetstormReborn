// 인트로 영상 존재 감지(00435de0)와 PlayIntro 일반 입력(004d8xxx → 004b3520)을 옮긴 모듈이다(cpppj `client/IntroMenu.cpp/.h`, 10.78 전용). 메인 메뉴 표시와 일반 입력이 같은 감지/요청 부모를 쓴다.
//  - detectIntroMovie(holder, path, probe): holder = { value }(인트로 사용 가능 DWORD)에 먼저 0 을 쓰고 현재 introMovie 설정 경로(path(), 첫 NUL 앞)를 읽는다. 비어 있으면 파일 장치를 요구하지 않고 0 을 돌려주며, 아니면 존재 조회 probe(경로)가 비영일 때 1 을 쓴다.
//    조회 출처 번호는 보관하지 않고 비영 여부만 1 로 정규화한다. 경로/조회 장치가 실패해도 선행 0 쓰기는 보존한다.
//  - dispatchPlayIntroInput(event, state): 수정키를 지운 코드가 PlayIntro(0x020000d8, 00543248→00542a54)가 아니면 false, 뗌은 소비만 하고 그 밖에는 State 의 next 블록에 next[0] = 39, next[35] = 1 만 쓴다(현재 두 인자와 나머지 블록은 사용하지 않음). state = { next: 배열형 }.
// 영상 파일이나 창은 열지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E33: 인트로 메뉴 이식으로 추가)
import { InputCode } from './input-queue.js';

// 00543248 의 PlayIntro 초기 명령 DWORD, 수정키 제거 마스크, State39 요청 값 위치이다. (2026-11-08 WB04-E33)
export const PLAY_INTRO_COMMAND = 0x020000d8;
const KEY_MASK = 0x03ffffff;
const STATE_INTRO = 39;

// 00435de0: 0 쓰기 → 현재 introMovie → 비어 있지 않으면 존재 조회 → 비영이면 1 쓰기이다. (2026-11-08 WB04-E33)
export function detectIntroMovie(holder, path, probe) {
    holder.value = 0;
    if (typeof path !== 'function') throw new TypeError('Intro 현재 경로 장치 없음');
    const raw = path();
    const nul = raw.indexOf('\0');
    const name = nul < 0 ? raw : raw.slice(0, nul);
    if (name === '') return holder.value;
    if (typeof probe !== 'function') throw new TypeError('Intro 파일 존재 장치 없음');
    if (probe(name)) holder.value = 1;
    return holder.value;
}

// 일반 입력의 현재 두 인자는 원문처럼 사용하지 않는다. State39 의 실제 부모가 준비/영상/복귀를 맡는다. (2026-11-08 WB04-E33)
export function dispatchPlayIntroInput(event, state) {
    if (((event.code & KEY_MASK) >>> 0) !== PLAY_INTRO_COMMAND) return false;
    if ((event.code & InputCode.release) === 0) {
        state.next[0] = STATE_INTRO;
        state.next[35] = 1;
    }
    return true;
}
