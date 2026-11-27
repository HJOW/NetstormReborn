using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 일반 객체 선택 막대가 현재 세션의 실제 공간·공장·프로세스를 조회한다.
namespace Netstorm.Client.Host;

/// <summary>같은 세션의 실제 객체 표시 조회를 조립한다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 다리 수명 숫자 옵션에 따라 원본 추가 그림을 처리한다.</summary>
    /// <param name="sid">다리 SID다. 기본 0 관문에서는 읽지 않는다.</param>
    /// <param name="x">현재 화면 가로 기준점이다.</param>
    /// <param name="y">현재 화면 세로 기준점이다.</param>
    /// <remarks>cpppj RawBridgeDraw::Additional과 같은 실제 005453ac 전역을 사용한다. 개발용 숫자 표시는 아직 미복원이다.</remarks>
    public void DrawBridgeAdditional(int sid, int x, int y)
    {
        if (Gameplay.BridgeLifeNumbers.Value != 0)
            throw new InvalidOperationException("개발용 다리 수명 숫자 표시 미복원");
    }

    /// <summary>현재 공간·공장·정기 프로세스와 객체 표시의 조회 경계를 연결한다.</summary>
    /// <param name="hash">현재 월드의 실제 공간 해시다.</param>
    /// <param name="types">같은 공장에서 선택할 현재 TYPE 표다.</param>
    /// <returns>픽셀·가상 그림과 함께 조립할 실제 조회 장치다.</returns>
    /// <remarks>cpppj RawCampaignFactoryDisplay::Draw의 현재 프로세스 조회와 누락된 상태 효과 오류를 따른다.</remarks>
    public SpriteDrawHooks BuildSpriteWorldRules(RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        var lookup = new RawContainedLookup(Pool, DestroyWorldObject);
        return new SpriteDrawHooks
        {
            Surface = SpriteDraw.MakeSpriteSurfaceLookup(hash),
            TypeHitPoints = type =>
            {
                if (type >= types.Count) throw new ArgumentOutOfRangeException(nameof(type), "객체 표시 TYPE HP 범위 오류");
                return (_fortFactory ?? throw new InvalidOperationException("객체 표시 실제 HP 공장 미연결"))
                    .ReadSelectedHitPoints(types[(int)type], types);
            },
            ConstructionActive = sid => PlayerProcesses.FindEvent(sid, 0x25d) is not null,
            Resource = sid =>
            {
                RegularProcess? cast = PlayerProcesses.FindEvent(sid, 0x493);
                if (cast is null) return null;
                var finder = new RawContainedFinder(Pool, new ContainedFinderState { SharedBoss = () => Gameplay.Authority.Boss });
                int content = 0;
                // 원본은 상태를 추가로 거르지 않고 mask 1인 첫 내용물을 선택한다.
                for (int candidate = finder.Begin(sid); candidate != 0; candidate = finder.Next())
                {
                    if ((BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(candidate)[20..]) & 1) == 0) continue;
                    content = candidate;
                    break;
                }
                uint kind = BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(content)[18..]);
                return new SpriteResourceStatus(kind, unchecked((int)cast.Count()));
            },
            Timers = (sid, type) =>
            {
                int form = lookup.Find(sid, type);
                if (form == 0) return null;
                FlyerProcess flyer = PlayerProcesses.FromForm(form) as FlyerProcess
                    ?? throw new InvalidOperationException("객체 표시 비행 타이머의 실제 프로세스 미연결");
                ReadOnlySpan<byte> bytes = flyer.Fields.Bytes;
                return [BinaryPrimitives.ReadDoubleLittleEndian(bytes[72..]), BinaryPrimitives.ReadDoubleLittleEndian(bytes[104..]), BinaryPrimitives.ReadDoubleLittleEndian(bytes[40..])];
            },
            StatusProcesses = () =>
            {
                // 원본은 예약 슬롯 0을 건너뛰고 현재 Kernel 순서의 상태 효과를 방문한다.
                for (uint pid = 1; pid <= Kernel.Capacity(); pid++)
                {
                    if (Kernel.Get(pid) is not SquidProcess process || Pool.Slot(process.Form())[10] != 57) continue;
                    throw new InvalidOperationException("객체 표시 실제 상태 효과 타이머 미연결");
                }
                return [new SpriteStatusProcess()];
            },
        };
    }
}
