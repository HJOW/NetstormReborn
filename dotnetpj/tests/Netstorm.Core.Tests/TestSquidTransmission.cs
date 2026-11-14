using Netstorm.Core.Original;
using Netstorm.Net;

// 단독 프레임 검사에 실제 writer·압축과 명시적으로 빈 Player 연결 표를 공급한다.
namespace Netstorm.Core.Tests;

/// <summary>cpp TestSquidTransmission.h와 같은 무접속 입력이며 네트워크 장치를 열지 않는다.</summary>
internal static class TestSquidTransmission
{
    /// <summary>실제2048BYTE writer와 원문9칸의 빈 분배기를 연결하고 관측할 공통 상태를 반환한다.</summary>
    public static RawSquidTransmissionState? Bind(RawPop pop, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types)
    {
        if (pool.Edition != OriginalEdition.Patch1078) return null;
        var state = new RawSquidTransmissionState();
        var broadcast = new RawBattleBroadcast(pool.Edition, new byte[9 * 180], new Action<byte[]>?[9], () => 1, () => true);
        var writer = new RawSquidTransmission(pool, state, new RawSquidTransmissionHooks
        {
            Type = number => new SquidPopTypeReference(() => types[number]), DefaultType = () => 82,
            MaxHp = number => types[number].MaxHitPoints, BroadcastMutable = broadcast.SendMutable,
            Assertion = (_, _, _) => throw new InvalidOperationException("독립 프레임 검사의 실제 writer 원문 진단"),
        });
        pop.BindTransmission(writer.Pop); return state;
    }
}
