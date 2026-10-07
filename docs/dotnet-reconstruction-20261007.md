# dotnetpj 자료 계층 복원과 x86 대조 — 2026-10-07

사용자 요청으로 `cpppj`와 병행하여 dotnetpj 계획의 1~2단계를 진행했다. 기준은 원본 10.78이다. `cpppj/`·AGENTS.md·원본 자료는 수정하지 않았고 게임·클론 창을 실행하지 않았다. 작업 PC는 Debian 13의 `vm-debian-codex`, SDK는 .NET 10.0.401이다.

## 이 PC의 디컴파일 상태

문서의 최신 Windows 작업 결과와 이 PC의 로컬 자료는 다르다. 아래 수치는 `.c`의 내보낸 함수 머리말 수이며 의미 복원률을 뜻하지 않는다.

| 자료 | 로컬 함수 머리말 |
|---|---:|
| `extracted/decomp/Netstorm.c` (10.78) | 4,506 |
| `extracted/originalCD/decomp/NETSTORM.c` | 3,711 |
| `extracted/original1037/decomp/netstorm.c` | 3,711 |
| `extracted/original1062/decomp/Netstorm.c` | 3,729 |
| `extracted/original1082/decomp/netstorm.c` (V10) | 10,130 |
| `extracted/original1082/launcher/decomp/Netstorm.c` | 2,618 |
| `extracted/patch1062/decomp/zpatch.c` | 148 |

`extracted/refined/`와 `extracted/original1082v12/`는 없다. 원본 V12 파일 자체는 있다. 최신 SID·Graph 복원 등을 다시 내보낸 전용 디렉터리도 이 PC에는 없다. 이번 범위는 이미 커밋된 C++ 복원 소스·분석 문서·x86 TSV로 검증할 수 있어 추가 디컴파일을 실행하지 않았다.

## 원본 기계어 기대값 연결

TSV는 두 테스트 프로젝트에서 `cpppj/tests/fixtures/` 원본 파일을 링크한다. `tests/Shared/X86Fixture.cs`를 소스로 공유한다. 원본 실행 파일·Ghidra·Python 없이 회귀 테스트를 실행할 수 있으며, 자료 누락은 실패로 보고한다.

| 입력 묶음 | 입력 수 | 수정 전 불일치 | 수정 후 |
|---|---:|---:|---:|
| 다리 `Draw` (패치판 열) | 10,005 | 4 | 0 |
| 전역 `Random` 반환값·상태 | 1,056 | 0 | 0 |
| 다리·영역 `Decode` 전체 목록 | 752 | 최초에 미연결 | 0 |
| `FrameFind*` | 1,536 | 42 | 0 |
| 설정 `Subst` | 495 | 285 | 0 |
| 설정 `Get` | 77 | 9 | 0 |
| `options` 저장 | 16 | 최초에 미연결 | 0 |
| 원시 설정 `Config` | 270 | 최초에 미연결 | 0 |
| 설정 `Section` | 408 | 최초에 미연결 | 0 |
| **합계** | **14,615** | | **0** |

수정 전 값은 새 대조 테스트를 기존 C# 구현에 적용한 결과다. 당시 프레임 검색의 넓은 마스크·음수 인자는 기존 byte enum API로 전달했다. 버퍼 없는 객체·환경 변수 문맥을 아직 표현하지 못한 설정 API도 기존 호출로 검사했다. 그 뒤 원본 문맥을 표현하는 API를 추가하고 최종 대조했다.

## 변경 내용

설정의 미등록 키는 `{KEY}`, 미등록 미션 키는 `{@MISSION}`으로 남긴다. 점 없는 `@키`는 빈 값이다. 환경 변수는 명시적으로 허용한 출발 객체와 공급자에서만 읽는다. 빈 버퍼와 버퍼 없는 객체를 구분하고 LF·CR·NUL·ASCII 대문자 처리도 원본 규칙을 따른다. 브리핑·안내 본문은 ConfigStore를 쓰므로 미등록 문구도 바뀐다. 기존 미션 치환과 한국어 선택·영어 대체 회귀는 통과했다.

설정 층은 기본값 → 영어 보충 → 선택 언어 용어표 → 전역 options/setup → 명시 언어 순서다. 실제 setup와 영어·프랑스어·독일어·스페인어 용어표의 키 교집합은 0개라 배포 자료의 값은 바뀌지 않는다. 겹치는 키를 넣은 합성 자료로 전역 우선순위를 확인했다. 명시 언어 층은 한국어 선택을 유지하는 클론 기능이다.

`ConfigFile.EncodeSections`는 파일 이름 섹션과 END를 원본 저장 형식으로 인코딩하는 별도 API다. 기존 `Decode`의 서명 제거·`Set`의 첫 줄 교체·`Encode`의 단일 파일 본문 인코딩은 analyzeManager의 복사본 편집을 위해 유지했다. 클론은 InstallDir을 읽지 않고 VFS 루트를 사용하며 options.cfg를 쓰지 않는다.

타입의 group 비트·HP/사용량 비트·container/목록 플래그·배치/표시 후처리를 반영했다. 모든 emplacement는 dropBlocking이므로 다리 부착 금지 칸에도 반영된다. 긴 자산 이름은 이름 앞 20바이트와 설명 필드를 이어 해시하며 `fakeThreeByThreeSurface`의 원본 해시 `0534a54b`를 확인했다. zorder·float 비용·level−1도 제공한다. 속성의 중복·별칭 순서 보존 목록을 추가했다.

별도의 정적 검증으로 C++ `RiftTypeTable`의 자산 타입 **116개**에 대해 두 플래그·zorder·비용·level·목록 플래그·이름 해시가 모두 같음을 확인했다. 이 결과는 x86 직접 실행 기대값과 구분하며 `type-metadata-1078.tsv`에 입력 아카이브·C++ 소스 SHA를 남겼다. 내장·프로세스·빈 번호를 포함한 전체 188개 타입 표는 SID/월드 연결 때 추가한다.

요새의 부호 있는 섹션 길이, 버전 0의 상태 바이트, 첫 파일 번호 중심 TypeNames 변환, 내용물의 항목 타입 → 그릇 타입 → 파일 바이트 순서를 정정했다. 변환 결과 0인 Deck 카드는 제외하고 원시 섹션은 보존한다. 저장 소유자 정규화 함수는 추가했지만 실제 미션 세션에 적용하는 시점은 후속이다. 기존 **465개 요새** 전수 판독과 합성 경계 입력을 검사했다.

다리 26개·영역 68개의 CanonDecoder를 제공하고, 원본 x86의 모든 회전·누락 프레임·단정밀도 좌표 비트·라벨·방향 목록에 대조했다. 기존 게임의 `BridgePiece.Cells`도 다리 입력에 직접 대조한다. 영역 JSON의 기존 64개 모양은 그대로 유지하고, 변형·라벨과 표 뒤의 4개 모양을 추가했다. `.fort`의 6비트 Shape 규칙은 유지한다.

SHP 프레임 앞의 **36바이트 Squid 추가 헤더**를 별도 API로 읽는다. 표시 폭·높이·hotspot은 signed short이며 일반 VFX 디코딩에는 이 헤더를 가정하지 않는다. 추가 헤더 없는 순수 VFX의 이미지 해독도 검사했다.

| 10.78의 3,783개 프레임 참조에서 비교한 항목 | VFX 내부 u16 값과 다른 참조 수 |
|---|---:|
| 표시 폭 | 3,711 |
| 표시 높이 | 3,697 |
| hotspot X | 3,388 |
| hotspot Y | 3,434 |

첫 프레임만 보아도 내부 `(15,9,14,4)`와 표시 헤더 `(11,17,12,16)`이 다르다. 따라서 표시 경계·클릭 판정은 별도 헤더로 전환해야 한다. 이번에는 자료 판독·회귀까지 완료했으며 뷰어 적용은 계획 5-2에 남긴다.

## 재현

```bash
cd dotnetpj
dotnet build -c Release
dotnet test -c Release
dotnet test -c Release --filter 'FullyQualifiedName~X86'
```

정적 C++ 메타데이터와 영역 JSON을 다시 생성할 때만 Python·g++가 필요하다. 임시 폴더에서 순수 C++ 자료 판독기만 빌드·실행하며 게임이나 창을 시작하지 않는다.

```bash
python3 dotnetpj/tools/export_cpp_reference.py
```

기준선은 Assets 262 + Core 541 = **803개 통과**였다. 최종 Release 빌드는 **경고 0·오류 0**, 전체 테스트는 Assets 281 + Core 544 = **825개 통과·실패/스킵 0**이다. 기존 CA2014 경고는 테스트의 stackalloc을 루프 밖으로 옮겨 해소했다. 보호된 원본 7개 디렉터리의 **2,782개 파일 SHA·목록이 그대로**이며 수정·삭제·추가 파일은 없다. cpppj/·AGENTS.md·LEFT_JOBS.md 변경 없음.

## 다음 범위

계획 3단계의 객체 단위 연결·이웃·열린 방향·수명·붕괴 방문 목록을 원본 기대값에 맞춘다. 아직 미복원인 섬 객체 등록·SID 월드 연결·다리 배치·붕괴 스캔·전투·AI는 임의로 교체하지 않는다. 화면 순서·표시 경계 적용·영어 비트맵 글꼴·커서는 별도 후속이며, 한국어에는 계속 D2Coding을 쓴다.
