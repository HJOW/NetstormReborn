# 단위468: 실제 writable Fort namespace와 목록 재진입

2026-10-27 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 정밀 export는 변경하지 않았다. Source1의 잘못된 합성 목록 설계를 수정하여 실제 제품에 연결했다. 최종 검증과 인계는 [통합 문서](cpp-editor-input-runtime-20261027.md)에 기록한다.

## Source1에서 잘못 정한 기대

Source1의 원본 disk 목록→신규 work 이름 append는 이미50개 수락되는 실제 기본 폴더에서 새 이름을 언제나 뒤에 두었다. 이는 저장한 이름이 단일 실제 디렉터리의 OS순서상 앞50개에 들어갈 가능성까지 없앤다. 또한 원본 `004cafd0`/현재 `RawSoloMapList`는 `Same.fort`와 `Same.extra.fort`를 동일한 `Same` 두 행으로 유지하는데, 새 host NameKey dedup은 원본끼리도 합쳤다. 이번 미커밋 Source1의 설계/신규 검사 기대 오류이며 기존 baseline 검사나 원본 증거를 정정하는 작업이 아니다.

원문 FileSpec은 디렉터리 속성도 제외하지 않는다. 와일드카드가 없는 부재 이름도 존재 확인 없이1경로를 반환한다. Source1 `EditorFiles::MatchDisk`의 regular-file 필터/부재 명시 이름 제거도 이번 새 API의 오류이므로 수정 대상이다.

## 최종 의미

1. 명시한 source root 아래 loose Fort의 **물리 상대 경로**를 동일 work 상대 경로에 한 번 이관한다. `d/Same.fort`와 `d/Same.extra.fort`는 서로 다른 이관 항목이다.
2. 이미 있는 작업 파일은 내용·속성·시각을 변경하지 않는다. 원본 복사는 Windows `CopyFileW(source,target,TRUE)`로 기존 대상 덮어쓰기를 금지하고 원본 readonly 속성을 보존한다. 새 복사본의 실제 생성 순서가 source와 같다고 주장하지 않는다.
3. 이후 목록 입력은 **work 폴더 자체의 실제 Win32 FindFirst/Next 결과만**이다. 정렬·source 합성·논리 이름 dedup·새 이름 우선순위가 없다. 원문 첫점 제거와 종류 필터/수락50개는 기존 SoloMapCatalog/RawSoloMapList가 수행한다.
4. source wildcard가 반환한 `*.fort` 디렉터리는 work에 대응 빈 디렉터리만 만든다. 재귀 복사는 하지 않는다. 목록에서 숨기지 않으며 후속 파일 Open 실패도 기존 reader가 처리한다. 이미 대응 파일/디렉터리가 있으면 원본 종류를 강제로 덮어씌우지 않는다.
5. 이관 완료된 물리 경로가 없어졌다면 **Native 삭제/사용자 삭제 상태**다. 원본 재복사나 원본 disk/TAFF fallback으로 부활시키지 않는다. 명시한 부재 source 패턴은 이관 항목으로 기록하지 않고 FileSpec이 돌려준1경로의 후속 부재 처리로 넘긴다.

새 저장 이름은 그 실제 work OS 위치가 수락50개 안이면 보이고 밖이면 원본처럼 보이지 않는다. 새 이름을 항상 표시하는 기능이나 전체 목록 확장은 추가하지 않는다.

## 장기 장치와 필요한 API

`EditorCatalogWorkspace` 한 개를 UberGump의 장기 owner로 둔다. 실제 `EditorFiles`와 source root를 명시적으로 소유/대여하고, Catalog의 두 callback 및 EditorRuntime/현재 Fort 경로가 같은 owner를 빌린다. 현재 지역 capture만으로 둔 catalogFiles와 별도 EditorRuntime 파일 정책을 분리해 두지 않는다.

| API | 역할 |
| --- | --- |
| `Resolve(configuredPath)` | 현재 설정 경로를 검사된 work 절대 경로와 장부 물리 상대 키로 변환. source/work 절대 경로는 명시 root 아래 전체 경로로 역매핑하며 basename으로 추측하지 않음 |
| `ImportDisk(pattern)` | source FileSpec을 현재 조회하고 아직 기록하지 않은 실제 loose 파일/디렉터리만 한 번 이관. 명시 부재 패턴은 기록하지 않음 |
| `Prepare(configuredPath)` | 목록을 거치지 않은 직접 기존 Fort 설치도 같은 이관/tombstone 정책으로 준비 |
| `Enumerate(pattern)` | ImportDisk 후 work MatchDisk만 반환. 디렉터리/중복/부재 명시1경로를 그대로 유지 |
| `Read(configuredPath)` | 매번 현재 Resolve/장부를 확인하고 work 읽기. 완료키는 missing이어도 source fallback 금지 |
| `Files()` | 기존 Native 권한/쓰기/삭제 부모가 쓰는 동일 EditorFiles 대여 |
| `EditorFiles::RootPath/RelativePath` | 기존 canonical 보호 검사와 함께 명시 root의 전체 상대 경로를 얻는 좁은 장치. 외부 경로를 basename으로 바꾸지 않음 |
| `EditorFiles::SiblingNamespace("-state")` | 같은 보호 roots와 work root 자체를 보호 목록에 추가한 별도 metadata namespace 생성. 생성만으로 파일 쓰기 없음 |
| `EditorFiles::CopyMissing/WriteAtomic` | 검사된 work 대상의 fail-if-exists 복사/디렉터리 생성 및 metadata 임시파일→flush→교체. 어느 API도 원본을 수정하지 않음 |

`EditorFortFileDevices::PrepareCurrent`에는 선택적 실제 prepare callback을 끝에 추가하여 standalone 기본 동작/기존 baseline 검사 본문은 유지한다. 실제 Uber는 callback으로 같은 workspace.Prepare를 공급한다. `UberEditor::InstallExistingEditorState`의 현재 `source.TryRead→PreparePath` lazy 복사도 같은 Prepare로 교체한다. 둘 중 하나를 남기면 삭제 원본이 직접 Edit 재진입에서 부활한다.

Save as의 configured→work 경로, FortCurrentNameDevices의 old/new 경로도 같은 Resolve 정책을 빌려야 source 절대 fortSpec을 쓰는 경우에도 Native snapshot/readonly/old 삭제가 같은 namespace에 적용된다. 기존 현재 키/이름은 각 원문 callback 시점에 재조회하며 Refresh 시작 시 캡처하지 않는다.

## metadata/장부 범위

장부는 **`<work>-state` 형제 디렉터리** 안에 둔다. `d/*.fort` 밖에 두는 것만으로는 fortSpec=`*`에서 노출되므로 work root 내부에는 기록하지 않는다. metadata 장치도 모든 원본/CD/work root와 외부 링크를 검사한다. metadata root가 work/source로 향하는 junction이면 파일 쓰기 전에 거부한다.

장부는 버전·canonical source root identity·canonical work root identity와 각 **전체 physical relative 경로**의 상태를 기록한다. first-dot MapName은 사용하지 않는다. `./`·`..` 정규화/ASCII 대소문자와 insideSource absolute/insideWork absolute 표기를 같은 물리 키로 다룬다. `Same.extra.fort`는 `Same.fort`와 합치지 않는다. 각 실제 경로의 canonical 대상은 source/work root와 보호 경계 안이어야 하며 외부/보호 reparse는 거부한다.

source가 바뀌어도 이미 완료된 같은 work 항목을 자동 초기화하지 않는다. root identity가 다른 장부를 발견하면 자동 재이관하지 않고 명시 오류로 보고한다. 원본 파일이 갱신되어도 완료 항목을 덮어쓰지 않는다. 새로운 source physical 경로는 새 항목으로 추가할 수 있다.

## durable 상태와 부분 실패

각 항목은 `Pending`과 `Completed`를 구분한다. 매 durable 갱신은 metadata의 검증된 임시파일에 전체 새 장부를 쓰고 FlushFileBuffers 후 같은 metadata 내 atomic replace/write-through를 수행한다. 잘린/손상/버전 불일치 장부는 빈 장부로 취급하지 않는다. 한 프로세스의 같은 workspace가 동기 직렬화하고 다른 프로세스의 이관은 잠금/동시 열기 실패로 명시 거부한다.

| 시작 상태 | 처리 |
| --- | --- |
| 미기록 + work 존재 | 기존 work를 보존하고 Completed를 durable 기록한 뒤 성공. 이후 Native rename이 지워도 재이관 금지 |
| 미기록 + 실제 source 존재 + work 부재 | Pending durable→CopyFileW fail-if-exists 또는 빈 directory 생성→Completed durable. source 종류/내용을 성공으로 합성하지 않음 |
| 미기록 + 명시 source 부재 | 장부 변경 없음. 원문 FileSpec의1경로/후속 reader 부재 처리 |
| Completed + work 존재 | 현재 work만 사용. 원본을 읽거나 다시 복사하지 않음 |
| Completed + work 부재 | 삭제 상태 유지. 목록은 실제 work에 없는 항목을 합성하지 않고 reader는 부재 반환 |
| 재시작 Pending + work 존재 | 이미 복사/기존 충돌 대상이 존재하므로 그대로 보존 후 Completed durable 가능. 내용 덮어쓰기 금지 |
| 재시작 Pending + work 부재 | 이관 중단과 이후 삭제를 구별할 수 없어 자동 재복사하지 않음. 정확한 항목의 중단 오류로 남김 |
| 복사/장부 쓰기/flush/rename 실패 | 앞선 파일/장부 효과를 보존하고 명시 실패. 성공/빈 목록/새 장부로 대체하지 않음 |

Pending을 쓰기 전에 source가 없어졌다면 명시 부재로 취급할 수 있지만, Pending 뒤 실패는 위 부분 실패로 남긴다. Completed 갱신이 실패해도 같은 실행에서 Native 저장/rename을 계속 허용하지 않는다. 새 이름을 사용자가 명시 Save as/NewFort로 다시 만드는 것은 자동 원본 복사와 다르므로 정상 Native writer는 계속 허용한다.

첫 도입 전의 구형 작업본에는 삭제 장부가 없다. 현재 없는 source 항목이 예전 Native 삭제였는지 아직 이관하지 않은 것인지 복구할 증거가 없으므로 **과거 삭제까지 보존한다고 주장할 수 없다.** 첫 adoption부터 장부를 만든 뒤 수행한 rename/삭제의 영속성을 검증한다.

## archive-only 읽기

완료키 missing에서 기존 `source.TryRead`를 호출하면 같은 이름의 source disk/TAFF가 재등장한다. 따라서 이 분기는 어떤 형태의 source fallback도 호출하지 않는다.

아직 이관하지 않은 경로의 archive-only 읽기는 **새 const `BaseFileSystem::TryReadArchiveOnly`**로 현재 등록 archive 순서만 조회한다. 기존 TryRead/Read/Match의 의미는 바꾸지 않으며 특정 netstorm.tarc 한 개를 새로 하드코딩/중복 소유하지 않는다. archive-only 내용은 메뉴 열거/물리 이관 장부에 넣지 않는다. 직접 Edit가 archive-only Fort를 실제 저장 대상으로 설치할 때에는 명시 work 복사/장부 항목을 만든 뒤 후속 삭제는 같은 정책을 따른다.

전역 BaseFileSystem의 순서/열거/의미는 변경하지 않는다. 기본 SoloMapCatalog의 선택 장치를 주지 않은 기존 디스크/TAFF/보조 동작도 유지한다. 존재 확인과 실제 OS open 사이의 외부 변경을 원자적으로 방어했다고 주장하지 않는다.

## 수정 검사 범위

Source1에서 추가한7개는 새 설계 가정 검사였으므로 아래 원문 의미로 정정한다. 원래 baseline 검사와 원본 fixture는 고치지 않는다.

1. 실제 work.MatchDisk 결과를 독립 스냅샷으로 읽고, 원문 첫점 이름/원시 State 종류로 수락한 첫50개와 Catalog 전체 행을 대조한다. source order 기대/논리 dedup 기대를 제거한다.
2. Same.fort/Same.extra.fort 두 physical 항목을 실제 이관하여 Same 두 행/실제 OS 위치를 유지한다. 디렉터리 이름도 숨기지 않고 명시 부재 패턴은1경로를 반환한다.
3. source50개 이상 + work 새 이름을 만들고, 새 이름이 실제 work 수락50개 안일 때 반드시 포함되고 밖일 때 제외됨을 OS 스냅샷에서 독립 판정한다. 항상 append/항상 노출을 기대하지 않는다.
4. 기존 사용자 work 내용과 readonly 속성 보존, 신규 source readonly 복사, Native 현재 이름 부모의 old/new snapshot/readonly/old 삭제를 한 namespace에서 확인한다.
5. 이관한 항목 Delete→Catalog 재조회→workspace/프로세스 역할의 새 객체 재생성→직접 Prepare/Kind/metadata에서 원본·TAFF로 부활하지 않음을 확인한다. 같은 이름의 명시 새 저장은 허용한다.
6. 장부 미기록/Completed/Pending 존재·부재/손상·동시사용/IO 실패의 부분 효과를 별도 입력 장치로 검증한다. source missing은 Completed로 기록하지 않는다.
7. 전체 relative 키의 ./·case·insideSource/insideWork absolute 동치와 Same.extra 비동치, 외부/보호/형제 접두부/reparse 경계, metadata가 임의 work 패턴에 보이지 않음을 확인한다.
8. 분류/메타의 현재 읽기와 FileSpec callback 뒤 재조회는 기존 회귀와 함께 유지한다. 실제 Save as→메뉴→같은 이름 재개·rename 삭제→재시작 GUI는 통합 담당자가 별도 수행한다.

이 namespace 이관과 장부는 원본의 추가 게임 규칙이 아니라 원본 불변을 지키기 위한 호스트 I/O 경계다. 기존 게임 메모리/미션 진행을 성공처럼 합성하지 않는다. 이 I/O 경계의 C++ 집중 검사와 실제 클론 GUI를 수행했으며 원본 게임은 실행하지 않았다. 결과와 최초 실패는 통합 보고서에 구분한다.

## 준비본 교정과 peer 검토

Source1의 실제 빌드는 성공했지만 Catalog 신규7개 중6개 통과/절대경로 구분자 기대1개 실패였다. Clipboard2개와 SaveFortName6개는 통과했고 전체 회귀는 실행하지 않았다. 이 통과를 source-first/dedup 설계의 원본 동일성 증거로 사용하지 않는다. 기존 원문/선행 baseline 검사와 fixture는 변경하지 않고 이번 미커밋 신규7개를 수정하여11개로 준비했다.

두 peer 결함도 수정했다. 첫째, 최초 초안의 case-folded 장부 키를 실제 복사 대상에 재사용하여 source 물리 이름의 대소문자 표기를 바꾸던 문제를 실제 경로/장부 비교키 분리로 고쳤다. 명시 부재 FileSpec의 입력 철자도 유지한다. 둘째, 기존 장부의 권한/공유거부 읽기 실패가 nullopt라는 이유로 새 빈 장부가 되던 문제를 실제 path existence/status 조회와 분리했다. 기존이면서 읽지 못하는 장부는 실패하며 외부 `CreateFileW` share0 HANDLE 반례에서 삭제 원본/장부가 바뀌지 않도록 검사한다.

추가11개는 실제 work OS순서/readonly 보존, 새 이름 재조회/owner 재시작, 원문 같은 논리명 두 행, 실제 work 수락50개, 완료삭제 alias/직접 prepare/TAFF 차단, 디렉터리·명시 부재·`.`/`..`/형제 metadata, Pending/손상/잠금/장부 IO 실패, 실제 Native rename의 readonly/old 삭제, archive-only 등록순서/disk 분리, 읽기 불가능한 기존 장부, 분류/메타의 현재 재읽기를 확인한다. 기대는 source 합성 순서나 항상 새 이름 표시가 아니다.

장부 IO 실패 검사에서 실제 유도한 위치는 `catalog-v1.dat.pending`을 디렉터리로 만들어 발생시키는 **임시 파일 open 실패**와 외부 share0 HANDLE의 **기존 장부 read open 실패**다. WriteFile/FlushFileBuffers/CloseHandle/MoveFileExW 반환 검사는 코드로 구현했으나 각 실패 반환을 따로 강제 관측한 것은 아니다. 따라서 실제 flush 실패를 검사했다고 보고하지 않는다.

수정한11개 검사는 제품에 적용해 실행했다. 디렉터리 Open 실패 뒤 manager0인 원문00460c40/004bd550은 진단/물리 실패이며 성공한 빈 행으로 바꾸지 않는다. 이 신규 기대 정정은 기존 검사 본문이나 원본 fixture를 변경하지 않았다. H: exFAT 장부 교체 오류5에 제한적 atomic 재시도를 추가했고, 영구 실패 보존/외부 reader 반납을 추가2개 검사로 확인했다. 실제 exFAT의 새 이름이50개 밖인 관찰과 NTFS의50개 안 새 이름 재진입을 별도로 검증했다. 장기 실제 미션 전역이나 전체 원본 동일성을 이 I/O 수정의 완료로 합산하지 않는다.
