# webpj Ctrl+special 지점 진단 원본 관측 연결 (WB04-E487, 2026-11-15)

3.8절 이월 목록의 `pointer-special-trace`를 원본 10.78 독립 x86 관측 **36행**에 대조했다. 36행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다.

## 비교한 것

`tests/pointer-special-native-fixtures.test.mjs`가 cpppj `PointerMenuParentTests`(`ReplaysNativeSpecialTrace36`)의 입력 구성을 옮긴다.

- **입력 열**: special·control·후보 목록 번호(비어 있음 / 하나 `[50]` / 중복 포함 `[50, 51, 50]`)·사건 좌표. 투영 결과(`123.25, -9.5`)와 finder 후보는 관측과 같은 명시 외부 입력이다.
- **비교**: 호출 순서 문자열(`P,x,y` 역투영 → `B,x비트,y비트,0` 첫 후보 → `V,sid` 직접 가상 70 → `N` 다음 후보)이 trace 열과 정확히 같다. 조건 단락(control 또는 special 이 0)은 `-`(호출 없음)이며 중복 후보도 임의 제거하지 않는다.

같은 cpppj 시험 파일의 `pointer-object-menu-parent`(8행)는 `RawObjectMenuOpen`이 PORT_MAP `todo`(미이식)라 이 단위에서 제외한다. 그 이식 때 연결한다.

## 변형 확인

자동 변형 2개(`||`↔`&&`·`!==`↔`===`) 모두 잡혔다. 모듈이 18줄이라 나머지는 호출 순서 비교에 직접 걸린다.

## 남은 것

3.8절의 `pointer-object-menu-parent`(RawObjectMenuOpen 이식 때)와 HUD/minimap/menugump/inventory-about/gump-pointer-boundary 계열은 후속이다.
