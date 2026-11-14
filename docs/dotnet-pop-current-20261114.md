# dotnetpj 0.512 — 현재 Pop·프레임 공간·필수 전송기

2026-11-14, HJOW-Athlon. 이식 기준은 cpppj 소스 `7313ba1c6a0e4984b434bb562c7335d4383bb075`다. 원본 실행 파일/디컴파일을 새로 수정하거나 생성하지 않았다. M1 전체 상태 일치는 여전히 0/34이고 새 경로 전체 플레이는 미완성이다.

## 구현 범위

`RawPop`의 PATCH 분기는 0.511의 `RawSquidPopParent`와 `RawSquidActivate`를 실행한다. 실제 월드는 `PopCurrent`로 들어가 사전 TYPE/SHP 조회 없이 좌표·spot·현재 SHP·해시·표시·최초 flags·Activate 순서를 따른다. 현재 TYPE 칸은 getter로 빌리므로 콜백이 같은 표 원소를 교체한 뒤에도 해당 칸의 현재 필드를 읽는다. 진단 묶음은 함수 객체를 복사하고, 선택된 장치만 도달 위치에서 요구한다. 예외가 나더라도 앞서 기록한 공간/슬롯 상태는 원문대로 남긴다.

`SquidDisplay`를 같은 Pop/Unpop 양쪽에 전달한다. `SquidFrameBinding`은 풀·표시기·해시·spot 정체성을 구성 시점에 검사하고 실제 프레임 크기/표시·Unpop·Pop을 연결한다. 현재 TYPE 대여는 0.510 계약을 유지한다. 다른 SHP 크기로 넘어갈 때 다음 프레임 호출에서 이전/새 해시 단계가 바뀌는 원문 순서도 유지한다.

서버 권한의 일반 Pop은 멀티플레이가 없어도 직렬화 자식을 선택한다. 이를 성공 상수로 건너뛰지 않고 `RawSquidTransmission`의 실제35BYTE writer, 16필드 압축, 카운터/소유자 캐시, 양수 길이 이후 현재 연결/방송 분배를 옮겼다. `RawSurfaceWorld`는 cpp typed 독립 월드와 같이 writer를 실행한 다음 runtime 미연결 방송 관문에서 반환한다. runtime을 설치했으면 실제 SquidPacket을 요구한다. 대상 연결·송신·ProcessForm writer는 해당 분기에 도달했을 때 실제 장치를 요구한다. Native 전체2048BYTE 예약과 typed 반환 길이 관측은 별개 계약이다. MP 실행·외부 서버 접속은 하지 않았다.

`RawGameAuxState.SquidTransmission`이 버퍼·헤더·압축 누적을 소유하며 일반 월드 준비에서도 재생성하지 않는다. 공장의 선택 TYPE 최대HP getter는 PATCH checking/받침 예외→정상 진단 복귀→현재 약화/사제 번호→선택 TYPE HP 순서를 구현한다. 장기 약화 BoolBox와 사제/받침 UIntBox를 같은 공장에 연결한다. C# getter 입력은 같은 전체 IReadOnlyList 표이며 C++의 겹치는 부분 span을 별도 API로 노출하지 않았다. 공장 생성 HP 경로의 나머지 최신 getter 교체는 후속 R6 영향 검토 범위다.

삭제 writer는 헤더를 먼저 쓴 뒤 client SID 진단을 수행하고 정상 복귀하면 현재 삭제 stamp를 소비한다. `RawSurfaceWorld.TransmitDeletion`도 같은 실제 writer를 선택한다. `SynchronizePeer` 전체 조립은 후순위 로컬 MP 작업으로 남는다. CD는 기존 Pop/삭제 몸체를 유지하며 PATCH 전송기를 CD 레이아웃으로 사용하지 않는다.

## 검사 근거

- 공유 x86 fixture249개는 원본 관측 자료다. 해당 기대값/manifest 해시를 바꾸지 않았다. 기존 Graph/Fort 조립 검사는 현재 cpp 검사와 같이 grid 로그 또는 필수 Pop 자식 도달 인자를 관측하도록 공급 의존성만 바꿨다.
- 별도 `squid-wire-cpp.tsv`256입력은 현행 **C++ 정적 모듈**의 직접 관측이다. x86 원본 자료나 M1/L2 증거로 세지 않는다. 각 행의 반환 크기·첫64BYTE·나머지1984BYTE 보존·카운터·deletion stamp·소유자 캐시·로그·현재 getter 호출 횟수를 비교한다. NaN/inf/분수 좌표, WORD/DWORD 차이, signed HP/zOrder, frame DWORD 비영/하위BYTE 출력, uint wrap, 현재 TYPE 교체와 부작용을 포함한다.
- 프로브는 `cpppj/build/lib/Release/netstorm.lib` SHA256 `d2e445e56f3280f6ff86f6fe1f4b4ca2ab0a6ab3a534eb318258e999b752901c`를 링크했다. 산출물 TSV SHA256은 `0ecd7d0d1be0b027c65eacf3515063e7e7e84664e28cf84b2fd5f84455a04b43`이며 내보내기 재실행도 동일하다.
- 첫 전체 회귀의 Core11/Client42 실패는 실제 전송기 또는 grid 로그를 호출자가 공급하지 않은 것으로 분류했다. 제품의 독립 typed 방송 관문과 테스트 의존성을 C++ 계약으로 교정했다.

최종 빌드·회귀 결과: 원복 뒤 비증분 Release 빌드0경고/0오류, 최종 Client1172/1172·Assets366/366 통과, Core2054중2053통과/1실패였다. 유일한 실패는 PORT_MAP에 없는 RawSurfaceStorage.h 경로를 적은 메타데이터 오타였다. 해당 행을 제거한 뒤 PORT_MAP/fixture metadata6/6이 통과했다. 제품 소스는 같은 빌드이며 변경된 메타데이터만 재대조했고 미해결 실패는 없다. 집중177/177도 통과했다. 전체 회귀 실패 이력과 마지막 재검사 범위를 구별한다. PORT_MAP734행(L1 346+port388), 공유fixture249개와 별도CPP 모듈fixture1개다.

프레임의 DWORD 비영 검사를 BYTE로 바꾼 변이는 writer 검사9건 중 C++256관측 대조1건이 실패해 검출했다. 소스 SHA256 `09f9cde27af5ca4711f6c1f7c7672bd5fab6b6c396d22b51c8a67375b2855440`로 바이트 원복했고 다시 전체 비증분 빌드했다.

최종 단순 실행은 `thewarbegins`·`savetheisland` 각각 cpp/dn4회 모두 종료0이다. 고정25ms/시드1760000000의 미션 진입/Play Mission 팝업 확인만 수행했다. 전후 originals1394파일은 해시까지 동일하다. 전자는 cpp1941/dn1854슬롯, 공통1854/dn전용0/전체동일78/그래프240/255이고 후자는 cpp2926/dn2619슬롯, 공통2619/dn전용0/전체동일62/그래프245/255다. CRT296677442는 두 쪽 동일하다. L2 도구 종료1은 전체 상태 차이가 남았다는 뜻이며 네 게임 실행은 종료0이다. 0.504와 상태 내용은 같고 일부 캡처의 frame 행 번호만 다르다. M1 전체 일치0/34를 유지한다. 산출물은 `extracted/dotnet-pop-current-0512/` 아래에 보존한다.

## 프로브 사용법

저장소 루트에서 `powershell -ExecutionPolicy Bypass -File dotnetpj/tools/export_squid_wire_cpp.ps1`를 실행한다. 먼저 cpppj의 Release `netstorm` 대상과 QuickJS 라이브러리를 빌드해야 하며 Visual Studio x64 C++ 도구 집합이 필요하다. `-OutputPath <TSV 절대/상대경로>`로 별도 관측 위치를 지정할 수 있다. 기본값은 `dotnetpj/tests/Netstorm.Core.Tests/Fixtures/squid-wire-cpp.tsv`다.

스크립트는 vswhere로 도구 집합을 찾아 `extracted/dotnet-squid-wire-probe/probe_squid_wire.exe`를 만든다. 이 실행 파일은 창·게임·네트워크 없이 현행 writer256입력을 수행하고 stdout에 관측 TSV를 쓴다. 직접 실행 시 현재 작업 디렉터리와 무관하며 명령행 매개변수는 없다. 파일 메타데이터/해시는 상위 PowerShell 스크립트가 붙인다. 원본 디렉터리에는 쓰지 않는다.

## 이후 작업

R4 다음 순서는 낙하/포스필드·소리 수명과 실제 파생 postPop, Fort/Player/AI 시작 조립이다. 일반31+데모3 경로 전체 L2 일치가 M1 완료 기준이다. 사제 포획/희생·특정 적 공격·시나리오 승리의 실제 재현 검사는 사용자 직접 플레이 확인으로 남긴다. 이번 단위는 그러한 입력이나 승리 재현을 수행하지 않는다.
