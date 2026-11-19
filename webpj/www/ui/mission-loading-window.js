// 미션 로그인 진행 문구를 기존 HTML 대화상자와 표시 문자 부호화에 연결한다.
// 사용: createMissionLoadingWindow({ message, cancelText, resources, gameData, screen, onCommand }).
import { displayTextCodec } from '../assets/display-text.js';
import { buildMissionLoadingMessageBody } from '../core/mission-loading-message.js';
import { LocalMissionProgress } from '../core/raw-local-mission-state.js';
import { prepareDialogContent } from './html-text.js';
import { TellWindow } from './tell-window.js';

// 원본 진행 상태별 번역 전 문구다. 빈 서버 주소와 남은 초는 번역 뒤에 치환한다.
const PROGRESS_TEXT = Object.freeze({
    [LocalMissionProgress.MissionStarting]: 'Initializing.',
    [LocalMissionProgress.ReconnectStarting]: 'Game Server disconnected.  Reconnecting to new Game Server.',
    [LocalMissionProgress.ConnectingCountdown]: 'Connecting to Game Server %s - Countdown %d.',
    [LocalMissionProgress.ReconnectCountdown]: 'Reconnecting to Game Server %s - Countdown %d.',
    [LocalMissionProgress.ServerLost]: 'Connection to Game Server lost.',
    [LocalMissionProgress.ConnectingStarting]: 'Connecting to Game Server. %s',
    [LocalMissionProgress.Connecting]: 'Connecting to Game Server %s',
    [LocalMissionProgress.Reconnecting]: 'Reconnecting to Game Server %s',
    [LocalMissionProgress.Transferring]: 'Connected to server %s - Transferring.',
    [LocalMissionProgress.ReconnectTransferring]: 'Reconnecting to new Game Server. Transferring.',
    [LocalMissionProgress.MissionTransferring]: 'Starting Mission...',
});

// 상태별 원문을 현재 번역기로 번역하고 첫 주소·정수 서식만 원본처럼 바꾼다.
export function missionLoginProgressText(kind, remaining, translate) {
    if (!Object.hasOwn(PROGRESS_TEXT, kind)) throw new RangeError('로그인 진행 안내 종류 범위');
    if (typeof translate !== 'function') throw new TypeError('로그인 진행 안내의 번역 장치 누락');
    return String(translate(PROGRESS_TEXT[kind])).replace('%s', '').replace('%d', String(remaining | 0));
}

// 표시 문자열을 원본처럼 한 문자에 한 바이트인 임시 문자열로 바꾼다.
function narrow(text) { return Array.from(displayTextCodec.encode(text), (byte) => String.fromCharCode(byte)).join(''); }
// 표시 바이트의 확장 문자를 같은 앱 글꼴이 받는 문자열로 복원한다.
function wide(text) { return displayTextCodec.decode(Uint8Array.from(text, (character) => character.charCodeAt(0))); }

// 원본 진행 본문·Cancel 메타와 같은 돌 대화상자를 만들고 화면 가운데에 배치한다.
export function createMissionLoadingWindow({ message, cancelText, resources, gameData, screen, nowSeconds = 0, now = () => 0, onCommand = null, onSound = null }) {
    if (!resources || !gameData || !screen || typeof gameData.substitute !== 'function') throw new TypeError('미션 진행 창의 실제 자원·설정·화면 누락');
    const body = buildMissionLoadingMessageBody(narrow(message), () => narrow(cancelText));
    const content = prepareDialogContent(wide(body), { substitute: (text) => gameData.substitute(text), okButton: gameData.get('OkButton') ?? undefined });
    if (content.empty) throw new Error('미션 진행 창의 표시 본문이 비어 있음');
    const window = new TellWindow({ resources, content, screen, requested: { width: 400, height: 30 }, nowSeconds, now, onCommand, onSound });
    window.fit(); window.place(); return window;
}
