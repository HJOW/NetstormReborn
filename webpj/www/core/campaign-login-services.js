// 실제 미션 로그인에 필요한 현재 설정/시계/표시/전송 장치만 **빌리는** 서비스 묶음의 계약을 옮긴 모듈이다(cpppj `client/CampaignLoginServices.h`, 10.78 전용, 기준선 1f00a25a 뒤 20줄 반영).
// C++ 의 `struct CampaignLoginServices` 는 칸마다 비어 있을 수 있는 `std::function` 이고, 로그인 부모는 **필요한 선택 위치에서** 칸이 비었으면 명시 거부한다(빈 성공으로 대신하지 않음).
// 웹은 plain 객체로 같은 칸을 둔다: `createCampaignLoginServices(칸들)` 이 이름·형식을 검증해 만들고, `requireLoginService(services, 이름, 위치)` 가 선택 위치에서 비어 있으면 위치와 이름을 담아 거부한다.
// 칸(함수): configInt(키, 기본)·wallSeconds·gameSeconds·clock·refresh·playerUiBusy·playerNotice·observePacket·log·initializeRenderer·uiMode·battlePalette·closeProgress·startingProgress·creatingIslands·progress·initializeChat·selectReconnectFort·
//   attachCommander·queueAlliance·status·abortBattle·translate·frameWallSeconds·showTimerProgress·closeTimerProgress·clearStatus·reconnectUi·metaBattleDescription·metaBattleOptions.
// 칸(값): requestPhase(`{ value }` 상자, 005c85ac 결과 DWORD 를 빌림 — 독립 fixture 는 지역 fallback)·transport(`CampaignTransportServices` 객체, 없으면 기존 내부 미션 양끝)·reconnectFile(실제 Client 만 제공하는 저장 파일 장치)·frameLoginTimer(boolean, 기본 false).
// 사용: const services = createCampaignLoginServices({ configInt: (key, fallback) => …, wallSeconds: () => …, frameLoginTimer: true }); const attach = requireLoginService(services, 'attachCommander', 'State13 비서버');
// 기준선 뒤 cpppj 변경 판정(LEFT_JOBS 3.6절): 새 칸 17개(metaBattleDescription·metaBattleOptions·frameLoginTimer·gameSeconds·creatingIslands·progress·selectReconnectFort·queueAlliance·status·abortBattle·translate·frameWallSeconds·showTimerProgress·
//   closeTimerProgress·reconnectFile·clearStatus·reconnectUi)는 **로그인 부모가 부를 장치 이름**일 뿐 규칙이 아니므로 ② 로 모두 칸으로 둔다. 각 칸의 소비(State7/8/9·재접속·일반 71h/67h 설명)는 그 로그인 단위(RawCampaignSessionLogin 등)에서 옮긴다.
//   `reconnectFile` 은 새 파일 RawSavedStateWrite(대응표 밖)의 훅을 담으며 웹 저장소 적응은 재접속을 옮길 때 정한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-12 WB04-E326: 4-C CampaignLoginServices 이식)

// 비어 있을 수 있는 함수 칸 이름 전체이다. 이름 밖 칸은 오타로 보아 거부한다. (2026-11-12 WB04-E326)
export const CAMPAIGN_LOGIN_SERVICE_FUNCTIONS = Object.freeze([
    'metaBattleDescription', 'metaBattleOptions', 'configInt', 'wallSeconds', 'gameSeconds', 'clock', 'refresh', 'playerUiBusy', 'playerNotice',
    'observePacket', 'log', 'initializeRenderer', 'uiMode', 'battlePalette', 'closeProgress', 'startingProgress', 'creatingIslands', 'progress',
    'initializeChat', 'selectReconnectFort', 'attachCommander', 'queueAlliance', 'status', 'abortBattle', 'translate', 'frameWallSeconds',
    'showTimerProgress', 'closeTimerProgress', 'clearStatus', 'reconnectUi',
]);

// 함수가 아닌 값 칸 이름이다(requestPhase 상자·transport 서비스 객체·reconnectFile 장치 객체·frameLoginTimer 불리언). (2026-11-12 WB04-E326)
const VALUE_FIELDS = Object.freeze(['requestPhase', 'transport', 'reconnectFile', 'frameLoginTimer']);

// 로그인 서비스 묶음 전체 칸 이름이다. (2026-11-12 WB04-E326)
export const CAMPAIGN_LOGIN_SERVICE_FIELDS = Object.freeze([...CAMPAIGN_LOGIN_SERVICE_FUNCTIONS, ...VALUE_FIELDS]);

// 이름·형식을 검사해 서비스 묶음을 만든다. 주어지지 않은(undefined/null) 칸은 비어 있는 채로 두고, 알 수 없는 이름·잘못된 형식은 거부한다. frameLoginTimer 는 기본 false 이다. (2026-11-12 WB04-E326)
export function createCampaignLoginServices(fields = {}) {
    const services = { frameLoginTimer: false };
    // 지정한 모든 칸을 하나씩 검사해 복사한다.
    for (const [name, value] of Object.entries(fields)) {
        if (!CAMPAIGN_LOGIN_SERVICE_FIELDS.includes(name)) throw new TypeError(`알 수 없는 로그인 서비스 칸: ${name}`);
        if (value === undefined || value === null) continue;
        if (CAMPAIGN_LOGIN_SERVICE_FUNCTIONS.includes(name)) {
            if (typeof value !== 'function') throw new TypeError(`로그인 서비스 ${name} 은(는) 함수여야 합니다.`);
        } else if (name === 'frameLoginTimer') {
            if (typeof value !== 'boolean') throw new TypeError('로그인 서비스 frameLoginTimer 는 boolean 이어야 합니다.');
        } else if (name === 'requestPhase') {
            if (typeof value !== 'object' || typeof value.value !== 'number') throw new TypeError('로그인 서비스 requestPhase 는 { value } 상자여야 합니다.');
        } else if (typeof value !== 'object') {
            throw new TypeError(`로그인 서비스 ${name} 은(는) 객체여야 합니다.`);
        }
        services[name] = value;
    }
    return services;
}

// 선택 위치에서 필요한 함수 칸을 꺼낸다. 비어 있으면 어느 위치에서 어떤 칸이 필요했는지 담아 명시 거부한다(빈 성공으로 대신하지 않음). (2026-11-12 WB04-E326)
export function requireLoginService(services, name, where) {
    if (!CAMPAIGN_LOGIN_SERVICE_FUNCTIONS.includes(name)) throw new TypeError(`함수 칸이 아닌 로그인 서비스 요청: ${name}`);
    const service = services?.[name];
    if (typeof service !== 'function') throw new Error(`로그인 서비스 ${name} 누락 (${where})`);
    return service;
}
