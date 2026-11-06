# 991 — 자기 일반 Fort 수신과 CoreData 최소 병합

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78 원문 `004b3d30`, `004beca0`, `004be8e0`, `004beee0`, `004bf190`, `00490250`을 대조하고 client2a의 자기 ordinary Fort 경계를 실제 Session에 연결했다. 대응10.72 함수는 `004307f0`, `00429200`, `00428fc0`, `004293d0`, `00429670`, `00407400`이다.

외곽 수신기는 현재 packet owner==myPlayer와 !missionPost에서만 자기 Fort를 읽는다. savedGame 두 분기는 원문에서 같은 사슬로 합쳐진다. 같은 고정 Template 헤더를 Reload한 뒤 기존 원시 Badges reader, 실제 Player 생성, CoreData 인자1, 실제 Technology/Contents/변환표 정리, 기존 Deck reader를 순서대로 호출한다. Player 생성 번호는 배지 적재 뒤 현재 battleOwner를 다시 읽는다. 수신 이미지 병합·전투섬 생성은 기존 server/hidden 관문에 남긴다.

CoreData 인자1은10BYTE 적재 뒤 현재 lastKnown과 랭크 BYTE+4, 승리 WORD+0, 패배 WORD+2의 unsigned 최소값을 순서대로 저장한다. 비교·선택·저장마다 실제 현재 포인터를 다시 조회하며 Chat 갱신·seed lowWORD·무결성 꼬리는 기존 위치를 유지한다. 기본 인자0 호출과 missionPost/Editor의 자연 반환은 유지한다. 별도 성공 상태나 loaded/Ready를 쓰지 않는다.

첫 Release 빌드는 DWORD 전용 Read/Put에 가변 폭을 전달한 컴파일 오류6개로 종료1이었다. BYTE/WORD 직접 접근으로 교정한 최종 빌드는 종료0·경고/오류0이다. 첫/최종 로그는 `extracted/cpp-resume-991/build.log`, `build-final.log`이며 제품 보고서는 `cpppj/local-fort-20261107.json`이다. 제품11.01은11,625,472바이트, SHA-256은 `7e1d3643741a08420e8aadacee83e60fafa33c9b5c3ef6b6991d27a5da6991bf`다.

신규 MP/단위/게임/통신 검사는 사용자 지시대로0이며 새 fixture도 없다. 보호 AGENTS.md·원본/CD·fixture2800파일은 모두 불변이다. 컴파일을 LAN 동작 검증으로 세지 않는다. 일반 State6·7/Commander/roster/LAN UI, 전체 파생 writer/decoder, 메타/저장/재접속/Outpost/checking8은 남는다.
