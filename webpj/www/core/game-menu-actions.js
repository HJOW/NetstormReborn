// 원본 UserInput(4d62b0) 의 Guide/Gump 전처리가 소비하지 않은 "일반 메뉴 명령" 몸체를 같은 현재 전역·진단 뒤 계속·외부 효과 순서로 옮긴 모듈이다(cpppj `client/GameMenuActions.cpp/.h`).
// 전체 UserInput 의 OS 메시지 폴링/Guide/Gump 전처리는 이 모듈의 범위가 아니며, 앞단이 소비하지 않은 명령만 `handle` 로 넘긴다.
// 사용:
//   const actions = new GameMenuActions(globals, hooks);
//   actions.handle(event, first, second) → null(대상 아님) | 0(처리됨/release) | 1(QuitApp 의 원문 즉시 반환)
//   globals = { read(주소) → uint32(0055c85a4/594fb8/594fc8/594fa4/595344/540c70 을 호출 위치에서 현재 조회), hideBuildings: { value }(0059a8ac), redraw: { value }(0059a8b0) }
//   hooks   = { command(이름) → 현재 명령 ID(순수 조회), queue(명령, 첫째, 둘째), tell(문장), config(설정문), sound(이름, 0, 0, 0, 0, 0),
//               paused() → boolean, status(a, b, c, d, 문장), cancelCursor(), pauseClock(), resumeClock(), muteSound(), resumeSound(),
//               player(owner) → Uint8Array(null 포인터는 빈 배열), centerObject(sid), viewPriest(owner),
//               reloadCount(목록) → signed 개수, reloadSid(목록, 번호) → SID, form(sid) → Uint8Array(최소 0x16BYTE), reloadAt(x, y, 0),
//               assertion(줄 번호), getFullDeck(), text(주소) → 문자열, addLocalSp(float) }
//   선택한 분기가 요구하는 효과만 필수이며 미연결 효과를 성공으로 대체하지 않는다(미연결은 Error, 범위 밖 Player/Form 읽기는 RangeError).
// 원본 관측: `game-menu-actions-x86.tsv` 610행·`cheat-money-x86.tsv` 144행·`full-deck-input-x86.tsv` 384행을 tests/game-menu-actions.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인).
// 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E535: 6단계 게임 메뉴 GameMenuActions 이식)

// 원문 명령 전역의 주소 식별자이며 실제 명령 ID 는 `command` 훅이 현재 값을 돌려준다. (2026-11-15 WB04-E535)
export const GameMenuActionCommand = Object.freeze({
    QuitApp: 0x00542700,
    RequestQuitApp: 0x00542944,
    SoundVolume: 0x00542a8c,
    MusicVolume: 0x00542a90,
    Pause: 0x00542a30,
    ViewHome: 0x005429ac,
    ViewYourPriest: 0x005429d4,
    HideBuildings: 0x00542ab8,
    ReloadTheams: 0x00542b54,
    // Editor 또는 missionPost 에서 선택되는 실제 전체 Deck 명령이다.
    GetFullDeck: 0x00542a28,
    // HTML/대본의 문자열 이름으로 004a9cb0 을 호출하는 일반 명령이다.
    PlaySound: 0x0054295c,
    // Editor/missionPost 의 signed 숫자를 실제 로컬 SP 가산에 전달한다.
    AddSp: 0x0054287c,
});

// 두 실제 SID 목록(0055a70c/714 와 005e189c/18a4)을 구별하는 값이다. (2026-11-15 WB04-E535)
export const GameMenuReloadList = Object.freeze({ First: 0, Second: 1 });

// 이번에 분리한 실제 원문 명령만 이 순서로 조회하며 MenuJustClosed 는 부모 Gump 에 남긴다. (2026-11-15 WB04-E535)
const ACTIONS = Object.freeze([
    GameMenuActionCommand.QuitApp, GameMenuActionCommand.RequestQuitApp, GameMenuActionCommand.SoundVolume, GameMenuActionCommand.MusicVolume,
    GameMenuActionCommand.ReloadTheams, GameMenuActionCommand.ViewHome, GameMenuActionCommand.ViewYourPriest, GameMenuActionCommand.Pause,
    GameMenuActionCommand.HideBuildings, GameMenuActionCommand.GetFullDeck, GameMenuActionCommand.AddSp, GameMenuActionCommand.PlaySound,
]);

// 도달한 외부 효과를 미연결 성공으로 처리하지 않도록 함수가 아니면 거부한다. (2026-11-15 WB04-E535)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('현재 게임 메뉴의 실제 효과 미연결');
    return hook;
}

// 정렬되지 않은 Player/Form 원문 DWORD 를 현재 바이트에서 읽으며 범위 밖은 거부한다. (2026-11-15 WB04-E535)
function read(bytes, offset) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('현재 게임 메뉴의 실제 기록 범위');
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// Guide/Gump 가 소비하지 않은 일반 메뉴 명령의 효과를 실행하는 처리기이다. 전역과 장치의 수명은 이 처리기보다 길어야 한다. (2026-11-15 WB04-E535)
export class GameMenuActions {
    // 전역은 사본 없이 현재 주소 getter 와 `{ value }` 상자로 보관하고 명령 값 조회만 생성 시 요구한다. (2026-11-15 WB04-E535)
    constructor(globals, hooks) {
        need(globals.read);
        need(hooks.command);
        this.globals = globals;
        this.hooks = hooks;
    }

    // 대상이 아니면 null, 처리된 일반 명령은 0, QuitApp 의 원문 즉시 반환은 1 이며 release(상위 비트)는 효과 없이 0 이다. (2026-11-15 WB04-E535)
    handle(event, first, second) {
        const g = this.globals;
        const h = this.hooks;
        const code = (event & 0x03ffffff) >>> 0;
        // 원문 인자는 DWORD 이므로 음수 입력도 하위 32비트로 정규화한다(signed 해석은 쓰는 분기에서 `| 0`).
        second >>>= 0;
        let selected = null;
        // 정상 원문 이름표의 서로 다른 현재 ID 중 대상만 고른다.
        for (const action of ACTIONS) {
            if (code === (h.command(action) >>> 0)) {
                selected = action;
                break;
            }
        }
        if (selected === null) return null;
        if ((event & 0x80000000) !== 0) return 0;
        switch (selected) {
            case GameMenuActionCommand.AddSp: {
                // 원본 두 분기 모두 같은 가산 부모를 호출하며 일반 모드의 0 을 생략하지 않는다.
                const amount = g.read(0x005c85a4) || g.read(0x00594fa4) ? Math.fround(second | 0) : 0;
                need(h.addLocalSp)(amount);
                break;
            }
            case GameMenuActionCommand.GetFullDeck:
                if (g.read(0x005c85a4) || g.read(0x00594fa4)) need(h.getFullDeck)();
                break;
            case GameMenuActionCommand.PlaySound: {
                // 원문 4d7c79..89 의 현재 두 번째 인자와 고정 0 다섯 개이며 모드·소유자 관문은 없다.
                const name = need(h.text)(second);
                need(h.sound)(name, 0, 0, 0, 0, 0);
                break;
            }
            case GameMenuActionCommand.QuitApp:
                if ((second >>> 0) !== 99) need(h.assertion)(993);
                need(h.sound)('selectTab.wav', 0, 0, 0, 0, 0);
                return 1;
            case GameMenuActionCommand.RequestQuitApp:
                if (g.read(0x005c85a4)) need(h.tell)('QuitEditMode');
                else if (!g.read(0x00594fb8) && !g.read(0x00594fc8)) need(h.tell)('QuitAbandon');
                else need(h.queue)(h.command(GameMenuActionCommand.QuitApp), 0, 99);
                break;
            case GameMenuActionCommand.SoundVolume:
            case GameMenuActionCommand.MusicVolume: {
                // 원문 %d 는 signed DWORD 를 그대로 전달한다.
                const assignment = `${selected === GameMenuActionCommand.SoundVolume ? 'soundVolume=' : 'musicVolume='}${second | 0}`;
                need(h.config)(assignment);
                need(h.sound)('priestmove.wav', 0, 0, 0, 0, 0);
                break;
            }
            case GameMenuActionCommand.Pause:
                if (g.read(0x00594fa4) || g.read(0x005c85a4)) {
                    const paused = need(h.paused)();
                    need(h.status)(11, 0, paused ? 0 : 1, 0, '');
                    if (paused) {
                        need(h.resumeClock)();
                        need(h.resumeSound)();
                    } else {
                        need(h.cancelCursor)();
                        need(h.pauseClock)();
                        need(h.muteSound)();
                    }
                }
                break;
            case GameMenuActionCommand.ViewHome:
                if (g.read(0x00595344) && !g.read(0x00594fc8)) {
                    // 소유자 0 은 진단 뒤 계속하며 같은 소유자를 다시 조회해 Player 를 얻는다.
                    if (!g.read(0x00540c70)) need(h.assertion)(1895);
                    const player = need(h.player)(g.read(0x00540c70));
                    if (player.length !== 0) {
                        const firstSid = read(player, 0x7c);
                        if (firstSid) need(h.centerObject)(firstSid);
                        else {
                            const secondSid = read(player, 0x84);
                            if (secondSid) need(h.centerObject)(secondSid);
                        }
                    }
                }
                break;
            case GameMenuActionCommand.ViewYourPriest:
                need(h.viewPriest)(second);
                break;
            case GameMenuActionCommand.HideBuildings:
                g.redraw.value = 1;
                g.hideBuildings.value = g.hideBuildings.value === 0 ? 1 : 0;
                break;
            case GameMenuActionCommand.ReloadTheams:
                if (!g.read(0x00594fc8)) {
                    need(h.reloadCount);
                    need(h.reloadSid);
                    need(h.form);
                    need(h.reloadAt);
                    // 원문은 첫 목록을 끝낸 뒤 현재 둘째 목록을 조회한다.
                    for (const list of [GameMenuReloadList.First, GameMenuReloadList.Second]) {
                        // 콜백 뒤 현재 개수를 다시 읽고 다음 SID/슬롯도 현재 주소에서 읽는다.
                        for (let index = 0; (index | 0) < h.reloadCount(list); index += 1) {
                            const sid = h.reloadSid(list, index);
                            const form = h.form(sid);
                            const x = read(form, 0x0e);
                            const y = read(form, 0x12);
                            h.reloadAt(x, y, 0);
                        }
                    }
                }
                break;
            default:
                break;
        }
        return 0;
    }
}
