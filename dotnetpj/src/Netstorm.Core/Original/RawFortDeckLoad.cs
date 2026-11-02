namespace Netstorm.Core.Original;

/// <summary>
/// TypeNames 및 같은 Client가 직접 공유하는 현재 전역이다(cpppj <c>o/RawFortDeckLoad.h</c>의 <c>FortDeckLoadGlobals</c>).
/// 부모가 읽을 때 값 사본을 고정하지 않는다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public sealed class FortDeckLoadGlobals
{
    /// <summary>현재 로딩 깊이 읽기다(원본 005c89b8). 진입·정상 종료·별도 cleanup이 현재값을 직접 쓴다.</summary>
    public Func<uint>? GetDepth { get; set; }

    /// <summary>현재 로딩 깊이 쓰기다.</summary>
    public Action<uint>? SetDepth { get; set; }

    /// <summary>현재 255BYTE 변환표 토큰 읽기다(원본 005c89bc). TypeNames와 같은 소유자를 빌린다.</summary>
    public Func<uint>? GetConversion { get; set; }

    /// <summary>현재 255BYTE 변환표 토큰 쓰기다.</summary>
    public Action<uint>? SetConversion { get; set; }

    /// <summary>0054db14의 현재 값 읽기다. Clear·Open 복귀 뒤 원문 위치에서 읽는다.</summary>
    public Func<uint>? FullDeck { get; set; }
}

/// <summary>
/// 실제 Deck 선택·초기화·fullDeck·저장 항목 복원·변환표 반납 순서를 보존한다(cpppj <c>o/RawFortDeckLoad.h</c>의 <c>FortDeckLoadHooks</c>).
/// 큰 하위 몸체만 연결하고 slot clamp·직접 BYTE 읽기·변환 조건과 부모 순서는 본문이 소유한다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortDeckLoadHooks
{
    /// <summary>고정 this의 현재 manager를 진단 시와 Select 직전에 각각 읽는다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>실제 두 진단의 선택 경계다. 없으면 도달 시 거부한다.</summary>
    public Action<FortDeckLoadAssertion>? Assertion { get; set; }

    /// <summary>0044f700(owner)의 실제 현재 Deck 반환이다. owner는 slot 1..8 또는 0이다.</summary>
    public Func<uint, uint>? Deck { get; set; }

    /// <summary>0044ceb0(manager,"Deck",1)의 같은 반환 stream이다.</summary>
    public Func<uint, string, int, uint>? Select { get; set; }

    /// <summary>선택한 같은 16BYTE·버퍼를 동기적으로 빌리며 호출자 지역 사본을 쓰지 않는다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }

    /// <summary>0044f230은 선택 뒤 항목·합계·remaining을 초기화한다.</summary>
    public Action<uint>? Clear { get; set; }

    /// <summary>0044f250(deck,0)은 같은 게임 난수를 소비하고 seed·open을 쓴다.</summary>
    public Action<uint, uint>? Open { get; set; }

    /// <summary>0044fbf0→0044f680의 실제 FullDeck이다. 빈 성공으로 대체하지 않는다.</summary>
    public Action<uint>? Full { get; set; }

    /// <summary>004bd800은 같은 고정 this를 받는다. 별도 복원된 실제 변환표 부모를 연결한다.</summary>
    public Action<int>? PrepareTypeNames { get; set; }

    /// <summary>현재 토큰의 255BYTE 물리 범위를 순수 조회한다. 이 getter에 추가 게임 효과를 넣지 않는다.</summary>
    public RawBytesLookup? ConversionBuffer { get; set; }

    /// <summary>0044f330(deck,type,chance,power,remaining)의 실제 복원 추가다. 세 저장 필드는 BYTE를 DWORD로 영확장한다.</summary>
    public Action<uint, uint, uint, uint, uint>? Append { get; set; }

    /// <summary>_free에 그 시점 현재 pointer를 넘긴다. 복귀하면 부모가 현재 전역을 무조건 0으로 쓴다.</summary>
    public Action<uint>? ReleaseConversion { get; set; }

    /// <summary>0044f7c0(owner)은 비 full·비어 있지 않은 stream 경로 끝에만 실행한다.</summary>
    public Action<uint>? Refresh { get; set; }

    /// <summary>실제 깊이 쓰기를 같은 월드 소비자에 동기화하는 관측 경계다. 추가 게임 효과를 넣지 않는다. null이면 통지하지 않는다.</summary>
    public Action<uint>? LoadingDepthChanged { get; set; }
}

/// <summary>
/// 실제 Deck 진단 두 가지다(cpppj <c>o/RawFortDeckLoad.h</c>의 <c>FortDeckLoadAssertion</c>).
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public enum FortDeckLoadAssertion : uint
{
    /// <summary>진입 깊이 증가 뒤 고정 this의 현재 manager 0이다.</summary>
    MissingTemplate = 0x4d6,

    /// <summary>Get이 반환한 Deck 0이다. 진단으로 현재 owner·반환값을 다시 조회하지 않는다.</summary>
    MissingDeck = 0x4d8,
}

/// <summary>
/// 원본 004bf190의 Deck 선택·초기화·fullDeck·저장 항목 복원·변환표 반납 순서를 보존한다(cpppj <c>o/RawFortDeckLoad</c>).
/// 본문만 실행한다. 선택 callback·물리 중단 시 이미 쓴 자료와 증가 깊이를 보존하며 자동 cleanup을 추가하지 않는다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public static class RawFortDeckLoad
{
    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 Select·초기화·직접 읽기의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort Deck 로드의 실제 경계가 연결되지 않았다.</exception>
    private static T NeedDeckLoad<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort Deck 로드의 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 같은 16BYTE MemStream의 DWORD를 호스트 정렬과 무관하게 읽는다. 필요한 DWORD만 조합한다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Deck 현재 stream DWORD 물리 범위다.</exception>
    private static uint ReadDeckLoad(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort Deck 현재 stream DWORD 물리 범위");
        }
        uint value = 0;
        // 현재 네 BYTE 조립값이다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 원본이 쓴 현재 pos 한 DWORD만 대입한다. data·end·capacity는 변경하지 않는다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다(8BYTE 이상).</param>
    /// <param name="value">대입할 pos 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Deck 현재 pos 물리 범위다.</exception>
    private static void PutDeckLoad(Span<byte> bytes, uint value)
    {
        if (bytes.Length < 8)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), bytes.Length, "Fort Deck 현재 pos 물리 범위");
        }
        for (int index = 0; index < 4; index++)
        {
            bytes[4 + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// depth 자체를 먼저 쓰고 같은 값만 관측 소비자에 전달한다.
    /// </summary>
    /// <param name="globals">현재 깊이·변환표 전역이다.</param>
    /// <param name="hooks">관측 경계다(깊이 통지는 없어도 된다).</param>
    /// <param name="value">쓸 깊이 값이다.</param>
    private static void DeckLoadDepth(FortDeckLoadGlobals globals, FortDeckLoadHooks hooks, uint value)
    {
        NeedDeckLoad(globals.SetDepth)(value);
        hooks.LoadingDepthChanged?.Invoke(value);
    }

    /// <summary>
    /// 직접 BYTE 읽기는 논리 end를 검사하지 않는다. 실제 물리 입력이 없을 때만 명시 거부한다.
    /// </summary>
    /// <param name="bytes">읽을 버퍼다.</param>
    /// <param name="position">읽을 위치다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Deck 직접 BYTE의 실제 물리 범위다.</exception>
    private static byte DeckLoadByte(ReadOnlySpan<byte> bytes, uint position)
    {
        if (position >= (uint)bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(position), position, "Fort Deck 직접 BYTE의 실제 물리 범위");
        }
        return bytes[(int)position];
    }

    /// <summary>
    /// slot 1..8이 아니면 두 owner 하위에 0을 전달하며 고정 this의 Select·TypeNames를 다른 slot으로 바꾸지 않는다.
    /// </summary>
    /// <param name="slot">고정 this의 검사 입력 슬롯(signed).</param>
    /// <param name="globals">같은 Client가 직접 공유하는 현재 전역이다. 호출자가 소유한다.</param>
    /// <param name="hooks">큰 하위 몸체·실제 물리 저장소다.</param>
    public static void LoadFortDeck(int slot, FortDeckLoadGlobals globals, FortDeckLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        DeckLoadDepth(globals, hooks, NeedDeckLoad(globals.GetDepth)() + 1u);
        if (NeedDeckLoad(hooks.Manager)(slot) == 0)
        {
            NeedDeckLoad(hooks.Assertion)(FortDeckLoadAssertion.MissingTemplate);
        }
        uint owner = slot >= 1 && slot <= 8 ? (uint)slot : 0u; // 원본 this의 슬롯을 계산한 owner clamp다
        uint deck = NeedDeckLoad(hooks.Deck)(owner); // 이 반환값은 뒤 callback이 현재 Player 포인터를 바꾸어도 유지한다
        if (deck == 0)
        {
            NeedDeckLoad(hooks.Assertion)(FortDeckLoadAssertion.MissingDeck);
        }
        uint manager = NeedDeckLoad(hooks.Manager)(slot); // 진단·Deck getter 뒤 현재 manager를 다시 읽고 Select 장치 조회보다 먼저 보관한다
        uint stream = NeedDeckLoad(hooks.Select)(manager, "Deck", 1); // 같은 원문 CALL의 반환 참조를 이후 전체 루프가 유지한다
        NeedDeckLoad(hooks.Clear)(deck);
        NeedDeckLoad(hooks.Open)(deck, 0);
        if (NeedDeckLoad(globals.FullDeck)() != 0)
        {
            NeedDeckLoad(hooks.Full)(deck);
        }
        else
        {
            bool present = false; // Select 뒤 Clear·Open이 바꾼 실제 현재 data·end를 같이 확인한다
            NeedDeckLoad(hooks.Access)(stream, (fields, memory) => { present = ReadDeckLoad(fields, 0) != 0 && ReadDeckLoad(fields, 8) != 0; });
            if (present)
            {
                NeedDeckLoad(hooks.PrepareTypeNames)(slot);
                uint count = 0; // 변환표 부모 복귀 뒤 같은 stream의 현재 count를 읽는다
                NeedDeckLoad(hooks.Access)(stream, (fields, memory) =>
                {
                    uint position = ReadDeckLoad(fields, 4);
                    ReadOnlySpan<byte> bytes = NeedDeckLoad(memory.Buffer)(ReadDeckLoad(fields, 0)); // 현재 data·pos 쌍이다
                    count = DeckLoadByte(bytes, position);
                    PutDeckLoad(fields, position + 1u);
                });
                for (uint index = 0; index < count; index++)
                {
                    // 처음 count BYTE를 보관하며 append callback이 뒤의 현재 stream·표를 바꿀 수 있다
                    uint[] entry = new uint[4]; // type·chance·power·remaining의 현재 네 BYTE를 영확장한 인자다
                    NeedDeckLoad(hooks.Access)(stream, (fields, memory) =>
                    {
                        uint position = ReadDeckLoad(fields, 4);
                        ReadOnlySpan<byte> bytes = NeedDeckLoad(memory.Buffer)(ReadDeckLoad(fields, 0)); // 원문 EAX·ECX가 캡처한 한 항목의 pos·data다
                        entry[0] = DeckLoadByte(bytes, position);
                        PutDeckLoad(fields, position + 1u);
                        if (entry[0] < 0xf6u && NeedDeckLoad(globals.GetConversion)() != 0)
                        {
                            ReadOnlySpan<byte> table = NeedDeckLoad(hooks.ConversionBuffer)(NeedDeckLoad(globals.GetConversion)());
                            entry[0] = DeckLoadByte(table, entry[0]); // 246 이상은 변환표를 읽지 않는다
                        }
                        for (int field = 1; field < entry.Length; field++)
                        {
                            entry[field] = DeckLoadByte(bytes, position + (uint)field);
                            PutDeckLoad(fields, position + (uint)field + 1u); // 같은 처음 data·pos에서 나머지 세 BYTE를 읽는다
                        }
                    });
                    if (entry[0] != 0)
                    {
                        NeedDeckLoad(hooks.Append)(deck, entry[0], entry[1], entry[2], entry[3]);
                    }
                }
                if (NeedDeckLoad(globals.GetConversion)() != 0)
                {
                    NeedDeckLoad(hooks.ReleaseConversion)(NeedDeckLoad(globals.GetConversion)());
                }
                NeedDeckLoad(globals.SetConversion)(0);
                NeedDeckLoad(hooks.Refresh)(owner);
            }
        }
        DeckLoadDepth(globals, hooks, NeedDeckLoad(globals.GetDepth)() - 1u);
    }

    /// <summary>
    /// 명시 부분 중단과 원본 예외 cleanup 호출을 분리해 같은 현재 DWORD 감소만 관측 가능하게 한다.
    /// </summary>
    /// <param name="globals">같은 Client가 직접 공유하는 현재 전역이다.</param>
    /// <param name="hooks">관측 경계다. 깊이 통지만 쓴다.</param>
    public static void UnwindFortDeckLoad(FortDeckLoadGlobals globals, FortDeckLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        DeckLoadDepth(globals, hooks, NeedDeckLoad(globals.GetDepth)() - 1u);
    }
}
