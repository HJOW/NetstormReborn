// 아이스 타워의 파괴·재성장·주기를 같은 장기 물리/TYPE/Host와 가상 부모에 연결한다.
// cpppj/src/client/RawSurfaceIce.cpp 의 IceTowerGlobals/IceTower 장기 본문을 이식한다.
// 0.583에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Ice 우선)
namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 Pool/TYPE/Host로 아이스 타워를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>현재 역할의 주소만 설치한다. 0.583에서 추가됐다.</summary>
    private readonly IceTowerTypes _iceTowerTypes = new();

    /// <summary>모든 큰 선택 효과는 원문 위치에서만 요구한다. 0.583에서 추가됐다.</summary>
    private RawIceTower? _iceTower;

    /// <summary>현재 역할의 주소만 설치하며 예전 독립값이나 Session 초기값을 현재 전역에 쓰지 않는다. 0.583에서 추가됐다.</summary>
    public IceTowerTypes IceTowerGlobals()
    {
        if (!UsesNativeTransmissionDevices())
        {
            throw new InvalidOperationException("장기 아이스의 실제 Native 전역 미연결");
        }

        _iceTowerTypes.SharedIce = GameAux().RainBlockerTypeBox;
        _iceTowerTypes.SharedGrowing = GameAux().GrowingRainBlockerTypeBox;
        _iceTowerTypes.SharedNoIsland = Links.NoIslandTypeBox;
        return _iceTowerTypes;
    }

    /// <summary>모든 큰 선택 효과는 원문 위치에서만 요구하며 이미 있는 실제 장기 부모를 직접 사용한다. 0.583에서 추가됐다.</summary>
    public RawIceTower IceTower()
    {
        if (_iceTower is null)
        {
            var hooks = new IceTowerHooks
            {
                ReserveRegular = () => AllocateResourceRegular(),
                Regular = (sid, @event, period) => ConstructResourceRegular(sid, @event, period),
                BasePop = (sid, flags) => PostPopExecutor().PostPopBase(sid, flags),
                Grid = (x, y) =>
                {
                    int cx = IceCell(x);
                    int cy = IceCell(y);
                    if (cx < 0 || cy < 0 || cx > 255 || cy > 255)
                    {
                        return 0;
                    }

                    return Hash.Cell(0, cx, cy);
                },
                GenusAt = (x, y, genus) => DamageablePost().GenusAt(x, y, genus),
                Create = (type, flags) => FactoryExecutor().Create(type, (Netstorm.Core.Simulation.SidAllocation)flags),
                Unpop = (sid, flags) => UnpopAsset(sid, flags),
                Word = (sid, value) =>
                {
                    Span<byte> raw = Pool.RawSlot(sid);
                    raw[12] = (byte)value;
                    raw[13] = (byte)(value >> 8);
                    if (raw[10] == Links.BridgeType)
                    {
                        BuildingExecutor().NotifySurface(sid);
                    }
                },
                Owner = (sid, owner) => SetAssetOwner(sid, owner),
                Pop = (sid, x, y, flags) => PopAsset(sid, x, y, flags),
                Destroy = (sid, flags) => DestroyAsset(sid, flags),
                DropIsland = (x, y, flags, owner) => (_damageableDevices.DropIsland ?? throw new InvalidOperationException("장기 아이스의 선택된 실제 받침 정리 미결합"))(x, y, flags, owner),
                DamageablePost = (sid, flags) => DamageablePost().PostDestroy(sid, flags),
                BasePost = (sid, flags) => CommonDeletion().PostDestroy(sid, flags),
                SetFrame = (sid, frame, flags) => FrameExecutor().Set(sid, frame, flags),
            };
            _iceTower = new RawIceTower(
                Pool,
                Frames,
                IceTowerGlobals(),
                () => Authority().Server,
                () => Authority().Boss,
                () => LoadingDepth,
                hooks);
        }

        return _iceTower;
    }

    /// <summary>원문의 부호 있는 64 절삭과 하위 DWORD를 보존한다.</summary>
    private static int IceCell(float value)
    {
        double wide = value;
        if (!double.IsFinite(wide) || wide < -9223372036854775808.0 || wide >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((int)(uint)(long)wide);
    }
}
