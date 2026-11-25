// 실제 로컬 로그인의 지역 송신용 읽기와 서버 최초 설치가 미션 작업 source를 소유하는지 검증하는 장면이다.
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// R4-7c3b 미룬 세션 검사의 작업본 읽기 장면이다. cpppj tests/CampaignMissionReloadSourceTests.inc의
// ReloadSourceScene483과 같은 보호 VFS·다른 실제 작업 폴더/장부/Config를 함께 유지한다. Fort 경로·재적재
// 검사가 같은 장면을 빌린다. 1.003에서 추가됐다.
internal sealed class ReloadSourceScene : IDisposable
{
    /// <summary>읽기 전용 실제 원본 파일 시스템이다.</summary>
    public readonly GameFileSystem Assets;

    /// <summary>이 검사만 쓰는 임시 작업 폴더이며 기존 폴더를 삭제하지 않는다.</summary>
    public readonly string Root;

    /// <summary>각 실제 파일 읽기 경로를 독립 관측한다.</summary>
    public readonly List<string> Reads = [];

    /// <summary>최초 적재용 실제 원본 Fort 입력이다.</summary>
    public readonly byte[] Original;

    /// <summary>로그인 직전 재작성할 서로 다른 실제 원본 Fort 입력이다.</summary>
    public readonly byte[] Changed;

    /// <summary>Session reader보다 오래 사는 Client Config 경계다.</summary>
    public readonly ConfigInterface Config;

    /// <summary>읽기 중 fortSpec을 한 번만 바꾸었는가다.</summary>
    private bool _redirected;

    /// <summary>보호 VFS와 다른 실제 작업 폴더/장부/Config를 함께 유지한다.</summary>
    /// <param name="assets">읽기 전용 실제 원본 파일 시스템이다.</param>
    /// <param name="label">임시 폴더 이름에 붙는 꼬리표다.</param>
    public ReloadSourceScene(GameFileSystem assets, string label)
    {
        Assets = assets;
        Root = Path.Combine(Path.GetTempPath(), "netstorm-login-source-" + label + "-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(Path.Combine(Root, "maps"));
        Directory.CreateDirectory(Path.Combine(Root, "other"));
        Directory.CreateDirectory(Path.Combine(Root, "d"));
        Config = new ConfigInterface(path => Assets.TryReadAllBytes(path));
        Config.Configuration.Assign("DataDir=\"d\"\nmissionSpec=\"d/{local.1}.english\"\nfortSpec=\"d/{local.1}.fort\"\n[END]\n");
        Original = assets.ReadAllBytes("d/thewarbegins.fort");
        using var reference = new Netstorm.Client.Mission.MissionScript(Config, "Tutorial2");
        Assert.True(reference.Loaded);
        Changed = assets.ReadAllBytes(reference.FortPath());
        Assert.False(Original.SequenceEqual(Changed));
        const string script = "[Header]\nmissionType=\"Tutorial\"\nloadFort=\"WorkingReload483\"\n[A.]\nReload scenario\n[END]\n";
        File.WriteAllBytes(Path.Combine(Root, "d", "WorkingReload483.english"), Encoding.Latin1.GetBytes(script));
        File.WriteAllBytes(Path.Combine(Root, "maps", "WorkingReload483.fort"), Original);
        Config.Set("fortSpec", "maps/{local.1}.fort");
    }

    /// <summary>작업본 기준 상대 경로를 검사 전용 절대 경로로 바꾼다.</summary>
    /// <param name="configured">설정 경로다.</param>
    /// <returns>같은 작업 폴더의 절대 경로다.</returns>
    public string Absolute(string configured) =>
        Path.GetFullPath(Path.Combine(Root, configured.Replace('/', Path.DirectorySeparatorChar)));

    /// <summary>파일을 미리 읽지 않고 현재 경로 선택 장치를 Session 수명에 빌린다. 첫 Fort 읽기 때 다음 fortSpec으로 바꾼다.</summary>
    /// <param name="path">읽을 설정 경로다.</param>
    /// <returns>읽은 파일 바이트다.</returns>
    private byte[]? ReadAndObserve(string path)
    {
        Reads.Add(path);
        byte[] bytes = File.ReadAllBytes(Absolute(path));
        if (path.EndsWith(".fort", StringComparison.OrdinalIgnoreCase) && !_redirected)
        {
            _redirected = true;
            Config.Set("fortSpec", "other/{local.1}.fort");
        }

        return bytes;
    }

    /// <summary>source 자체와 읽기 관측의 소유 복사를 보유하는 미션 스크립트를 만든다.</summary>
    /// <returns>작업본 읽기 장치를 쓰는 미션 스크립트다.</returns>
    public Netstorm.Client.Mission.MissionScript Mission() => new(Config, "WorkingReload483", "", ReadAndObserve);

    /// <summary>임시 작업 폴더를 지운다. 지우지 못하면 남긴다.</summary>
    public void Dispose()
    {
        try
        {
            Directory.Delete(Root, true);
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
        }
    }
}
