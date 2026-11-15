# dotnetpj 0.561 — 장기 공간과 현재 자산 소유자

2026-11-15, HJOW-Athlon. cpppj 제품1313의 RawAssetOwner 전체 분배와 RawSurfaceStorage의 typed 장기 PostPop/Pop·프레임·연결/표면을 연결했다. M1은0/34이며 새 경로는 플레이할 수 없다. 현재 개발 HostSession의 별도 풀은 아직 전환하지 않았다.

## 구현

자산 소유자는 호출 순간 실제 VT/extra를 읽는다. 두 공장 VT는 매립/추상 비트가 없을 때 같은 생산 DWORD를 먼저 증가시킨 뒤 기존 표면/공통 부모를 호출한다. 부모가 중단해도 증가를 되돌리지 않는다. 젬의 고정 중립 VT는 해당 판본의 소유자 BYTE만0으로 쓰고 부모를 호출하지 않는다. 전체 owner DWORD 인자·감김과 높은 payload를 보존한다. World와 장기 표면 생성 효과에 같은 분배기를 연결했다. 생산 카운터의 속성/ref/새 box는 동일한 backing DWORD다.

Storage는 다섯 공간 실행기를 처음 한 번 소유한다. 같은 물리/현재 TYPE/깊이·장부·진단/표시/공장/전송과 실제 연결/표면 접두를 사용하며 getter에서 World/lease/게임 값을 만들거나 초기화하지 않는다. 표면은 복사 지도를 만들지 않고 같은 Hash0의 현재 WORD를 조회한다. 원문 TypeWas 진단 문맥도 GameAux의 최초 BSS0/현재 DWORD에 연결했고 일반 World 준비가 지우지 않는다.

World는 같은 실행기를 직접 빌린다. 얇은 독립 경로만 종전 지역 실행기를 사용한다. 장기 Pop/PostPop 접두/진단을 World 캡처로 덮지 않으며 기존 짧은 파생/Runtime 반납 계약을 유지한다. 현재 실제 World의 changed 관찰은 Storage의 기존 표시 대여를 통해서만 전달한다.

최초 자산/로딩 비용 소유, Native 기본 PostPop 장치/전용 자원 파생 설치와 Client Registry/현재 finder/파생 내용물, 실제 Runtime/Host 전환 및 상위 피해·해방/삭제·배치 조립은 잔여다. 여기서 장기 조립을 닫은 것은 이식된 typed 공통 공간이며 NativeStorage 전체/새 경로 플레이 완료로 세지 않는다.

## 검증

신규7검사 중 첫 검사가 원본 생산/중립 가상 몸체 **128행(PATCH64/CD64)**을 직접 대조한다. CD 관측은 실제36BYTE·PATCH50BYTE 전체 payload와 종료 원인/부모 이벤트/생산 값을 비교했다. 별도 검사에서 현재 VT/extra 재조회와 부모 중단 뒤 접두, 같은 속성/ref/box 감김, 실제 PATCH 생성자5종(두 공장/젬/받침/종유석)의 World 분배·생산 장부, 다섯 getter의 재사용/불변, 준비/일반 World 재대여와 실제 Pop·프레임 변경, 진단 문맥 보존, 종료 World 수집을 확인했다. CD 전체 World 조립은 이 단위에서 확장하지 않았다.

첫 빌드의 using/내부 해시 어댑터2진단을 같은 물리를 읽는 Storage 어댑터로 교정한 뒤 기존 관련 **469/469** 통과다. 신규 검사 빌드의 ConfigureInstallModes 인자 누락1건을 교정했다. 이후476건 중474통과/2실패는 검사 입력의 CD payload 폭44 가정/프레임 플래그0x10 오류였으며 실제 원본36BYTE와 API의0으로 바로잡았다. 원본 기대값은 변경하지 않았다. 계약 포함 최종 **Core482/482·Client120/120** 통과다.

TRX는 `extracted/dotnet-storage-space-0561-initial2/`·`-new2/`·`-final/`·`-client/`다. Client 검사에서 Host Release까지 빌드했고 경고/오류는 없었다. 전체 회귀 기준은0.546을 유지한다.

L2는 cpp34 SHA 확인 재사용·dn일반31 신규 실행 종료0·수집 오류0·원본1,394파일 전후 동일이다. 0.560 대비 상태 변화는 없고 전체 파일 동일17·frame행만 차이14경로다. **전체 일치0/34·데모dn3 미연결**이다. 자료 `extracted/dotnet-l2-0561/`. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP942행(L1433·port509), cpppj 대응416단위·fixture313개다. 새 fixture는 `asset-owner-current-local-x86.tsv`128행이다. 대응5행을 추가했고 기존 행/fixture 해시/순서와 floor/reflected는 유지했다. RawAssetOwner40줄을 닫아 R4 누락137단위/13,873줄(B28단위/2,055줄), R6 누락382단위/45,090줄이다. 다음은 최초 자산/비용과 실제 Native 조립을 이어간다.
