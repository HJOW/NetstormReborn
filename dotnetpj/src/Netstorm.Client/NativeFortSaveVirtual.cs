using System.Buffers.Binary;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;

// 현재 실제 Pool의 저장용 가상60/7c/80을 원문 몸체에 연결한다.
namespace Netstorm.Client;

/// <summary>cpppj/src/client/NativeFortSaveVirtual.cpp/.h의 저장 조회다. World·Session·난수 상태를 만들거나 초기화하지 않는다.</summary>
public sealed class NativeFortSaveVirtual
{
    /// <summary>같은 장기 Surface와 현재 Pool/TYPE를 빌린다.</summary>
    public ClientSurfaceGlobals Surface { get; }
    /// <summary>이미 연결된 제단/Dais/base 가상80의 같은 현재 조회다.</summary>
    private readonly SceneMusicVirtualCount _level;

    /// <summary>동일 PATCH 권한/보조 전역만 검증하며 실제 자산 조회는 선택된 분기까지 늦춘다.</summary>
    public NativeFortSaveVirtual(ClientSurfaceGlobals surface)
    {
        _level = new(surface);
        Surface = surface;
    }

    /// <summary>DWORD SID를 자르기 전에 실제 Pool/WORD 범위를 확인한다. free/dead·예약 슬롯을 선행 배제하지 않는다.</summary>
    private int Slot(uint sid)
    {
        if (sid > ushort.MaxValue || sid >= Surface.Storage.Pool.Capacity)
            throw new ArgumentOutOfRangeException(nameof(sid), "Fort 저장 가상 조회의 실제 DWORD SID 범위 부족");
        return (int)sid;
    }

    /// <summary>현재 가상표를 먼저 선택하고 generic 분기에서만 현재 TYPE flags1을 읽는다.</summary>
    public uint ObjectRobust(uint sid)
    {
        RawSurfaceStorage storage = Surface.Storage;
        ReadOnlySpan<byte> raw = storage.Pool.Slot(Slot(sid));
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        if (!RawSquidFactory.SupportsConstructedVtable(storage.Pool.Edition, table))
            throw new InvalidOperationException("Fort Robust의 현재 가상표 몸체 미연결");
        if (table is 0x5070c0 or 0x508cc8 or 0x5092d0 or 0x5096a0 or 0x509758
            or 0x509810 or 0x50a8e8 or 0x50db78 or 0x50ee08) return 0;
        if (table is 0x5040c0 or 0x504198)
        {
            int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
            int type = storage.Pool.Slot(Slot(sid))[10];
            if (type >= storage.Types.Count || type >= storage.Frames.Count)
                throw new ArgumentOutOfRangeException(nameof(sid), "Fort 벌룬 Robust의 현재 TYPE/프레임 범위 부족");
            return frame < storage.Frames[type].Codes.Count ? 1u : 0u;
        }
        int currentType = storage.Pool.Slot(Slot(sid))[10];
        if (currentType >= storage.Types.Count)
            throw new ArgumentOutOfRangeException(nameof(sid), "Fort Robust의 현재 TYPE 범위 부족");
        return (storage.Types[currentType].Flags1 & 0x20000000u) == 0 ? 1u : 0u;
    }

    /// <summary>가상60의 world 플래그 진단 뒤 같은 현재 BYTE를 재조회한다. 진단의 정상 복귀를 실패로 바꾸지 않는다.</summary>
    public uint ContentRobust(uint sid)
    {
        RawSurfaceStorage storage = Surface.Storage;
        if ((storage.Pool.Slot(Slot(sid))[20] & 0x80) != 0)
            storage.AssertDisplay("!(getListFlags() & CFF_WORLD)", @"\Ns\O\Squid.cpp", 0xbb1);
        return (storage.Pool.Slot(Slot(sid))[20] & 0x34) != 0 ? 1u : 0u;
    }

    /// <summary>원문 hasLevel의 OR 조건을 보존하는 기존 base 몸체를 같은 현재 TYPE로 호출한다.</summary>
    public uint HasLevel(uint sid)
    {
        RawSurfaceStorage storage = Surface.Storage;
        return new RawSquidLevel(storage.Pool, storage.Types,
            SquidLevelDiagnostics.MakeSquidLevelAssertion(storage.Pool.Edition, storage.AssertDisplay)).Has(Slot(sid)) ? 1u : 0u;
    }

    /// <summary>제단/Dais/base를 현재 가상표로 분배하는 기존 공통 Count를 그대로 사용한다.</summary>
    public uint Level(uint sid) => _level.Count(Slot(sid));
}
