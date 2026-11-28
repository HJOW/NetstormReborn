using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 주 프레임에서 전역 붕괴 뒤 장면 감독을 실행한다.
namespace Netstorm.Client.Host;

/// <summary>실제 세션의 이른 붕괴와 장기 Scene 감독의 제품 연결이다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>현재 세션의 입자/음향 시각과 현재 슬롯 위치의 균열 소리를 결합한다.</summary>
    private void BindNativeBridgeDecay(HostSession session, HostMissionBurning burning)
    {
        HostMissionAudio audio = _missionAudio ?? throw new InvalidOperationException("다리 붕괴의 실제 장기 음향 미설치");
        session.BuildBridgeDecayDevices(_surfaceGlobals.Storage, sid =>
        {
            ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
            float x = BitConverter.Int32BitsToSingle(BinaryPrimitives.ReadInt32LittleEndian(raw[14..]));
            float y = BitConverter.Int32BitsToSingle(BinaryPrimitives.ReadInt32LittleEndian(raw[18..]));
            audio.PlayEffectAt(x, y, "bridgeCrack.wav", 0);
        }, time =>
        {
            burning.PrepareFrame(time.Game, unchecked((uint)time.Number));
            audio.SetFrame(unchecked((uint)time.Number));
        });
    }

    /// <summary>Movie 잠금 없는 Wind/MonitorTime 뒤에 호출하며 게임 Pause에서도 Scene은 현재 벽시각을 읽는다.</summary>
    private void UpdateNativeWorldScene()
    {
        ClientBridgeDecay.Run(_surfaceGlobals.Storage, _gameplay, _clientClock.Frame, _clientClock.Clock.IsPaused());
        (_missionAudio ?? throw new InvalidOperationException("주 프레임의 실제 Scene 음향 미설치")).SceneFrame();
    }
}
