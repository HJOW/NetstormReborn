# 기술/객체 공용 내용물 writer (WB04-E629)

기준선 `225553df`의 RawFortContentSave.cpp/h와 Session2566..2584·
5847..5851·5899..5901를 이식했다. 캠페인 공용 snapshot의 Technology와
객체 내용물이 같은 writer를 쓰므로 이 한 단위의 편집기 전용 제외를
교정했다. 해당 C++ 파일과 Session의 기준선 뒤 변경은0이다.

## 구현

각 목록 진입 당시 data+offset의 개수 BYTE 주소와 독립 finder/current를
보관한다. 현재 stream data가 바뀌어도 원래 개수 주소를 증가시킨다.
robust 뒤 후보와 metadata 재조회, QA BYTE/WORD의 독립 분기, 고정 부모의
현재 목록 플래그 fallback, child 뒤 부모 Next와 깊이 우선 순서를 보존했다.
개수 BYTE 감김·무진단 헤더 쓰기·인라인 용량 진단 뒤 현재 필드와 부분
효과를 유지한다. 명시 프레임과 active 부모 집합으로 원문 순서를 지키며
순환은 C++와 같은 손상 경계로 거부한다.

실제 Session의 같은 Form6/ContentForm Robust·현재 TYPE·독립 검색기와
생성 당시 Template stream을 연결했다. 내부 WriteFortTechnology는 부모가
이미 만든 노드를 다시 만들지 않는다. 공개 SnapshotFortTechnology는
설치/슬롯 관문 뒤 현재 기술 부모를 먼저 캡처하고 한 번만 새 섹션을 만든다.
Native 장기 가상 대여는 설치 단계에 남겨두고 현재 typed 경로를 바꾸지 않았다.

## 검증

원본706행의 두 버퍼·stream·현재 Form/metadata·pool 상한·trace와 정상/
부분 중단을 전수 대조했다. 기존 자료를 수정하거나 새 관측을 만들지 않았다.
추가 계약은 빈 목록의 미선택 장치/무진단 헤더·선택된 누락·순환·256항목
개수 BYTE 감김을 검사한다. 실제 Session 검사에서는 같은 Form6을 찾아
종류71을 저장하고, State로 current를 바꿔도 원래 Technology 노드에 쓰며,
Pool/난수 불변·재생성으로 무효화한 옛 stream과 반납 뒤 접근 거부를 확인했다.
설치 전 공개 snapshot은 거부하며 실제 미션 Load를 성공으로 만들지 않았다.

최초3검사는2통과/1실패였다. 추가 감김 입력에서 고정 부모 kind를 빠뜨려
ParentType의 외부 장치가 실패했다. 부모 kind71을 명시한 뒤 제품 코드를
바꾸지 않고 신규3건 모두 통과했다. manifest에는 원본 fixture 한 항목만
추가해809항목이며 실제 Session 결선을 직접 fixture 소비자로 등록하지 않는다.
앞선 두 snapshot과 대응표/manifest를 포함한 집중20건이 통과했다.
대응표1,160행/오류0, `git diff --check` 오류0이다.
전체 Node v22.23.3·v26.13.0은 각각5,982건 통과, 실패·취소·건너뜀0이다.
로그는 `extracted/webpj-e629-node22.log`·`webpj-e629-node26.log`에 보존했다.
v24는 사용자 지시대로 생략하고 호환을 가정한다.

## 다음

객체/영역·Money/Mission·CoreData 등 공용 snapshot의 나머지 자식 직렬화를
도달 지점에서 연결한 뒤 외곽 Session callback/공개 수명과 Load를 계속한다.
다음 선행은 `RawFortObjectSave`의 단일 객체/청크 writer와 실제 Session의
ObjectWriter·청크 검색기다(원본 객체3,294행·청크672행).
앱 session0·실제 플레이 불가와 기존 자연 완주/포획·희생·공중3종 새 전용
생성/생산/행동 검사 제외를 유지했다. 게임 창은 실행하지 않았다.
