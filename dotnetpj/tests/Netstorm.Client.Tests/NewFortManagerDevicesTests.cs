using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// 새 맵 manager 부모(<see cref="RawNewFort.CreateNewFortManager"/>)가 실제 Template 저장소·공유 이름표·보호된 작업본 파일 장치를 함께 쓰는지 검사한다.
/// cpppj <c>NewFortManagerDevicesTests</c> 5개를 옮겼고, 한글·이모지 경로의 UTF-8 보관은 직접 검사로 더했다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다(부모 <see cref="RawNewFort"/>의 L1은 Core 검사 4종).
/// cpppj는 실제 보호 루트(<c>originals</c>)를 읽어 보호 경로로 넘겼지만 이 검사는 임시 폴더의 복사본을 보호 루트로 쓴다.
/// 작업본과 보호 폴더 이름에 한글·이모지를 넣어 manager가 UTF-8 바이트로 보관하는지도 함께 확인한다.
/// </remarks>
public sealed class NewFortManagerDevicesTests
{
    /// <summary>한글과 이모지가 든 보호 원본 폴더 경로다. 만들지는 않는다.</summary>
    private static string Protected(EditorTempRoot temp) => Path.Combine(temp.Root, "보호 원본 😀");

    /// <summary>한글과 이모지가 든 작업본 폴더 경로다. 만들지는 않는다.</summary>
    private static string WorkFolder(EditorTempRoot temp) => Path.Combine(temp.Root, "새 맵 작업본 😀");

    /// <summary>보호 원본 하나를 둔 작업 파일 장치를 만든다. 생성만으로는 파일을 만들지 않는다.</summary>
    private static EditorFiles Open(EditorTempRoot temp) => new(WorkFolder(temp), [Protected(temp)]);

    /// <summary>manager +12의 파일 이름을 manager 바이트 문자열로 읽는다(UTF-8 바이트를 문자 하나씩 담는다).</summary>
    /// <param name="storage">저장소.</param>
    /// <param name="token">manager 핸들.</param>
    private static string ManagerName(CampaignTemplateStorage storage, uint token) => RawTemplateLoad.FileName(storage.ManagerBytes(token));

    /// <summary>파일 부재는 디스크에 빈 Fort를 조립하지 않고 실제 manager의 35개 섹션을 만든다. 쓰지 않는 헤더 필드는 보존한다.</summary>
    [Fact]
    public void MissingFileInitializesRealImageAndKeepsStaleHeaderWords()
    {
        using EditorTempRoot temp = new();
        CampaignTemplateStorage storage = new();
        EditorFiles files = Open(temp);
        string name = files.ResolvePath("d/New.fort");
        SetHeader(storage, 28, 0xa55a1234);
        SetHeader(storage, 32, 0x1234a55a);
        storage.SerializationVersion = 0xdeadbeef;
        NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, name, 0x89abcdef);
        uint token = Header(storage, 20);
        Assert.NotEqual(0u, token);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(0u, Header(storage, 24));
        Assert.Equal(0xa55a1234u, Header(storage, 28));
        Assert.Equal(0x1234a55au, Header(storage, 32));
        Assert.Equal(0x89abcdefu, Header(storage, 36));
        Assert.Equal(0u, storage.SerializationVersion);
        Assert.Equal(name, storage.Name(1));
        Assert.Equal(TemplateFileNames.ToManager(name), ManagerName(storage, token));
        Assert.Equal(FortTemplate.SectionNames.ToArray(), storage.SectionOrder(1).ToArray());
        Assert.Equal("Terr19", storage.CurrentSection(1));
        Assert.Equal(1u, storage.SectionNamesOwner.CurrentToken);
        Assert.Null(files.TryRead(name));
    }

    /// <summary>존재 재조회가 참이면 작업 파일만 삭제하고 임시 manager를 소멸·해제한 뒤 최종 manager를 공개한다.</summary>
    [Fact]
    public void ExistingWorkUsesTemporaryLifetimeBeforeFinalImage()
    {
        using EditorTempRoot temp = new();
        CampaignTemplateStorage storage = new();
        EditorFiles files = Open(temp);
        byte[] payload = [(byte)'O', (byte)'L', (byte)'D', 0];
        string path = files.PreparePath("d/Existing.fort", payload);
        Assert.Equal(payload, files.TryRead(path));
        NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, path, 0);
        Assert.Equal(2u, Header(storage, 20));
        Assert.Equal(1, storage.OwnedImages);
        Assert.Null(files.TryRead(path));
        Assert.Equal(path, storage.Name(1));
        Assert.Equal(35, storage.SectionOrder(1).Count);
        Assert.Equal("Terr19", storage.CurrentSection(1));
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
        Assert.Equal(1u, storage.SectionNamesOwner.CurrentToken);
    }

    /// <summary>상대 파일명은 같은 작업 이름공간에서 존재 검사와 삭제를 수행하되 manager의 원문 이름은 그대로 보존한다.</summary>
    [Fact]
    public void RelativeNameChecksAndDeletesSameWorkspaceFile()
    {
        using EditorTempRoot temp = new();
        CampaignTemplateStorage storage = new();
        EditorFiles files = Open(temp);
        byte[] payload = [(byte)'O', (byte)'L', (byte)'D'];
        string path = files.PreparePath("d/Relative.fort", payload);
        Assert.Equal(5, files.Exists("d/Relative.fort"));
        NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, "d/Relative.fort", 0);
        Assert.Null(files.TryRead(path));
        Assert.Equal(0, files.Exists("d/Relative.fort"));
        Assert.Equal("d/Relative.fort", storage.Name(1));
        Assert.Equal(2u, Header(storage, 20));
        Assert.Equal(1, storage.OwnedImages);
    }

    /// <summary>보호 경로 거부는 실제 파일 효과 전에 중단하지만 원문이 먼저 쓴 두 헤더 필드를 복원하지 않는다.</summary>
    [Fact]
    public void ProtectedPathRejectionKeepsNativePrefixWithoutFileMutation()
    {
        using EditorTempRoot temp = new();
        CampaignTemplateStorage storage = new();
        EditorFiles files = Open(temp);
        string source = Path.Combine(Protected(temp), "Netstorm.exe");
        Directory.CreateDirectory(Protected(temp));
        File.WriteAllBytes(source, [1, 2, 3, 4, 5]);
        long size = new FileInfo(source).Length;
        DateTime modified = File.GetLastWriteTimeUtc(source);
        SetHeader(storage, 24, 0x12345678);
        SetHeader(storage, 28, 0xa5a5a5a5);
        Assert.ThrowsAny<Exception>(() => NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, source, 7));
        Assert.Equal(0u, Header(storage, 20));
        Assert.Equal(0u, Header(storage, 24));
        Assert.Equal(0xa5a5a5a5u, Header(storage, 28));
        Assert.Equal(7u, Header(storage, 36));
        Assert.Equal(0, storage.OwnedImages);
        Assert.Equal(0u, storage.SectionNamesOwner.CurrentToken);
        Assert.Equal(size, new FileInfo(source).Length);
        Assert.Equal(modified, File.GetLastWriteTimeUtc(source));
        Assert.Equal([1, 2, 3, 4, 5], File.ReadAllBytes(source));
    }

    /// <summary>기존 manager 진단을 무시하고 먼저 Clear하는 편의 처리를 추가하지 않는다. 진단이 중단하면 어떤 변경도 일어나지 않는다.</summary>
    [Fact]
    public void ExistingManagerAssertionPrecedesAllMutations()
    {
        using EditorTempRoot temp = new();
        CampaignTemplateStorage storage = new();
        EditorFiles files = Open(temp);
        string first = files.ResolvePath("d/First.fort");
        NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, first, 0);
        byte[] before = storage.Headers.ToArray();
        uint line = 0;
        NewFortManagerDiagnostics diagnostics = new()
        {
            TemplateAssertion = value =>
            {
                line = value;
                throw new InvalidOperationException("명시 진단 중단");
            }
        };
        Assert.Throws<InvalidOperationException>(() =>
            NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, files.ResolvePath("d/Other.fort"), 9, diagnostics));
        Assert.Equal(0x172u, line);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(first, storage.Name(1));
        Assert.Equal(before, storage.Headers.ToArray());
    }

    /// <summary>진단 장치를 연결하지 않으면 진단 지점에서 빈 성공이 아니라 명시 오류로 멈추고, 9칸 밖 슬롯은 거부한다.</summary>
    [Fact]
    public void UnconnectedDiagnosticAndSlotRangeAreExplicit()
    {
        using EditorTempRoot temp = new();
        CampaignTemplateStorage storage = new();
        EditorFiles files = Open(temp);
        string first = files.ResolvePath("d/First.fort");
        NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, first, 0);
        byte[] before = storage.Headers.ToArray();
        Assert.Throws<InvalidOperationException>(() => NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 1, first, 3));
        Assert.Equal(before, storage.Headers.ToArray());
        Assert.Equal(1, storage.OwnedImages);
        ArgumentOutOfRangeException range = Assert.Throws<ArgumentOutOfRangeException>(() =>
            NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 9, first, 0));
        Assert.Equal("slot", range.ParamName);
    }

    /// <summary>한글·이모지 작업 경로는 manager +12에 UTF-8 바이트로 보관되고 저장소가 실제 문자열로 되돌려 준다.</summary>
    [Fact]
    public void UnicodeWorkPathIsStoredAsUtf8()
    {
        using EditorTempRoot temp = new();
        CampaignTemplateStorage storage = new();
        EditorFiles files = Open(temp);
        string name = files.ResolvePath("d/새 맵 😀.fort");
        NewFortManagerDevices.CreateNewEditorFortManager(storage, files, 2, name, 0);
        uint token = Header(storage, 40);
        Assert.NotEqual(0u, token);
        Assert.Equal(name, storage.Name(2));
        Assert.Equal(System.Text.Encoding.UTF8.GetBytes(name).Length, ManagerName(storage, token).Length);
        Assert.NotEqual(name, ManagerName(storage, token));
    }
}
