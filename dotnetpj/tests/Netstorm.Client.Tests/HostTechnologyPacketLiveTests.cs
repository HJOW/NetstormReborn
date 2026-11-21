using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 기술 생성 통지 안에서 owner를 반납하여 항목별 Runtime 선택을 관측한다.
namespace Netstorm.Client.Tests;

/// <summary>동일 실제 월드/Player/내용물과69h 장기 목록을 연결한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>현재 owner는 이번 표시 끝까지 살아 있고 다음 항목은 변경된 목록/현재 Session을 다시 선택한다.</summary>
    [Theory, SupportedOSPlatform("windows")]
    [InlineData(true)] [InlineData(false)]
    public void ProductTechnologyPacketPinsCurrentOwnerPerIndication(bool stopList)
    {
        if (!OperatingSystem.IsWindows()) return;
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 0);
        using var scene = new BurningContext(sharedSurface: surface); HostSession session = scene.Session;
        using var owner = HostMissionRuntime.Create(session, surface.Storage, scene);
        int parent = session.BuildPlayerWorldHooks().Create!(152, 2), gump = session.BuildPlayerWorldHooks().Create!(3, 2);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x9c), (uint)parent);
        var forms = new RawSquidForms(session.Pool); forms.Attach(gump, parent, 0x110); BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(gump)[18..], 7);
        int handled = 0;
        CampaignProductionDevices devices = ProductionDevices(); devices.Gump = id => { Assert.Equal(7u, id); return 77; };
        devices.Handle = (token, _, form, _) =>
        {
            handled++; Assert.Equal(77u, token); Assert.Equal(71, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot((int)form)[18..]));
            Assert.True(game.RuntimeCallActive(session)); owner.Dispose(); Assert.False(owner.Released);
            if (stopList) game.GameAux.TypeUse[1] = 0;
        };
        session.BuildProductionWorldDevices(devices);
        game.GameAux.TypeUse[0] = 71;
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, ProductionPacketHooks(session, []));
        ClientTechnologyPacketDefaults.Install(dispatch, game, surface.Storage);
        byte[] packet = [0, 0, 0, 0, 0, 72, 0];
        if (stopList) dispatch.InvokeDirect(0x69, uint.MaxValue, null, packet);
        else Assert.Contains("Runtime", Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x69, uint.MaxValue, null, packet)).Message);
        Assert.Equal(1, handled); Assert.True(owner.Released); Assert.False(game.RuntimeCallActive(session)); Assert.False(game.Borrowed());
        Assert.Equal(71, game.GameAux.TypeUse[0]); Assert.Equal(stopList ? 0 : 72, game.GameAux.TypeUse[1]);
    }
}
