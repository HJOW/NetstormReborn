# 자산 bare cdecl의 물리 recipe와 진단 후 저장

단위888 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물 변경0이다. 직전887은 `a1c820a5 / 20261105-21 CP`, 현재 제품888/11.01이며 전체 목표/Pop은 미완료다.

새 SquidFactory.ConstructByAddress는 현재 TYPE의 함수 주소를 기존 recipe에서 직접 선택해 **기존 물리 SID**에 원문 mov/OR만 적용한다. TYPE 번호 재선택·Create/Take·새 할당·공통 초기화를 추가하지 않는다. 현재 슬롯의 free/dead/contained/예약 상태도 생성자가 검사하지 않는 조건으로 거부하지 않는다. 기존 ApplyConstructor와 새 경로는 같은 ApplyRecipe를 사용하며 typed Create/Construct/Retype/Take의 기존 사전 계약은 유지한다. 원문이 실제로 쓰는0/8/26/35/40 위치만 판본별 폭/순서로 적용하며 pool 카운터를 바꾸지 않는다. SID0의 VT가 특수 free-list 머리와 겹치는 의미는 보정하지 않는다. 실제 소유 범위와32비트 NULL pool/포인터 감김은 별도 호스트 경계다.

PATCH mog153/lightning159의004429e0은 아무 인자/풀 조회 없이0을 반환한다. 호출 자체가 없는 함수 포인터0과 구분해 지원하며 원문 NULL 결과를 무시하는 Teleport는 공통 부모로 복귀한다. PATCH daisExtraFrames169의0044a420은 `false`,`.\Dais.cpp`,줄0x484를 항상 진단한 뒤 현재 물리 슬롯에 VT00507950을 저장한다. 이 함수의 refined C는 없으므로0044a420~0044a451 전체 기계어/실제 실행으로 확인했다. 진단 callback은 선택한 사본으로 호출하여 자기 연결 해제 중에도 살아 있고, 정상 복귀 뒤에 주소를 조회한다. 중단 전 외부 쓰기는 남기고 다음 VT 저장은 하지 않는다. 이 경로의 저장/진단만 복원했으며 Dais 가상 몸체/의식/희생은 실행하지 않았다.

[격리 도구](../../tools/decomp_asset_constructor.py)는 기존 주소 목록을 **입력 함수 탐색에만** 쓰고 실제 PE가 출력한 전체 슬롯을 [새 fixture](../../cpppj/tests/fixtures/asset-constructor-current-x86.tsv)에 저장했다. PATCH 일반78/CD67 recipe×SID0/4/5/50/23001/32767/65534=1015행과 PATCH NULL7·진단 정상 복귀/타입·owner 변경14행을 합쳐 **1036행**이다. 전부 다시 관측해 일치했다. 별도 NULL의 SID65535도 조회/쓰기 없이0으로 복귀했다. 목표 밖 저장/스택 변경/다른 진단을 거부한다. 공중3종과 MP Outpost의 주소는 양판본에서 각각4개씩 **실행 목록에서 제외**했다. 해당 recipe의 공유 범용 코드/정적 주소 연결만 유지한다.

새 C++ 순수2사례는 fresh pool의1036행 전체 바이트/카운터와 NULL/미지원/진단 수명·중단·주소 조회 순서를 대조한다. TYPE 메타데이터의 생성 주소는 모두0인 입력에서도 직접 주소 호출은 Native와 일치한다. 새 실제 Session2사례는 현재 TYPE61에서 거주 구역89·마나103·NULL153을 선택한 VT/타입 보존/할당·Kernel·renderer 불변/공통 장부 복귀와, Load 전 Client 진단 뒤 단일 Dais VT 저장만 확인한다. 후자는 원래 바이트를 복원하며 이후 Dais 가상 몸체/Pop을 호출하지 않는다. 공유 Storage에 남는 새 검사 진단은 heap 소유 상태를 캡처하고 지역 span을 반납한다.

첫 검사에서 새 입력2곳이 실패했다. 순수 계약 검사의 pool64는 기존 최소 한도보다 작아24000으로 바꿨다. 실제 Teleport 검사는 생성자0인 TYPE74를 직접 호출하려 했으므로 유효한 TYPE89로 바꾸고 비영 주소 관문을 추가했다. **직접 함수 포인터0은 base fallback이 아니다.** 제품/Native 기대값을 변경하지 않았다. Native1036행·실제 Client 진단과 기존 회귀는 첫 통과를 보존하고 실패했던 새2사례만 재실행해 통과했다. 고유32개/총12필터 실행(최초10+교정2), 총34사례 실행·최종 실패0이다.

제품 Release 첫897.8333399초·첫 검사 전 검사 서비스 소유권 정리29.7053424초·입력 교정 검사 대상43.8477855초 모두 종료0/경고·오류0이다. 제품 코드는 첫 빌드 뒤 불변이며 검사 파일2개만 교정했다. 최종8파일 SHA를 보존했다. 기존 보호2767개/947fixture 불변·새1개 후 기준은 **2768개/948개**다. EXE는10661888BYTE, SHA-256 `5dd55f8efa83f999b366457f642c78f7ef0d2a8edb5b6568c9fd79f83b034da1`다. [검증 JSON](../../cpppj/asset-constructor-current-20261105.json)과 Git 제외 `extracted/cpp-resume-888/`에 Native 관찰·최초 실패/교정·빌드/검사·보호/SHA가 있다.

PATCH 자산 비영101행/고유84주소(recipe82+특수2)의 bare 분배가 정적으로 모두 연결됐다. 887의 내장0~69 분배와 별개로 집계하며 전체 Pop/현재 TYPE writer·typed factory 모든 경계 완료가 아니다. CD generic recipe71개 중 새 실행은 제외4개를 뺀67개이며, CD 특수00414110/0044bfb0/00483520은 새 지원 없이 별도10.72 잔여다. checking10(부모4/부분3/MP3)·완료부모38/부분3을 유지한다. 다음889에서 공통 Pop 부모004b02d0의 파생+20/명시 Validate 경계를 현재 구현과 다시 대조하고 TYPE 초기화 writer0049ebb0/CD004434d0·군도004c0e20을 이어간다. 원문 사본은 같은 raw 폴더 next-*.c.txt다. 게임/GUI/AI·전투 Kernel·포획/희생/특정 공격/승리/사용자 확인·공중3종 전용/신규MP·전체71/CTest/34미션/ZIP 반복0이며 목표는 active다.
