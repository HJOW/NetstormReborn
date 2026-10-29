// 메인 메뉴 펼침 메뉴(Help·Options)의 항목 정의를 만드는 모듈이다. 항목 순서·라벨·체크 규칙은 원본 스크린샷(mainMenu - Help/Options*.png)과
// cpppj RawGameMenus::Options(원본 004cc6a0 계열의 항목 작성)·RawGameMenuBar(About 하위)를 따른다.
//  - Options: [Direct Draw / Full Screen, Resolution >] | [Sound On, Play Music, Wind Noise, Speaker Swap L/R, Sound Effect Volume >, Music Volume >]
//             | [Edge Scroll in Fullscreen, Auto-Demo, Tell Tips at Startup, Pause - Shift-F9] | [Pass Server Diagnostic]  (| 는 구분선)
//  - Help:    [General Help - F1, Technical Help, Version]  (구분선 없음)
// 켜짐 표시(파란 점)는 토글 값이 1 일 때와 음량 하위 목록의 현재 단계에만 붙는다(docs/screens/options-menu.md).
// 웹 판본은 원본 항목 중 웹에서 의미가 없는 것을 뺀다(includeAll 이 참이면 모두 만든다. 원본 스크린샷 비교용):
//  - Direct Draw / Full Screen·Resolution(해상도·전체화면 설정은 웹 제외, AGENTS.md), Pass Server Diagnostic(멀티플레이·방화벽 진단),
//    Pause(메인 메뉴에는 멈출 게임이 없음), Technical Help(외부 help.exe 실행).
// 명령은 { name, argument } 이며 Toggle(설정 키 뒤집기)·Volume(`키=단계`)·Tell(대화상자 열기)·F1Help(도움말 창, 아직 없음) 등이다. (2026-10-29 WB03-F)

// 메뉴 항목 정의 도우미들이다. (2026-10-29 WB03-F)
const command = (name, argument) => Object.freeze({ name, argument });
const separator = () => ({ kind: 'separator' });
const text = (label, name, argument, enabled = true) => ({ kind: 'text', label, command: command(name, argument), enabled });
const check = (label, checked, name, argument) => ({ kind: 'check', label, checked: Boolean(checked), command: command(name, argument) });

// 토글 항목이다. 설정 값이 1 이면 켜짐 표시가 붙고, 누르면 설정 키를 뒤집는다. (2026-10-29 WB03-F)
const toggle = (settings, key, label) => check(label, settings.get(key) === 1, 'Toggle', key);

// 음량 하위 목록이다. Volume 1~5 중 현재 단계에 표시가 붙고, 누르면 그 단계가 된다. (2026-10-29 WB03-F)
function volumeItems(settings, key) {
    const items = [];
    // 1~5 단계를 차례로 만든다.
    for (let step = 1; step <= 5; step += 1) items.push(check(`Volume ${step}`, settings.get(key) === step, 'Volume', `${key}=${step}`));
    return items;
}

// 해상도 하위 목록(원본 세 값)이다. 웹에서는 만들지 않으며 스크린샷 비교(includeAll)에서만 쓴다. 현재 값은 1024 by 768 이다. (2026-10-29 WB03-F)
function resolutionItems() {
    return [
        check('640 by 480', false, 'Tell', 'SETSIZE(640,480)'),
        check('800 by 600', false, 'Tell', 'SETSIZE(800,600)'),
        check('1024 by 768', true, 'Tell', 'SETSIZE(1024,768)'),
    ];
}

// 빈 묶음을 빼고 묶음 사이에 구분선을 넣어 한 줄 배열로 합친다. (2026-10-29 WB03-F)
function joinGroups(groups) {
    const items = [];
    // 비어 있지 않은 묶음마다 앞에 구분선을 넣는다(첫 묶음 제외).
    for (const group of groups.filter((entry) => entry.length > 0)) {
        if (items.length > 0) items.push(separator());
        items.push(...group);
    }
    return items;
}

// Options 펼침 메뉴의 항목 정의를 만든다. settings 는 Settings(platform/settings.js)이다.
// includeAll 이 참이면 원본 항목(Direct Draw·Resolution·Pause·Pass Server Diagnostic)도 만든다. 원본 메인 메뉴 상태: 서버 진단 미통과, 일시정지 아님. (2026-10-29 WB03-F)
export function buildOptionsItems(settings, { includeAll = false } = {}) {
    return joinGroups([
        includeAll ? [text('Direct Draw / Full Screen', 'Tell', 'DirectDrawManager'), { kind: 'submenu', label: 'Resolution', items: resolutionItems() }] : [],
        [
            toggle(settings, 'sound', 'Sound On'),
            toggle(settings, 'music', 'Play Music'),
            toggle(settings, 'playWind', 'Wind Noise'),
            toggle(settings, 'swapLeftRightSpeakers', 'Speaker Swap L/R'),
            { kind: 'submenu', label: 'Sound Effect Volume', items: volumeItems(settings, 'soundVolume') },
            { kind: 'submenu', label: 'Music Volume', items: volumeItems(settings, 'musicVolume') },
        ],
        [
            toggle(settings, 'edgeScroll', 'Edge Scroll in Fullscreen'),
            toggle(settings, 'autoDemo', 'Auto-Demo'),
            toggle(settings, 'tellTips', 'Tell Tips at Startup'),
            ...(includeAll ? [check('Pause - ~iShift-F9~.', false, 'Pause', '0')] : []),
        ],
        includeAll ? [check('Pass Server Diagnostic ', false, 'Tell', 'ChangeFirewall')] : [],
    ]);
}

// Help 펼침 메뉴의 항목 정의를 만든다. includeAll 이 참이면 외부 도움말 실행 항목(Technical Help)도 만든다. (2026-10-29 WB03-F)
// 메인 메뉴의 Help 목록은 구분선 없이 세 행이 붙어 있다(스크린샷 높이 51 = 3×17). 게임 안 맨 위 메뉴 막대의 About 하위에는 Technical Help 와 Version 사이에 구분선이 있다.
export function buildHelpItems({ includeAll = false } = {}) {
    return joinGroups([
        [text('General Help - ~iF1~.', 'F1Help', '0'), ...(includeAll ? [text('Technical Help', 'TechnicalHelp', '0')] : []), text('Version', 'Tell', 'About')],
    ]);
}

// 메인 메뉴 버튼의 menu 대상 이름('help'·'options')으로 항목 정의를 만든다. 모르는 이름이면 null 이다. (2026-10-29 WB03-F)
export function buildPopupItems(target, settings, options = {}) {
    if (target === 'options') return buildOptionsItems(settings, options);
    if (target === 'help') return buildHelpItems(options);
    return null;
}
