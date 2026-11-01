using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>
/// 현재 선택이 바뀌어도 같은 manager의 같은 생성 수명에 쓰기 위한 불투명 참조다(cpppj <c>client/CampaignTemplateStorage.h</c>의 <c>TemplateWriteStream</c>).
/// 호스트 포인터를 DWORD로 자르지 않으며, 기본값(모두 0)은 "없음"이다. manager가 반납되거나 같은 이름의 섹션이 재생성되면
/// 이 참조로 쓰는 것은 명시 오류로 거부된다.
/// </summary>
/// <param name="Manager">실제 호스트 manager 핸들.</param>
/// <param name="Section">같은 생성 수명의 동적 섹션 노드 토큰.</param>
/// <param name="Generation">같은 이름의 섹션을 재생성한 뒤 이전 참조로 쓰는 것을 거부하는 생성 수명.</param>
/// <remarks>2026-11-01 0.132(TS2)에서 추가했다.</remarks>
public readonly record struct TemplateWriteStream(uint Manager, uint Section, ulong Generation);

/// <summary>
/// 인라인 쓰기에 같은 16바이트 MemStream 필드와 버퍼 장치를 동기적으로 빌려 주는 연산이다.
/// <see cref="Span{T}"/>를 받으므로 <see cref="Action{T1,T2}"/> 대신 전용 대리자를 쓴다.
/// </summary>
/// <param name="fields">노드의 첫 16바이트(data 토큰, pos, end, alloc).</param>
/// <param name="hooks">같은 노드의 버퍼 조회·임시 할당·반납 장치. 연산이 끝난 뒤에는 쓰지 않는다.</param>
/// <remarks>2026-11-01 0.132(TS2)에서 추가했다.</remarks>
public delegate void TemplateSectionAccess(Span<byte> fields, MemStreamHooks hooks);
