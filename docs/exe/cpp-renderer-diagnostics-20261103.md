# 776 — 네 Renderer 진단과 같은 경로 검색 작업 배열

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 디컴파일 산출물은 수정하지 않았다. 사용자 /goal 재개 뒤 775의 후속 준비본을 현재 소스에 최소 병합했다. **단위 완료·전체 목표 진행 중이다.**

원문 Hash `00434410`, Fire `00434880`, Spot `00434a00`, Board `00434bf0`의 후단을 같은 Pool/Hash/TYPE/spot과 현재 카메라·팔레트·기존 VFX에 연결했다. 일반 프레임은 Hash → Fire → Spot → 기존 Water → Board → Present → snapshot 순서를 유지한다. 적재 중 즉시 표시는 기존 경로다. 최초 플래그는 모두 0이며 기본 UI나 양성 설정을 추가하지 않는다.

현재 진단 DWORD는 Client가 장기 소유하고 775의 명시 주소 해석기도 동일한 다섯 저장소를 읽고 쓴다. 일반키 raw7/10/13은 기존 Guide/Gump/Console 뒤·공통 정지 앞에서 현재 special/보조 DWORD를 읽는다. 옵션 재해석은 현재 Checking을 읽고 원문 `004356a6`처럼 Fire만 0으로 쓴다. 개발자키를 실제 게임에서 활성화하지 않는다.

Fire의 거리선은 기존 BFS가 실제로 초기화한 거리표만 읽는다. `RawSurfaceStorage`의 장기 작업 배열을 기존 `RawCampaignPathBindings` 검색기에 빌려 주어 Session 교체 뒤에도 마지막 거리와 미사용 큐 흔적을 보존한다. 진단이 검색을 새로 실행하거나 별도의 전술 결과를 합성하지 않는다. 검색기 자체의 복사 금지와 기존 독립 생성·이동 계약도 유지한다.

준비본 정적 검토에서 발견한 ReadOptions 닫는 중괄호와 빈 hash의 SearchCells false 기대를 교정했다. 표면 SID0 실패가 목적 좌표 일치보다 먼저라는 기존 원문/제품 순서는 바꾸지 않았다. 775의 실제 Checking과 AI 입력·글꼴 변경, 769 이후 파일 정책과 CMake 등록은 보존했다. 삼방향 병합의 줄끝 변환은 기존 추적 줄의 바이트를 복원해 최소 diff로 정리했다.

새 C++ 검사 4개는 원문 252입력의 전체 메모리 픽셀·padding·clip·효과 순서, 비활성 관문/장기 플래그/주소 해석기의 실제 저장소, 검색 배열 교체 수명, 일반키 576입력의 DWORD·정지/Console 관문을 다룬다. 기존 경로 검색과 Water·AI 진단 회귀를 함께 확인한다. 상세 결과는 [보고서](../../cpppj/renderer-diagnostics-20261103.json)와 `extracted/cpp-resume-776/`에 기록한다.

메모리 관측의 Lock/Unlock/Present와 assertion은 명시 장치 경계다. 실제 OS/GUI·물리 입력·PCM·전체 프레임/DirectDraw/32비트 포인터·SEH ABI 동일성, Lock 자체 실패 수명까지 확인했다고 주장하지 않는다. 원문 미정 메모리를 정상 결과로 만들지 않는다. 새 게임·멀티플레이·공중3종 전용 관측·포획/희생·특정 적 공격·승리/자연 완주는 실행하지 않는다. 전체 C++/34경로/ZIP의 과거 결과와 이번 집중 결과도 구별한다.

다음 단위는 777 TYPE 실시간 파일 읽기다. 이후 순서는 [재개 준비 인계](cpp-pause-handover-20261103.md)의 779/783 checking, 781/782 GIF·초기화면, 784 입력 꼬리, 785 보조 음향을 따른다. 780은 정적 scanner 감사이며 가짜 EOF·offset API를 추가하지 않는다.

Release 세 빌드의 경고/오류는0이며 집중 **21개(새4개)/실패0**이다. 최초 AI 필터4회는 등록되지 않은 실행 대상을 선택해 실행0/빈 필터 오류로 끝났고, 실제 script_bindings/world_generation 대상으로 교정해6개가 통과했다. 표시504·키1152의 새 재관측은 최초 fixture와 바이트 일치했다. 기존 보호2710개 불변·현재2712개/fixture892이며 다음 기준은 `extracted/cpp-resume-776/protected-after.json`이다. EXE SHA-256은 `87a7aca2e7bd94f7424b8f75c147e46be3cf1a843f6944d2b91f88d2ea607df3`다. 새 게임/MP/전체 C++/34/ZIP 검사는0이다.
