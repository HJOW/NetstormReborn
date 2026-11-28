using Netstorm.Assets;
using Netstorm.Client.Host;

// 설정 적재 전 CD 확인과 같은 파일 이름/Client 소유자의 인계를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>CD 확인 callback의 실제 시점을 사용자 옵션 읽기 결과로 관측한다.</summary>
public sealed class HostCdStartupTests
{
    /// <summary>별도 사용자 파일만 변경해 CD 초기화→Config Startup의 상대 순서를 확인한다.</summary>
    [Fact]
    public void InitializesSameOwnerBeforeReadingUserOptions()
    {
        string data = GameDataLocator.FindDataDirectory() ?? throw new DirectoryNotFoundException("보호 데이터 부재");
        string root = Path.Combine(Path.GetTempPath(), "Netstorm-CDOrder-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root); string options = Path.Combine(root, "options.cfg");
        try
        {
            var baseline = new HostStartupScene(data, "cdStartupOrderProbe=1", userOptionsPath: options); baseline.SaveOptions();
            byte[] first = File.ReadAllBytes(options); baseline.Configuration.Configuration.Set("cdStartupOrderProbe", "41"); baseline.SaveOptions();
            byte[] changed = File.ReadAllBytes(options); File.WriteAllBytes(options, first);
            ClientCdGlobals? observed = null; RawFileName? names = null;
            var startup = new HostStartupScene(data, "", null, options, (owner, fileNames) =>
            {
                observed = owner; names = fileNames; Assert.Equal(0u, owner.StartupPresent);
                owner.InitializeStartup(new()
                {
                    Path = (current, marker) => CdPresence.JoinRootPath(fileNames, current, marker), Exists = _ => true,
                    Roots = output => { "D:\\\0\0"u8.CopyTo(output.Span); return 5; },
                });
                File.WriteAllBytes(options, changed);
            });
            Assert.Equal(41, startup.Configuration.GetInt("cdStartupOrderProbe"));
            Assert.Same(observed, startup.CdGlobals); Assert.Same(names, startup.Files.FileNameOwner);
            Assert.Equal(1u, startup.CdGlobals.StartupPresent); Assert.Equal((byte)'D', startup.CdGlobals.CachedRoot.Span[0]);
        }
        finally { if (File.Exists(options)) File.Delete(options); Directory.Delete(root); }
    }
}
