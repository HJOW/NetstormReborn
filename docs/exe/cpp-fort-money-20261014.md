# 실제 Fort Money와 SP 초기화

마지막 디컴파일 수행 PC는 **HJOW-Athlon, 2026-10-14**다. 현재PC의 정밀 프로젝트에서 패치8/CD4 함수를 읽기 전용으로 내보냈다. 원본 파일·실행파일/AGENTS.md를 변경하거나 원본/복사본 게임·GUI를 실행하지 않았다.

## 현재 모드별 동작

패치 `004bf340`/CD `004297f0`는 로딩깊이를 증가시키고 Template을 진단한 뒤 `Money` 섹션을 조회한다. 실제 `DataManager` 조회는 pos를0으로 초기화한다. 본문 `00483300`/CD `00414720`는 pos에4를 감김 가산하고 signed pos/end를 비교한다. **저장된4바이트의 float값을 SP에 대입하지 않는다.**

| 조건/처리 | 10.78 | CD 10.72 |
|---|---|---|
| fort모드 | 현재localOwner에194162.0f를분산저장소로직접지정 | 로컬SP float전역에100000.0f대입 |
| 시작SP모드이고economy guard가0 | 난이도별1000/2500/3000에4000을더해Player1..8순회 | 현재signed시작SP 전역을float로변환 |
| 제외Player | 현재Player+50의bit0x20이면순회지정을생략 | 로컬Player의같은비트면대입생략 |
| 로컬플레이어가활성인패치지정 | 현재special모드면194162.0f, 아니면int금액→float, 지정뒤표시갱신 | 해당패치setter를사용하지않음 |
| 패치비로컬1..8지정 | special이면194162.0f, 아니면int금액→float. 실제분산SP만쓰기/AI지갑가산없음 | 해당없음 |

분산저장소 초기화와 쓰기는 기존 `ScrambledSpStore`/`GameRandom`의 실제복원 몸체를 재사용한다. 한SP쓰기마다32비트×5회의 잡음난수를 소비하며 필요한 경우 초기화의 재시드/33회 난수도 먼저 소비한다. SP를 그냥 float변수에 복사하면 이후 게임난수 상태와 달라진다. `SquidReward::AddSp`의 지급/AI지갑 정책으로 이 지정몸체를 대신하지 않는다.

패치 wrapper는 시작SP조건에서 `005c89c4`에99를 쓴다. 주변 실제참조는 SP/저장소 검사와 감소를 수행한다. 이값은 지형의99개 CRT 변형cache와 **별개**이며 이번구현도 지형cache를 초기화하지 않는다.

## 새 코드와 실제 원본 대조

`RawFortMoneyLoad`는 기존 SP모드/CD현재SP/장부의localOwner, 실제9개Player 바이트배열과 같은 로딩깊이를 참조한다. `Load`는 wrapper/조회pos초기화/깊이를, `ReadMoney`는 스트림본문만, `SetPlayerSp`는 패치의 직접지정을 실행한다. 원본진단 뒤 계속하는 효과와 조건부 표시를 필수hook으로 전달한다. 물리Player0..8 밖이나 난이도배열 밖은 임의SP를 만들지 않고 계약 위반으로 거부한다.

현재 독립 원본 입력은 **408개(정상388/assert20)**다. 패치244개(정상234/assert10), CD164개(정상154/assert10)다. 각입력의 두x87 정밀도 결과가 같은지 확인했으며816회반환을408에 합산하지 않는다. 전체288DWORD 분산배열/논리SP/난수/offset/초기화여부·현재깊이·pos·99회검사카운터·진단/표시순서를 저장했다. 관찰시 lazy저장소를 새로 초기화하지 않도록 이미초기화된경우에만 실제Get을 읽는다.

첫3개 smoke의조회경계는 cdecl로 잘못 해석했다. 기존 실제wrapper/metadata를 확인해 **thiscall 이름/required 두인자와 ret8**로 교정했다. 조회의pos초기화도 확인해 wrapper와직접본문을 분리했다. 최초실패로그와 전체당시코드를 `extracted/fortmoney-before-lookup-correction`에 보존했다. 초기6개/첫전체404개와해당자료도 각각 별도로보존하며 최종408에 다시더하지않는다. 마지막확장은 wrapper의missingTemplate/추가끝진단4개 새입력이며 기존404개를중복합산하지않는다.

`FortMoneyLoadTests.cpp`의3검사는 두판본 전수와 실제RawSurfaceWorld장부/current owner·현재모드/같은SP저장소·게임난수 연결을 검사한다. 실제World연결검사의사용하지않는타입/프레임/SHP는 합성입력이며 새실제자산 관찰이아니다. 공유Release/집중/전체검사 최종상태는단위11 인수인계에서기록한다.

첫 C++ 대조에서 패치 비로컬 special 지정과 fort 고정값의 차이가 드러났다. 실제 `00447861~00447880`은 비로컬 지정에도 special 고정값을 적용하며, 패치 Money의 실제 상수 `0x483d9c80`은194162.0f다. CD의100000.0f와 분리해 수정했다. 실제 월드 검사에는 초기 다리 연결 타입 번호를 공급하고 로컬 BYTE 지정도 명시변환했다. 전체 이전11파일과 첫 실패/빌드 로그를 `cpppj/hist/fortmoney-be-e2121/`에 보존했다. 정확 역변환과 전체408 원본 결과 불변을 `tools/analyze_fortmoney_cpp_correction.py`로 감사한다. 두 번째 공유 Release에서 새3검사 모두 통과했으며 새 원본 입력은0이다.

현재파일섹션조회/시각재시드/힙확보/진단/표시는명시경계다. Badges/Technology/전체Fort부모·실제GameWorld 설치/캠페인완주 동일성은 후속이다. 이번Money 복원만으로 캠페인시작SP가최종3000이라고 확정하지않는다. 뒤의실제미션/Player 초기화순서를 함께연결해야한다.
