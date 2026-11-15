# dotnetpj 0.554 — 현재 PostPop 공통부와 마나 등록

2026-11-15, HJOW-Athlon. cpppj 제품1313을 기준으로 `RawSquidPostPopParent`·`SquidPostPopPatch`·`RawManaRegistration` 계열을 옮겼다. R4-1의 장기 실행기 선행 의존성이다. 공통 부모와 PATCH 몸체가 완료됐으며 `SquidPostPop.cpp` 외곽 가상 파생 분배 전체와 NativeStorage 장기 보유는 아직 남아 있다. M1은0/34다.

## 구현

억제 관문은 raw/TYPE 조회보다 먼저 실행하고 깊이 DWORD를 한 번 감소시킨다. Graph 비활성 단축, 영역 통지 뒤 현재 Graph 활성 재조회,80b 단언 정상 복귀, 현재 TYPE와 저장한 genus의 구분, 비용→공급원→owner 공장→전체 공장→AI→로컬/전체 통계→선택 순서를 보존한다. 실패 전의 효과와 깊이를 롤백하지 않는다. 처음 로딩한 비용 snapshot과 현재 수정 가능한 TYPE를 구분한다.

마나 등록은 signed 개수/용량, 첫 원소를 다음 count 조회보다 먼저 검사하는 순서, PATCH의 최초 count와 CD의 반복 count, 중복 진단에서 바뀐 현재 목록/생산 DWORD를 구분한다. 저장한 중복 판단은 진단 뒤 다시 계산하지 않는다. 가득 찬 음수 용량에서도 포인터에 접근하지 않고 생산 알림은 한 번 감김 증가한다.

World와 개발 Host의 실제 TYPE를 현재 공통부에 연결했다. World 진단은 저장소의 현재 checking 재결합도 따른다. 같은 공급원/공장 목록 별칭과 기본/현재 장치 함수 사본, 실제 외곽 owner pin 연결 API를 마련했다. 외곽 가상 분배·완전한 pin 수명·장기 Storage 기본 장치 조립은 후속 작업이다. 미구현 자산 가상 함수를 공통 성공으로 대체하지 않는다.

## 검증

신규21검사는 현재 부모의 분기/재진입·진단·부분 효과와 실제 장부를 확인한다. 마나 등록의 원본 x86 CPU1,084행(PATCH542·CD542)을 전체 배열/stale 꼬리·현재 DWORD·진단 순서로 대조했다. 기존 PostPop·Carrier·사제 등의 원본 fixture도 관련 검사에서 유지된다. CD 마나 목록 연산 대조이며 CD 전체 월드가 완성됐다는 뜻은 아니다.

최초 관련68검사에서6건은 기존 사전 검사/실행 혼용과 비용 snapshot 기대 때문에 실패했다. 명시 사전 검사는 Validate로 검사하고 현재 TYPE/최초 비용 입력을 구분했다. 후속224검사에서 Carrier의 두 번째 실제 실행이 AI 실패 전에 남기는 비용이 누적돼 기대133/실제256으로1건 실패했다. 실제 부분 효과 계약에 맞춰 합성 기대를 교정했다. 원본 fixture는 수정하지 않았다. 최종 관련 **344/344** 통과, Host Release **경고0·오류0**다. TRX는 `extracted/dotnet-postpop-current-0554-initial/`·`-native/`·`-final/`·`-final2/`에 있다. 전체 회귀는0.546 기준과 환경 누락 실패 기록을 유지한다.

L2는 동일 SHA의 cpp34개 실제 기준선을 재사용하고 dn일반31개를 새로 실행했다. 모두 종료0·수집 오류0이고 보호 원본1,394파일은 전후 동일하다. 0.553 대비 상태 변화는 없고 파일 전체 동일14경로·frame행만 다른17경로다. 전체 일치0/34, 데모dn3 미연결이다. 자료는 `extracted/dotnet-l2-0554/`다. 사용자 제외 플레이/MP 실행은 수행하지 않았다.

## 인수인계

PORT_MAP911행(L1 430·port481), 연결 fixture312개, cpppj 대응415단위다. 신규 대응7행과 CPU fixture1개를 추가했으며 기존 행/해시/순서와 floor/reflected를 보존했다. 이전 `RawPostPop.cs`의 큰 cpp/header 기준은 외곽 미완 부분이 있어 올리지 않았다. R4 누락138단위/13,913줄, R6 누락382단위/45,090줄이다.

다음은 NativeStorage 장기 공장·공간·World 수명과 실행기 보유, 현재 PostPop 가상 파생 분배/전체 호출 pin, 실제 HostSession 별도 풀 전환과 피해/해방·삭제/배치 상위 결선이다. R4-2 적재 장부/그래프/owner 결선과 후속 우선순위를 이어간다.
