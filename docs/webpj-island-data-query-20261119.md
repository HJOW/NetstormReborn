# WB04-E824 World 없는 섬 자료 조회

기준선 `225553df`의 `RawIslandExists`·`RawIslandOwner`·`RawIslandBounds`를 World 없이 같은 3,204바이트 장기 기록을 읽는 함수로 이식하고 기존 `RawIslandRuntime`도 같은 조회를 사용하게 했다. signed count 관문과 물리 100기록 경계, 비정규 존재 DWORD, 부재 진단 뒤 현재 기록 재조회 및 경계 float 원문 비트를 유지한다. 기준선 이후 대응 C++ 파일 변경은 없다.

cpppj의 World 없는 조회 계약과 기존 원본 10.78 섬 런타임 1,009행을 두 Player 모드로 재생했다. 섬 검색과 Vortex 관련 검사를 포함해 집중 검사 14건이 통과했다. 현재 Player base getter와 원문 진단 위치 및 실제 Client/Native 소비자 연결은 후속이다. 게임 창은 실행하지 않았다.

검증: Node v26.13.0 전체 6,317건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. E819 Node v22 전체 검사 뒤 v26 전체 회귀 5/8회다.
