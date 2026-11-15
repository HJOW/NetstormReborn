# dotnetpj 0.559 — 현재 건물 공간과 장기 생성·해제 부모

2026-11-15, HJOW-Athlon. cpppj 제품1313의 RawBuildingSpace 현재 몸체와 RawSurfaceStorage의 공장/건물/Unpop·기본 종속 종단을 연결했다. M1은0/34이며 새 경로는 플레이할 수 없다.

## 구현

건물 공간이 같은 현재 섬/다리 TYPE DWORD와 전체 checking을 소비 위치에서 읽는다. NotifySurface는 TYPE/매립 관문 뒤 nibble을 먼저 고정하고 checking을 읽어 원문의 식/파일/줄로 진단한다. 진단의 정상 복귀·자기 함수 교체를 유지하고 높은 WORD 바이트를 변경하지 않는다. CD는171타입을 받고 NotifySurface에서 PATCH 슬롯/진단을 읽지 않는다. 기존 bool Debug 입력도 같은 진단 뷰를 사용한다.

Storage는 공장·건물 공간·Unpop과 독립 기본 공통 종속 종단을 각각 한 번 소유한다. 공장 getter는 표시/Unpop/World를 미리 만들지 않는다. 실제 현재 TYPE/HP·약화/사제/받침/가짜 표면·공통 깊이 주소와 장기 진단을 사용한다. 공간 해제는 같은 표시/건물/Hash/spot을 사용한다. 종속 종단은 같은 풀만 약하게 빌리고 선택 때 실제 현재 종단을 사용하며, 없으면 원문 가상 링크/부모 통지의 기본 공통 몸체를 쓴다. UI 명령은 실제 Gump Form을 찾았을 때만 현재 Client 명령 함수와 전체 DWORD를 소비한다.

World는 typed 공장/건물/Unpop을 직접 재사용하고 공장 Runtime과 현재 종단만 짧게 빌린다. 장기 진단을 World 콜백으로 덮지 않는다. 내용물/Gump/UI 번호도 같은 Storage 상자를 사용한다. 생성 중 다른 공장 대여가 거부되면 기존 대여는 유지하며 새 lease/표시/자기 종단만 반납한다. C#의 기존 CD 껍데기 진입은 PATCH188 전용 조립으로 들어가지 않도록 판본 조건을 고쳤다.

Native finder의 현재 내용물 TYPE, 장기 Native 파생 부모 통지/전송 결합, 최초 로딩 비용/자산 소유, PostPop/Pop·프레임·연결/표면과 실제 Client/Runtime/현재 Host 전환은 잔여다. 미설치 파생 통지를 공통 빈 성공으로 바꾸지 않는다. 이 단위는 NativeStorage 전체 조립 완료가 아니다.

## 검증

신규11검사는 현재 섬 TYPE의 조회 위치/전체 폭·음수 절삭, nibble/checking/원문 진단의 순서와 정상 복귀, 현재 다리/매립 관문, CD Notify 무조회와 실제 타입 개수, 공장 지연 구성/현재 HP, 세 World의 실행기 재사용/현재 진단 교체, 다른 공장 대여 거부 후 lease 반납, 기본 종단/현재 DWORD·풀 정체성, 실제 공장 종단 선택/반납 복귀, UI 명령 선택, 종료 World 수집을 확인했다.

초기 관련198건 중194통과/4실패는 CD188 껍데기가 PATCH용 조립으로 진입한 원인이었고 판본 조건을 교정했다. 다음 빌드의 테스트용 throw-only getter 형식 모호성1건은 명시 uint getter로 해결했다. 신규 포함246건 중244통과/2실패는 검사 입력의 HP 플래그 누락/풀 최소 크기 위반으로 바로잡았다. 기대 HP250/1001과 원본 fixture 기대값은 변경하지 않았다. 최종 **Core305/305·Client120/120** 통과다. 기존 buildingspace-x86 전체1,759행 중 PATCH 분기와 공장/종속 fixture 검사를 포함한다. CD 전체 Pop/Unpop fixture 재생 확장은 기존 풀/공장 판본 작업으로 남겼고 CD Notify의 무효과 계약은 직접 확인했다.

TRX는 `extracted/dotnet-storage-creation-0559-initial/`·`-final/`·`-fixed/`·`-client/`에 있다. Client 검사에서 Host Release까지 빌드했고 경고/오류는 없었다. 전체 회귀 기준은0.546을 유지한다.

L2는 cpp34 SHA 확인 재사용·dn일반31 신규 실행 종료0·수집 오류0·원본1,394파일 전후 동일이다. 0.558 대비 상태 변화는 없고 전체 파일 동일17·frame행만 차이14경로다. **전체 일치0/34·데모dn3 미연결**을 유지한다. 자료 `extracted/dotnet-l2-0559/`. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP933행(L1431·port502), cpppj 대응415단위·fixture312개다. 대응4행과 재대조한 건물 몸체의 기준만 반영했고 기존 fixture 해시/행 순서와 floor/reflected는 유지했다. R4 누락138단위/13,913줄·R6 누락382단위/45,090줄은 이전 그대로다. 다음은 나머지 장기 공간/전송과 실제 Native 조립을 이어간다.
