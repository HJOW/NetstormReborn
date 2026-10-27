# 로그인 Subscriber의 컴파일러별 참조 분리 수정

단위505, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 이번에는 같은 산출물의 원본 몸체를 읽었다. 새 디컴파일이나 원본 게임 실행은 하지 않았다. [완료 보고서](../../cpppj/subscriber-binding-runtime-20261027.json)에 실행파일과 증거 해시를 기록했다.

## 문제와 원본 근거

HJOW-X3D의 VS2022/MSVC19.44 제품은 캠페인1-1 로그인에서 요청/현재 Subscriber가 `0xffffffff`, Player1의 Subscriber가1로 달라 가입 대상을 찾지 못했다. 이어 실제 Session에 연결되지 않은 거부 응답 장치에서 종료했다. 이 조사와 첫 실패 원시는 [인수인계의2.5절](../../LEFT_JOBS.md)에 보존되어 있다.

10.78 디컴파일 `004c3290`은 fileName 설정 callback 뒤 **004c335c에서 현재 Login0054265c를 My00540c74에 복사**한다. 저장 월드 부모 `004b5300`도 **004b5315에서 같은 두 전역**을 사용한다. 이 두 DWORD는 Client/Init/World/Session에서 주소와 현재값을 공유해야 한다. 기존 `InitializeTutorialMission`의 쓰기 순서는 이 원문과 일치한다. CD 대응 월드 부모는004322e0, My0050f6cc/Login00511c7c이며 이번 수정은 CD 게임 전체 동작을 새로 확인한 것이 아니다.

## 같은 제품 헤더의 최소 재현

현재 PC에는 MSVC19.44와19.51이 함께 있다. 기존 헤더를 사용하는 작은 콘솔 프로그램에서 PE 최초 My=`ffffffff`/Login=1로 직접 factory와 조건식 factory를 각각 구성하고, My=Login을 쓴 뒤 주소와 값을 관측했다.

| 컴파일러 | 직접 factory | 기존 조건식 | 외부 My와 같은 주소 |
| --- | --- | --- | --- |
| VS2022 / MSVC19.44 | 1 | 4294967295 | 직접1, 조건식0 |
| VS2026 / MSVC19.51 | 1 | 1 | 직접1, 조건식1 |

`CampaignWorldGlobals`의 복사/이동 생성자는 원래 계약대로 현재값을 자기 fallback에 복사한다.19.44의 조건식 구성에서는 이 값 복사 경로로 외부 참조가 분리된다. 따라서 Init이 실제 Client My를 갱신해도 Session의 사본에는 최초−1이 남는다. 기존 Subscriber 검사 두 사례의 참조 주소/변경 전파 실패와 같은 현상이다. 이 관측으로 다른 컴파일러 버전 전체의 정상/비정상을 일반화하지 않는다.

첫 도구 호출은 `cmd /c`의 상대 경로에 슬래시를 써서 실행 전 실패했다. 백슬래시 경로로 교정한 재실행 로그와 함께 원시 디렉터리에 보존했다. 이 도구 실패를 제품 검사 실패로 세지 않는다.

## 변경

`CampaignWorldGlobals::SubscriberBinding`으로 두 장기 DWORD 주소를 전달하고, `ClientGameplayGlobals::world`를 해당 입력으로 직접 구성한다. 조건은 포인터 선택에만 사용한다. owner를 생략한 독립 장치는 기존0/0 저장소를 사용한다. 외부 값 초기화, 임의 ID1 덮어쓰기, 복사/이동의 alias 전파를 추가하지 않았다.

기존 공용 검사의 조건식 구성 사례를 제품이 사용하는 직접 구성으로 바꾸되 같은 주소/변경 전파 기대값을 유지했다. 기존 조건식의 컴파일러 차이는 별도 최소 재현에 남긴다. 새 fallback/부분 binding/값 복사 검사를 추가하고, 실제 Client 생성 직후 world와 Subscriber owner의 주소·값 일치를 확인한다. 미발견 owner에 대한 거부 응답 장치는 이 수정 범위에서 빈 성공으로 연결하지 않는다.

## 검증

Release 증분 빌드 종료0·경고/오류0이다. 최종 제품의 집중 검사는 Subscriber10·MissionLogin3·CampaignWorldLoad2의 **15개/실패0**이다. 두 컴파일러 모두 제품과 같은 `/std:c++20 /O2 /MD /DNDEBUG /utf-8 /W4 /permissive-`로 최소 재현을 컴파일했으며, 직접 binding은 둘 다 같은 주소와 현재1/77·독립 fallback0/0·독립 값 복사를 유지했다. 기존 조건식의19.44 실패와19.51 성공도 같은 옵션에서 다시 관측했다.

문제 구간의 실제 `ClientGameplayGlobals.cpp`와 공용 Subscriber 검사2파일/runner를 **v143으로 다시 컴파일한10개 검사도 실패0**이다. 이 추가 관측은 최종 제품의 집중15개와 중복된다. 첫 연결은 v145로 만든 `netstorm.lib`에 v143 STL을 섞어11개 기호를 찾지 못했다. 기존 v143 object를 바꾸지 않고 v145의 link/STL로 재연결해 통과했다. **나머지 라이브러리는 v145이므로 전체 제품의 v143 빌드 성공으로 간주하지 않는다.** 실패와 교정 로그를 함께 보존했다.

같은 최종 게임 `db5b2456d2233c09b2ebc2730d275c099bd670fd62d0bf3cc8aafeaac9907d35`로 캠페인1-1의 실제 Play Mission 안내→Start/Session→비정지 플레이·전체화면을 확인했다. 관측frame20~179의 실제 게임 시간은 **10.612초 증가**, 종료0이다. 이는 `--mission` 진입이며 Campaign 메뉴의 모든 클릭 경로를 새로 확인한 기록은 아니다.

보호 원본/CD·기존 fixture·나머지 검사와 AGENTS는 바이트 동일하다. 기존 검사 변경은 위 공용2파일뿐이며 기대 주소/변경 전파를 약화하지 않았다. 원시는 `extracted/cpp-subscriber-binding-505/`와 그 접두사의 재현 소스/드라이버다. 같은 최종 제품의 전체34경로/CTest는 **단위506에서 진행 중이며 이번 완료 수에 합산하지 않는다.**

자연 완주/사용자 개입과 `.english` 부재 개선은 계속 보류한다. Windows11/다른 모니터·DPI, 전체 Edit/시나리오·HUD/음향 동일성, 후속 멀티플레이 사전 구현은 이 수정의 완료 범위가 아니다.
