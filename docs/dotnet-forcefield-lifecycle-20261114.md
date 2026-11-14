# dotnetpj 0.513 — 보호막 최초 예약·Regular 수명

2026-11-14, HJOW-Athlon. cpppj `o/RawForcefieldLifecycle.h/.cpp`, `o/SquidPostPop.cpp`의 현재 계약을 이식했다(소스 대조 기준 `5388f45e29f083230a661655907bb07e597918e8`, cpp 소스는 이전7313ba1c6과 동일). 일반 Pop/프레임/직렬화 결선의 선행 단위는 [0.512](dotnet-pop-current-20261114.md)다. 원본·디컴파일 산출물은 변경하지 않았다.

## 실행 순서

`RawForcefieldPostPop.Prefix`는 flags bit0에서 사건1/0.08과 사건2/0.01의 두 new(0x28) 확보를 각각 시도한다. 첫 실패여도 두 번째를 실행한다. PATCH에서는 미선택 효과나 부모 raw를 선행 검사하지 않으며, 선택된 확보/생성 장치만 도달 위치에서 요구한다. 명시 Validate와 CD 경로의 기존 사전 계약은 구별한다. 장치 객체는 생성 시 값 복사하고 캡처한 장기 owner는 공유한다.

`RawPostPop.SetForcefieldPostPop`은 같은 풀의 접두만 등록한다. PATCH 가상 분배는 현재 VT0x507770으로 선택하고 TYPE BYTE를 추가 관문으로 쓰지 않는다. 접두 뒤 공통 장부/깊이를 정확히 한 번 실행한다. PATCH 공통 부모의 억제 관문은 raw/TYPE 선행 검사보다 먼저 반환한다. 실제 분배기에서 지원하지 않는 가상 함수를 공통 성공으로 넘기는 경로도 제거했다. CD 공통 부모의 기존 사전 검사는 유지했다.

`RawForcefieldRegular` 사건1은 실제 Advance(sid,1,0) 뒤 원본0.08f를 반환한다. 사건2는 현재 위치에서 사제를 찾으면0.01f, 없으면 실제 가상 Destroy(sid,0) 뒤-1을 반환한다. count/payload는 읽지 않는다. 기타 PATCH 사건은 현재 checking이 비영이면 원문 `0`, `.\Dais.cpp`, 0x479 진단을 호출하고 정상 복귀하면0이다. 효과가 마지막 처리기 소유자를 놓거나 부모/Regular를 삭제한 뒤에는 그 객체를 다시 읽거나 쓰지 않는다.

사제 조회는 두 가지 명시 입력 계약을 유지한다. 실제 typed finder가 연결됐으면 현재 사제 TYPE DWORD를 먼저 읽은 뒤 raw float x/y를 그대로 넘긴다. 독립 원본 재생은 좌표를0방향 절삭해 같은 일반 finder/해시/TYPE 표에서 첫 일치를 찾는다. 소유자·HP·void/dead 상태를 별도 필터로 추가하지 않고 finder의 매장/발자국 조건을 그대로 쓴다. 사제 TYPE를 BYTE로 줄이지 않는다.

`MakeForcefieldPostPopProcessHooks`는 같은 실제 Host의 AddRegular/Form/Kernel에 연결한다. `MakeForcefieldFrameHooks`는 같은 풀의 실제 RawFrameAdvance만 연결한다. 이 단위는 부품과 일반 분배 등록 API를 완성했으며 **실제 월드/호스트의 전체 보호막 예약·Regular 조립은 후속 R4 작업**이다. 실제 캠페인에서 보호막이 완성되었다고 세지 않는다.

## 대조 및 검증

공유 `forcefieldpostpop-x86.tsv`3624행 중 PATCH1208입력을 재생한다. 나머지 두 판본2416행은 동일 파일로 보존하고 이번 PATCH 원본 대조 건수에 더하지 않는다. 기대한 사건 순서·정확한 float 반환 비트·전체50BYTE를 직접 비교하며 기대값을 재계산하거나 덮어쓰지 않는다. LF 정규화 SHA256은 `001ee50b7a9f42e6e834a3c0bf271ccf7ea9af33ea7682a7ef50aa2416189355`다. 공유fixture250개, PORT_MAP736행(L1 346+port390)이다. manifest의 기존249개 해시는 유지했다.

현재 계약 검사에는 첫 확보 실패 후 두 번째 성공, 생성 콜백의 raw 변경/소유자 반납, 현재 TYPE DWORD0x1009e, typed float 전달, finder 부작용 뒤 미연결 destroy 실패, 현재 checking 진단, CD의 기존 구성/객체 사전 검사, PATCH 잘못된TYPE/free 상태에서 VT 분배와 억제된 공통 깊이, 다른 풀 거부를 포함한다.

제품에 적용하기 전 `extracted/dotnet-forcefield-lifecycle-0513/prototype/`에서 같은 소스/공통 부모 사본을 분리 컴파일했다. 이 검사 어셈블리는 friend 접근을 위해 Netstorm.Core.Tests라는 이름을 쓰지만 제품/정식 검사 출력 디렉터리에 쓰지 않는다. `dotnet test extracted/dotnet-forcefield-lifecycle-0513/prototype/Prototype.csproj -c Release`로 실행할 수 있고 게임 창/네트워크를 열지 않는다. 정식 검사는 Netstorm.Core.Tests의 해당 클래스에 통합했다.

첫 확보 실패를 조기 반환하도록 바꾼 변이는 같은 소스의 분리 검사6건 중2건(golden·현재 예약 순서)이 실패해 검출했다. 모듈 SHA256 `74342b373c4ca3ecd9554620d8b1a50a54e505d0f644de77dce44b119607ee8b`로 원복했다. 제품 소스와 바이트 동일성을 확인한 사본에서만 변이했고 제품 빌드는 바꾸지 않았다.

제품 비증분 Release 빌드0경고/0오류. 첫 관련Core211건 중210건이 통과했고, 나머지1건은 미연결 Player VT를 공통 성공으로 넘기던 과거 검사 기대였다. cpp의 실제 미연결 가상 분배 실패 계약으로 기대를 교정했다. 최종 관련Core305/305·Client전체1172/1172 통과(실패0·건너뜀0·각종료0), 검사 기대 교정 뒤 Core.Tests 빌드도0경고/0오류다. 직전0.512에서 전체Core2054건을 수행한 뒤 제품 변경에 관련된 사제/후처리/Pop/공간/메타데이터를 집중 재검증했고, 같은 전체 회귀를 불필요하게 반복하지 않았다. 산출물은 `extracted/dotnet-forcefield-lifecycle-0513/`다. 이번 단위의 실제 게임 실행은 없으며 포획/희생·특정 적 공격·승리 입력을 사용하지 않았다. M1 전체 상태 일치0/34는 유지한다. 다음은 사제 낙하/소리 수명과 실제 호스트 장치 조립이다.
