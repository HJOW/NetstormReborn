using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

/// <summary>실제 미션·타입 자산과 같은 상태 저장소를 사용하는 초기화 조립을 검사한다.</summary>
public sealed class HostTutorialInitializationTests
{
    [Fact]
    public void ProtectedTutorialsPrepareSharedRequestAndRestrictions()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var types = new TypeCatalog(startup.Files);
        int Find(string name) => types.Types.FirstOrDefault(type => string.Equals(type.RuntimeName, name, StringComparison.OrdinalIgnoreCase))?.RuntimeIndex ?? -1;
        byte[] players = new byte[9 * PlayerParticipation.PlayerStride]; uint[] allies = new uint[81]; byte[] hidden = new byte[9];
        var participation = new PlayerParticipation(players, allies, hidden, new(), new()); var release = new TutorialMissionReleaseGlobals();
        var requestGlobals = new MissionFortRequestGlobals { Port = 6799 }; Encoding.ASCII.GetBytes("zlocal,*\0").CopyTo(requestGlobals.LocalAddress, 0);
        byte[] options = new byte[87]; uint[] restrictions = new uint[256]; restrictions[255] = 0x7391;
        string? pending = null; HostTutorialInitialization? init = null;
        var guide = new GuideMissionOwner(startup.Configuration, new(), new(), participation, new(), release, new(), new()
        {
            Created = _ => { }, ReleaseFortRequest = () => { pending = null; init!.ReleaseRequest(); },
            SetAllowAll = value => Array.Fill(restrictions, value == 0 ? 1u : 0u, 0, 255)
        });
        int clears = 0;
        init = new HostTutorialInitialization(guide, requestGlobals, options, restrictions, () => clears++, name => name, Find,
            () => pending != null, fort => pending = fort);
        guide.InitGlobals.InitialSubscriber = 1;
        for (int index = 1; index <= 6; index++)
        {
            guide.Begin("Tutorial" + index); init.Initialize();
            Assert.Equal(index, clears); Assert.Equal((uint)index, guide.Runtime!.Gate.TutorialNumber);
            Assert.Equal(1u, guide.Runtime.State.Flags); Assert.Equal(1u, guide.InitGlobals.LoginSubscriber);
            Assert.NotNull(init.Request); Assert.Equal(init.Request!.Fort, pending); Assert.NotEmpty(pending!);
            Assert.Equal(7u, requestGlobals.Next[0]); Assert.Equal((uint)index, requestGlobals.Sequence); Assert.Equal(86, init.Request.Zone.Length);
            Assert.Equal((byte)2, options[1]); Assert.Equal((byte)3, options[4]); Assert.Equal(0x7391u, restrictions[255]);
            Assert.Equal(0u, restrictions[Find("windVortex")]);
            if (index <= 2) Assert.Equal(index == 1 ? 1u : 0u, restrictions[Find("sunArcher")]);
        }
        guide.Release(); Assert.Null(init.Request); Assert.Null(pending);
    }
}
