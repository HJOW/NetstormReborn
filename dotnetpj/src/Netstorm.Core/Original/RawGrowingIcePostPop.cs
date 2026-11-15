// 성장형 아이스의 최초 확보 뒤 현재 프레임 수와 공통 부모 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>150의48ead0 최초 후처리이며 성장/재생성/공격 몸체와 구별한다.</summary>
public static class RawGrowingIcePostPop
{
    /// <summary>원문5069bc의 전체 성장시간40초다.</summary>
    private const float GrowthSeconds = 40;
    /// <summary>원문 성장 예약 사건0x3a6이다.</summary>
    private const uint GrowthEvent = 0x3a6;
    /// <summary>선택한 실제 장치만 요구한다. 미도달 장치를 앞당겨 요구하지 않는다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("성장형 아이스 실제 후처리 하위 미연결");
    /// <summary>확보→현재 TYPE/numFrames→생성자 뒤 공통 부모를 실행하며 extra/editor 관문을 추가하지 않는다.</summary>
    public static void PostPop(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, int sid, uint flags,
        Func<bool>? reserve, Action<int, uint, float>? regular, Action<int, uint>? basePop)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frames);
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != 188) throw new ArgumentException("성장형 아이스 후처리 판본/프레임 오류", nameof(frames));
        if ((flags & 1) != 0 && Need(reserve)())
        {
            int type = pool.Slot(sid)[10];
            if (type >= frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "아이스 현재 프레임 TYPE 오류");
            float period = GrowthSeconds / frames[type].Codes.Count; Need(regular)(sid, GrowthEvent, period);
        }
        Need(basePop)(sid, flags);
    }
}
