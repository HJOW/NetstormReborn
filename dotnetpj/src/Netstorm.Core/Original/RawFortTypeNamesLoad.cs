namespace Netstorm.Core.Original;

/// <summary>
/// Client와 같은 로더가 직접 공유하는 두 현재 DWORD다(cpppj <c>o/RawFortTypeNamesLoad.h</c>의 <c>FortTypeNamesLoadGlobals</c>).
/// 이전 pointer를 별도 사본에 보존하지 않는다. 이전 단위들의 전역 쌍과 같이 읽기·쓰기 대리자로 공유한다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public sealed class FortTypeNamesLoadGlobals
{
    /// <summary>현재 로딩 깊이 읽기다(원본 005c89b8). 진입 증가·정상 감소·별도 cleanup이 현재값을 직접 쓴다.</summary>
    public Func<uint>? GetDepth { get; set; }

    /// <summary>현재 로딩 깊이 쓰기다.</summary>
    public Action<uint>? SetDepth { get; set; }

    /// <summary>현재 255BYTE 변환표 토큰 읽기다(원본 005c89bc). 각 일치 쓰기 때 다시 읽는다.</summary>
    public Func<uint>? GetConversion { get; set; }

    /// <summary>현재 255BYTE 변환표 토큰 쓰기다.</summary>
    public Action<uint>? SetConversion { get; set; }
}

/// <summary>
/// 실제 Select·할당·TYPE·진단과 물리 메모리만 분리하고 zero·hash·검색·필드 쓰기는 본문이 소유한다(cpppj <c>o/RawFortTypeNamesLoad.h</c>의 <c>FortTypeNamesLoadHooks</c>).
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortTypeNamesLoadHooks
{
    /// <summary>깊이 증가 뒤 고정 this 헤더의 현재 manager를 읽는다. 이 본문에는 manager 0 진단이 없다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>깊이 증가 뒤 현재 manager의 실제 선택 stream 토큰이다(원본 0044ceb0(manager,"TypeNames",1)).</summary>
    public Func<uint, string, int, uint>? Select { get; set; }

    /// <summary>같은 반환 stream의 실제 16BYTE다. 진단·할당 callback 뒤도 주소를 유지한다.</summary>
    public RawBytesLookup? Stream { get; set; }

    /// <summary>선택한 data 토큰의 실제 물리 범위다. 끝을 기본 0으로 확장하지 않는다.</summary>
    public RawBytesLookup? Buffer { get; set; }

    /// <summary>실제 할당이다(원본 004e4391(255)). 본문이 반환 주소를 현재 pointer에 쓰고 그 블록만 zero한다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>해당 현재 토큰의 실제 255BYTE다. 0·없음은 물리 거부다.</summary>
    public RawBytesLookup? ConversionBuffer { get; set; }

    /// <summary>00541348의 현재 signed DWORD다. 최초·매 타입 끝에 다시 읽는다.</summary>
    public Func<int>? TypeCount { get; set; }

    /// <summary>그 시점 실제 NUL 포함 이름이다(원본 0049a840(index)+4). 20자에서 자르지 않는다.</summary>
    public Func<int, byte[]>? TypeName { get; set; }

    /// <summary>Template.cpp의 기존 conversion 진단이다. 복귀하면 그대로 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }

    /// <summary>실제 깊이 쓰기를 같은 월드 소비자에 동기화하는 관측 경계다. 추가 게임 효과를 넣지 않는다.</summary>
    public Action<uint>? LoadingDepthChanged { get; set; }

    /// <summary>선택 뒤 같은 fields·data를 동기 범위 전체에서 빌린다. 지정하면 stream·buffer getter보다 우선하며 뷰·장치를 밖으로 탈출시키지 않는다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }
}

/// <summary>
/// 원본 004bd800의 TypeNames 변환표 읽기와 별도 004fb820·004bcc50 cleanup을 복원한다(cpppj <c>o/RawFortTypeNamesLoad</c>).
/// 원문 본문만 실행한다. 명시 callback·물리 중단 시 앞선 변경과 증가한 깊이를 보존하며 자동 cleanup하지 않는다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public static class RawFortTypeNamesLoad
{
    /// <summary>원본 할당 폭이다.</summary>
    private const uint ConversionBytes = 255;

    /// <summary>Template.cpp의 기존 conversion 진단행이다.</summary>
    private const uint ExistingConversionLine = 608;

    /// <summary>
    /// 실제 도달한 필수 경계를 빈 성공으로 대체하지 않는다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">TypeNames 실제 경계가 연결되지 않았다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("TypeNames 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 현재 stream·data에서 비정렬 little endian DWORD를 읽는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <exception cref="ArgumentOutOfRangeException">TypeNames DWORD 물리 읽기 범위다.</exception>
    private static uint Read(ReadOnlySpan<byte> bytes, ulong offset)
    {
        if (offset > (ulong)bytes.Length || (ulong)bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "TypeNames DWORD 물리 읽기 범위");
        }
        uint result = 0;
        // 지정 네 BYTE만 조립한다
        for (int index = 0; index < 4; index++)
        {
            result |= (uint)bytes[(int)(offset + (ulong)index)] << (index * 8);
        }
        return result;
    }

    /// <summary>
    /// 같은 stream의 pos·end 네 BYTE만 기록한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">TypeNames stream 쓰기 범위다.</exception>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "TypeNames stream 쓰기 범위");
        }
        // 다른 stream 필드는 보존한다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 첫 NUL까지 signed char를 위치 mod 4만큼 밀어 32비트 합을 감는다.
    /// </summary>
    /// <param name="name">이름 바이트다. 빈 이름은 0이고 NUL 뒤 바이트는 해시에 포함하지 않는다.</param>
    /// <exception cref="ArgumentOutOfRangeException">TypeNames 현재 이름에 NUL이 없어 물리 범위를 벗어났다.</exception>
    private static uint Hash(ReadOnlySpan<byte> name)
    {
        uint result = 0;
        // 원본 EDI의 감김 합이다
        for (int index = 0; index < name.Length; index++)
        {
            byte current = name[index];
            if (current == 0)
            {
                return result;
            }
            uint value = unchecked((uint)(int)(sbyte)current);
            result += value << ((index & 3) * 8);
        }
        throw new ArgumentOutOfRangeException(nameof(name), name.Length, "TypeNames 현재 이름 NUL 물리 범위");
    }

    /// <summary>
    /// 현재 DWORD를 쓰고 실제 소비자에 같은 깊이를 통지한다.
    /// </summary>
    /// <param name="value">쓸 깊이 값이다.</param>
    /// <param name="globals">현재 깊이·변환표 전역이다.</param>
    /// <param name="hooks">관측 경계다.</param>
    private static void Depth(uint value, FortTypeNamesLoadGlobals globals, FortTypeNamesLoadHooks hooks)
    {
        Need(globals.SetDepth)(value);
        Need(hooks.LoadingDepthChanged)(value);
    }

    /// <summary>
    /// count는 할당 전 캡처하고 hash 주소는 할당·zero 뒤 캡처하며 각 변환 쓰기 주소는 그때 재조회한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호(signed).</param>
    /// <param name="globals">같은 Client가 직접 공유하는 현재 전역이다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 Select·할당·TYPE·진단·물리다.</param>
    public static void LoadFortTypeNames(int slot, FortTypeNamesLoadGlobals globals, FortTypeNamesLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        Depth(Need(globals.GetDepth)() + 1u, globals, hooks);
        uint token = Need(hooks.Select)(Need(hooks.Manager)(slot), "TypeNames", 1);
        // 선택된 같은 필드·버퍼의 동기 빌림 전체에서 기존 본문을 실행한다
        void Load(Span<byte> fields, RawBytesLookup? buffer)
        {
            uint data = Read(fields, 0);
            if (data == 0 || Read(fields, 8) == 0)
            {
                Need(globals.SetConversion)(0); // 기존 pointer를 해제하지 않는다. 빈 경로에는 할당·TYPE 조회도 없다
            }
            else
            {
                uint position = Read(fields, 4);
                ReadOnlySpan<byte> bytes = Need(buffer)(data);
                if (position >= (uint)bytes.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(fields), position, "TypeNames count BYTE 물리 범위");
                }
                byte count = bytes[(int)position];
                Put(fields, 4, position + 1u);
                if (Need(globals.GetConversion)() != 0)
                {
                    Need(hooks.Assertion)(ExistingConversionLine);
                }
                uint allocated = Need(hooks.Allocate)(ConversionBytes);
                Need(globals.SetConversion)(allocated);
                Span<byte> conversion = Need(hooks.ConversionBuffer)(allocated);
                // 원문 rep stosd의 63번 DWORD 쓰기를 보존하여 짧은 물리 블록에도 반 DWORD를 쓰지 않는다
                for (int word = 0; word < 63; word++)
                {
                    Put(conversion, word * 4, 0);
                }
                if (conversion.Length < 254)
                {
                    throw new ArgumentOutOfRangeException(nameof(conversion), conversion.Length, "TypeNames 할당 WORD 물리 범위");
                }
                conversion[252] = conversion[253] = 0; // stosw의 정확한 2BYTE다
                if (conversion.Length < ConversionBytes)
                {
                    throw new ArgumentOutOfRangeException(nameof(conversion), conversion.Length, "TypeNames 할당 BYTE 물리 범위");
                }
                conversion[254] = 0; // 마지막 stosb 뒤의 바이트는 유지한다
                uint hashPosition = Read(fields, 4);
                uint hashData = Read(fields, 0); // callback이 바꾼 현재 stream에서 주소를 한 번 캡처한다
                if (unchecked((int)hashPosition) > unchecked((int)Read(fields, 8)))
                {
                    Put(fields, 8, hashPosition);
                }
                // 매 반복 끝 현재 signed count를 읽는다. 타입 0과 256 이상의 낮은 BYTE wrap도 그대로다
                for (int type = 0; type < Need(hooks.TypeCount)(); type++)
                {
                    uint hash = Hash(Need(hooks.TypeName)(type));
                    // 같은 hash가 여러 저장 칸에 있으면 첫 칸만 갱신하고 다음 현재 타입으로 간다
                    for (uint stored = 0; stored < count; stored++)
                    {
                        if (Read(Need(buffer)(hashData), (ulong)hashPosition + stored * 4u) != hash)
                        {
                            continue;
                        }
                        Span<byte> current = Need(hooks.ConversionBuffer)(Need(globals.GetConversion)());
                        if (stored >= (uint)current.Length)
                        {
                            throw new ArgumentOutOfRangeException(nameof(stored), stored, "TypeNames 현재 변환 쓰기 물리 범위");
                        }
                        current[(int)stored] = (byte)type;
                        break; // 이름 getter가 현재 conversion을 교체했어도 새 주소에 쓴다
                    }
                }
            }
        }
        if (hooks.Access != null)
        {
            hooks.Access(token, (fields, memory) => Load(fields, memory.Buffer));
        }
        else
        {
            Load(Need(hooks.Stream)(token), hooks.Buffer); // 기존 독립 장치·664 관측은 동일 getter 경로를 유지한다
        }
        Depth(Need(globals.GetDepth)() - 1u, globals, hooks);
    }

    /// <summary>
    /// 정상 본문과 별도로 원문 cleanup의 현재값 감소 효과만 공급한다.
    /// 전체 OS SEH나 이미 정상 감소한 본문에 중복 적용하지 않는다.
    /// </summary>
    /// <param name="globals">같은 Client가 직접 공유하는 현재 전역이다.</param>
    /// <param name="hooks">관측 경계다. 깊이 통지만 쓴다.</param>
    public static void UnwindFortTypeNamesLoad(FortTypeNamesLoadGlobals globals, FortTypeNamesLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        Depth(Need(globals.GetDepth)() - 1u, globals, hooks);
    }
}
