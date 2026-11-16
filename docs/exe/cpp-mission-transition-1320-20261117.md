# cpppj1320 — 미션 전환의 이전 연결 처리

사용자가 캠페인 1-1 승리 후 **Next Missions**를 누르면 종료된다고 제보한 `20261117003804.log`를 조사하고, 같은 미션 전환에서 발생하는 이전 연결의 flush·수신 누락을 교정했다. 원본 로그는 `%USERPROFILE%/Saved Games/Netstorm/logs/`에서 읽었으며 수정하지 않았다.

## 원인과 재현

- 최종 예외는 `main 최상위: class std::logic_error: 복합 전송이 소유하지 않은 현재 flush 노드`다. State7의 client9 정리 후 State8 진입 직후 발생했다.
- 같은 Client 전역 등록표에는 이전 미션의 server1이 정상 종료 통지를 기다리며 남아 있을 수 있다. 새 `CompositeMissionTransport`는 자신의 backend에 속하지 않는 이 노드를 flush할 수 없다고 판단했다.
- 원문 `004e2920`은 전역22칸의 현재 노드에 가상+0c를 직접 호출한다. 현재 coordinator의 소유 여부를 조건으로 삼지 않는다.
- 승리 재현 없이 기존 `mcp_cpp_mission_abort_smoke.py`의 Tutorial1 **Leave → Replay Mission**에서도 같은 오류를 재현했다. 최초 실패 자료는 `extracted/cpp-next-mission-1320/replay-before/`에 보존했다.
- flush만 수정한 중간본은 다음 단계에서 `원문 연결 등록 진단:182`로 실패했다. 새 coordinator의 수신 순회가 이전 노드를 생략하여 type2 종료 통지가 소비되지 않았고, 새 로그인이 아직 점유된 server1에 등록하려 했기 때문이다. 중간 실패는 `replay-after/`에 남겼다.

## 변경

`RawConnectionRegistry`의 실제 노드 장치에 직접 flush·수신 callback을 추가했다. 새 coordinator는 이전 Source의 현재 노드를 만났을 때 해당 노드의 실제 backend로 전달한다. 호출 중에는 선택된 Source와 callback을 보유하여, 종료 callback이 자기 등록을 반납해도 복귀 전까지 물리가 살아 있게 한다.

내부 미션과 Editor는 전체 backend Poll을 중첩하는 대신 선택된 노드 한 개의 기존 수신 본문을 재사용한다. TCP·UDP도 같은 노드의 기존 수신 본문을 연결했다. 실제 종료 통지가 기존 종료 부모를 통해 server1을 반납한 뒤에만 다음 로그인이 같은 번호를 사용한다. 등록 진단을 생략하거나 전역 칸을 미리 지우지 않으며, 로그인·ACK·승패를 합성하지 않는다.

## 검증

- 새 비MP 회귀 검사 **4/4 통과**: 이전 server1의 flush와 type2 종료 후 재등록, 직접 flush 중 owner 반납, 직접 수신 중 owner 반납과 실제 결과 보존, 장치 미연결 오류 유지.
- `netstorm_mission_entry_tests` 전체 **119/119 통과**. 기존115건과 새4건이다. 표준 오류의 로그인 장치 미연결2줄은 기존 오류 입력 검사의 진단이며 실패 단언은0이다.
- `NetstormCpp`·`NetstormRoot`·`netstorm_mission_entry_tests` Release 최종 빌드 **종료0**. 실제 TCP/UDP/Editor 경로는 컴파일·기존 본문 정적 확인까지이며 MP 실행 결과로 간주하지 않는다.
- 실제 Tutorial1의 **Leave → Continue / Leave → Replay Mission** 반복2회, 진입4회 **종료0**. 최초와 중간본이 실패했던 Replay를 최종 수정본에서 통과했다.
- 한국어·영어 각각 **1-1 → 1-2 → 1-1**(진입3회·전환2회), **종료0·오류 로그0**. 실제 시나리오 버튼의 문자열 `MissionBegin,MasterOfWhirligigs`와 같은 명령을 입력하고, 다음 브리핑의 실제 Play Mission 버튼을 눌렀다. 각 미션은 phase4·paused0·실제 세션1/1 도달 후50프레임 대기와 화면을 확인했다.
- 세 실행은 분리된 사용자 저장소와 같은 자료 사본1,392파일을 사용했고, 실행 전후 자료 해시는 불변이었다. 제보 로그·AGENTS.md·직접 참조한 디컴파일 원문의 시작 해시도 불변이다. 중간/최종 실행 원시와 화면은 `replay-final/`·`transition-korean/`·`transition-english/`, 집중/전체 검사는 `tests-transition.*.log`·`tests-mission-entry.*.log`에 있다.
- 주석 정리 후 최종 빌드의 한국어 양방향 전환도 다시 확인했다(`transition-korean-final/`, 종료0·오류 로그0·자료 불변). 최종 `cpppj/build/bin/Release/Netstorm.exe` SHA-256은 `f45c229693cfa4edf116a7cc461f5dbc3e8325f71974ba8903678afe41a453e3`다. ZIP 재작성은 다음 M8 단위에 남긴다.
- 승리·사제 포획/희생·특정 적 공격·MP 실행은 하지 않는다. 실제 승리 창의 Next Missions 클릭은 사용자 확인 대상으로 유지한다.

## 근거와 환경

마지막 전체 디컴파일 PC는 **HJOW-Athlon**이며 현재 PC와 같다. Client 양판본 2026-11-03, R전용 2026-11-07 산출물을 사용했다. 이 단위에서는 디컴파일 소스를 수정하거나 다시 생성하지 않았다. 직접 참조한 내보내기 원문은 `extracted/cpp-resume-1048/004e28b0.c`(현재 노드 수신)와 `004e2920.c`(flush)다.

VS 업데이트로 기존 CMake cache의 MSVC14.51 경로가 없어져, 같은 VS18 x64 테스트 구성으로 cache를 재생성했다. 현재 도구는 MSVC14.52·MSBuild18.12.2·CMake4.4.3이다. 이전 cache와 로그·원본 제보 로그 사본·해시는 `extracted/cpp-next-mission-1320/`에 보존했다. AGENTS.md·원본 게임·기존 fixture·DN/WB 코드와 인계 문서는 수정하지 않았다.
