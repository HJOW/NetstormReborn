# 경로 삭제 후 정리 복원 — 2026-10-13

마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-13**이다. 최신 같은 정밀 프로젝트의 읽기 전용 내보내기는 `pathcleanup` **패치27/CD19 함수**이며 보호 PE/AGENTS.md/앞 단위 근거를 수정하지 않았다.

## 실제 몸체와 기존 소비자

`RawPathProcess::OnFormDestroy`의 부모 공간 접두는 그대로 재사용한다. 새 `RawPathCleanup::AfterSpatial`은 그 다음 본문만 맡는다.

| 역할 | originals 10.78 | originalCD 10.72 | C++ 소비자 |
| --- | --- | --- | --- |
| cleanup 전체 원본 | `0048aa00` | `0047f0a0` | 공간 접두는 기존 Process, 이후는 새 모듈 |
| helper 주변 finder 생성 | `00422230` | 본문에 인라인 | 기존 `RawSquidFinder` |
| 귀환 장소 조회 | `004263a0` | `004e3d70` | `ReturnPlaceAt` |
| 주변 같은 타입 조회 | `004b21d0` | `004eb7b0` | `NearbyType` |
| contained mask/kind 조회 | `004ac9f0` | `004ac560` | 기존 `RawCarrierPreDestroy::FindContained` |
| 현재 Graph 번호/동일성 | `00462c80/00462bc0` | 해당 cleanup 분기 없음 | 기존 `RawPlayerPlacementAnchor::GraphFor` |
| 지연1 helper 예약 | `00496f00` | `0048f1d0` | 기존 `Host.AddSharedRegular` |
| 실패 helper 환불 | `00442a40` | `004d1bc0` | 기존 `RawConstructionPlace::Refund` |
| 신전 의식 | `00447e40` | `00481fc0` | 실제 하위 몸체는 후속 경계 |

순서는 다음과 같다.

1. helper가 유효 번호이고 genus0x404200에 속하면 현재 부모의 거의 올림 좌표 주위 반경2의 실제 일반 finder를 탐색한다. helper를 찾은 경우 new0x28 성공 시 **helper/event0x1a2c/payload1**의 SharedRegular를 요청한다. 확보 실패를 포함해 helper 필드는0으로 지운다.
2. **10.78만** 살아 있는 부모의 귀환 장소와 mask0x20/kind154의 contained nugget을 찾는다. 같은 owner와 Graph 포인터 동일성, 현재 boss가 성립하면 nugget을 flags0으로 삭제한다. Graph 번호254 두 개는 원본의 null 포인터 두 개와 같다. **10.72 본문에는 이 분기가 없다.**
3. 현재 부모가 사제158이면 반경2의 실제 타입165 신전을 찾고 buried가 아니면 의식(신전,parent,0)을 요청한다.
4. 현재 boss/남은 helper를 다시 읽는다. free가 아니고 abstract이며 genus0x404200이면 현재 타입/owner를 환불한 뒤 보존한 helper를 flags0으로 삭제한다. 패치 checking에서 조건이 틀리면 `shouldDestroy` assert이며 CD는 조용히 반환한다.

`ReturnPlaceAt`는 이름이 신전 하나를 뜻하지 않는다. **좌표±정수 radius**를 절삭한 사각형에서 abstract/dead가 아닌 첫 genus0x200 또는 현재 outpost126을 찾는다. cleanup의 radius는1이다. `NearbyType`/helper 사각형은 이와 달리 `0041d7a0/CD00440a80`의 실제 float **0.9999899864196777** 편향을 먼저 적용한다. 두 좌표 처리를 합치지 않는다.

`MakePathCleanupHooks`는 기존 contained/Graph/SharedRegular/Refund 몸체를 같은 풀로 검사해 연결한다. 원본 가상 삭제·예약 확보 및 아직 남은 의식은 caller가 실제 하위 구현을 공급해야 한다. 효과가 없는 기본 callback을 제공하지 않는다.

## 독립 PE/C++ 대조

`tools/decomp_pathcleanup_oracle.py`는 두 보호 PE를 메모리에 읽어 실제 cleanup, 두 새 finder 조회, contained/Graph/CRT 몸체를 실행한다. 공간 접두 요청은 별도 필드로 관찰하고 C++ 새 본문과 섞지 않는다. malloc/SharedRegular ctor/ritual/Refund/가상 삭제/assert 보고는 이름과 규약을 명시한 관찰 경계다.

고유 입력은 **1732개 = 정상1716 + 패치 assert16**이다. 각 판본은 **cleanup704 + 독립 query162**를 수행한다. 두 x87 정밀도 반복과 같은 입력 재실행은 합산하지 않는다. helper의 부류/위치/abstract/free/buried, 부모 dead/권한/종류, 확보 실패, 현재 owner/Graph, callback 뒤 권한 변경, snap 편향 경계와 CD 귀환 조회를 포함한다. 근거는 `cpppj/recovery-pathcleanup-evidence.json`, fixture는 `cpppj/tests/fixtures/pathcleanup-x86.tsv`다.

`PathCleanupTests.cpp`의 전수 검사는 원본 이후 효과/현재 helper·boss·일곱 raw 슬롯 전체와 경로의 helper 이외 바이트 불변을 대조한다. 두 번째 검사는 **기존 실제 RawPathProcess 공간 접두 → 새 cleanup → 같은 Host/Kernel의 실제 SharedRegular**를 연결하여 공유 이벤트/부모/시각과 helper0을 확인한다. 별도 C++ 빌드/대조 결과는 root 통합 뒤 기록한다.

초기 공간 소비자 검사는 합성 vtable의 사제에게 파생 postPop을 연결하지 않아 실패했다. 실제 type158 생성자와 기존 **PriestPostPop 접두→CarrierPostPop→SquidPostPopBase→PriestPostPopTail**을 명시 등록하도록 교정했다. 이미 최초 등록된 extra0x80, HP200/최대200, 현재 거의 올림 칸의 spot6인 사제의 이동 가능 분기를 실행한다. 초기 Pop와 cleanup Repop 각각 실제 Forcefield finder 결과0을 읽고 공통 depth0을 확인한다. 낙하/보호막 생성·삭제/로딩 지면 소유자/표시/Graph 효과는 이 소비자 검사의 미실행 범위다. 그 분기가 요청되면 명시 실패하므로 전체 파생 월드 완료로 세지 않는다.

`tools/analyze_pathcleanup_test_lineage.py`는 빈 프레임 생성자 한 줄 교정을 그대로 보존하고, 파생 연결 전 test/report snapshot 및 두 번째 검사 diff를 기록한다. 첫1732개 전수 검사/입력 준비 본문, production/fixture/PE/export/oracle는 모두 그대로다. 현재 테스트 SourceSHA 연결의 신규 원본 관찰 수는0개다.

단위07 `repaired3` 공유 Release는 **종료0·경고0·오류0**, 집중 C++ 두 검사 모두 성공했다. **1732개 전수 대조(정상1716/assert16)**와 위의 실제 사제 후처리/공간 접두/같은 SharedRegular 사슬을 통과했다. 전체 검사 결과는 root의 최종 통합 기록에서 확정한다.

## 기존 근거 보존

Runtime의 `Helper`/`ClearHelper` 두 API는 준비 단위 **4e532fd(20261013-05)**에 포함되었다. 새 `tools/analyze_pathcleanup_runtime_lineage.py`는 두 API와 설명만 제거했을 때 **8abd00a의 전체 기존 소스/토큰**과 같음을 증명한다. 앞 단위의 `SetCommandFields`/`ReplacePoint`도 비교 범위에 포함한다. 이전 Runtime360/Start512와 기존03/04 근거를 바꾸거나 새 관찰로 다시 더하지 않는다.

```text
python -X utf8 tools/decomp_pathcleanup_oracle.py --verify
python -X utf8 tools/analyze_pathcleanup_runtime_lineage.py --verify
```

## 남은 범위

최종 단위07 Release 두 타깃 종료0·경고/오류0, 집중2개 및 **전체983개 검사/실패0**이다. 통합 SHA/관찰 분류는 `recovery-campaign-runtime7-evidence.json`, 초기/교정/최종 로그는 `extracted/campaign-runtime7-*`를 따른다.

신전 의식 본문/하위 프레임·소리·예약, 사제/골렘의 실제 CarrierArrival과 운반/채굴 예약·명령, type40 경로 처리, 실제 raw GameWorld 소비자 연결과 UI·완주 대조는 남아 있다. 현재 신전 의식 요청 관찰을 의식 완료로 세지 않는다. 기존 건설 운반 ConstructionArrival과 CarrierCheck는 재사용한다.
