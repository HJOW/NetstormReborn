# cpppj 단위141 — 객체 메뉴의 실제4인자 제목·요약과 현재 Session

기준은 `0514654`다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 날짜는 **2026-10-16**이다. AGENTS·양쪽 LEFT와 깨끗한 작업 트리를 다시 확인했다. 새 디컴파일/원본 게임 실행 및 전체화면 DirectDraw 수정은 없다.

## 원본 정보 작성

`RawObjectMenuInfo`는 아래10.78 몸체를 복원한다.

- **4ad780:** `type/count/owner/sid` 네 인자를 보존한다. 설명 제목 뒤 제단(164)의 count를DWORD로 감소시킨다. 현재 class가 있고 현재 flags2에80074000이 있을 때만 음수 count를0으로 바꿔 로마숫자 레벨 제목을 작성한다. 너깃(154)의 owner는0으로 바꾸고 현재 이름·로컬 번호·방향별 동맹으로 Owner/Enemy/Ally를 선택한다. 편집 모드와 SID=-1을 구별하고 두_ftol로 좌표를 읽는다. 현재 class/theme를 원본 위치에서 다시 읽어 Alignment/Class와 마지막 줄바꿈을 작성한다.
- **4ada70:** 현재 owner/TYPE를 가상+80 호출 **전에** 보관한다. 네 인자를4ad780으로 전달하고, 그 뒤 현재 extra의 Invisible/Paralyzed 비트와 현재 그림 TYPE117/118을 읽는다. 번역·머리 작성 사이의 변경을 다음 실제 읽기에서 반영한다.
- **42a2f0:** 실제492090이 거짓이면 현재 미션/Player를 읽지 않는다. 참이면 현재594fa4, 현재 owner의Player+50 비트1 순서로 Not Connected!/Capturable!을 작성한다.

로마숫자4a8980/분리4a8800, 원소 이름4a8940, owner 이름 복사484480 및_ftol도 원본 실행으로 관측했다. 로마숫자는0~9가I~X이고 범위 밖 반환은 원본의 **FF sentinel 바이트**다. 원소 포인터 표의8개 이름을 보존하며 제목은 theme<5에서 첫5개를 번역하고 나머지는None을 사용한다. 원본 유효 버퍼/배열 계약 밖은 명시 오류이며 손상 메모리 접근을 재현하지 않는다.

## 독립 PE 관측과 입력 교정

`decomp_object_menu_info.py`는8개 실제 몸체를 격리 실행한다. 번역/순수printf·append, TYPE 설명49af80, Config 치환, GUI, 가상count, PlayerFinder841c0 및492090은 명시 경계다. 이 경계 내부를 복원했다고 주장하지 않는다. 네 인자·현재값 재조회·외부효과 순서는 원본이 선택한다. C++를 읽거나 실행해 기대 분기를 만들지 않았다.

최종 관측은 **1,434개 고유 입력·2,868회 정상 반환·1,067,162명령·53/64비트 차이0**이다. 실제 진입은4ad780 2,804회,4ada70 436회,42a2f0 452회,4a8980/4a8800 각1,256회,4a8940 1,264회,484480 1,416회,_ftol 80회다. 모든 외부 호출에서 보존GP/스택/canary/x87 제어·스택을 확인했다.

초기 도구의 번역 경계 ret4와 로마숫자 표의 마지막1바이트 쓰기 범위를 바로잡았다. 첫 완성 관측과 C++ 재생 사이에는 포획 상태 **49건**이 달랐다. 실제 명령 **42a318 `mov edx,[59534c]`**는 Player 배열의 포인터를 읽지만, 첫 관측은59534c 자체를 배열처럼 채웠다. 별도 Player9×180 영역을 만들고 포인터를 설치한 뒤 원본을 다시 실행했다. 이 교정에서 **C++의 포획 분기나 기대 문구는 바꾸지 않았다.** 해당8몸체에는NULL 페이지 데이터 읽기가 없으므로 이제 그런 읽기는 즉시 거부한다.

수정 전 새 fixture/보고서/도구는 `extracted/goal-object-menu-info-20261017-before-player-pointer/`에 보존했다. 기존 프로젝트 fixture는 수정하지 않았다. 최종 `object-menu-info.json`은 Player 포인터/배열 배치와NULL 읽기 거부를 기록하고 `--verify`가 이를 확인한다. C++ 재생은1,434입력의 효과 순서·문자열 바이트·현재 raw/전역/class/TYPE를 모두 통과했다.

## 같은 Session과 실제 controlled 제목

`RawCampaignObjectMenus`가 정보 몸체와 class 보충표를 같은 고정 수명으로 소유한다. 기존 RiftTypeRecord에 빠진+aa만 실제 GameAssets 정의에서23바이트 계약으로 보충하며 TYPE/flags/theme를 새로 복제하지 않는다. 가상count는 회수 메뉴와 동일한 실제VT 검증·가상80을 공유한다. 미연결된 두 파생count를 기본 숫자로 대체하지 않는다.

새 `RawPlayerIdentity.NameForOwner`는 mode0의 owner 입력으로 기존 Resolve를 실행한다. subscriber 입력과 구별하며 부분완료의 이름을 성공으로 승격하지 않는다. 정보는 이 같은 Identity, 현재 로컬/편집/미션, 방향별81동맹, 같은 Player9×180의+50, 실제 GroundImmobile492090을 읽는다. 이름·동맹·Player의 별도 사본이나 가짜 로그인 번호를 추가하지 않았다.

기존 단위140의 Form 부착 검사는 명시 제목을 제거하고 **같은 실제 Summary→controlled 제목/머리→사제+a8→실제 Form 부착→About 원본 입력**으로 진행한다. 새 Session 검사는 명시 peer22의 현재 이름/동맹 변경과 subscriber0의 부분 Resolve 실패(SOMEONE), 명시 저HP 입력의 실제492090→Capturable!을 확인한다. peer22와 저HP는 검사 입력이며 로그인/자연 피해·전투를 검증한 것은 아니다. Config 치환은 같은 보호VFS를 읽는 실제 ConfigInterface로 수행한다.

설명49af80·영어 번역/printf는 이 headless UI가 제공하는 명시 경계다. 특히 **실제 Client의 완전한 설명/번역·DWORD 명령 본문·자연 우클릭 진입은 아직 연결하지 않았다.** 원본4ad780 자체와 외부 경계를 구별한다.

## 검증 상태와 다음

Release 빌드1·2·3·4는 모두 종료0·경고0·오류0이다. 최종 집중은 ObjectMenuInfo2개, CampaignObjectMenus7개가 전부 통과했다. **전체 CTest는1,574개·실패0·종료0·208.36초**다. 로그 접두부는 `extracted/goal-object-menu-info-20261017-`다.

최종 원본 근거 감사59종은 **48종 성공·기존11종 실패·새 실패0**이며 기존11종의 지점은 단위138 기록을 따른다. 직전 기준과 같던 **기존29보고서·현재82핀**만 갱신하고 각 integration_updates에 이전 값을 보존했다. 기존 fixture 및 원본/AGENTS/DN은 바꾸지 않았다. CMake 두 파일의 기존 바이트 접두를 보존했고 보호SHA3·UTF-8/LF도 확인했다. 연결 보고서는 `cpppj/campaign-object-menu-info.json`이다.

최신 실행 파일의 클론 창 진입 **5회·실패0·originals_unchanged=true**다. 1-1/1-2는 브리핑→Play Mission→월드, tutorial1/2/3은 브리핑 범위다. 실제 Client가 새 객체 메뉴를 자연 우클릭으로 여는 검증이나 사용자 자연 완주로 확대하지 않는다.

CD10.72의 기존 정밀 C 대응은 제목4ae8f0·요약4aeb30·포획4e80d0·owner 이름401920이다. 이번 CPU 실행과 새 정보 몸체는10.78 전용이며 CD를 거부한다. 공통 Identity에 owner 이름 입구를 추가했지만 새 CD 원본 관측으로 확대하지 않는다.

다음은 실제49af80 설명·현재 Client 번역/설정/정적 문자열 수명, VT별+a8/+ac와원본DWORD 명령 소비자, 게임 시간을 정지하지 않는 우클릭 입구다. 로그인된 로컬 전송 양끝/큐잉·등록 수명, 생산·경제·AI/전체Kernel 및 대상11스테이지 자연 완주·승패·저장·다음 미션·전체 동일성은 계속 미완료다.
