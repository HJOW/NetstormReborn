using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Netstorm.Core.Original;

/// <summary>
/// 부모 Template와 현재 로컬 플레이어·conversion 포인터의 논리 상태다(cpppj <c>o/RawFortMetaLoad.h</c>의 <c>FortMetaLoadState</c>).
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다. 참조로 공유하며 수명은 호출자가 진다.</remarks>
public sealed class FortMetaLoadState
{
    /// <summary>실제 Template 첫 DWORD가 0이면 거짓이다. 거짓이면 보고 후 계속한다.</summary>
    public bool TemplatePresent { get; set; } = true;

    /// <summary>실제 this = TemplateBase + slot*20의 Template 칸 번호다.</summary>
    public int TemplateSlot { get; set; } = 1;

    /// <summary>현재 플레이어 DWORD와 비교하는 로컬 owner다.</summary>
    public int LocalOwner { get; set; } = 1;

    /// <summary>원본 typeConversion이 null이 아닌가. 매 해제 시점에 다시 읽는다.</summary>
    public bool HasTypeConversion { get; set; }
}

/// <summary>
/// 원본 보고 뒤 계속할 수 있는 각 진단의 의미다(cpppj <c>o/RawFortMetaLoad.h</c>의 <c>FortMetaLoadAssertion</c>).
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다.</remarks>
public enum FortMetaAssertion
{
    /// <summary>Badges wrapper의 Template 첫 DWORD가 0이다.</summary>
    MissingBadgesTemplate,

    /// <summary>직접 setter count가 0..255 범위를 벗어났다.</summary>
    BadgeCountRange,

    /// <summary>Technology wrapper의 Template 첫 DWORD가 0이다.</summary>
    MissingTechnologyTemplate,

    /// <summary>실제 현재 플레이어 Template 주소와 this가 다르다.</summary>
    TechnologyNotMine,
}

/// <summary>
/// 내용물 생성·TypeNames·화면 출력은 실제 하위 몸체로 공급하며 noop으로 복원을 대체하지 않는다(cpppj <c>o/RawFortMetaLoad.h</c>의 <c>FortMetaLoadHooks</c>).
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다.</remarks>
public sealed class FortMetaLoadHooks
{
    /// <summary>원본 진단 보고 뒤 계속하는 진단 경계다.</summary>
    public Action<FortMetaAssertion>? Assertion { get; init; }

    /// <summary>같은 Money·객체·사제 모듈에 현재 깊이를 동기화한다.</summary>
    public Action<uint>? LoadingDepthChanged { get; init; }

    /// <summary>현재 technology 부모 SID를 조회한다(원본 0048fc60·CD 00406d90).</summary>
    public Func<Sid>? TechnologyParent { get; init; }

    /// <summary>실제 변환표 준비 경계다(원본 004bd800·CD 00427e00).</summary>
    public Action? PrepareTypeNames { get; init; }

    /// <summary>같은 풀의 실제 Contents 몸체다(원본 004bd130·CD 00426dd0, flags 17·완화 변환 참).</summary>
    public Action<Sid, FortMetaStream, uint, bool>? Contents { get; init; }

    /// <summary>현재 null이 아닌 표의 해제·CD 00426830 하위 해제 경계다.</summary>
    public Action? ReleaseTypeConversion { get; init; }

    /// <summary>마지막 표시 경계다(원본 0042f540·CD 004b2e70).</summary>
    public Action? RefreshTechnologyUi { get; init; }
}

/// <summary>
/// 실제 badge 유형 DWORD·수량 DWORD와 Technology wrapper 순서를 보존하는 독립 wrapper다(cpppj <c>o/RawFortMetaLoad</c>).
/// 조회 pos→개수 BYTE→type·count BYTE 쌍→실제 setter 순서로 실행한다.
/// </summary>
/// <remarks>
/// 2026-11-02 F2a에서 추가했다. badges는 장기 소유한 원본 28×220BYTE이며 이 객체보다 오래 살아야 한다.
/// 이미 원본 TypeNames 파싱을 끝낸 같은 <c>FortTemplate</c>의 실제 Contents 몸체만 연결하는 <c>MakeFortMetaContentsHooks</c>는
/// F2c-3에서 옮겼다.
/// </remarks>
public sealed class RawFortMetaLoad
{
    /// <summary>실제 MetaDisplay 기록 수다.</summary>
    private const int BadgeRecords = 28;

    /// <summary>실제 기록 stride BYTE 수다.</summary>
    private const int BadgeStride = 220;

    /// <summary>원본 count DWORD의 기록 안 위치다.</summary>
    private const int BadgeCount = 216;

    /// <summary>같은 판본·실제 하위 소비자 풀의 SID 저장소다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>실제 type·count·중간 padding을 가진 28개 220BYTE 장기 기록이다.</summary>
    private readonly byte[] _badges;

    /// <summary>Money·Template·사제와 공유하는 현재 DWORD를 읽고 쓴다.</summary>
    private readonly Func<uint> _getDepth;

    /// <summary>현재 DWORD를 쓴다.</summary>
    private readonly Action<uint> _setDepth;

    /// <summary>진단·하위 호출 뒤 현재 Template·conversion 상태를 다시 읽는다.</summary>
    private readonly FortMetaLoadState _state;

    /// <summary>미복원 하위 몸체를 기본 성공값으로 대체하지 않는 실제 효과다.</summary>
    private readonly FortMetaLoadHooks _hooks;

    /// <summary>
    /// badges·깊이·상태·효과를 빌려 Meta 로더를 만든다.
    /// </summary>
    /// <param name="pool">같은 판본·실제 하위 소비자 풀의 SID 저장소다.</param>
    /// <param name="badges">장기 소유한 원본 28×220BYTE 기록이다.</param>
    /// <param name="getDepth">현재 로딩 깊이 읽기다.</param>
    /// <param name="setDepth">현재 로딩 깊이 쓰기다.</param>
    /// <param name="state">진단·하위 호출 뒤 다시 읽는 Template·conversion 상태다.</param>
    /// <param name="hooks">미복원 하위 몸체를 대체하지 않는 실제 효과다.</param>
    /// <exception cref="ArgumentException">부모·stream 원본 몸체보다 앞서 자료 크기·필수 하위 효과가 맞지 않는다.</exception>
    public RawFortMetaLoad(RawSidPool pool, byte[] badges, Func<uint> getDepth, Action<uint> setDepth,
        FortMetaLoadState state, FortMetaLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(badges);
        ArgumentNullException.ThrowIfNull(getDepth);
        ArgumentNullException.ThrowIfNull(setDepth);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (badges.Length != BadgeRecords * BadgeStride || hooks.Assertion is null || hooks.LoadingDepthChanged is null ||
            hooks.TechnologyParent is null || hooks.PrepareTypeNames is null || hooks.Contents is null ||
            hooks.ReleaseTypeConversion is null || hooks.RefreshTechnologyUi is null)
        {
            throw new ArgumentException("Fort meta 실제 badge 기록·하위 효과 누락", nameof(badges));
        }
        _pool = pool;
        _badges = badges;
        _getDepth = getDepth;
        _setDepth = setDepth;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>
    /// 범위 진단 뒤 현재 표의 첫 같은 type에 전체 count DWORD를 쓴다(원본 0047d800·CD 00475660).
    /// </summary>
    /// <param name="type">찾을 badge 유형 DWORD다.</param>
    /// <param name="count">쓸 수량이다. 0..255를 벗어나면 보고 후 계속한다.</param>
    public void SetBadge(uint type, int count)
    {
        if (count < 0 || count > 255)
        {
            _hooks.Assertion!(FortMetaAssertion.BadgeCountRange);
        }
        int index = FindBadge(type);
        if (index != -1)
        {
            WriteBadge(index * BadgeStride + BadgeCount, unchecked((uint)count));
        }
    }

    /// <summary>
    /// CD 00475560와 patch setter 안의 같은 28개 linear 검색이다. 못 찾으면 -1이다.
    /// </summary>
    /// <param name="type">찾을 badge 유형 DWORD다. 종류 중복도 원본처럼 첫 기록만 반환한다.</param>
    public int FindBadge(uint type)
    {
        // 종류 중복도 원본처럼 첫 기록만 반환한다
        for (int index = 0; index < BadgeRecords; index++)
        {
            if (ReadBadge(index * BadgeStride) == type)
            {
                return index;
            }
        }
        return -1;
    }

    /// <summary>
    /// 004be8e0·CD 00428fc0: 조회 pos 0→개수 BYTE→type·count BYTE 쌍→실제 setter 순서다.
    /// </summary>
    /// <param name="stream">현재 pointer 존재·pos·end·실제 저장 바이트다. 독립 원문의 cursor도 같은 시점에 초기화한다.</param>
    public void LoadBadges(FortMetaStream stream)
    {
        ArgumentNullException.ThrowIfNull(stream);
        LoadBadgesSelected(() => { stream.Position = 0; return stream; });
    }

    /// <summary>
    /// 실제 Badges 선택은 CD의 즉시 깊이 증감과 현재 Template 진단 뒤에 수행한다.
    /// </summary>
    /// <param name="select">현재 노드를 선택해 같은 cursor를 돌려준다.</param>
    /// <exception cref="ArgumentException">실제 섹션 선택이 없다.</exception>
    /// <exception cref="ArgumentOutOfRangeException">손상된 물리 메모리의 stream을 읽었다.</exception>
    public void LoadBadgesSelected(Func<FortMetaStream> select)
    {
        ArgumentNullException.ThrowIfNull(select);
        if (_pool.Edition == OriginalEdition.Cd1072)
        {
            ChangeDepth(_getDepth() + 1);
            ChangeDepth(_getDepth() - 1);
        }
        if (!_state.TemplatePresent)
        {
            _hooks.Assertion!(FortMetaAssertion.MissingBadgesTemplate);
        }
        FortMetaStream stream = select();
        if (!stream.Present || stream.End == 0)
        {
            return;
        }
        uint count = ReadByte(stream);
        // 저장 BYTE 순서를 그대로 따르며 앞 setter 효과를 새 종류·개수로 정렬하지 않는다
        for (uint index = 0; index < count; index++)
        {
            uint type = ReadByte(stream);
            uint quantity = ReadByte(stream);
            SetBadge(type, (int)quantity);
        }
    }

    /// <summary>
    /// 004beee0·CD 004293d0: 깊이→Template·isMine→부모 조회→Tech 조회→변환 준비·Contents→해제→UI→깊이 순서다.
    /// </summary>
    /// <param name="stream">현재 pointer 존재·pos·end·실제 저장 바이트다. 독립 원문의 cursor 초기화도 실제 기술 부모 조회 뒤에 한다.</param>
    public void LoadTechnology(FortMetaStream stream)
    {
        ArgumentNullException.ThrowIfNull(stream);
        LoadTechnologySelected(() => { stream.Position = 0; return stream; });
    }

    /// <summary>
    /// 실제 Technology 선택을 깊이·Template·isMine 진단과 기술 부모 조회 뒤의 원본 위치에서 수행한다.
    /// </summary>
    /// <param name="select">현재 manager에서 Technology를 선택한다. 선택 중 실패도 깊이를 복구한다.</param>
    /// <exception cref="ArgumentException">실제 섹션 선택이 없다.</exception>
    public void LoadTechnologySelected(Func<FortMetaStream> select)
    {
        ArgumentNullException.ThrowIfNull(select);
        ChangeDepth(_getDepth() + 1);
        try
        {
            if (!_state.TemplatePresent)
            {
                _hooks.Assertion!(FortMetaAssertion.MissingTechnologyTemplate);
            }
            if (_state.TemplateSlot != _state.LocalOwner)
            {
                _hooks.Assertion!(FortMetaAssertion.TechnologyNotMine);
            }
            Sid parent = _hooks.TechnologyParent!();
            FortMetaStream stream = select();
            if (stream.Present && stream.End != 0)
            {
                _hooks.PrepareTypeNames!();
                _hooks.Contents!(parent, stream, 0x11, true);
                if (_state.HasTypeConversion)
                {
                    _hooks.ReleaseTypeConversion!();
                }
                _state.HasTypeConversion = false;
            }
            _hooks.RefreshTechnologyUi!();
        }
        catch
        {
            ChangeDepth(_getDepth() - 1);
            throw;
        }
        ChangeDepth(_getDepth() - 1);
    }

    /// <summary>
    /// 실제 contents 어댑터가 다른 SID 풀을 섞지 않았는지 확인한다.
    /// </summary>
    public RawSidPool Pool() => _pool;

    /// <summary>
    /// 비정렬 little endian DWORD의 현재 바이트를 조합해 raw 표 DWORD를 읽는다.
    /// </summary>
    /// <param name="offset">읽을 기록 안 위치다.</param>
    private uint ReadBadge(int offset)
    {
        uint value = 0;
        // 원본 DWORD 폭의 각 BYTE만 읽는다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)_badges[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 지정 DWORD만 쓰며 28개 기록의 다른 필드와 padding은 보존한다.
    /// </summary>
    /// <param name="offset">쓸 기록 안 위치다.</param>
    /// <param name="value">쓸 DWORD 값이다.</param>
    private void WriteBadge(int offset, uint value)
    {
        // type·padding·이웃 기록은 쓰지 않는다
        for (int index = 0; index < 4; index++)
        {
            _badges[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 원본 badge BYTE 읽기다. 손상된 물리 메모리는 계약 밖이다.
    /// </summary>
    /// <param name="stream">읽을 cursor다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Badges 실제 stream 메모리를 소진했다.</exception>
    private static byte ReadByte(FortMetaStream stream)
    {
        if (stream.Position >= stream.Bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(stream), stream.Position, "Fort Badges 실제 stream 메모리 소진");
        }
        byte value = stream.Bytes.Span[(int)stream.Position];
        stream.Position++;
        return value;
    }

    /// <summary>
    /// 원본 현재 로딩 깊이 쓰기와 같은 소비자의 관찰을 함께 실행한다.
    /// </summary>
    /// <param name="value">쓸 깊이 값.</param>
    private void ChangeDepth(uint value)
    {
        _setDepth(value);
        _hooks.LoadingDepthChanged!(value);
    }

    /// <summary>
    /// 같은 풀의 실제 Contents 몸체만 연결한다. 변환표 준비·실제·기술부모·UI 전체 복사는 이 범위가 아니다(cpppj <c>o/RawFortMetaLoad.cpp</c>의 <c>MakeFortMetaContentsHooks</c>).
    /// 같은 FortTemplate의 실제 해석 Contents 몸체를 호출하며 Technology parsed 바이트와 같은 stream 구간을 요구한다.
    /// </summary>
    /// <param name="pool">검사할 풀이다.</param>
    /// <param name="objects">같은 풀의 실제 객체 적재 몸체다.</param>
    /// <param name="fort">이미 원본 TypeNames 파싱을 끝낸 같은 FortTemplate이다.</param>
    /// <param name="hooks">물을 훅 묶음이다.</param>
    /// <exception cref="ArgumentException">Fort meta Contents SID 풀 불일치·Technology 바이트와 stream 불일치다.</exception>
    /// <exception cref="InvalidOperationException">Fort Technology는 완화 변환 호출만 처리한다.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Fort Technology parsed 뒤꼬리 범위 오류다.</exception>
    public static FortMetaLoadHooks MakeFortMetaContentsHooks(RawSidPool pool, RawFortObjectLoad objects, FortTemplate fort, FortMetaLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(objects);
        ArgumentNullException.ThrowIfNull(fort);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, objects.Pool()))
        {
            throw new ArgumentException("Fort meta Contents SID 풀 불일치", nameof(pool));
        }
        return new FortMetaLoadHooks
        {
            Assertion = hooks.Assertion,
            LoadingDepthChanged = hooks.LoadingDepthChanged,
            TechnologyParent = hooks.TechnologyParent,
            PrepareTypeNames = hooks.PrepareTypeNames,
            Contents = (parent, stream, flags, relaxed) =>
            {
                if (!relaxed)
                {
                    throw new InvalidOperationException("Fort Technology는 완화 변환 호출만 처리한다");
                }
                ReadOnlySpan<byte> bytes = fort.Section("Technology");
                if (fort.TechnologyTrailingBytes < 0 || fort.TechnologyTrailingBytes > bytes.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(fort), fort.TechnologyTrailingBytes, "Fort Technology parsed 뒤꼬리 범위 오류");
                }
                if (stream.Bytes.Length != bytes.Length)
                {
                    throw new ArgumentException("Fort Technology parsed 바이트와 stream 불일치", nameof(stream));
                }
                // 같은 할당의 같은 구간인지 첫 바이트 주소로 확인한다. C++의 포인터 비교와 같다
                if (bytes.Length > 0 && !Unsafe.AreSame(ref MemoryMarshal.GetReference(bytes), ref MemoryMarshal.GetReference(stream.Bytes.Span)))
                {
                    throw new ArgumentException("Fort Technology parsed 바이트와 stream 불일치", nameof(stream));
                }
                objects.Contents(parent.Value, fort.Technology, flags);
                stream.Position = unchecked((uint)(bytes.Length - fort.TechnologyTrailingBytes));
            },
            ReleaseTypeConversion = hooks.ReleaseTypeConversion,
            RefreshTechnologyUi = hooks.RefreshTechnologyUi,
        };
    }
}
