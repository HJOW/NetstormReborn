# Session의 실제 Fiery Tail·화면 연결 — 단위102

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 새 디컴파일이나 원본 게임·GUI 실행은 하지 않았다. 기존 생성/프레임/공통 Host 몸체를 실제 Session과 Renderer에 연결한 작업이다. 전체 목표나 사용자 자연 완주가 완료됐다는 기록은 아니다.

## 실제 동작 연결

`RawCampaignParticles`는 Session과 같은 raw 풀·Host·Kernel·게임 난수를 사용한다. 썬 캐논의 `flightEffect(11,parent)`는 실제 `FieryTailVisualSystem`을 만들며 기존 **표시 자산 Pop→effect11 부착→Missile 부착** 순서를 유지한다. 프레임마다 같은 게임 시각을 전달하고 최초10입자·후속 시간 조각·수명·원본 공유11색 계산을 실행한다. 다른 미복원 효과를 빈 성공으로 처리하지 않는다.

공통 원본 collector의 현재 부모 x/y+0.001·깊이-25로 `RenderProcedure`를 제출한다. GameWorld는 이를 SHP와 같은 전체 Renderer 목록에 넣는다. 부분 갱신과 대화상자 배경용 `SceneImage`도 같은 Draw를 사용한다. Draw는 현재 raw 부모의 실제 좌표 변환(`SoundToScreen`, 원본 `00497220`)과 현재 카메라를 읽고 `DrawVfxLine`으로 잠금된8비트 표면에 선을 쓴다. 종류별 후처리 overlay가 아니다. 장치 연결 전 dirty는 기존100항목/전체 갱신 규칙으로 보관하며 실제 Renderer 연결 뒤 전달한다. 해제된 토큰의 지연 Draw는 아무 객체도 역참조하지 않는다.

## 실제 색 메모리와 한계

원본 mode1의 `HeapAlloc(heap,0,44)` 분기를 따라 `ParticlePaletteMemory`가 실제 Win32 heap에서11DWORD만 요청하고 앞11색만 쓴다. `ReadProcessMemory(GetCurrentProcess(),...)`로 그 배열과 바로 뒤 DWORD를 복사한다. Draw마다 갱신하므로 C++ 배열 범위 밖 역참조나 임의의12번째 색을 만들지 않는다.

단위101의 원본 데모 실행에서 인접 DWORD가1이었던 사실은 그대로 보존한다. **새64비트 프로세스의 할당 배치·heap 내용은 원본32비트 실행과 같다고 보장할 수 없다.** 이번 구현은 실제 할당/읽기 경계를 제공한 것이며 최초 최대 수명 색의 매 실행 동일성을 증명한 것이 아니다. 고정1/0·끝색 clamp로 이 차이를 숨기지 않는다. [원본 한 실행의 관찰 범위](cpp-fierytail-live-heap-20261017.md)를 따른다.

## 독립 원본 픽셀 근거

`tools/decomp_fierytail_pixels.py`는 실제 **`004583a0 → 004dade0 → 004011f1`**과 색 조회/`__ftol`을 대체 없이 실행한다. 이전 프레임 Oracle의 선 요청 경계를 실제 wrapper·VFX 몸체로 확장했다.32비트 flat GDT/ES·스택과 원본 pane/surface 구조를 설정하고,64×48 메모리 표면의 전체3072바이트를 읽는다. wrapper의 원점 감산과 pane의 원점 처리를 모두 실행한다.

- 클립5종×끝점/카메라/offset6종의 **고유30입력**, 두 정밀도 **Draw60회 정상 반환**, 화면 차이0이다.
- 실제 VFX 선/화면 wrapper/색 조회는 각각600회, `__ftol`은2400회 실행했다.
- 반환 주소·스택 purge·비휘발 레지스터·x87 균형, 객체/입자/색 불변과 픽셀 쓰기 범위·앞뒤 보호 바이트를 검사했다.
- `0xabcdef01`은 최대 수명 조회를 구별하는 **명시 인접 입력**이다. 실제 게임 heap 값으로 세지 않는다. 입력을 만들 뿐 Python으로 픽셀 기대값을 계산하지 않는다.
- 결과는 `cpppj/fierytail-pixels.json` 및 `tests/fixtures/fierytail-pixels-x86.tsv`에 보존한다. `python tools/decomp_fierytail_pixels.py --verify`가 전체 SHA/입력 고유성을 확인한다.

## 검증과 남은 경계

첫 Release 전체 증분 빌드는 종료0으로 끝났다. Renderer 공통 헤더를 읽는 검사들도 다시 컴파일했다. **빌드 종료 전 `demo-first.log`는 이전 실행 파일 결과여서 제외**했다. 새 실행 파일의 데모3 검사는 기존113프레임/2.825초의 TYPE128 effect11을 통과하고 **145프레임/3.625초의 TYPE131 `Missile 미복원 반사/부모/포구 생성 경로`**에서 멈췄다. 빈 효과로 넘기거나 검사 기대값을 바꾼 결과가 아니다.

최종 Release 전체 빌드는 **경고0·오류0·종료0**이며 새 집중 **5개가 통과**했다. 원본30행의 전체 픽셀 대조, 실제 OS 할당/인접 읽기, Renderer 깊이/clip/pitch padding/삭제 잔상, 같은 Host의 실제 VFX 픽셀/카메라/삭제, 보호 데모의 자연 발사/부착 순서/실제 픽셀을 확인했다. 첫 통합 검사는 월드의 연결 타입 입력을 빠뜨렸으므로 실제 PATCH 번호를 공급해 고쳤다. 제품/원본 기대값은 바꾸지 않았다.

전체 CTest 내부는 **1429개 중1427통과·기존 두 검사 실패,222.44초,종료8**이다. 데모1 TYPE148 stage5는 같은145프레임이며 데모3은 위 TYPE131 생성 경계로 진행했다. 새 실패는 없고9미션의400프레임 검사는 통과했다. 빌드/검사 프로세스는 모두 종료했다. 로그 접두사는 `extracted/goal-campaign-particles-20261017-`다.

새 픽셀30입력·기존 프레임65/생성31/삭제48·실제 heap1257읽기·필요 에너지2352행의 SHA/고유 입력 감사가 통과했다. 공유 client/tests CMake의 실제 변경 전후 유효 핀/이력만 갱신했으며 기존 TYPE 제품·원본2352행은 보존했다. 문서/인수인계 뒤 **`20261017-02 CP 캠페인 Fiery Tail과 실제 Renderer 픽셀 연결`**로 로컬 커밋한다.

다음 구현은 데모3 TYPE131 투사체 생성, 데모1 TYPE148 stage5다. 전체 GUI/입력·AI 전략·건설/경제·승패/저장/다음 미션과 사용자 자연 완주·원본 전체 동일성은 남아 있다. 전체화면 DirectDraw는 요청대로 이번1차 범위에서 제외한다. AGENTS.md·보호 원본·dotnetpj는 수정하지 않으며 원격 push는 하지 않는다.
