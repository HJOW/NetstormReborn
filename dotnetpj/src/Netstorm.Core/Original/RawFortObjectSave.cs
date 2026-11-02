using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 객체 writer가 각 분기에서 다시 읽는 실제 전역 DWORD다(cpppj <c>o/RawFortObjectSave.h</c>의 <c>FortObjectSaveGlobal</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public enum FortObjectSaveGlobal
{
    /// <summary>00594fbc 전투 전역이다.</summary>
    Battle,

    /// <summary>00594fa0 미션 전역이다.</summary>
    Mission,

    /// <summary>005c85a4 편집 전역이다.</summary>
    Editor,

    /// <summary>005c89b0 Terr1·Chaff2 구분이다.</summary>
    Scope,

    /// <summary>005c89c0 서식 버전이다.</summary>
    Version,

    /// <summary>00540c70 현재 플레이어다.</summary>
    LocalOwner,

    /// <summary>005411a0 다리 약식 저장 판정 타입이다.</summary>
    BridgeType,

    /// <summary>005412ec 로컬 다이스 저장 판정 타입이다.</summary>
    LocalOnlyType,

    /// <summary>0054263c 현재 flags2와 비교하는 마스크다.</summary>
    BattleMask,

    /// <summary>00542644 현재 flags2와 비교하는 마스크다.</summary>
    OwnerMask,
}

/// <summary>
/// 원본은 보고 뒤 위치 BYTE 또는 owner 쓰기를 계속할 수 있다(cpppj <c>o/RawFortObjectSave.h</c>의 <c>FortObjectSaveAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public enum FortObjectSaveAssertion
{
    /// <summary>Template.cpp 667행 진단이다.</summary>
    XOffset,

    /// <summary>Template.cpp 668행 진단이다.</summary>
    YOffset,

    /// <summary>Template.cpp 707행 진단이다.</summary>
    Owner,
}

/// <summary>
/// 같은 stream에 캡처한 BYTE·WORD를 쓰는 경계다(cpppj <c>o/RawFortObjectSave.h</c>의 write).
/// </summary>
/// <param name="stream">쓸 stream 토큰이다.</param>
/// <param name="bytes">쓸 바이트 구간이다.</param>
/// <remarks>2026-11-03 F2c-5에서 추가했다. <c>Span</c>을 담는 대리자는 전용 대리자로 둔다.</remarks>
public delegate void FortObjectSaveWriter(uint stream, ReadOnlySpan<byte> bytes);

/// <summary>
/// 실제 TYPE·가상·stream 경계를 공급하며 저장 순서와 필터는 본체가 수행한다(cpppj <c>o/RawFortObjectSave.h</c>의 <c>FortObjectSaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortObjectSaveHooks
{
    /// <summary>매 호출 현재 DWORD를 읽으며 최초 값으로 캐시하지 않는다.</summary>
    public Func<FortObjectSaveGlobal, uint>? Global { get; set; }

    /// <summary>고정 SID의 raw 50바이트를 빌린다. 빌림 자체는 효과가 없다.</summary>
    public RawReadOnlyBytesLookup? Form { get; set; }

    /// <summary>004ac1e0: 현재 raw TYPE를 매번 다시 조회한다.</summary>
    public Func<uint, uint>? Flags1 { get; set; }

    /// <summary>004ac200: 현재 raw TYPE를 매번 다시 조회한다.</summary>
    public Func<uint, uint>? Flags2 { get; set; }

    /// <summary>현재 가상+60이다. ContentForm 판정으로 대체하지 않는다.</summary>
    public Func<uint, uint>? Robust { get; set; }

    /// <summary>004ad0a0 가상이다.</summary>
    public Func<uint, uint>? HasLevel { get; set; }

    /// <summary>가상 80이다.</summary>
    public Func<uint, uint>? Level { get; set; }

    /// <summary>0046f6e0: 현재 x·y float 비트를 인자로 캡처한다.</summary>
    public Func<uint, uint, uint>? Island { get; set; }

    /// <summary>0044cce0: 캡처한 BYTE·WORD만 같은 stream에 쓴다.</summary>
    public FortObjectSaveWriter? Write { get; set; }

    /// <summary>004bcea0: 고정 SID와 동일 stream의 내용물을 저장한다.</summary>
    public Action<uint, uint>? Contents { get; set; }

    /// <summary>실제 선택한 진단만 요구한다.</summary>
    public Action<FortObjectSaveAssertion>? Assertion { get; set; }
}

/// <summary>
/// 청크 Begin이 받는 네 단정도 좌표와 고정 flags0이다(cpppj <c>o/RawFortObjectSave.h</c>의 <c>FortChunkSaveArea</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public struct FortChunkSaveArea
{
    /// <summary>원본 int&lt;&lt;4→float 저장 후 왼쪽 값이다.</summary>
    public float Left;

    /// <summary>원본 int&lt;&lt;4→float 저장 후 위쪽 값이다.</summary>
    public float Top;

    /// <summary>원본 int&lt;&lt;4→float 저장 후 오른쪽 값이다.</summary>
    public float Right;

    /// <summary>원본 int&lt;&lt;4→float 저장 후 아래쪽 값이다.</summary>
    public float Bottom;
}

/// <summary>
/// 원본 Finder의 각 청크별 독립 현재·다음 수명이다(cpppj <c>o/RawFortObjectSave.h</c>의 <c>FortChunkSaveCursor</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public sealed class FortChunkSaveCursor
{
    /// <summary>현재 DWORD 후보이며 다른 효과 뒤 다시 조회한다.</summary>
    public Func<uint>? Current { get; set; }

    /// <summary>저장 후 원본 Next를 실행한다.</summary>
    public Action? Next { get; set; }
}

/// <summary>
/// 청크 검색·단일 객체와 실제 stream만 연결하며 표지·count는 본체가 쓴다(cpppj <c>o/RawFortObjectSave.h</c>의 <c>FortChunkSaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortChunkSaveHooks
{
    /// <summary>0041def0: 사각형·flags0으로 첫 후보까지 진행한다.</summary>
    public Func<FortChunkSaveArea, FortChunkSaveCursor>? Begin { get; set; }

    /// <summary>각 후보 경계에서 현재 물리 SID 수를 읽는다.</summary>
    public Func<uint>? PoolLimit { get; set; }

    /// <summary>004bd940의 고정 stream·후보·최초 origin이다.</summary>
    public Func<uint, uint, int, int, uint>? Object { get; set; }

    /// <summary>실제 stream 필드와 같은 물리 버퍼의 동기적 빌림이다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }
}

/// <summary>
/// 004bd940의 초기 좌표 절삭·필터→위치·타입→현재 폭별 필드→레벨·owner→내용물 순서와
/// 004be020의 청크 marker·count 예약을 복원한다(cpppj <c>o/RawFortObjectSave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public static class RawFortObjectSave
{
    /// <summary>원본 __ftol의 signed64 불확정값 경계다.</summary>
    private const double Int64Boundary = 9223372036854775808.0;

    /// <summary>
    /// 실제 선택한 장치가 없으면 앞선 부분 쓰기를 보존한 채 중단한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort 객체 저장의 실제 장치 누락이다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort 객체 저장 실제 장치 누락");
        }
        return function;
    }

    /// <summary>
    /// 비정렬 raw 값을 지정 폭으로 읽으며 물리 범위 밖은 거부한다.
    /// </summary>
    /// <param name="bytes">읽을 raw다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <param name="width">읽을 너비(BYTE 수)다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        if (at > bytes.Length || width > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Fort 객체 저장의 raw 필드 범위");
        }
        uint value = 0;
        // 지정 BYTE만 조합하고 이웃 자료를 읽지 않는다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 지정 DWORD를 little endian으로 기록한다.
    /// </summary>
    /// <param name="bytes">쓸 16바이트 필드다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int at, uint value)
    {
        if (at > bytes.Length || bytes.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Fort 청크 저장의 stream 필드 범위");
        }
        // 이웃 필드는 보존한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// DWORD 산술의 감김 뒤 같은 비트를 signed로 읽는다.
    /// </summary>
    /// <param name="value">바꿀 값이다.</param>
    private static int Signed(uint value)
    {
        return unchecked((int)value);
    }

    /// <summary>
    /// 현재 BYTE·WORD 값은 외부 Write 이전에 캡처한다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비(BYTE 수)다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    private static void Write(uint stream, uint value, int width, FortObjectSaveHooks hooks)
    {
        byte[] bytes = [(byte)value, (byte)(value >> 8)];
        Need(hooks.Write)(stream, new ReadOnlySpan<byte>(bytes, 0, width));
    }

    /// <summary>
    /// 실제 __ftol의 signed64 불확정값과 하위 DWORD 감김을 float32 좌표에 적용한다.
    /// 입력 float는 double에 정확히 표현되므로 중간 x87 정밀도 반올림이 없다.
    /// </summary>
    /// <param name="input">바꿀 float32다.</param>
    public static int FortSaveCoordinate(float input)
    {
        double value = input;
        if (!double.IsFinite(value) || value >= Int64Boundary || value < -Int64Boundary)
        {
            return 0;
        }
        return Signed(unchecked((uint)unchecked((ulong)unchecked((long)value))));
    }

    /// <summary>
    /// 초기 좌표는 고정하지만 뒤의 실제 virtual·TYPE·필드는 매 원본 소비 위치에서 읽는다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="sid">고정 SID다.</param>
    /// <param name="originX">최초 원점 X다.</param>
    /// <param name="originY">최초 원점 Y다.</param>
    /// <param name="hooks">실제 조회·쓰기 경계다.</param>
    public static uint SaveFortObject(uint stream, uint sid, int originX, int originY, FortObjectSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ReadOnlySpan<byte> raw() => Need(hooks.Form)(sid);
        uint global(FortObjectSaveGlobal key) => Need(hooks.Global)(key);
        int x = FortSaveCoordinate(BitConverter.UInt32BitsToSingle(Read(raw(), 14)));
        int y = FortSaveCoordinate(BitConverter.UInt32BitsToSingle(Read(raw(), 18)));
        if (x >= Signed(unchecked((uint)originX + 16U)) || y >= Signed(unchecked((uint)originY + 16U)) || (Read(raw(), 40, 1) & 9U) != 0)
        {
            return 0;
        }
        if (global(FortObjectSaveGlobal.Battle) != 0 && global(FortObjectSaveGlobal.Mission) == 0 && global(FortObjectSaveGlobal.Editor) == 0)
        {
            uint flags = Need(hooks.Flags2)(sid);
            // getter의 효과 뒤 현재 마스크를 다시 읽는다
            if ((flags & global(FortObjectSaveGlobal.BattleMask)) == 0)
            {
                return 0;
            }
        }
        // 원본 가상60 뒤의 현재 float 비트를 캡처한다
        uint island() => Need(hooks.Island)(Read(raw(), 14), Read(raw(), 18));
        bool selected = false;
        if (global(FortObjectSaveGlobal.Scope) == 1)
        {
            selected = Need(hooks.Robust)(sid) != 0 && island() != 0;
        }
        if (global(FortObjectSaveGlobal.Scope) == 2)
        {
            if (Need(hooks.Robust)(sid) == 0 || island() != 0)
            {
                return 0;
            }
        }
        else if (!selected)
        {
            return 0;
        }
        if (Read(raw(), 10, 1) == global(FortObjectSaveGlobal.LocalOnlyType) && Read(raw(), 34, 1) != global(FortObjectSaveGlobal.LocalOwner))
        {
            return 0;
        }
        int cx = Signed(unchecked((uint)x) - unchecked((uint)originX));
        int cy = Signed(unchecked((uint)y) - unchecked((uint)originY));
        if (cx < 0 || cx > 15)
        {
            Need(hooks.Assertion)(FortObjectSaveAssertion.XOffset);
        }
        if (cy < 0 || cy > 15)
        {
            Need(hooks.Assertion)(FortObjectSaveAssertion.YOffset);
        }
        Write(stream, unchecked((uint)cx << 4) | (byte)cy, 1, hooks);
        uint kind = Read(raw(), 10, 1);
        Write(stream, kind == global(FortObjectSaveGlobal.BridgeType) ? ~Read(raw(), 34, 1) : kind, 1, hooks);
        if ((Need(hooks.Flags1)(sid) & 0x20U) != 0)
        {
            Write(stream, Read(raw(), 36, 1), 1, hooks);
        }
        if ((Need(hooks.Flags1)(sid) & 0x1000U) != 0)
        {
            Write(stream, Read(raw(), 12, 1), 1, hooks);
        }
        if ((Need(hooks.Flags1)(sid) & 0x2000U) != 0)
        {
            Write(stream, Read(raw(), 12, 2), 2, hooks);
        }
        if ((Need(hooks.Flags2)(sid) & 4U) != 0)
        {
            Write(stream, Read(raw(), 36, 1), 1, hooks);
        }
        if (global(FortObjectSaveGlobal.Version) == 0 && Need(hooks.HasLevel)(sid) != 0)
        {
            Write(stream, Need(hooks.Level)(sid), 1, hooks);
        }
        if (Signed(global(FortObjectSaveGlobal.Version)) > 1 && (Need(hooks.Flags2)(sid) & 0x4000U) != 0)
        {
            Write(stream, Need(hooks.Level)(sid), 1, hooks);
        }
        if (Signed(global(FortObjectSaveGlobal.Version)) > 0)
        {
            // getter의 효과 뒤 현재 마스크를 다시 읽는다
            uint flags = Need(hooks.Flags2)(sid);
            if ((flags & global(FortObjectSaveGlobal.OwnerMask)) != 0)
            {
                if (Read(raw(), 34, 1) > 8)
                {
                    Need(hooks.Assertion)(FortObjectSaveAssertion.Owner);
                }
                Write(stream, Read(raw(), 34, 1), 1, hooks);
            }
        }
        if ((Need(hooks.Flags1)(sid) & 0x20000U) != 0)
        {
            Need(hooks.Contents)(sid, stream);
        }
        return 1;
    }

    /// <summary>
    /// 청크 원점은 감긴 int&lt;&lt;4를 먼저 float32에 저장하고 원본 __ftol의 EAX를 보관한다.
    /// Begin→marker99→count WORD 위치 예약→후보 저장→탈출 시 고정 위치에 count 쓰기다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="chunkX">청크 X다.</param>
    /// <param name="chunkY">청크 Y다.</param>
    /// <param name="hooks">실제 검색·쓰기 경계다.</param>
    public static void SaveFortChunk(uint stream, int chunkX, int chunkY, FortChunkSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint x = unchecked((uint)chunkX) << 4;
        uint y = unchecked((uint)chunkY) << 4;
        FortChunkSaveArea area = new()
        {
            Left = Signed(x),
            Top = Signed(y),
            Right = Signed(x + 15U),
            Bottom = Signed(y + 15U),
        };
        int originX = FortSaveCoordinate(area.Left);
        int originY = FortSaveCoordinate(area.Top);
        FortChunkSaveCursor cursor = Need(hooks.Begin)(area);
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            byte[] marker = [99];
            RawMemStream.Write(fields, marker, 1, memory);
        });
        uint data = 0;
        uint offset = 0;
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            data = Read(fields, 0);
            offset = Read(fields, 4);
            uint next = offset + 2U;
            Put(fields, 4, next);
            if (Signed(Read(fields, 8)) < Signed(next))
            {
                Put(fields, 8, next);
            }
        });
        // count 예약은 0을 쓰지 않는다
        ushort count = 0;
        // 원본 현재 후보가 물리 범위를 벗어날 때만 count를 고정 주소에 쓴다
        for (; ; )
        {
            uint candidate = Need(cursor.Current)();
            if (candidate == 0 || candidate >= Need(hooks.PoolLimit)())
            {
                break;
            }
            count = unchecked((ushort)(count + Need(hooks.Object)(stream, candidate, originX, originY)));
            Need(cursor.Next)();
        }
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            Span<byte> countBuffer = Need(memory.Buffer)(data);
            if (offset > (uint)countBuffer.Length || (uint)countBuffer.Length - offset < 2)
            {
                throw new ArgumentOutOfRangeException(nameof(stream), offset, "Fort 청크 count WORD의 고정 출력 범위");
            }
            countBuffer[(int)offset] = (byte)count;
            countBuffer[(int)offset + 1] = (byte)(count >> 8);
        });
    }
}
