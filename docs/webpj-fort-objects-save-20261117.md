# 전체 Objects 부모와 영역/Chaff 순회 (WB04-E631)

기준선 `225553df`의 RawFortRegionSave.cpp/h·RawFortObjectsSave.cpp/h와
Session5869..5896을 같은 전체 Objects 저장 단위로 연결했다. 캠페인 공용
snapshot도 두 부모를 쓰므로 편집기 전용 제외를 교정했다. 두 C++ 단위와
Session의 기준선 뒤 변경은0이다. 편집기 UI/파일 저장을 추가하지 않는다.

## RawFortRegionSave

최초 height를 읽은 뒤 공유 scope1/2를 쓰고 비양수면 반환한다. 행 시작은
직전 셀 끝에서 읽은 width를 사용하며 새 getter를 추가하지 않는다. 셀
영역의 signed BYTE·고정 owner/region·현재 크기 변경과 signed INC 감김,
Chaff에서도 생략하지 않는 실제 셀 조회, 예외 뒤 scope 비복구를 보존했다.
원본930행의 현재 크기/scope·외부 trace와 정상/부분 중단을 전수 대조했다.

## RawFortObjectsSave

현재 manager/owner 진단 뒤 공유 version에2를 쓴다. 20개 영역별로 없는
Terr 생성→새 Temp→현재 version 하위BYTE→전체 영역 저장→현재 manager의
Temp 선택→data/end가 모두 비영일 때 rename을 수행하고, 마지막 현재
fort가 비영일 때 Chaff를 만든다. 진단 전에 캡처한 version BYTE와 진단
뒤 현재 pos/data·출력 뒤 pos/end, 조건부 rename과 부분 효과를 유지한다.
최초357입력×두 정밀도=714행의 현재7전역·21개 stream/각64BYTE 출력·
trace·부분 중단48을 전수 대조했다. 기존 관측은 수정/재수집하지 않았다.

## 실제 Session 결선과 검증

각 Find/Create/Select 반환을 같은 manager/노드/생성 수명 registry로
보관하며 current나 논리 길이로 참조를 대체하지 않는다. 같은 실제 청크
지도·IslandList.Matches·전체 청크 writer·공유 scope/version을 연결했다.
공개 SnapshotFortObjects의 loaded 관문과 내부 설치 중 writer를 구별한다.

신규5건은 최초 실행부터 모두 통과했다. 두 원본 재생 외에 빈 지도/
미선택 Chaff·선택된 누락·셀 조회의 scope/크기 변경을 검사했다. 실제
Session에서는 같은 청크 지도 첫2셀과 섬 목록을 입력으로 선택 영역
marker/count, 다른19영역의 version, Temp 소멸/이름 교체·조건부 Chaff,
Pool/난수 불변·현재 scope/version 유지와 설치 전/반납 관문을 확인했다.
이 입력은 미션 Load·자연 플레이 성공을 뜻하지 않는다. 다른 snapshot
자식과 외곽 Session 수명은 계속 잔여다. 원본 두 fixture를 manifest에
추가하며 실제 결선을 직접 fixture 소비자로 등록하지 않는다.
관련 TypeNames/Deck·내용물·단일 객체/청크·snapshot 부모와 대응표/manifest를
합친 집중29건도 통과했다. 두 C++ 단위는 헤더 포함140줄이며 대상
1,013단위/port854·제외147, manifest813항목(original-x86 774)이다.
전체 Node v22.23.3·v26.13.0 각각5,991건 통과, 실패·취소·건너뜀0이다.
v24는 사용자 지시에 따라 생략하고 호환을 가정했다. 대응표 오류0·
기준선 뒤 대상 C++ 변경0과 git diff --check도 확인했다.

다음은 Territory·Money/Mission·CoreData 등의 snapshot 자식과 외곽
callback/공개·Load다. 앱 session0·실제 플레이 불가 및 기존 자연 완주/
포획·희생·공중3종 새 전용 생성/생산/행동 검사 제외를 유지했다.
게임 창은 실행하지 않았다.
