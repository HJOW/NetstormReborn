using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 이미 월드에 놓여 있던(void 가 아닌) 오브젝트를 Take 가 다시 받을 때 먼저 공간 등록을 푸는 원본 Unpop 의 연결 지점.
/// 원본 월드 코어의 Unpop 모듈(W02 후속)이 구현해 <see cref="RawSquidFactory"/> 에 넘긴다.
/// </summary>
public interface IRawUnpop
{
    /// <summary>오브젝트를 공간(해시·spot·표면 그래프)에서 푼다. 끝나면 슬롯은 void 상태여야 한다.</summary>
    /// <param name="sid">풀 슬롯 번호.</param>
    /// <param name="type">그 오브젝트의 타입 레코드.</param>
    void Unpop(int sid, OriginalTypeRecord type);
}

/// <summary>
/// 오브젝트 슬롯을 할당해 타입별 생성자 필드를 채우는 원본 팩토리(Squid.cpp 의 생성·Take·공통 초기화, 10.78).
/// 순서는 원본 그대로 "슬롯 할당 → 생성자 쓰기 → 타입 바이트 → 공통 postCreate" 이고, Take 는 풀 목록·카운터를 건드리지 않고
/// free/dead 를 지운 뒤 생성자 필드만 다시 쓴다(<c>004af530</c> 생성, <c>004af610</c> Take, <c>004b0c60</c> postCreate, <c>004ad4a0</c> postTake,
/// <c>004ad470</c> 최초 Pop 플래그). 미복원 생성자(표에 없는 주소)·null/assert 생성자·잘못된 슬롯은 풀을 바꾸기 전에 예외로 거부하며,
/// 기본 객체로 조용히 대신하지 않는다.
/// 슬롯 필드 위치(10.78): 가상 표 +0, 섬 번호 +8, 타입 +10, 상태 +11, HP +26(4바이트), 소유자 +34, zOrder +35, 추가 비트 +40.
/// 생산 칸 contained Form 과 GumpForm 의 생성은 Form 모듈(W03)에서 옮긴다.
/// 근거: docs/exe/cpp-creation-reconstruction.md, docs/exe/cpp-derived-reconstruction.md, cpppj/src/o/SquidFactory.cpp (W02 첫 조각, 2026-10-16).
/// </summary>
public sealed class RawSquidFactory
{
    /// <summary>슬롯 안 가상 표(클래스 식별 값, 4바이트)의 위치.</summary>
    private const int VtableOffset = 0;

    /// <summary>슬롯 안 HP(4바이트)의 위치.</summary>
    private const int HpOffset = 26;

    /// <summary>슬롯 안 소유자 바이트의 위치.</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 zOrder 바이트의 위치.</summary>
    private const int ZOrderOffset = 35;

    /// <summary>슬롯 안 추가 비트 바이트의 위치. 받침의 abstract 비트(1)와 최초 Pop 의 매립 비트(8)를 담는다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>상태 바이트의 contained 비트(생산 칸에 들어 있는 내용물).</summary>
    private const byte ContainedBit = 8;

    /// <summary>mana 타입 번호. 약화 옵션이 켜지면 최대 HP 를 4 로 나눈다.</summary>
    private const int ManaType = 158;

    /// <summary>패치판에서 생성이 금지된 fakeThreeByThreeSurface 타입 번호.</summary>
    private const int FakeSurfaceType = 162;

    /// <summary>슬롯을 할당받을 풀. 팩토리는 풀의 목록·카운터를 Allocate 로만 바꾼다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 호출자의 배열 내용이 바뀌면 그대로 반영된다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>mana 약화 옵션. 켜지면 mana 의 최대 HP 를 0 쪽으로 절삭한 정수 1/4 로 쓴다.</summary>
    private readonly bool _weakenedMana;

    /// <summary>void 가 아닌 오브젝트를 Take 할 때 쓰는 공간 해제. 연결 전이면 null 이며 그 경우 Take 는 거부한다.</summary>
    private readonly IRawUnpop? _unpop;

    /// <summary>팩토리를 만든다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 개수는 <see cref="OriginalConstructors.TypeCount"/> 와 같아야 한다.</param>
    /// <param name="weakenedMana">mana 약화 옵션.</param>
    /// <param name="unpop">non-void Take 의 공간 해제. 없으면 null.</param>
    public RawSquidFactory(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, bool weakenedMana = false, IRawUnpop? unpop = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types;
        _weakenedMana = weakenedMana;
        _unpop = unpop;
    }

    /// <summary>이 팩토리가 쓰는 슬롯 풀. 생성 효과 어댑터가 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 오브젝트를 만든다. 슬롯 할당 → 생성자 쓰기 → 타입 바이트 → postCreate 순서이며 새 슬롯 번호를 돌려준다.
    /// 지원하지 않는 타입(미복원 생성자, 패치판 금지 타입 162, 자산 타입 밖)은 슬롯을 할당하기 전에 예외로 거부한다.
    /// </summary>
    /// <param name="type">타입 번호(70 이상).</param>
    /// <param name="flags">할당 영역 비트(1 예측, 2 클라이언트).</param>
    public int Create(uint type, SidAllocation flags = SidAllocation.Server)
    {
        OriginalTypeRecord record = Type(type, requireSupported: true);
        if (type == FakeSurfaceType)
        {
            throw new InvalidOperationException("패치판 fakeThreeByThreeSurface 는 생성할 수 없습니다");
        }
        int sid = _pool.Allocate(flags);
        Span<byte> bytes = _pool.AllocatedBytes(sid);
        ApplyConstructor(bytes, record);
        bytes[RawSidPool.TypeOffset] = (byte)type;
        PostCreate(sid);
        return sid;
    }

    /// <summary>
    /// 이미 확보된 슬롯에 생성자 쓰기만 적용한다. 타입 바이트·상태·풀 목록·HP·소유자는 덮지 않는다(독립 검사와 재초기화용).
    /// 5 번 미만·빈·contained 슬롯은 거부한다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="sid">대상 슬롯 번호.</param>
    public int Construct(uint type, int sid)
    {
        OriginalTypeRecord record = Type(type, requireSupported: true);
        Span<byte> bytes = _pool.RawSlot(sid);
        if (sid < RawSidPool.FirstOrdinary || (bytes[RawSidPool.StateOffset] & (RawSidPool.FreeBit | ContainedBit)) != 0)
        {
            throw new InvalidOperationException("생성자 슬롯 상태 오류");
        }
        ApplyConstructor(bytes, record);
        return sid;
    }

    /// <summary>
    /// 서버가 정한 번호의 슬롯을 받는다(원본 Take). 풀 목록·빈 번호 카운터는 바꾸지 않는다.
    /// 이미 쓰는 슬롯이면 같은 타입·같은 클래스(가상 표)여야 하고, void 가 아니면 공간 해제(<see cref="IRawUnpop"/>)를 먼저 한다.
    /// 그다음 free/dead 를 지우고 void 를 켠 뒤 생성자 필드와 타입 바이트를 쓰고 postTake 를 적용한다. 그 밖의 필드는 보존한다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="sid">서버 영역의 슬롯 번호.</param>
    public int Take(uint type, int sid)
    {
        OriginalTypeRecord record = Type(type, requireSupported: true);
        if (sid < RawSidPool.ServerFirst)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "Take 는 서버 영역 번호만 받습니다");
        }
        Span<byte> bytes = _pool.RawSlot(sid);
        if ((bytes[RawSidPool.StateOffset] & ContainedBit) != 0)
        {
            throw new InvalidOperationException("contained 오브젝트의 base Take 는 지원하지 않습니다");
        }
        if ((bytes[RawSidPool.StateOffset] & RawSidPool.FreeBit) == 0)
        {
            if (bytes[RawSidPool.TypeOffset] != type)
            {
                throw new InvalidOperationException("Take 타입이 기존 오브젝트와 다릅니다");
            }
            if (ReadVtable(bytes) != ConstructorVtable(record))
            {
                throw new InvalidOperationException("Take 의 미복원 가상/공간 해제 효과가 필요합니다");
            }
            if ((bytes[RawSidPool.StateOffset] & RawSidPool.VoidBit) == 0)
            {
                if (_unpop is null)
                {
                    throw new InvalidOperationException("Take 의 non-void 공간 해제 연결이 없습니다");
                }
                _unpop.Unpop(sid, record);
            }
            // 이미 void 인 오브젝트의 기본 Unpop 은 쓰기 없이 돌아온다
        }
        bytes[RawSidPool.StateOffset] = (byte)((bytes[RawSidPool.StateOffset] & 0xFC) | RawSidPool.VoidBit);
        ApplyConstructor(bytes, record);
        bytes[RawSidPool.TypeOffset] = (byte)type;
        PostTake(sid);
        return sid;
    }

    /// <summary>
    /// 최초 Pop 의 공통 플래그를 추가 비트에서 읽는다(원본 004ad470). 추가 비트(+40)의 1·8 이 모두 꺼져 있으면 1,
    /// 아니면 1 비트(abstract)가 2, 8 비트(매립)가 4 를 더한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public uint FirstPopFlags(int sid)
    {
        ReadOnlySpan<byte> bytes = LiveBytes(sid);
        OriginalTypeRecord type = Type(bytes[RawSidPool.TypeOffset], requireSupported: true);
        if (ReadVtable(bytes) != ConstructorVtable(type))
        {
            throw new InvalidOperationException("firstPop 가상 표 오류");
        }
        byte extra = bytes[ExtraOffset];
        return (extra & 9) == 0 ? 1U : (((extra & 1) != 0 ? 2U : 0U) | ((extra & 8) != 0 ? 4U : 0U));
    }

    /// <summary>
    /// 공통 postCreate(원본 004b0c60). zOrder 하위 바이트를 복사하고, 서버이면 소유자를 0 으로 만들며 필요한 타입은 HP 를 최대치로 채운다.
    /// HP 가 필요한 타입은 일반 타입이면 플래그1 의 hit points 비트, 받침(dais)이면 추가 비트의 abstract 비트로 정한다.
    /// 파생 클래스가 base 초기화를 부를 때도 이 메서드를 쓴다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public void PostCreate(int sid)
    {
        Span<byte> bytes = LiveBytes(sid);
        OriginalTypeRecord type = Type(bytes[RawSidPool.TypeOffset], requireSupported: false);
        bytes[ZOrderOffset] = (byte)type.ZOrder;
        if (!_pool.IsServer)
        {
            return;
        }
        bytes[OwnerOffset] = 0;
        bool needsHp = (type.Flags2 & OriginalTypeRecord.Dais) != 0
            ? (bytes[ExtraOffset] & 1) != 0
            : (type.Flags1 & OriginalTypeRecord.HasHitPoints) != 0;
        if (needsHp)
        {
            int hp = _weakenedMana && bytes[RawSidPool.TypeOffset] == ManaType ? type.MaxHitPoints / 4 : type.MaxHitPoints;
            Put(bytes, HpOffset, unchecked((uint)hp), 4);
        }
    }

    /// <summary>공통 postTake(원본 004ad4a0). zOrder 하위 바이트만 복사하고 소유자·HP 는 보존한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public void PostTake(int sid)
    {
        Span<byte> bytes = LiveBytes(sid);
        bytes[ZOrderOffset] = (byte)Type(bytes[RawSidPool.TypeOffset], requireSupported: false).ZOrder;
    }

    /// <summary>
    /// 타입 번호의 레코드를 찾는다. 자산 타입 밖 번호는 예외이고, <paramref name="requireSupported"/> 이면
    /// 생성자 주소가 0 이 아닌데 표에 없는(미복원·null·assert) 타입도 거부한다.
    /// </summary>
    /// <param name="number">타입 번호.</param>
    /// <param name="requireSupported">생성자 지원 여부까지 확인할지.</param>
    private OriginalTypeRecord Type(uint number, bool requireSupported)
    {
        if (number < OriginalConstructors.FirstAssetType || number >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(number), number, "base 오브젝트 타입 번호 오류");
        }
        OriginalTypeRecord type = _types[(int)number];
        if (requireSupported && type.ConstructorAddress != 0 && OriginalConstructors.Recipe(type.ConstructorAddress) is null)
        {
            throw new InvalidOperationException("null/assert 또는 미복원 생성자는 지원하지 않습니다");
        }
        return type;
    }

    /// <summary>
    /// 사용 중인 슬롯의 바이트를 돌려준다. Take 로 받은 예측 머리 같은 슬롯도 허용하며, 5 번 미만·빈·contained 슬롯과
    /// 타입 바이트가 자산 타입 밖인 슬롯은 거부한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private Span<byte> LiveBytes(int sid)
    {
        Span<byte> bytes = _pool.RawSlot(sid);
        if (sid < RawSidPool.FirstOrdinary || (bytes[RawSidPool.StateOffset] & (RawSidPool.FreeBit | ContainedBit)) != 0)
        {
            throw new InvalidOperationException("base 초기화의 번호 상태 오류");
        }
        Type(bytes[RawSidPool.TypeOffset], requireSupported: false);
        return bytes;
    }

    /// <summary>타입의 최종 가상 표 값. 생성자 주소가 0 이면 기본 Squid, 아니면 표의 파생 생성자가 마지막에 쓰는 값이다.</summary>
    /// <param name="type">타입 레코드.</param>
    private static uint ConstructorVtable(OriginalTypeRecord type) => type.ConstructorAddress != 0
        ? OriginalConstructors.Recipe(type.ConstructorAddress)!.Vtable
        : OriginalConstructors.BaseVtable;

    /// <summary>
    /// 타입의 생성자 쓰기를 슬롯에 순서대로 적용한다. 주소가 0 이면 기본 가상 표 하나만 쓴다.
    /// OR 쓰기는 같은 폭의 기존 바이트를 읽어 합치며 이웃 필드는 건드리지 않는다. 호출 전에 지원 여부를 검사해야 한다.
    /// </summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="type">타입 레코드.</param>
    private static void ApplyConstructor(Span<byte> bytes, OriginalTypeRecord type)
    {
        if (type.ConstructorAddress == 0)
        {
            Put(bytes, VtableOffset, OriginalConstructors.BaseVtable, 4);
            return;
        }
        // 쓰기 순서와 폭을 유지해 섬 번호·플래그·anim HP 처리를 원본대로 한다
        foreach (ConstructorWrite write in OriginalConstructors.Recipe(type.ConstructorAddress)!.Writes)
        {
            uint value = write.Value;
            if (write.OrBits)
            {
                // OR 에 필요한 기존 바이트만 읽는다
                for (int i = 0; i < write.Width; i++)
                {
                    value |= (uint)bytes[write.Offset + i] << (i * 8);
                }
            }
            Put(bytes, write.Offset, value, write.Width);
        }
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 4 의 배수가 아닌 위치(+26 의 HP 등)에도 안전하다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>슬롯 +0 의 가상 표 값(4바이트 리틀 엔디언)을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    private static uint ReadVtable(ReadOnlySpan<byte> bytes) => (uint)(bytes[VtableOffset] | (bytes[VtableOffset + 1] << 8)
        | (bytes[VtableOffset + 2] << 16) | (bytes[VtableOffset + 3] << 24));
}
