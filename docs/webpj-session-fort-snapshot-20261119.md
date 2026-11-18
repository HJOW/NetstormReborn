# WB04-E829 Fort 모드 Session snapshot과 재적재

기준선 `225553df`의 `RawCampaignSession::SnapshotFort` 및 `SnapshotFortBody`와 기존 자식 저장기를 사용했다. 기준선 이후 해당 C++ 구현의 변경은 없다. `TheWarBegins` Fort 자료를 같은 실제 Session에 적재하기 전 Fort/Battle 및 Player·다리 모드 전역을 명시했다. 게임의 편집기 진입이나 준비된 Editor 수명을 실행한 검사는 아니다.

현재 기술과 28개 배지 장부를 제공해 Fort snapshot의 `TypeNames`, `Technology`, `Badges`, `Territory`, `Money`, 지형 객체 섹션을 검사했다. 결과를 별도 Session에 재적재해 객체 1,875개의 타입·소유자·프레임·좌표 다중집합을 비교했다. SID는 재적재 순서에 따라 달라 비교하지 않았다. Mission은 없는 경우를 사용했다.

최초에는 `configureInstallModes`를 Session 구성 뒤 호출했으나 이미 Runtime이 결합돼 해당 API의 선행 관문에서 거부됐다. 이 관문을 우회하거나 코드를 바꾸지 않고, 검사에서 실제 소유 전역을 명시했다. 따라서 기존 Editor 소유자의 설치·종료·호출 순서와 앱 전체 수명은 검증하지 않았다. 별도 게임 창은 실행하지 않았다.

검증: 실제 Fort 모드 snapshot·재적재 집중 검사 및 Node v26.13.0 전체 6,322건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. E827의 Node v22 전체 검사 뒤 v26 회귀 2/8회다.
