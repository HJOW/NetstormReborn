using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// non-mine Notify의 실제 DataManager 저장 부모 결선을 검증한다.
// 2026-11-26 1.057에서 SL-4용으로 추가했다.
public sealed class HostTemplateSaveTests
{
    /// <summary>실제 자산을 읽는 세션과 TYPE 표를 묶는다.</summary>
    /// <returns>세션과 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) Scene()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        var session = new HostSession(new Kernel(), new GameRandom(71), new CrtRandom());
        session.MissionPlayer.BattleOwner = 1;
        return (session, table.Types);
    }

    /// <summary>TEST01을 세션 Template 1번에 설치한다.</summary>
    /// <param name="session">설치할 세션이다.</param>
    /// <param name="path">관리자 이름으로 쓸 파일 경로다.</param>
    private static void InstallTest01(HostSession session, IReadOnlyList<OriginalTypeRecord> types, string path)
    {
        byte[] original = TemplateStorageTestSupport.RealFortBytes("TEST01");
        session.Templates.InstallFile(1, FortTemplate.ReadSections(original), original, path, types.Count);
    }

    /// <summary>미연결 non-mine Notify는 명시 실패하며 isMine은 자연 복귀한다.</summary>
    [Fact]
    public void UnconnectedNonMineNotifyFailsAndMineReturnsNaturally()
    {
        var (session, types) = Scene();
        using var temp = new EditorTempRoot();
        var files = new EditorFiles(Path.Combine(temp.Root, "work"), [Path.Combine(temp.Root, "source")]);
        string path = files.ResolvePath(Path.Combine("d", "TEST01-unconnected.fort"));
        InstallTest01(session, types, path);
        session.Templates.Headers.Span[20 + 8] = 0;
        session.Templates.Headers.Span[20 + 16] = 0;
        Assert.Throws<InvalidOperationException>(() => session.NotifyPlayerTemplateState(1));
        // isMine이면 저장 부모 없이도 자연 복귀한다.
        session.Templates.Headers.Span[20 + 8] = 1;
        session.NotifyPlayerTemplateState(1);
        session.Templates.Headers.Span[20 + 8] = 0;
    }

    /// <summary>연결된 non-mine Notify는 실제 DataManager 저장으로 파일을 쓴다.</summary>
    [Fact]
    public void ConnectedNonMineNotifySavesThroughRealDataManager()
    {
        var (session, types) = Scene();
        using var temp = new EditorTempRoot();
        var files = new EditorFiles(Path.Combine(temp.Root, "work"), [Path.Combine(temp.Root, "source")]);
        files.EnsureDirectory("d");
        string path = files.ResolvePath(Path.Combine("d", "TEST01-session.fort"));
        InstallTest01(session, types, path);
        session.Templates.Headers.Span[20 + 8] = 0;
        session.Templates.Headers.Span[20 + 16] = 0;
        session.BuildTemplateSaveDevices(types, files.Writer());
        session.NotifyPlayerTemplateState(1);
        Assert.Throws<InvalidOperationException>(() => session.BuildTemplateSaveDevices(types, files.Writer()));
        byte[] saved = File.ReadAllBytes(path);
        Assert.Equal((byte)'F', saved[0]);
        FortTemplate written = FortTemplate.ReadSections(saved);
        FortTemplate current = session.Templates.Template(1);
        Assert.Equal(current.Sections.Count, written.Sections.Count);
        for (int index = 0; index < current.Sections.Count; index++)
            Assert.Equal(current.Sections[index].Span.ToArray(), written.Sections[index].Span.ToArray());
        // 두 번째 Notify도 같은 부모로 저장한다.
        session.NotifyPlayerTemplateState(1);
        Assert.Equal(saved, File.ReadAllBytes(path));
    }

    /// <summary>억제·숨김 관문은 저장 부모를 부르되 파일을 바꾸지 않는다.</summary>
    [Fact]
    public void SuppressedOrHiddenSaveKeepsFile()
    {
        var (session, types) = Scene();
        using var temp = new EditorTempRoot();
        var files = new EditorFiles(Path.Combine(temp.Root, "work"), [Path.Combine(temp.Root, "source")]);
        files.EnsureDirectory("d");
        string path = files.ResolvePath(Path.Combine("d", "TEST01-suppress.fort"));
        InstallTest01(session, types, path);
        session.Templates.Headers.Span[20 + 8] = 0;
        session.Templates.Headers.Span[20 + 16] = 0;
        int permissions = 0, writes = 0;
        session.BuildTemplateSaveDevices(types, new TemplateFileWriteHooks(
            (_, _) => permissions++,
            (_, _) => writes++),
            readOnlyForts: () => 1u);
        session.NotifyPlayerTemplateState(1);
        Assert.False(File.Exists(path));
        Assert.Equal(0, permissions);
        Assert.Equal(0, writes);
    }
}
