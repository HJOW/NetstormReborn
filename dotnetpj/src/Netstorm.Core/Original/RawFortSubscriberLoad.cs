namespace Netstorm.Core.Original;

/// <summary>
/// 정상 파일 꼬리와 등록 helper가 동일하게 읽고 쓰는 실제 현재 DWORD다(cpppj <c>o/RawFortSubscriberLoad.h</c>의 <c>FortSubscriberLoadGlobals</c>).
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public sealed class FortSubscriberLoadGlobals
{
    /// <summary>현재 Subscriber ID 읽기다(원본 00540c74). 파일 ID를 직접 쓴 뒤 등록 helper가 현재값을 다시 읽는다.</summary>
    public Func<uint>? GetSubscriber { get; set; }

    /// <summary>현재 Subscriber ID 쓰기다.</summary>
    public Action<uint>? SetSubscriber { get; set; }
}

/// <summary>
/// 원문 진단은 복귀하면 현재 상태로 계속한다. 물리 접근 실패와 별개다(cpppj <c>o/RawFortSubscriberLoad.h</c>의 <c>FortSubscriberLoadAssertion</c>).
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public enum FortSubscriberLoadAssertion : uint
{
    /// <summary>고정 this의 첫 DWORD 0이다. 진단 뒤 현재 manager를 다시 읽는다.</summary>
    MissingManager = 0x1d8,

    /// <summary>ID·길이·복사 각 현재 pos 쓰기 뒤 signed pos가 end를 넘었다.</summary>
    PositionPastEnd = 0x2e,
}

/// <summary>
/// 원문 parent·string·등록 판정 외 실제 저장소·Config·새 ID 생성만 분리한다(cpppj <c>o/RawFortSubscriberLoad.h</c>의 <c>FortSubscriberLoadHooks</c>).
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortSubscriberLoadHooks
{
    /// <summary>같은 고정 Template의 현재 manager를 각 원문 조회 위치에서 읽는다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>0044ceb0(currentManager,"Subscriber",0)의 반환 stream 토큰이다.</summary>
    public Func<uint, string, int, uint>? Select { get; set; }

    /// <summary>같은 선택 노드 fields·실제 버퍼를 ID와 문자열 전체 동안 동기 빌린다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }

    /// <summary>진단 후 바뀐 현재 data·pos·end·ID를 그다음 원문 위치에서 읽는다.</summary>
    public Action<FortSubscriberLoadAssertion>? Assertion { get; set; }

    /// <summary>00441270("registeredSubId",현재 ID 참조)의 값 변경 여부다. GetInt·존재 여부로 대체하지 않는다.</summary>
    public SubscriberRegistrationRead? ReadRegistered { get; set; }

    /// <summary>004bc950의 실제 새 ID 생성이다. 선택 시에만 요구하며 반환 후 현재 ID에 쓴다.</summary>
    public Func<uint>? Generate { get; set; }

    /// <summary>00441580에 원문 오탈자 registerdSubId와 unsigned 십진수를 포함한 설정식을 넘긴다.</summary>
    public Action<string>? Configure { get; set; }
}

/// <summary>
/// "registeredSubId" 설정 읽기의 값 변경 여부다. 현재 ID를 참조로 받아 바꿀 수 있다.
/// </summary>
/// <param name="key">설정 키(원문 오탈자 registerdSubId가 아니라 registeredSubId로 조회한다).</param>
/// <param name="current">현재 Subscriber ID다. 읽은 값으로 바꿀 수 있다.</param>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public delegate bool SubscriberRegistrationRead(string key, ref uint current);

/// <summary>
/// 원본 004bd450·004bcb60·004b38d0의 실제 Subscriber ID·이름·등록 마감을 복원한다(cpppj <c>o/RawFortSubscriberLoad</c>).
/// handle은 진입 때 전달받은 실제 출력 버퍼다. 논리 end나 임의 256자 제한으로 선행 복사·부분 효과를 생략하지 않는다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public static class RawFortSubscriberLoad
{
    /// <summary>
    /// 선택한 실제 장치만 필수로 요구해 이미 완료한 앞선 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort Subscriber 실제 경계가 연결되지 않았다.</exception>
    private static T NeedSubscriber<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort Subscriber 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 비정렬 raw 필드·원문 BYTE를 little endian으로 읽으며 논리 end는 검사하지 않는다. 원문 이번 접근의 최대 DWORD값이다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 너비(BYTE 수)다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Subscriber 직접 읽기 물리 범위다.</exception>
    private static uint SubscriberRead(ReadOnlySpan<byte> bytes, uint offset, uint width)
    {
        if (offset > (uint)bytes.Length || width > (uint)bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort Subscriber 직접 읽기 물리 범위");
        }
        uint value = 0; // 원문 이번 접근의 최대 DWORD값이다
        for (uint index = 0; index < width; index++)
        {
            value |= (uint)bytes[(int)(offset + index)] << ((int)index * 8); // 서명·길이·각 복사 단위를 구별한다
        }
        return value;
    }

    /// <summary>
    /// 각 원문 DWORD·BYTE 쓰기 단위만 범위를 확인해 앞선 복사 prefix를 유지한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="width">쓸 너비(BYTE 수)다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Subscriber 직접 쓰기 물리 범위다.</exception>
    private static void SubscriberWrite(Span<byte> bytes, uint offset, uint width, uint value)
    {
        if (offset > (uint)bytes.Length || width > (uint)bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort Subscriber 직접 쓰기 물리 범위");
        }
        for (uint index = 0; index < width; index++)
        {
            bytes[(int)(offset + index)] = (byte)(value >> ((int)index * 8)); // 원문 이번 단위의 이웃 BYTE를 보존한다
        }
    }

    /// <summary>
    /// 현재 pos·end를 먼저 읽은 뒤 DWORD pos를 쓰고 signed 진단한다.
    /// </summary>
    /// <param name="fields">현재 stream 필드다.</param>
    /// <param name="length">진행할 길이(BYTE)다.</param>
    /// <param name="hooks">진단 경계다.</param>
    private static void SubscriberAdvance(Span<byte> fields, uint length, FortSubscriberLoadHooks hooks)
    {
        uint next = SubscriberRead(fields, 4, 4) + length;
        uint end = SubscriberRead(fields, 8, 4);
        SubscriberWrite(fields, 4, 4, next); // 진단 callback 전에 현재 pos를 공개한다
        if (unchecked((int)next) > unchecked((int)end))
        {
            NeedSubscriber(hooks.Assertion)(FortSubscriberLoadAssertion.PositionPastEnd);
        }
    }

    /// <summary>
    /// 004bcb60은 signed WORD를 캡처하고 진단 뒤 현재 data·pos를 고정해 앞에서부터 DWORD·잔여 BYTE를 복사한다.
    /// </summary>
    /// <param name="fields">현재 stream 필드다.</param>
    /// <param name="memory">현재 data getter다.</param>
    /// <param name="handle">복사 대상 출력 버퍼다.</param>
    /// <param name="hooks">진단 경계다.</param>
    private static void SubscriberString(Span<byte> fields, MemStreamHooks memory, Span<byte> handle, FortSubscriberLoadHooks hooks)
    {
        uint initialData = SubscriberRead(fields, 0, 4);
        uint initialPosition = SubscriberRead(fields, 4, 4); // 길이를 읽을 때의 실제 주소다
        ushort word = (ushort)SubscriberRead(NeedSubscriber(memory.Buffer)(initialData), initialPosition, 2); // 진단 전의 signed 길이 WORD다
        SubscriberAdvance(fields, 2, hooks);
        uint length = unchecked((uint)(int)(short)word); // 음수를 양의 16비트 길이로 고치지 않는다
        uint data = SubscriberRead(fields, 0, 4);
        uint position = SubscriberRead(fields, 4, 4); // 진단 뒤의 현재 복사 시작 주소이며 중간 전체 사본을 만들지 않는다
        ReadOnlySpan<byte> source = NeedSubscriber(memory.Buffer)(data);
        uint copied = 0;
        for (uint chunk = 0; chunk < (length >> 2); chunk++, copied += 4u)
        {
            // 원문 REP MOVSD의 각 단위를 읽고 쓰고 다음으로 간다
            uint value = SubscriberRead(source, position + copied, 4);
            SubscriberWrite(handle, copied, 4, value);
        }
        for (uint index = 0; index < (length & 3u); index++, copied++)
        {
            // 잔여 BYTE도 앞 방향으로 복사하고 앞선 DWORD 쓰기를 보존한다
            uint value = SubscriberRead(source, position + copied, 1);
            SubscriberWrite(handle, copied, 1, value);
        }
        SubscriberAdvance(fields, length, hooks);
        SubscriberWrite(handle, length, 1, 0); // 복사 뒤 현재 pos·end 진단이 끝나야 저장 길이 위치에 NUL을 쓴다
    }

    /// <summary>
    /// 등록 설정이 이미 같은 -1이면 ReadInt는 false이므로 실제 새 ID 생성과 오탈자 설정식도 필요하다.
    /// </summary>
    /// <param name="globals">정상 파일 꼬리와 등록 helper가 동일하게 읽고 쓰는 현재 DWORD다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 저장소·Config·새 ID 장치다.</param>
    public static void FinishFortSubscriberRegistration(FortSubscriberLoadGlobals globals, FortSubscriberLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedSubscriber(globals.GetSubscriber)() == 0xffffffffu)
        {
            uint current = NeedSubscriber(globals.GetSubscriber)();
            try
            {
                if (NeedSubscriber(hooks.ReadRegistered)("registeredSubId", ref current))
                {
                    return;
                }
            }
            finally
            {
                // hook이 바꾼 값을 예외 여부와 관계없이 같은 저장소에 둔다(원문 참조 전달과 같다)
                NeedSubscriber(globals.SetSubscriber)(current);
            }
            NeedSubscriber(globals.SetSubscriber)(NeedSubscriber(hooks.Generate)());
            string expression = "registerdSubId = \"" + NeedSubscriber(globals.GetSubscriber)().ToString(System.Globalization.CultureInfo.InvariantCulture) + "\""; // CRT %u의 32비트 unsigned 십진수와 원문 키를 그대로 유지한다
            NeedSubscriber(hooks.Configure)(expression);
        }
    }

    /// <summary>
    /// 004bd450: 현재 manager 진단·Select→실제 ID 쓰기→pos 진단→signed 길이·직접 복사·NUL→등록 마감이다.
    /// 실제 파일이 선택한 고정 this와 선택 반환 stream을 유지하며 등록 helper는 stream 빌림·반납 뒤 호출한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호(signed).</param>
    /// <param name="handle">진입 때 전달받은 실제 출력 버퍼다. 복사가 끝나야 저장 길이 위치에 NUL을 쓴다.</param>
    /// <param name="globals">정상 파일 꼬리와 등록 helper가 동일하게 읽고 쓰는 현재 DWORD다.</param>
    /// <param name="hooks">원문 parent·string·등록 판정 외 실제 저장소·Config·새 ID 생성이다.</param>
    public static void LoadFortSubscriber(int slot, byte[] handle, FortSubscriberLoadGlobals globals, FortSubscriberLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedSubscriber(hooks.Manager)(slot) == 0)
        {
            NeedSubscriber(hooks.Assertion)(FortSubscriberLoadAssertion.MissingManager);
        }
        uint manager = NeedSubscriber(hooks.Manager)(slot); // 진단 복귀 뒤 현재 manager를 재조회한다
        uint stream = NeedSubscriber(hooks.Select)(manager, "Subscriber", 0);
        NeedSubscriber(hooks.Access)(stream, (fields, memory) =>
        {
            uint data = SubscriberRead(fields, 0, 4);
            uint position = SubscriberRead(fields, 4, 4);
            NeedSubscriber(globals.SetSubscriber)(SubscriberRead(NeedSubscriber(memory.Buffer)(data), position, 4)); // ID를 먼저 직접 써서 뒤 진단이 현재 ID를 보게 한다
            SubscriberAdvance(fields, 4, hooks);
            SubscriberString(fields, memory, handle, hooks);
        });
        FinishFortSubscriberRegistration(globals, hooks);
    }
}
