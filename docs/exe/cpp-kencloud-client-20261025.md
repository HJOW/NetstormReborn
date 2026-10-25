# 실제 Client의 Kencloud 수명과 전체 팔레트 전환

2026-10-25 단위424다. 이전423은 `a1bfa97d / 20261025-17 CP`로 공통 시작 SP와 추적 DWORD의 실제 수명을 연결했다. AGENTS와 양 인계를 다시 읽었다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 수행일은 **2026-10-23**이다. 이번에는 새 전체 디컴파일이나 원본 게임 실행 없이 기존 원문과 보호 PE의 초기값을 읽는다. 보호1821파일 기준은 `extracted/cpp-kencloud-client-424-baseline.json`이다.

## 원문 근거와 실제 소유자

기존 `RawKencloud`의 생성·파일·행 이동과 `RawFortPaletteTransition`의 전체 부모를 재사용한다. 기존 Native **246/676관측**과 fixture를 수정하지 않는다. 보호 PE의 옵션 초기값·고정16행·원문 함수표와 두 몸체/fixture의 SHA는 `extracted/cpp-kencloud-client-424-static.json`에 먼저 고정했다. 새 원문 실행 관측을 만들었다는 뜻은 아니다.

`ClientKencloudGlobals`가 같은 Client에서 옵션8DWORD, 구름 연속2054DWORD, 팔레트 이름과 실제 생존 블록을 소유한다. 구름 이름 뒤의 kenCloud/algorithm/dirty/phase도 같은 물리 범위에 있어 인접 별칭을 보존한다. 실제 파일 장치는 현재 ConfigInterface의 PathSpec, 같은 VFS, 첫 GIF descriptor의 치수, 할당 후 픽셀 해독 순서를 연결한다. 실제512×512 `Gifcloud.gif`, 다른 치수의 `Defeat.gif`, 없는 파일을 검사한다. Native 부모가 다른 치수 블록을 반납하고 fallback을 생성한다. 파일 존재 실패가 해독 예외를 삼키지 않는다.

난수는 공통 SP와 Session이 쓰는 **같은 ClientRandomGlobals의 GameRandom 참조**다. getter/옵션 해석/Renderer 연결은 구름을 생성하거나 난수를 소비하지 않는다. 기존 즉시 설정 콜백에 원문 일곱 옵션 읽기를 추가하고 이전 Fort/Script 설정 콜백을 유지한다. 실제 미작성 할당 입력은 `CurrentAllocationBytes`로 연결한다. 주소는 host 포인터를 자르지 않는32비트 생존 블록 토큰이며 내부/one-past/반납 뒤 접근을 구별한다.

## 실제 팔레트 장치와 검증 경계

실제 Uber의 전체 팔레트 미연결 예외를 같은 문자열 저장소와 `Client::ChangeFortPalette`에 연결한다. 기존 전체 부모가 이름 비교→반납/cache0→생성/행 이동→실제 Screen의 clear1 팔레트 적재→같은 고정 초기RGB의9색 표→현재 Renderer 재조회/dirty의 순서를 맡는다. SceneMusic의 별도 DataDir/clear0 경로는 유지한다. 같은 이름의 대소문자 무시 자연 반환도 원문 CRT와 같다.

Renderer 생성·해상도 교체·반납 위치에서 같은 소유자의 비소유 장치 토큰을 갱신한다. Uber의 worldRedraw는 같은 DWORD의 직접 참조다. 캐시·이름·옵션·phase는 해상도 교체나 Session 교체로 초기화하지 않는다.

일반 CTest는 창을 열지 않는다. 별도 `netstorm_cloud_probe`와 `tools/cpp_kencloud_client_probe.py`는 실제 Client/Win32 Screen에서 첫 팔레트→같은 이름→다른 팔레트→해상도 교체→다시 첫 팔레트의 상태를 기록한다. 이 검사는 명시 API 장치 검사이며 자연 사용자 플레이나 최종 Editor 설치의 증거로 세지 않는다. 사용자 입력 회귀는 같은 제품 실행 파일로 따로 수행한다.

## 실행 기록

최초11소스를 source1에 고정한 빌드1은 ClientMain의 로그 출력에 필요한 `<iostream>` 누락으로 종료1이었다. 로그와 최초 소스는 보존했다. 해당 include만 수정한 source2의 빌드2는 종료0이다. 실제 장치 도구의 세 함수 설명 주석을 보강한 source3의 빌드3도 종료0이며 경고/오류0이다. 최종11소스와 세 실행 파일을 `extracted/cpp-kencloud-client-424-source3/hashes.json`에 고정했다. 제품 SHA는 **e36f495c…**, 검사 **4f977376…**, 실제 장치 도구 **1eb65a41…**다.

최종 집중 검사는 **23개·실패0**이다. 새 실제 소유자6, 기존 Kencloud2/전체 팔레트2/Screen2/고정9표2/Client 색 표3/UberRoot6을 같은 검사 바이너리로 실행했다. 기존 Native246/676의 전체 fixture도 이 검사에 포함되며 수정하지 않았다.

실제 Win32 장치 검사는 **7단계·종료0·설정 복구/원본 동일**로 통과했다. 최초 cache0→Gifcloud의 한 블록/phase2→대문자 같은 이름의 모든 관측 동일→BULF의 이전 블록 반납/새 한 블록·phase4/실제 RGB 변경→800×600 장치 교체의 캐시/난수/phase 유지·Renderer 토큰 변경→Gifcloud의 새 한 블록/phase6/RGB 복귀→정상 종료를 관측했다. 이는 실제 Screen/전체 부모의 명시 API 검사다. 자연 메뉴/초기 진입·두 Early 플레이·전체 CTest·최종 보호 감사는 이어 진행한다.

같은 일반 제품의 실제 메뉴/안내/F4·목록/Back, 일반31개와 Demo3개 초기 진입, Edit TEST01 체크/TEST02 선택 예약·취소가 통과했다. 첫 플레이와 마지막 캠페인 화면을 확인했고 같은 현재 SP/Player/HUD·Kencloud 순수 관측을 대조한다. Demo는 실제 자연 timeout 뒤 플레이·로컬 AI를 확인한다. 두 Early는 상태 주입 없이 제품 타이머로 이어 검사한다.

자연 Early1은 **326.36초·종료0**으로 A~G/합법 다리·회전·한 번의 사제 이동 명령/세 번 수확·귀환/SP0→200→400→600→같은 프레임 Tutorial2 Guide4/SP10000을 통과했다. 실제 같은 SP/추적112→115를 대조했고 다음 안내 이미지를 확인했다. 상태 주입0·제품타이머1·기존 완료1 보존이다. 새 진행0→1이나 디스크 진행 저장 검사는 아니다. Early2와 전체 CTest는 이어 진행한다.

자연 Early2도 **197.20초·종료0**으로 A~I/생산·회수·환불75/SP3075→같은 프레임 Tutorial3 Guide4를 통과했다. 실제 같은 SP/추적36→42와 다음 안내 이미지를 확인했다. 상태 주입0·제품타이머1·기존 완료값 보존이며 새 진행0→1/디스크 진행 저장의 증거는 아니다. GUI 전체 종료 뒤 보호1821파일·원본/CD 목록과 고정11소스/세 실행 파일이 기준과 같은지 확인하고 전체 CTest를 시작했다.

다음 안내 캡처의 HUD 숫자는1700이고 같은 프레임의 실제 SP 저장소는10000이다. 추가 정적 대조에서 원문0043e530과 현재 `RawProductionControls::Idle`은 차이가300보다 크면 표시값에100씩 더하고, 그 뒤10/1씩 목표에 접근한다. 저장소와 점진 표시값을 같은 값으로 잘못 기록하거나 즉시10000으로 고치지 않았다. 이 관측만으로 모든 표시 타이밍의 원본 동일성을 판정하지 않는다.

전체 CTest는 **2843검사·실패check0·334.43초·종료0**으로 통과했다. [제품 보고서](../../cpppj/kencloud-client-runtime-20261025.json)는 기존 Native246/676·정적 PE 근거, 최초 빌드 실패와 최종11소스/세 실행 파일, 집중23/전체2843·실제 장치7단계·34개 초기 진입·두 Early 자연 진행의 원시 SHA를 대응한다. 최종 감사는 보호1821파일·원본/CD 목록·고정 소스/실행 파일을 종료 뒤 다시 확인하며 실제 장치 검사와 자연 플레이를 구별한다. 인계를 마친 뒤 **20261025-18 CP**로 로컬 커밋하고 push하지 않는다. 마지막 전체 정밀 디컴파일은 계속 **HJOW-Athlon / 2026-10-23**이다.

이번 단위는 Renderer의 원문 **004994b0 관문→00470c50 갱신→004711d0 표시→00470ca0 dirty 정리**, 별도 잠금 표시 경로, Native State1 전체 정리·최종 Editor 설치/편집/저장 UI를 완료하지 않는다. 기존 정적 GIF 배경을 새 동적 구름의 표시 증거로 취급하지 않는다. 전체 캠페인 자연 완주·저장/재실행과 완전 동일성은 계속 남으며 `/goal`은 활성 상태다.

GIF 해독은 기존 decoder의16MiB 물리 경계를 사용한다. 손상 GIF의 원본 SEH/범위 밖 메모리 오류 전체까지 동일하다고 판정하지 않는다. 다음 Renderer 접합은 cloud bit8/공통 모달 Gump 관문(00463fe0→00565dd8,004654c0의 토글), animating bit0x20, 같은 현재 cache/speed의 재조회, 실제 clip/lock/blit와 dirty 정리의 부분 효과를 원문에서 먼저 관측해야 한다. 모달 관문은 Guide 전용 포인터가 아니다. Frame에서 Step만 무조건 호출하는 방식으로 대신하지 않는다.
