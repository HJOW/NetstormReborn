using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 실제 Client 상태 블록의 요청·앞단 캡처·공통 확정을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>큰 적재 효과를 합성하지 않고 현재 저장소와 원문 부모의 경계를 확인한다.</summary>
public sealed class CampaignMissionStartingTests
{
    /// <summary>같은 Client/로컬/시계의 실제 Surface와 현재 프레임을 만든다.</summary>
    private static ClientSurfaceGlobals Scene()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        var clock = new ClientClockGlobals(local); clock.Frame.Value = new() { Game = 13, Wall = 17 };
        return new(game, clock, new(), () => 0);
    }
    /// <summary>원문 블록의 지정 DWORD를 쓴다.</summary>
    private static void Put(Memory<byte> memory, int index, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(memory.Span.Slice(index * 4, 4), value);
    /// <summary>원문 블록의 지정 DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlyMemory<byte> memory, int index) => BinaryPrimitives.ReadUInt32LittleEndian(memory.Span.Slice(index * 4, 4));
    /// <summary>실제 next와 요청 캡처에 같은7/Zone 핸들1의 명시 입력을 설치한다.</summary>
    private static MissionFortLoadRequest Request(ClientSurfaceGlobals surface)
    {
        var request = new MissionFortLoadRequest(); MissionFortRequest.CaptureZone(request, MissionFortRequest.MakeLocalMissionZone());
        Put(surface.Clock.LocalState.State.Next, 0, 7); Put(surface.Clock.LocalState.State.Next, 32, 1);
        surface.Clock.LocalState.ZonePending.Value = 1;
        surface.Clock.LocalState.State.Next.Span.CopyTo(request.NextSnapshot); return request;
    }

    /// <summary>상태가 쓰는 모든 상자·Player·제목과 조회 시각은 같은 실제 owner다.</summary>
    [Fact]
    public void BindsCurrentOwnersWithoutCopyOrEffects()
    {
        ClientSurfaceGlobals surface = Scene(); ClientLocalStateGlobals local = surface.Clock.LocalState;
        byte[] title = new byte[80]; title[0] = 65;
        LocalMissionStateGlobals globals = CampaignMissionStarting.BindGlobals(surface, title);
        Assert.Same(local.ZonePending, globals.ZonePending); Assert.Same(local.GameTicks, globals.GameTicks);
        Assert.Same(local.MenuWindow, globals.MenuWindow); Assert.Same(local.ActivityEnd, globals.ActivityEnd);
        Assert.Same(local.ConnectEnd, globals.ConnectEnd); Assert.Same(surface.Gameplay.Player, globals.Player);
        globals.SetInBattle!(true); globals.SetSavedGame!(true);
        Assert.True(surface.Storage.OwnerMode.Battle); Assert.True(local.SavedGame.Value);
        surface.Gameplay.Authority.Server = false; surface.Storage.Links.Mission = true; surface.Clock.Frame.Value.Game = 31;
        Assert.False(globals.Server!()); Assert.True(globals.MissionWorld!()); Assert.Equal(31, globals.GameSeconds!());
        globals.TitleInitial.Span[0] = 0; Assert.Equal(0, title[0]);
        globals.ActivityEnd.Value = 23; Assert.Equal(23, local.AmbientSound.Deadline);
        globals.MenuWindow.Value = 42; Assert.Equal(42u, Read(local.State.Immediate, 33));
    }

    /// <summary>현재 요청을 복사한 뒤 분모 효과가 변경한 현재 예약·시각·출처를 캡처한다.</summary>
    [Fact]
    public void PrepareCapturesCurrentValuesAfterDenominatorAndPreservesTail()
    {
        ClientSurfaceGlobals surface = Scene(); ClientLocalStateGlobals local = surface.Clock.LocalState;
        MissionFortLoadRequest request = Request(surface);
        Put(local.State.Next, 34, 0x12345678); byte[] title = Enumerable.Repeat((byte)65, 80).ToArray();
        CampaignMissionStarting.Prepare(surface, request, title, () =>
        {
            Assert.Equal(7u, Read(local.State.Current, 0)); Assert.Equal(0u, Read(local.State.Next, 0));
            Put(local.State.Current, 33, 1); local.ZonePending.Value = 23; local.GameTicks.Value = 31; local.SourceType.Value = 42;
        });
        Assert.Equal(new LocalMissionStartingSnapshot { ZonePending = 23, GameTicks = 31, SourceType = 42 }, request.Starting);
        Assert.Equal(0, title[0]); Assert.All(title[1..], value => Assert.Equal(65, value));
        Assert.Equal(0x12345678u, Read(local.State.Current, 34)); Assert.Equal(0x12345678u, Read(local.State.Next, 34));
        Assert.Throws<InvalidOperationException>(() => CampaignMissionStarting.Capture(surface, request, title, () => { }));
    }

    /// <summary>분모 실패는 선행 전체 복사·next 첫DWORD0을 유지하며 제목/캡처를 성공으로 바꾸지 않는다.</summary>
    [Fact]
    public void PrepareFailureKeepsOnlyCompletedPrefix()
    {
        ClientSurfaceGlobals surface = Scene(); MissionFortLoadRequest request = Request(surface); byte[] title = new byte[80]; title[0] = 65;
        Assert.Throws<IOException>(() => CampaignMissionStarting.Prepare(surface, request, title, () => throw new IOException("실제 분모 실패")));
        Assert.Equal(7u, Read(surface.Clock.LocalState.State.Current, 0)); Assert.Equal(0u, Read(surface.Clock.LocalState.State.Next, 0));
        Assert.Null(request.Starting); Assert.Equal(65, title[0]);
    }

    /// <summary>실제 next7의 부모 뒤 같은 즉시8을 확정하고 접속 callback 전에 phase7을 공개한다.</summary>
    [Fact]
    public void RunCommitsSevenBeforeImmediateEightAndPreservesCurrentAddress()
    {
        ClientSurfaceGlobals surface = Scene(); ClientLocalStateGlobals local = surface.Clock.LocalState; MissionFortLoadRequest request = Request(surface);
        Put(local.State.Next, 21, 0x11223344); Put(local.State.Next, 31, 6799);
        var log = new List<uint>(); bool connected = false; byte[] title = new byte[80]; title[0] = 65;
        CampaignMissionStarting.Run(surface, title, () =>
        {
            Assert.Equal(7u, Read(local.State.Current, 0)); CampaignMissionStarting.Capture(surface, request, title, () => { });
            Put(local.State.Immediate, 0, 8);
        }, (address, port) =>
        {
            Assert.Equal(7u, Read(local.State.Phase, 0)); Assert.Equal(8u, Read(local.State.Current, 0));
            Assert.Equal(0x11223344u, Read(address, 0)); Assert.Equal(6799u, port); connected = true;
        }, (_, _) => throw new InvalidOperationException("미션 후처리에서는 접속 진행창 미선택"), log.Add);
        Assert.True(connected); Assert.Equal(new uint[] { 7, 8 }, log);
        Assert.Equal(1u, request.Starting!.Value.ZonePending); Assert.Equal(0, title[0]);
        Assert.Equal(7u, Read(local.State.Old, 0)); Assert.Equal(8u, Read(local.State.Phase, 0));
        Assert.Equal(9u, Read(local.State.Next, 0)); Assert.Equal(0u, Read(local.State.Current, 0));
    }
}
