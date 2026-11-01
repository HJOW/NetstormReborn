namespace Netstorm.Client.Host;

/// <summary>
/// Template 저장 부모가 정한 현재 이름과 이미지 바이트를 작업 파일에 쓰는 장치 경계다.
/// <see cref="ReadOnlySpan{T}"/>를 받으므로 <see cref="Action{T1,T2}"/> 대신 전용 대리자를 쓴다.
/// </summary>
/// <param name="name">저장 부모가 결정한 현재 파일 이름(작업본 절대 경로 또는 작업본 상대 경로).</param>
/// <param name="bytes">그대로 쓸 Template 이미지 원문.</param>
public delegate void TemplateFileWrite(string name, ReadOnlySpan<byte> bytes);

/// <summary>
/// 실제 Template 저장 부모가 쓰는 권한 변경·쓰기 경계다. 이미지를 복사해 별도 규칙으로 바꾸지 않고 저장 부모가 정한 값을 그대로 인계한다.
/// cpppj <c>client/CampaignTemplateStorage.h</c>의 <c>TemplateFileWriteHooks</c>를 옮겼다.
/// </summary>
/// <param name="Permissions">현재 이름의 CRT 권한 요청(원문 0x180)을 실제 쓰기 경계에 넘긴다.</param>
/// <param name="Write">저장 부모가 정한 현재 이름에 같은 이미지 원문을 쓴다.</param>
/// <remarks>2026-11-01 0.127에서 추가했다. Template 저장소 본체(<c>CampaignTemplateStorage</c>)는 아직 옮기지 않았으므로 이 형식만 먼저 둔다.</remarks>
public sealed record TemplateFileWriteHooks(Action<string, uint> Permissions, TemplateFileWrite Write);

/// <summary>
/// 원문 Load의 실제 파일·CRT·현재 슬롯·제목 경계다. 선택한 호출이 필요로 하는 항목만 연결하면 되며,
/// 연결하지 않은 항목을 실제로 호출하는 쪽이 명시 오류를 낸다. cpppj <c>client/CampaignTemplateStorage.h</c>의 <c>TemplateFileLoadHooks</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.127에서 추가했다. <c>with</c> 식으로 복사해 일부 항목만 바꿀 수 있어 cpppj의 값 전달과 같다.
/// 파일 장치가 채우는 항목(<see cref="Read"/>, <see cref="ReadOnly"/>, <see cref="SameName"/>)은 <see cref="EditorFortFileDevices.BindLoad"/>가 연결한다.
/// </remarks>
public sealed record TemplateFileLoadHooks
{
    /// <summary>실제 읽기 시점의 파일 할당이다. 빈 배열은 원문 open 실패이며 예외와 구별한다.</summary>
    public Func<string, byte[]>? Read { get; init; }
    /// <summary>기존 manager 이름과 요청 이름의 실제 CRT <c>stricmp==0</c> 비교다.</summary>
    public Func<string, string, bool>? SameName { get; init; }
    /// <summary>기존 다른 이름의 isCalled 진단이다. 반환하면 원문대로 기존 manager를 유지한다.</summary>
    public Action? WrongName { get; init; }
    /// <summary>파일 출처·권한으로 판정하는 0041af80 경계다. 기존 header+8이 0이 아니면 호출하지 않는다.</summary>
    public Func<string, bool>? ReadOnly { get; init; }
    /// <summary>실제 서명 검사가 도달한 순간의 현재 signed TYPE 개수다.</summary>
    public Func<int>? TypeCount { get; init; }
    /// <summary>numTypes&lt;256 진단이다. 반환하면 현재 원문 서명 읽기를 계속한다.</summary>
    public Action? TooManyTypes { get; init; }
    /// <summary>잘못된 서명의 실제 파일 제거 요청이다. 보호 파일 삭제를 자동 허용하지 않는다.</summary>
    public Action<string>? RejectFile { get; init; }
    /// <summary>성공 뒤 원문 현재 Template 인덱스를 다시 조회한다.</summary>
    public Func<uint>? CurrentSlot { get; init; }
    /// <summary>현재 슬롯의 성공에서만 00460730의 중간 제목 효과를 수행한다.</summary>
    public Action? RefreshTitle { get; init; }
}
