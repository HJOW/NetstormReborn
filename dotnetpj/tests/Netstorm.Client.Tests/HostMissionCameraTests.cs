using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 미션 입장 카메라를 내 사제·월드 가운데로 옮긴다.
// 2026-11-26 1.076에서 R5-4c용으로 추가했다.
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>입장하면 내 사제 쪽을 목표로 두고 갱신 번호를 올린다.</summary>
    [Fact]
    public void ResetMissionCameraHomesOnOwnPriest()
    {
        var (session, types) = Scene();
        int priest = PlacePriest(session, types, 0, 9);
        ReadOnlySpan<byte> raw = session.Pool.Slot(priest);
        float wx = System.Buffers.Binary.BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
        float wy = System.Buffers.Binary.BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
        int footX = types[raw[10]].FootX;
        int footY = types[raw[10]].FootY;
        var state = new CameraHomeState { LocalPlayer = session.MissionPlayer.BattleOwner, Battle = true, Width = 1024, Height = 768 };
        uint revision = session.Gameplay.MapRevision.Value;
        uint redraw = session.CameraWorldRedraw.Value;
        session.ResetMissionCamera(state, types, 1024, 768);
        int expectedX = unchecked(-RawCameraView.FtolLow(-16.0 * ((double)wx - footX)) - 1024 / 2);
        int expectedY = unchecked(-RawCameraView.FtolLow(-11.0 * ((double)wy - footY)) - 768 / 2);
        Assert.Equal(new[] { expectedX, expectedY }, session.Gameplay.Camera.Target.Pixels);
        Assert.Equal(unchecked(revision + 1), session.Gameplay.MapRevision.Value);
        Assert.Equal(unchecked(redraw + 1), session.CameraWorldRedraw.Value);
    }

    /// <summary>사제가 없으면 월드 가운데(128,128)로 간다.</summary>
    [Fact]
    public void ResetMissionCameraFallsBackWithoutPriests()
    {
        var (session, types) = Scene();
        var state = new CameraHomeState { LocalPlayer = session.MissionPlayer.BattleOwner, Battle = true, Width = 1024, Height = 768 };
        session.ResetMissionCamera(state, types, 1024, 768);
        int expectedX = unchecked(-RawCameraView.FtolLow(-16.0 * 128.0) - 1024 / 2);
        int expectedY = unchecked(-RawCameraView.FtolLow(-11.0 * 128.0) - 768 / 2);
        Assert.Equal(new[] { expectedX, expectedY }, session.Gameplay.Camera.Target.Pixels);
    }
}
