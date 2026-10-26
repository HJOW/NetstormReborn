# 새 맵 저장소 기반 통합 — 단위469

2026-10-27, HJOW-Athlon에서 새 맵 생성의 선행 기능인 실제 빈 manager 초기화와 Subscriber 저장을 제품에 채택했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 산출물을 변경하지 않았다. [실행 보고서](../../cpppj/new-map-storage-runtime-20261027.json)에 빌드·집중 검사·최초 실패·보호 검사와 각 원시 로그의 SHA를 기록했다.

## 변경과 원본 근거

- `RawFortSubscriberWrite`는 10.78 `004bd400`의 manager 진단 → 등록 helper → 현재 manager 재조회 → 캡처 ID → 캡처한 물리 이름의 현재 내용 저장을 복원한다. 기존 등록·문자열·MemStream 본체를 재사용한다. `CreateManagerSection`은 선택한 실제 manager의 typed stream 수명을 보존한다.
- `RawTemplateSectionNames`와 `ClientTemplateSectionNames`는 `004bccb0`의 최초512BYTE 할당·포인터 공개·277BYTE 작성·재사용을 수행한다. 미작성235BYTE를 게임 규칙으로 읽거나 초기화하지 않는다.
- `RawSnart`는 `004a8800/004a8850`의 같은 이름표 제자리 변환을 파일 reader와 초기화에 공유한다. `RawDataManagerInitialize`는 `0044d870`의 진단 뒤 이름표를 한 번 빌리고, Create마다 그 물리 표의 다음 이름을 읽는다.
- 실제 `CampaignTemplateStorage`의 빈 Image 할당·원본 생성자·35개 섹션 초기화를 분리했다. 기존 파일 읽기와 `SoloMapCatalog`의 두 임시 manager도 주 저장소의 같은 이름표를 공유한다. 기존 맵 로드/저장 부모를 새 빈 파일 조립으로 대체하지 않았다.

10.78 기준의 독립 근거는 [Subscriber 관측](../../cpppj/fort-subscriber-write-native.json), [이름표·초기화 보관 자료](prepared/data-manager-initialize-469-20261027/README.md)다. 이 단위에서 CD 10.72 부모를 새로 복원하거나 실행하지 않았다.

## 검증

- 보관본5묶음/142파일/32,009,649BYTE의 SHA를 확인했다. 두 patch는 현재468 제품에 hunk로 적용했으며 Subscriber와 초기화의 Storage 변경을 함께 보존했다.
- Release 빌드3회 모두 종료0, 경고·오류0이다. 첫 빌드 뒤 두 번은 새 검사 정정에 따른 증분 빌드다. 최종 게임 SHA256은 `464464af2d7ac8cfb8c024e99f1a2bcb0cde69c0b1cc531dce2c344f5b2fab98`다.
- 보호 PE의 Subscriber90관측과 이름표/초기화76관측을 다시 실행해 최초 fixture와 바이트가 일치했다. 실제 게임·창·음향 장치 실행은 없다.
- 집중 검사 **171개/실패0**, 신규8개 전부 통과했다. 실제 TEST01 읽기, 선택된 manager 교체 중 Subscriber 저장, 빈 Image의 실제 섹션/커서/버퍼, 맵 목록 분류·메타 조회와 작업본 파일 회귀를 포함한다.
- 기존3,211개 등록 이름을 모두 보존했고 최종 등록은3,219개다. 이는 전체 검사를 다시 실행한 수가 아니다. 기존 검사 본문과 Native fixture, 보호 원본/CD/AGENTS를 합한 최초3,149파일은 바이트가 같다. 변경한 기존 테스트 파일은 CMake 등록뿐이다.

최초 집중에서 새 빈 Image 검사의34단언이 실패했다. 준비 초안은35개 빈 섹션 모두가 비영 버퍼와16KB 용량을 유지한다고 가정했다. 실제 `0044ce10`은 다음 노드를 연결하기 전에 `0044cd40`으로 이전 tail을 압축한다. 빈 앞34개는 버퍼 반납 뒤 pointer/position/end/capacity가0이고, 마지막 Terr19만 새 버퍼/16KB를 유지한다. 첫 정정의 용량 수정 뒤 포인터 단언34실패도 남겼다. 원문과 기존 독립 Create/List/MemStream 검사를 대조하여 **새 검사만** 정확한 두 경우로 정정했다. 제품 본문·기존 기대·Native fixture는 바꾸지 않았다.

원시는 `extracted/cpp-new-map-469/`, 드라이버는 `extracted/cpp-new-map-469-{audit,focused,finalize}.py`에 있다. Git 제외 자료이므로 커밋된 실행 보고서의 SHA와 범위를 기준으로 읽는다.

## 다음 작업

이 단위는 NewFort 전체 완료가 아니다. [NewFort 준비본](prepared/new-fort-469-20261027/README.md)의 세 부모와 실제 Client 전역/SP/Core/작업본 파일을 연결해야 한다. 이어 NewEditor 전체 설치·State 확정·자동 Add Island 큐, 전용 MetaImage/Bitmap/목록/창 수명을 복원한다. Test Battle·시나리오와 전체 미션 자연 완주·원본 화면/음향 동일성도 남아 있다. 전체 CTest와 실제 게임 검증은 기능 묶음이 완성된 뒤 진행한다.
