using System.Buffers.Binary;
using Netstorm.Core.Original;

// 현재 Surface의 실제 가상80을 분배한다. Session의 Dais 고정 hook이나 별도 Simulation을 사용하지 않는다.
namespace Netstorm.Client.Audio;

/// <summary>CPP NativeFortSaveVirtual.Level의 현재 Pool/TYPE/프레임/해시 조회를 장면 월드 경계에 제공하는 조회 경계다.</summary>
public sealed class SceneMusicVirtualCount
{
    /// <summary>원본 제단의 실제 최종 가상표이며 +80은0044a480이다.</summary>
    private const uint AltarVtable = 0x005076b8;
    /// <summary>원본 Dais의 실제 최종 가상표이며 +80은004478f0이다.</summary>
    private const uint DaisVtable = 0x00507950;
    /// <summary>조회하는 같은 장기 Surface다. World/Session 교체 때 새 물리 사본을 만들지 않는다.</summary>
    public ClientSurfaceGlobals Surface { get; }
    /// <summary>동기 중첩 조회의 손상된 자기 순환만 검출하며 정상/예외 복귀 때 지운다.</summary>
    private readonly HashSet<int> _activeCounts = [];

    /// <summary>실제 같은 저장소/권한을 확인하되 TYPE 적재·World 생성·시각·RNG를 실행하지 않는다.</summary>
    public SceneMusicVirtualCount(ClientSurfaceGlobals surface)
    {
        ArgumentNullException.ThrowIfNull(surface);
        if (surface.Storage.Pool.Edition != OriginalEdition.Patch1078
            || !ReferenceEquals(surface.Storage.Authority(), surface.Gameplay.Authority)
            || !ReferenceEquals(surface.Storage.GameAux(), surface.Gameplay.GameAux))
            throw new ArgumentException("가상80의 같은 실제 PATCH Surface/권한/전역 필요", nameof(surface));
        Surface = surface;
    }

    /// <summary>원문 좌표의 정상 signedDWORD 절삭만 허용하며 잘못된 값을 유효 칸으로 보정하지 않는다.</summary>
    private static int Cell(float value)
    {
        if (!float.IsFinite(value) || (double)value < int.MinValue || (double)value >= 2147483648.0)
            throw new ArgumentOutOfRangeException(nameof(value), "제단 Count의 현재 좌표 signedDWORD 범위 부족");
        return (int)value;
    }

    /// <summary>현재 raw 가상표만으로 공통 레벨/제단/Dais를 선택한다. TYPE 생성자와 가상표의 쌍 일치를 강제하지 않는다.</summary>
    public uint Count(int sid)
    {
        RawSurfaceStorage storage = Surface.Storage;
        if (sid <= 0 || sid >= storage.Pool.Capacity)
            throw new ArgumentOutOfRangeException(nameof(sid), "가상80의 현재 SID 물리 범위 부족");
        ReadOnlySpan<byte> raw = storage.Pool.Slot(sid);
        if (raw[10] >= storage.Types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "가상80의 현재 TYPE 물리 범위 부족");
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        if (!RawSquidFactory.SupportsConstructedVtable(OriginalEdition.Patch1078, table))
            throw new InvalidOperationException("가상80의 현재 실제 가상표 몸체 미연결");
        if (!_activeCounts.Add(sid)) throw new InvalidOperationException("가상80의 현재 SID 재귀 사슬 순환");
        try
        {
            if (table == AltarVtable)
            {
                float pointX = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
                float pointY = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
                uint type = storage.GameAux().DaisTypeBox.Value;
                if (type >= storage.Types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "제단 Count의 현재 Dais TYPE 범위 부족");
                int x = Cell(pointX), y = Cell(pointY);
                var finder = new RawSquidFinder(storage.Pool, storage.Hash, storage.Types);
                uint flags = (storage.Types[(int)type].Flags2 & 6u) == 0 ? 1u : 0u;
                // 원문 typed 조회의 첫 현재 후보를 고정하고 그 후보의 현재 가상80에 다시 위임한다.
                for (int current = finder.Begin(new(x, y, x, y), flags); current != 0; current = finder.Next())
                    if (storage.Pool.Slot(current)[10] == type) return Count(current);
                return 0;
            }
            if (table == DaisVtable)
            {
                // 기존 의식의 같은 현재 TYPE/프레임 몸체를 사용하고 별도 Dais 성공값을 만들지 않는다.
                return RawTempleRitual.CountIn(storage.Pool, storage.Types, storage.Frames, sid);
            }
            // 기존 원본 base 가상80 몸체를 사용하며 free/dead/contained 거부를 추가하지 않는다.
            return new RawSquidLevel(storage.Pool, storage.Types,
                SquidLevelDiagnostics.MakeSquidLevelAssertion(OriginalEdition.Patch1078, storage.AssertDisplay)).Get(sid);
        }
        finally { _activeCounts.Remove(sid); }
    }
}
