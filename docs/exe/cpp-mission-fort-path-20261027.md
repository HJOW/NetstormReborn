# 미션의 논리 요새 이름과 실제 Template 파일 경로

단위486은 Test Battle 복귀에서 원래 저장 파일을 다시 선택하기 위한 **파일 경로 경계**를 연결한다. 기존 Session은 논리 요새 이름을 manager의 파일 이름으로도 사용했다. 이제 실제 Client는 같은 fortSpec 선택으로 얻은 파일 원문/실제 경로를 함께 설치하고, 로그인 재로드는 Session이 보존한 논리 이름으로 수행한다. c2 일반 입력이나 전체 Editor 왕복을 완료했다는 뜻은 아니다.

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 원본/CD·AGENTS·기존 디컴파일은 수정하지 않는다. 사용자 설명에 따라 커스텀 맵의 대응 `.english` 부재 시 Test Battle에서 종료되는 원본 동작의 개선은 나중으로 미룬다. 임의 시나리오 fallback이나 파일 자동 생성을 추가하지 않는다.

## 실제 연결

- `MissionFortSourceImage`는 한 번 선택한 fortSpec 경로의 원문과 실제 manager 파일 이름을 함께 전달한다. `MissionScript::FortFileReader`는 Config 경로를 한 번 캡처하고 그 경로로 읽은 뒤, 같은 경로를 실제 namespace의 파일 이름으로 해석한다. I/O가 다음 Config를 바꿔도 이번 이미지의 경로를 새 값으로 다시 고르지 않는다.
- Script는 명시 source와 기본 자산 reader를 구별한다. `FileSource`는 명시 source만 소유 복사하며, 기본 자산 경계는 빈 함수로 표현한다. Guide Begin/Restart/Fork의 실제 공장이 해제 전에 캡처한 명시 namespace를 다시 사용한다.
- 실제 Client의 최초 설치는 같은 읽기의 바이트/파일 이름을 전달한다. 작업 source는 같은 EditorCatalogWorkspace의 실제 경로를 쓰고, 기본 자산은 같은 게임 디렉터리/설정 경로를 사용한다.
- Session은 논리 요새 이름을 독립적으로 보유한다. 실제 Login::Reload의 paired reader에는 이 이름을 전달하며, 반환한 같은 파일의 물리 경로를 실제 manager+c에 설치한다. 명시 paired reader의 오류를 기존 byte reader/원본 자산으로 우회하지 않는다.
- 기존 byte reader와 독립 Session의 논리 이름 계약은 유지한다. 이번 변경은 이미 복원한 파일/manager 읽기 계층의 실제 연결이며 원본 전체 LoadFile/State7을 새로 복원한 것으로 합산하지 않는다.

## 검증

최종 Release build2는 경고/오류0이다. 최초 build1은 정상 종료했지만 새 지역 이름 `source`가 Login의 기존 DWORD 멤버를 가리는 C4458 경고1건이 있었다. 지역 이름만 `fortFile`로 고치고 최초 로그를 보존했다. 신규2개 포함 집중19개/실패0을 확인했고, 등록은 기존3,274개를 유지해3,276개다.

신규 검사는 실제 작업 파일/장부·Config·원본 Fort·로컬 Session 로그인/패킷을 사용한다. 정상 경로와 현재 fortSpec 변경, Script/외부 workspace 반납 뒤 수명, manager+c 실제256BYTE 이름, 파일 읽기 도중 Config 변경에도 이미지와 경로가 같은 선택인지를 확인한다. 기존 source/재로그인/Guide 수명과 Init도 함께 검사했다. 기대값을 제품 결과로 바꾸지 않았다.

같은 최종 제품의 전체 **C++3,276개/실패0·CTest1개/실패0**을 확인했다(전체 약348.18초). 실제 War 직접 진입→Play Mission→수초 플레이→ESC/Game/Restart Mission→새 브리핑/수초 플레이도500프레임/약13.18초에 종료0·상한 미도달이다. 이 실행은 격리 복사본/무음이며 전체 스테이지 완주 증거가 아니다.

기존3,274등록은 모두 유지했다. 원본/CD/기존 검사3,195파일에서 CMake와 CampaignSessionTests에 끝 등록만 추가한 것을 제외한 바이트와 AGENTS를 보존했다. 두 기존 파일의 원래 전체 접두부는 각각162,631/759,426BYTE다. 소스/검사2,848파일과 실행 파일은 최종 빌드/집중/전체/GUI 뒤 동일하다. 원격은 fetch/pull로 최신 상태를 확인했고 우리 작업에서는 push하지 않았다.

최종 게임 SHA256은 `ac6d445ebd0a283b156a0b140d3ccdd2dab473cc36feea141a623ceb8a06002a`, 검사 실행 파일은 `c44cc8b0fcabab5cab3fde64241738023a1c4e187cccc0256d2b3cf88501a195`다. [실행 보고서](../../cpppj/mission-fort-path-20261027.json), 원시 `extracted/cpp-mission-fort-path-486/`을 본다. build1/2·집중·whole1·GUI 프로세스는 모두 종료했다. 신규 Native CPU 관측은 없으며 기존 원문 fixture를 전체 검사에서 재생했다.

## 다음 연결

실제 c2 입력에서 snapshot→작업본 Guide.Begin→+64를 연결하고, 복귀에서는 Guide.Close→현재 이름/pending→선택2→전체 RequestFortEdit를 연결한다. Battle14의 실제 파일 읽기/State/섬 수/Notify와 State1 재설치를 조립한다. 대응 언어 파일이 있는 TEST01 등으로 짧은 실제 왕복을 검증한다. 전체 미션 완주와 원본 전체 픽셀·음향 동일성은 계속 별도 목표다.

작업 도중 사용자가 목표를 확대했다. **Windows 10/11 호환성과 전체화면 DirectDraw 오류 해결도 현재 목표**이며, 원본과 다른 호환성 처리는 반드시 코드 주석에 명시한다. 이번 파일 경로 단위는 이 호환성 작업을 완료한 것으로 합산하지 않는다.
