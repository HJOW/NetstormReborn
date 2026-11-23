// 실제 장기 Native 프레임 정기를 같은 현재 TYPE와 Frame에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawSurfaceFrameRegular.cpp의 디스크 전용 장기 실행 입구다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>현재 Native 계약과 실제 TYPE+110/+114를 값복사해 signed 원문 산술로 지정한다.</summary>
    public float DiscRegular(int sid, uint ev, uint count, float payload)
    {
        if (!UsesNativeTransmissionDevices())
            throw new InvalidOperationException("장기 디스크 정기의 Native 판본 계약");
        _ = ev;
        _ = count;
        return RawDiscRegular.HandleIn(Pool, FrameExecutor(), type =>
        {
            if (type >= Types.Count) throw new ArgumentOutOfRangeException(nameof(type));
            return new(Types[(int)type].Directions, Types[(int)type].FrameCount);
        }, sid, payload, true);
    }
}
