# 지역 로그인 이미지의 DataManager 진단 (단위668)

원본 `004bf3e0`의 다섯 섹션 복제 뒤, 저장 이미지 생성 전에 `0044cf20("Stripped Image:")`를 연결했다. 단위667의 이미지 내용/할당 복원에서 따로 남긴 실제 진단 호출이다.

`DumpDataManager`는 같은276BYTE manager의 가상 파일명 조회→level29 파일 로그→가상 이름표 조회→SNART 이름별 현재 섹션 검색→각 end의 DWORD 합계→signed `%04d` 합계 로그 순서를 사용한다. 기존 `SelectSnartName`과 `FindDataManagerSection`을 재사용한다. 누락 섹션은 실제 current0과 오류 로그를 남긴다. 비교/로그 callback 뒤 바뀐 현재 노드와 앞으로 읽을 이름표를 다시 읽으며, 예외 앞의 부분 상태를 되돌리지 않는다. data/pos/alloc이나 이미지 payload는 읽지 않는다.

`CampaignTemplateStorage`는 같은 Client의 진단 callback을 보유하고 지역 NONAME의 덤프에 직접 빌려준다. `UberGump`는 주 Storage 생성 직후 `Client::WriteDiagnosticLog`에 연결한다. 별도 로그 상태나 `std::clog`를 만들지 않는다. 기존 실제 경로가 필터→서식/공유1024BYTE→stdout 초기화/개행 정리→Chat→stderr 복제→현재 FILE→flush를 수행한다. 필터되더라도 DataManager 검색은 수행하며 필터 판정은 실제 로그 부모에 맡긴다. 일반 파일 읽기/MapProbe에 새 덤프를 추가하지 않는다.

독립 무화면 Session 검사의 logger는 명시 비출력 장치다. 출력만 관측하지 않으며 덤프의 실제 검색/합계 본체는 실행한다. 별도 실제 Client 검사에서는 level28일 때 공유 버퍼/FILE0 보존, level29일 때 들여쓰기/두 원문 로그/실제 CRT 파일/NULL Chat 경계를 확인한다. 게임 창·음향 장치·실제 Chat UI는 실행하지 않는다.

## 검증

Release 게임/미션집중/세계집중의 빌드 명령1개가 경고/오류0으로 끝났다. 미션87등록/세계261등록 중 관련20개가 실패0으로 통과했다(새5·기존15, 총8회/20함수 호출). 최초 검사 실패는 없다. primitive4·실제 Client1·이미지2·로그인4·비동기2·부모2·작업본 source3·경로2다. 등록 전체나 주 검사 전체 통과로 세지 않는다. 상세는 [보고서](../../cpppj/datamanager-dump-20261101.json)에 있다.

실제 Client 검사는 필터/들여쓰기/같은 공유 버퍼/실제 임시 FILE/NULL Chat 접합을 확인했다. Chat→FILE→flush의 세부 호출 순서 자체는 기존 RawTraceLog 근거와 구별한다. 원문 함수는 보호PE `0044cf20..0044cff6`과 현행 정밀 디컴파일로 정적 대조했다. 새 Native 실행/fixture·기존fixture 재생·신규MP 검사0이다. 보호 AGENTS/양PE/TAFF의 관련4개는667과 불변이며 전체2632/fixture773 재감사는 하지 않았다.

제품 SHA-256은 `860044565bcc7fdf57e40d6655a41f67e297da515499ef9f7e3dc76a4fe01fcc`다. 게임 실행·전체34·새ZIP0이다. 최신 전체34/ZIP은645의644제품이며 이번 제품의 GUI 근거로 확대하지 않는다. 사제 포획/희생·특정 적 공격·승리를 재현하지 않았다.

원시 정적 근거는 `extracted/cpp-datamanager-dump-668-static.txt`다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon /2026-10-23**이며 이번 디컴파일 산출물 변경은 없다.10.72의 추가 복원/실행 대조 범위는 아니다. 단위667의 실제 GUI 연쇄·전체 Template/파일 ABI 등 현행 잔여를 이 진단 연결만으로 완료 처리하지 않는다.
