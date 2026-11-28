using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제Root장기자원과폭별State접근/음향소유자가복제없는같은물리를사용하는지확인한다.
namespace Netstorm.Client.Tests;

/// <summary>원문초기값/현재뷰/장치교체보존을검증한다.</summary>
public sealed class ClientStateResourcesTests
{
    /// <summary>자료/파일/시계읽기없이같은Client장기소유자를구성한다.</summary>
    private static (ClientSurfaceGlobals Surface, ClientLocalStateGlobals Local, ClientConnectionGlobals Connections) Scene()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        return (new(game, new(local), new(), () => throw new IOException("구성중시계읽기")), local, new(local));
    }

    /// <summary>실제최초Template헤더/이름표/표식/seed/음악기한을초기화하며곡/파일을열지않는다.</summary>
    [Fact]
    public void InitialOwnersMatchNativeStorageWithoutEffects()
    {
        var resources = new ClientStateResources(); Assert.Equal(180, resources.Templates.Headers.Length); Assert.All(resources.Templates.Headers.ToArray(), value => Assert.Equal(0, value));
        Assert.Equal(0u, resources.Templates.SectionNamesOwner.CurrentToken); Assert.Equal(0u, resources.FortCoreSeed.Value); Assert.Equal(0u, resources.Popup.Command);
        Assert.All(resources.Popup.ArgumentBytes, value => Assert.Equal(0, value)); Assert.Equal(3, resources.Music.Index); Assert.Equal(167u, resources.Music.Tint);
        Assert.Equal(0.0, resources.Music.SongEnd.Value); Assert.Null(resources.Music.CurrentBattle);
    }

    /// <summary>후속Context를만들어도현재동일포인터/상자/표식과상위seed비트를보존한다.</summary>
    [Fact]
    public void ContextSharesCurrentResourcesAndNsInfoWithoutResetting()
    {
        var resources = new ClientStateResources(); var (surface, local, connections) = Scene(); var cloud = new Netstorm.Client.Display.ClientKencloudGlobals(); var nsInfo = new ClientNsInfoGlobals();
        resources.FortCoreSeed.Value = 0xabcd9876; resources.Popup.Mark(0x12345678, "same"); nsInfo.Zone.Value = 0x87654321;
        ClientStateWorldContext first = resources.Context(surface, local, connections, cloud, nsInfo), next = resources.Context(surface, local, connections, cloud, nsInfo);
        Assert.Same(resources.Templates, first.Templates); Assert.Same(resources.Popup, first.Popup); Assert.Same(nsInfo.Zone, first.NsInfoZone); Assert.Same(resources.Music.SongEnd, first.SongEnd); Assert.Same(first.SongEnd, next.SongEnd);
        var globals = ClientStateWorldGlobals.Bind(first); globals.Write16!(0x5c89b4, 0); Assert.Equal(0xabcd0000u, resources.FortCoreSeed.Value);
        globals.Write32!(0x5ca938, 0); Assert.Equal(0u, resources.Popup.Command); Assert.Equal("same", resources.Popup.Argument());
        globals.Write64Bits!(0x565e68, 0x7ff8000012345678); Assert.Equal(0x7ff8000012345678ul, BitConverter.DoubleToUInt64Bits(resources.Music.SongEnd.Value));
        Assert.Equal(0x87654321u, nsInfo.Zone.Value); Assert.Equal(0u, resources.Templates.SectionNamesOwner.CurrentToken);
    }

    /// <summary>다른Client의로컬상태를잘못넘기면접근설치가실패하고기존장기자원은유지된다.</summary>
    [Fact]
    public void BindingRejectsMixedClientWithoutChangingResources()
    {
        var resources = new ClientStateResources(); var (surface, _, connections) = Scene(); var other = new ClientLocalStateGlobals(new(), new()); resources.FortCoreSeed.Value = 73;
        var context = resources.Context(surface, other, connections, new(), new()); Assert.Throws<ArgumentException>(() => ClientStateWorldGlobals.Bind(context));
        Assert.Equal(73u, resources.FortCoreSeed.Value); Assert.Equal(0u, resources.Templates.SectionNamesOwner.CurrentToken);
    }

    /// <summary>실제음향장치는음소거일때도동일장면상태를빌리고반납이그기한을재설정하지않는다.</summary>
    [Fact]
    public void MutedAudioBorrowsSameSceneAndDoesNotResetOnDispose()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var resources = new ClientStateResources(); resources.Music.SongEnd.Value = 123.5;
        // 창/장치/worker를 열지 않는 실제 준비 생성자와 명시 보류로 기존 수명 목적을 유지한다.
        using (var audio = new HostMissionAudio(new HostStartupScene(data), () => throw new IOException("생성중벽시계"), resources.Music))
        {
            audio.PushMute();
            Assert.Same(resources.Music, audio.Scene);
        }
        Assert.Equal(123.5, resources.Music.SongEnd.Value);
    }
}
