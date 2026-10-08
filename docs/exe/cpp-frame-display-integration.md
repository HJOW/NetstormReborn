# Squid 프레임 변경·표시 갱신·실제 SHP 크기 통합

2026-10-08, **VM-W11-CODEX**, 기준 10.78. 마지막 디컴파일 수행 PC는 **VM-W11-CODEX, 2026-10-08**다. 이번에는 같은 PC의 기존 `setframe`·`graphremove`·`display` 내보내기를 읽었다. 새 디컴파일·원본 게임/복사본·클론 창 실행은 없었다.

[SquidFrameBinding](../../cpppj/src/o/SquidFrameBinding.h)의 `MakeSquidFrameHooks`가 프레임 지정의 외부 효과를 실제 `SquidDisplay`·`SquidUnpop`·`SquidPop`으로 연결한다. SHP 추가 헤더는 기존 `ShapeDatabase::SquidMetrics`로 읽는다. [SquidRenderer::Shapes](../../cpppj/src/client/SquidRenderer.cpp)가 픽셀 크기/hotspot과 함께 칸 단위 크기 두 float을 `SquidDisplayFrame`에 보존한다.

## 연결한 동작

- `frameSize`: `SquidDisplay::FrameSize`가 현재 프레임의 SHP 추가 헤더 float 둘을 공급한다. 픽셀 크기나 타입 발자국으로 대체하지 않는다.
- `update`: 실제 `SquidDisplay::Update`를 부른다. 같은 단계에서는 old 영역 갱신→프레임 쓰기→new 영역 갱신이다. 같은 프레임 지정도 두 번 갱신한다.
- `unpop/pop`: 실제 일반 공간 모듈을 호출한다. 각각 옛 영역/새 영역의 Update를 수행하며 Pop은 새 현재 프레임의 실제 SHP 칸 크기로 해시 단계를 정한다.
- 연결 검증: 판본·타입 플래그/발자국/유효성 값, 같은 SID 풀·표시 대상·해시·spot 배열을 구성 때 확인한다. 표시가 꺼진 공간 모듈과 표시가 켜진 프레임 갱신의 혼용을 거부한다.

원본 프레임 지정의 단계 판단은 그대로다. 새 프레임이 아니라 **현재 프레임**의 크기로 저장된 단계와 비교한다. 원본과 같이 단계 바이트만 고치거나 다음 변경에서 공간 재등록이 일어날 수 있다. GUI의 애니메이션 스케줄을 새로 구현한 것은 아니다.

패치의 `00419850`은 logical 클러스터 수, `flags1 & 0x440000`의 두 배 범위, SHP 존재, 두 해제 흔적 값을 검사한다. 기존 표시의 `PatchFrameValid`를 크기 공급에도 사용했다. CD는 logical 범위/해제 흔적 검사를 추가하지 않으며, 두 판본 모두 호스트의 물리 프레임 배열 밖·없는 SHP 조회는 예외로 거부한다. 원본의 표 밖/널 메모리 접근을 실행하지 않는다.

## 콘솔 통합 검사

[FrameDisplayTests](../../cpppj/tests/FrameDisplayTests.cpp)의 새 5개 검사는 원본 파일 없이 실행된다.

1. 10.78/CD 실제 Factory·프레임 훅·표시 계산·Renderer 변경 표·Pop/Unpop으로 old/new 픽셀 영역과 해시 단계 이동을 확인한다. 합성 입력의 픽셀 크기와 칸 크기를 서로 다르게 두어 공급자를 혼동하면 실패한다.
2. 그림자 old/new 순서, 선택 표식의 판본별 확장량, `0x2000` 공통 표시 경로, 같은 프레임 반복, Renderer 전체 갱신 억제를 확인한다.
3. 잘못된 현재 프레임·없는 SHP는 단계/해시/영역 쓰기 전에 거부한다.
4. 다른 풀/판본/타입 내용·표시 미연결·다른 해시/spot 조합을 구성 때 거부한다.
5. 패치 logical 검사와 CD 물리 범위 보호, shadow 두 배 범위, 해제 흔적의 판본 차이를 확인한다.

표시 검사는 변경 영역을 실제 소프트웨어 Draw/Present로 넘기고 영역 안/밖 픽셀을 대조한다. GUI 월드의 스프라이트 목록을 raw 프레임과 동기화한 검사는 아니다. 최초 검사에서 선택 표식 old 영역의 좌우 기대값을 잘못 더한 2개 확인을 수정했다.

## 실제 자산 읽기 전용 검사

새 명령 `--inspect-frame-binding`은 창을 만들지 않고 실제 자산을 읽는다. 전체 SHP 물리 프레임의 pixel 크기/hotspot·칸 크기 float 비트가 표시 목록에 그대로 보존되는지 확인한다. 원본 logical 범위 안의 프레임은 `FrameSize` 공급 값도 대조한다. 실제 타입 155 연결 객체를 생성/Pop하고 프레임 변경 및 같은 프레임의 alternate 갱신을 실제 모듈로 수행한다.

| 판본 | 자산 타입 | 전체 물리 프레임 | FrameSize 조회 | 영역 전달 | 부분 Draw 영역 |
|---|---:|---:|---:|---:|---:|
| 10.78 | 116 | 3,783 | 3,779 | 4 | 1 |
| CD | 101 | 3,167 | 3,167 | 4 | 1 |

전체 물리 프레임 **6,950개**, 크기 공급 조회 **6,946개**다. 패치의 logical 범위 밖인 물리 프레임 4개는 메타데이터 보존만 확인하고 FrameSize 호출에서 제외했다. 이 검사는 같은 원본 자료를 기존 로더와 새 공급 경로로 전달한 C++ 통합 확인이며 새 독립 기계어 기대값이 아니다. 누적 제한 x86 **131,542개**는 그대로다.

```powershell
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
cpppj/build/bin/Release/NetstormCpp.exe --inspect-frame-binding originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-frame-binding originalCD --cd
```

최종 x64 Release **경고/오류 0**, 전체 콘솔 CTest **251개·실패 0**(기존 246 + 새 5, 테스트 57.48초·CTest 전체 57.52초)다. 관련 원본 근거 감사 `display`·`setframe`·`pop`·`unpop` **4종 모두 통과**했다. 로그는 `extracted/build-frame-display.log`, `extracted/ctest-frame-display.log`, `extracted/frame-binding-originals.log`, `extracted/frame-binding-cd.log`, `extracted/audit-frame-display.json`(Git 제외)이다.

## 다음 구현과 제한

다음은 Graph 활성 상태에서 끝 칸 생성/삭제/Pop 및 받침 생성을 같은 raw 흐름으로 통합하는 일이다. 이후 raw GameWorld/GUI와 Kernel 프레임·게임 시각을 연결한다. 프레임 지정에 쓰는 애니메이션 프로세스, 파생 표시 override, 실제 walker 낙하·파편·소리·건설·경제·전투·승패는 후속이다. 이번 통합 검사는 Graph 비활성이고 원본 파일을 수정하지 않았다.

기존 근거: [프레임 지정](cpp-setframe-reconstruction.md), [공통 표시](cpp-display-reconstruction.md), [섬 삭제 통합](cpp-surface-lifecycle-integration.md).
