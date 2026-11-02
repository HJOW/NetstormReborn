using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>
/// 새 맵 manager 생성 중 원문 진단 두 종류의 처리 방법이다. 진단에서 복귀할지 중단할지는 호출자가 실제 장치로 결정한다
/// (원본 <c>NewFortManagerDiagnostics</c>). 연결하지 않은 진단은 그 지점에 이르면 명시 오류로 멈춘다.
/// </summary>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public sealed class NewFortManagerDiagnostics
{
    /// <summary>Template.cpp:0x172의 기존 manager 진단이다.</summary>
    public Action<uint>? TemplateAssertion { get; init; }

    /// <summary>Datamanager.cpp/Snart.cpp의 선택된 초기화 진단이다.</summary>
    public Action<DataManagerInitializeAssertion>? InitializeAssertion { get; init; }
}

/// <summary>
/// 새 맵 manager 부모(<see cref="RawNewFort.CreateNewFortManager"/>)에 실제 Template 저장소와 보호된 작업본 파일 장치를 연결한다.
/// 독립 manager 사본이나 임시 Fort 파일을 만들지 않는다. cpppj <c>client/NewFortManagerDevices</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. cpppj는 자유 함수(<c>CreateNewEditorFortManager</c>)이지만 C#은 정적 클래스의 정적 메서드다.
/// 파일 이름은 호스트의 실제 문자열이며 manager +12에 쓰고 읽는 UTF-8 바이트 변환은 저장소(<see cref="TemplateFileNames"/>)가 맡는다.
/// </remarks>
public static class NewFortManagerDevices
{
    /// <summary>Template 칸 수다. 헤더는 한 칸에 20바이트다.</summary>
    private const int TemplateCount = 9;

    /// <summary>Template 기록 한 개의 실제 폭이다.</summary>
    private const int HeaderBytes = 20;

    /// <summary>
    /// 지정한 칸의 20바이트 헤더에 원문 004bfba0을 실행한다. 존재 재조회·삭제·임시 소멸·최종 공개·35개 섹션 초기화를 분리하지 않는다.
    /// 부모가 선택한 하위는 같은 저장소를 쓰며, 실패 이전에 이미 쓴 헤더·파일 효과를 되돌리지 않는다.
    /// 호출 중 저장소와 파일 장치는 살아 있어야 하며 <paramref name="fileName"/>은 같은 작업 이름공간의 경로여야 한다.
    /// </summary>
    /// <param name="storage">manager 이미지와 섹션 수명을 소유하는 장기 저장소.</param>
    /// <param name="files">원본 밖의 작업 이름공간만 쓰는 파일 장치.</param>
    /// <param name="slot">새 manager를 공개할 Template 칸(0~8).</param>
    /// <param name="fileName">새 manager의 파일 이름. 상대 이름도 같은 작업 경로로 해석하며 manager에는 입력 그대로 보관한다.</param>
    /// <param name="argument">헤더 +16에 쓸 인자.</param>
    /// <param name="diagnostics">원문 진단 처리. 생략하면 진단 지점에서 명시 오류로 멈춘다.</param>
    /// <exception cref="ArgumentOutOfRangeException">칸이 9칸 밖이다.</exception>
    public static void CreateNewEditorFortManager(CampaignTemplateStorage storage, EditorFiles files, uint slot, string fileName, uint argument,
        NewFortManagerDiagnostics? diagnostics = null)
    {
        ArgumentNullException.ThrowIfNull(storage);
        ArgumentNullException.ThrowIfNull(files);
        if (slot >= TemplateCount)
        {
            throw new ArgumentOutOfRangeException(nameof(slot), slot, "NewFort의 실제 Template9 슬롯 범위");
        }
        diagnostics ??= new NewFortManagerDiagnostics();
        TemplateLifetimeHooks lifetime = storage.ManagerLifetime();
        NewFortManagerHooks hooks = new()
        {
            Assertion = diagnostics.TemplateAssertion,
            Exists = name => files.Exists(name),
            Allocate = bytes => storage.AllocateEmptyManager(bytes),
            Construct = (token, name) => storage.ConstructEmptyManager(token, name),
            FileName = token => TemplateFileNames.FromManager(RawTemplateLoad.FileName(storage.ManagerBytes(token))),
            // 상대 이름도 존재 검사와 같은 작업 경로로 해석하며 manager에 저장한 원문 이름은 보존한다
            Erase = name => files.Delete(files.ResolvePath(name)),
            Destroy = lifetime.Destroy,
            Release = lifetime.Release,
            Initialize = token => storage.InitializeEmptyManager(token, diagnostics.InitializeAssertion)
        };
        RawNewFort.CreateNewFortManager(storage.Headers.Span.Slice((int)slot * HeaderBytes, HeaderBytes), fileName, argument, hooks);
    }
}
