# 전체 Deck 명령 부모와 실제 입력 연결 (단위451)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 수행일은 **2026-10-23**이다. 기준 HEAD는 `74bc5941`이다. 기존 정밀 C와 보호 PE를 읽기 전용으로 사용하고 새 디컴파일은 하지 않았다.

## 원문과 제품 연결

원본 `0044f9d0`은 `0054db14=1` → 현재 `00540cac` 소유자의 `0044f700` Deck → 실제 `0044f680` FullDeck → `0055a4a4=1` 순서다. 새 `GetFullDeckCommand`는 기존 `LoadFullDeck`과 실제 `RawProductionDeck` 메모리·공유 난수를 재사용한다. 하위 실패 뒤 생산 dirty를 쓰지 않으며 앞서 쓴 FullDeck 또는 콜백이 변경한 현재 값을 복구하지 않는다.

`RawCampaignSession::GetFullDeck`은 같은 Client FullDeck DWORD, 현재 Template 소유자, 실제 Player Deck, 게임 난수, 생산 dirty를 빌린다. 객체 기술 획득이나 즉시 HUD 재구성을 추가하지 않는다. 기존 생산 갱신 소비자가 원문 위치에서 dirty를 처리한다.

보호 PE의 `00542a28` 초기값은 **`020000cd`**, 명령 이름은 `GetFullDeck`이다. 기존 실제 입력 처리기에 연결하고 Editor(`005c85a4`) 또는 missionPost(`00594fa4`)가 비영일 때만 실행한다. Editor가 비영이면 missionPost 조회를 생략한다. release와 Guide/Gump 선행 소비는 기존 부모의 순서를 유지한다. 별도 메뉴 버튼은 추가하지 않는다.

## 제품 전에 고정한 독립 증거

[관측 도구](../../tools/decomp_full_deck_command.py)는 실제 부모와 FullDeck·Append·Entry·게임 난수 몸체를 **230회** 실행했다. 현재 owner·비영 DWORD·두 x87 정밀도·TYPE 조회 뒤 현재값 변경·하위 중단을 포함하며 정상226/명시 중단4다. GetDeck의 반환 메모리만 명시 외부 장치다. 전체352BYTE·난수·현재 전역과 부모 store/호출 순서를 [부모 fixture](../../cpppj/tests/fixtures/full-deck-command-x86.tsv)에 고정했다.

별도로 실제 Guide·Gump·UserInput 현재 사건 경로 **384회**, 부모 호출60회를 [입력 fixture](../../cpppj/tests/fixtures/full-deck-input-x86.tsv)에 고정했다. 이 입력 관측에서 부모 호출 자체는 위에서 검증한 별도 경계다. 기존 관측기의9명령 분류표에 새 ID를 넣지 않았으므로 새 호출은 원시 `envelopetrace` 열에 남아 있으며 `bodytrace`는 `-`다. C++ 비교도 이 실제 열을 사용한다.

최초 관측 도구 실행은 결과 열만 반환하는 기존 run API에 입력 name이 있다고 가정해 보고서 조립에서 실패했다. 원시 도구 출력과 `extracted/cpp-full-deck-command-451-native-first-failure.json`에 남겼다. case 입력과 실제 결과를 결합해 수정했으며 원문·제품·기대 계산은 바꾸지 않았다. 최종 관측 로그는 `native2.log`, [Native 보고서](../../cpppj/full-deck-command-native-20261026.json)에 실제 SHA를 고정했다. 원본 게임 창·OS 호출은 실행하지 않았다.

## 검증 상태

신규5개와 기존 Deck 복원·Player Deck·메뉴 입력·Campaign 회귀를 실행할 예정이다. Source1 빌드는 TYPE 뷰가 `std::span`인데 `vector::at()`으로 접근한 컴파일 오류로 종료1이 됐다. 실제 범위 검사와 인덱스 접근으로 수정했고 원시 `build1.log`·결과를 보존했다. Source2는 종료0·오류0이나 새 검사 optional/정수 비교 경고2가 있어 unsigned 리터럴로 고쳤다. 실제 FullDeck·owner·dirty의 순수 보고도 추가했다. Source3의 **2,039소스 동결**, build3 종료0·경고/오류0, 집중 **459고유/461관측·실패0**(신규5개 전부)을 확인했다. 기존222사례의 본문·기대는 체크아웃 개행 차이와 새 include/불변ID 입력 추가를 구별해 보존했고 Native fixture6개도 유지했다. 기존 RawDeckRestore.cpp 전체 원시 바이트도 부모 추가 외에는 그대로다.

같은 제품을 별도 물리 복사본에서 실행해 검사 Tell 버튼의 실제 클릭 → 정상 명령 큐 → UserInput 모드 관문 → 실제 Session 부모를 통과했다. 같은 Client의 FullDeck은0→1로 바뀌었고 생산 dirty는 같은 프레임에 기존 소비자가 처리해0으로 관측됐다. 실제 검사 창과 처리 뒤 캠페인·덱·미니맵 캡처2개를 확인했다. 이 버튼은 검사 자료에만 추가했으며 원본 게임 메뉴를 고치지 않았다. 원본/CD 전체1,819파일 집합과 보호1,821파일의 SHA도 유지했다. 화면 확인은 원본 픽셀 동일성 검증이 아니다.

게임 SHA는 `a6550096e223a7362a365226a4bc8a747ab2c791202f1170d08e79ced0a3771a`다. [실행 보고서](../../cpppj/full-deck-command-runtime-20261026.json), 원시 `extracted/cpp-full-deck-command-451-*`에 세 빌드·집중·GUI·최초 Native 보고서 조립 실패를 보존했다. 마지막 전체 통합은450의3,076개·GUI2·자연Early2다. 이 단위에서 긴 전체 회귀를 반복하지 않고 사용자 지정3~5단위 묶음 주기를 유지한다.

## 남은 연결

전체 Deck nullable `+ac` 실제 하위, SID16 잔여, PlayerNoMake255DWORD와 pool header20, 전체 Native State1 Effect/prefix·GameplayUse/고정뷰 반납 coordinator 및 실제 Edit 설치는 계속 남아 있다. 전체 Early·훈련·캠페인·데모 완주와 진행 저장·표시/음향 동일성 목표도 미완료다.
