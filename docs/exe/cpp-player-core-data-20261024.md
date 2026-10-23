# 현재 CoreData 선택·정리·조회 독립 복원

2026-10-24 후속 **단위385 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 신규 `RawPlayerCoreData.h/.cpp`·검사·관측 자료만 작성했으며 기존 공유 소스/Chat 동결파일/CMake/LEFT/git·공용 lib/빌드·게임/GUI를 변경하거나 실행하지 않았다.

제품이 없는 상태를 도구에서 확인한 뒤 실제10.78 PE의 **0048f0d0~0048f0fe(47BYTE)** 조회와 **0048f3b0~0048f42d(126BYTE)** 선택/정리를 Unicorn으로 먼저 실행했다. [도구](../../tools/decomp_player_core_data.py), [최초 TSV](../../cpppj/tests/fixtures/player-core-data-x86.tsv), [Native 보고서](../../extracted/cpp-player-core-data-20261024-native.json)는 **402고유입력×양 x87모드=804관측·10650명령**이다. 정상730·현재 source의 물리 읽기 fault72·진단의 명시 중단2를 구별하며, 양 모드 원시 결과 차이는0이다. 전체 Player1620BYTE/캐시10BYTE/외부 source16BYTE와 두 owner/known/active/metamap/현재 포인터·반환 포인터·진단trace를 고정했다.

그 뒤 신규 C++ 두 몸체와 검사만 작은 runner에 격리 링크했다. 올바른 격리 경로의 **build1 종료0·컴파일 경고/오류0**, **3검사/실패0**이다. 1개 검사가804행 전체를 대조하고, 나머지2개는 실제 반환뷰 쓰기의 Player/캐시 alias와 표현 불가능한 Player 물리를 거부하는 API 계약을 확인한다. 다른 작업자의 읽기 검토에서도 명백한 순서/수명 오류를 찾지 못했다. 최초 빌드 호출은 작업 디렉토리를 잘못 지정해 Runner.cpp C1083/종료2였고 로그를 보존했다. 그 호출이 생성한 신규 object 두 개는 절대경로를 확인한 뒤 격리 디렉토리의 `first-wrong-directory-*` 이름으로 이동 보존했다. 기존 파일/공용 라이브러리는 변경하지 않았다.

## 원문 효과와 실제 alias

[공개 API](../../cpppj/src/o/RawPlayerCoreData.h)의 `PlayerCoreDataGlobals`는 **같은 실제 두 owner/known/active DWORD와 현재 포인터 역할의 가변 span 슬롯**, 고정캐시10BYTE와 현재 Player9 span 슬롯을 빌린다. 타입이나 값이 같은 별도 버퍼를 현재 포인터로 가장하지 않는다.

`ReadPlayerCoreData`는 metamap이 비영이고 현재 `.data()`가 실제 캐시 `.data()`와 다를 때만 **Player.cpp85** 진단을 요청한다. 진단 뒤 current가 NULL/다른 Player/캐시로 바뀌거나 예외 중단되는 실제 경로를 대조했다. 복귀 시 **현재 span을 재조회**해서 가변 alias를 그대로 반환한다. `active`를 이 getter의 원문에 없는 조기 관문으로 추가하지 않는다.

`SelectPlayerCoreData(owner,templateOwner)`는 Template owner 인자2→battle owner 인자1을 먼저 쓴다. owner0에서는 캡처한 현재 포인터가 비영일 때 **DWORD0 읽기/캐시쓰기→DWORD4 읽기/캐시쓰기→WORD8 읽기/캐시쓰기**를 수행한다. source 길이가4 또는8이면 앞선4/8BYTE 캐시 효과가 남고, 마지막 current/active0쓰기는 아직 하지 않은 채 물리 오류로 중단한다. 정상 완료 때만 current를 empty로 만들고 active=0을 쓴다. **owner0은 known에0을 쓰지 않는다.**

비영 owner는 known=1을 먼저 쓴 뒤 현재 metamap이 비영이면 같은 캐시뷰를, 그 외에는 현재 Player base의 **owner×180+108에 있는 같은10BYTE**를 선택한다. active를 임의1로 만들지 않는다. 실제 반환뷰를 통해 rank/나머지 BYTE를 바꿨을 때 바로 같은 Player/캐시가 바뀌는 검사도 별도로 통과했다.

비영 owner의 원문은 물리 접근 없이 가상주소를 계산할 뿐이다. 잘못된 owner/base에서도 무효주소를 current에 쓸 수 있지만, C++ span은 그것을 실제 물리 alias로 표현할 수 없다. 이 API는 **선택된10BYTE가 실제 Player 물리 안에 있어야 한다**는 명시 계약을 둔다. 위반은 앞선 owner/known 쓰기 뒤 범위 오류로 드러내며, 그 오류를 Native의 무효포인터 반환과 동일하다고 세지 않았다.804행 Native 대조의 비영 선택은 유효 Player1/8이며, 범위 밖 장치 검사는 별도 계약 검사다.

## 연결 범위와 공용 등록

이 두 몸체는 실제 Client owner를 새로 소유하거나 Session 생성/소멸 때 암묵 호출하지 않는다. [Client 접합 제안과 수명 전제](cpp-client-chat-core-owner-proposed-20261024.md)에 실제 공통 정리004b2df0의 **48f3b0(0,1)→4910a0 Player free/new**와 현재 Login의 연결 위치를 기록했다. 여기서 helper 통과를 실제 Client/전체 Editor 설치 성공으로 확대하지 않는다. 최초 조사에서 이 정리 부모를 앞선004b2c60으로 잘못 귀속한 오류를 확인·교정했으며, 원문 범위 두 개만 직접 실행한804관측과 제품 두 몸체는 영향을 받지 않는다.

공용 등록은 `src/o/RawPlayerCoreData.cpp`, `tests/PlayerCoreDataTests.cpp`와 `NETSTORM_PLAYER_CORE_DATA_FIXTURE`에 위 고정 TSV 절대경로를 공급하면 된다. 최종 소스/로그/격리 실행파일 SHA는 [검증 manifest](../../extracted/cpp-player-core-data-20261024-validation.json)에 기록한다. 기존 Chat 독립 모듈과804행 원문 자료는 이후 교정하지 않는다.
