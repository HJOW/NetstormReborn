// 실제 Client의 HTML 장기전역/원시큐/벽시각·Factory/Metadata 선택장치를 빌리는 값 구조체다(cpppj client/RawHtmlWindowContext.h, 기준선1f00a25a).

// Factory의 현재 파서전역 포인터와 파서callbacks/Config치환·fallback을 빌린다. C++ 값 구조체처럼 callbacks표는 복사한다.
export function createRawHtmlFactoryServices(fields = {}) {
    return { globals: null, substitute: null, fallback: null, ...fields, html: { ...fields.html } };
}
// Metadata의 명령/문자열고리·맵/기술/파일목록·입력/전송 계약이다. 미연결 장치를 소비자가 선택한 위치에서 거부한다.
export function createRawHtmlMetadataServices(fields = {}) {
    return { strings: null, maps: null, fileSpec: null, currentMap: null, fallbackMap: null, tech: null, themeTech: null, namedList: null, fileList: null, fileMenus: null, inlineList: null, input: null, send: null, ...fields, commands: { ...fields.commands } };
}
// owner포인터는 공유하고 두 서비스 값 구조체는 복사한다. 실제 화면DWORD/read·queue/link·wallSeconds를 임의 PE0/빈성공으로 채우지 않는다.
// 사용: createRawHtmlWindowContext({globals,wallSeconds,queue,link,read,metadata,factory}). (2026-11-14 WB04-E456)
export function createRawHtmlWindowContext(fields = {}) {
    return { globals: null, wallSeconds: null, queue: null, link: null, read: null, ...fields, metadata: createRawHtmlMetadataServices(fields.metadata), factory: createRawHtmlFactoryServices(fields.factory) };
}
