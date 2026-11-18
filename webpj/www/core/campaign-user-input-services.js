// 현재 Client와 GameWorld에 사제·운반체 선택 입력의 외부 화면 장치를 연결한다.
// 사용: bindCampaignUserInputServices(client, world, selectCursor, tell). 사용한 World가 Client보다 먼저 반납되어야 한다.

// 도달한 선택 효과만 실제 함수인지 확인한다.
function need(hook, name) {
    if (typeof hook !== 'function') throw new Error(`사용자 입력의 실제 ${name} 미연결`);
    return hook;
}

// 같은 Client와 World를 빌려 기존 사용자 입력·강조·운반체를 한 번 설치한다.
export function bindCampaignUserInputServices(client, world, selectCursor, tell) {
    const selection = {
        // 현재 Client의 일시 정지 상태를 읽는다.
        paused: () => client.paused() ? 1 : 0,
        // 선택된 SID의 현재 진단을 Client에 맡긴다.
        dump: (sid) => client.traceObject(sid),
        // 잘못된 선택 SID의 원본 진단이다.
        assertion: () => { throw new Error('원본 Userinput.cpp selectedSid 진단'); },
        // 운반 미리보기가 Form을 잘못 선택한 진단이다.
        previewAssertion: () => { throw new Error('원본 Carrier.cpp !s.isForm 진단'); },
    };
    const highlight = { selectCursor };
    const carrier = {
        selectCursor,
        // 운반 입력의 원본 진단 줄을 표시한다.
        assertion: (line) => { throw new Error(`원본 Carrier.cpp 사용자입력 진단 ${line >>> 0}`); },
        // 로컬 사용자에게 조기 희생 안내를 표시한다.
        earlyLocal: () => need(tell, '조기 희생 Tell')('EarlySacrifice'),
        // 다른 소유자에게 같은 Session의 Tell 요청을 전달한다.
        earlyRemote: (owner) => world.session().sendPlayerTell(owner, 'EarlySacrifice'),
        // 선택된 희생 메뉴를 현재 Session에 요청한다.
        showSacrifice: (target) => {
            const session = world.session();
            if (typeof session.showSacrificeMenu !== 'function') throw new Error('희생의 실제 메뉴 부모 미연결');
            session.showSacrificeMenu(target, tell);
        },
        // 이동 경로 진단점을 같은 Client 화면에 전달한다.
        marker: (point, color) => client.drawNativePathMarker(point.x, point.y, color),
        // 현재 커서를 즉시 지우는 Client 표시 장치를 호출한다.
        refreshCursor: () => client.presentNativeCursorClear(),
    };
    world.bindUserInput(selection, highlight, carrier);
}
