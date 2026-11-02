using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 Template.cpp 957·958 및 MemStream 36행 진단 뒤 계속 지점을 구별한다(cpppj <c>o/RawFortObjectsSave.h</c>의 <c>FortObjectsSaveAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public enum FortObjectsSaveAssertion : uint
{
    /// <summary>004bf9a0 진입에 manager 0이다.</summary>
    MissingTemplate = 957,

    /// <summary>고정 this·현재 선택 주소 불일치다.</summary>
    NotMine = 958,

    /// <summary>일반 쓰기 용량 진단이다.</summary>
    WriteCapacity = 36,
}

/// <summary>
/// 실제 클라이언트·Session의 장기 전역을 같은 쓰기 위치에서 빌린다. C++의 참조 멤버는 읽기·쓰기 대리자 쌍으로 옮긴다(cpppj <c>o/RawFortObjectsSave.h</c>의 <c>FortObjectsSaveGlobals</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다. F2a의 loadingDepth 선례와 같이 <c>uint32_t&amp; version</c>은 getter·setter 쌍으로 둔다.</remarks>
public sealed class FortObjectsSaveGlobals
{
    /// <summary>00540cac: 현재 Template 선택이다. 고정 this 슬롯과 비교한다.</summary>
    public uint TemplateOwner { get; set; }

    /// <summary>005c89c0: 시작에 2를 쓰고 각 새 stream에 현재 하위 바이트를 쓴다. 호출자가 소유한다.</summary>
    public Func<uint>? GetVersion { get; set; }

    /// <summary>005c89c0: 시작에 2를 쓴다. 호출자가 소유한다.</summary>
    public Action<uint>? SetVersion { get; set; }

    /// <summary>00594fb8: 20개 영역이 끝난 뒤 한 번 읽는다.</summary>
    public Func<uint>? Fort { get; set; }
}

/// <summary>
/// 실제 같은 manager·DataManager와 기존 영역·Chaff writer를 연결하는 명시 하위 경계다(cpppj <c>o/RawFortObjectsSave.h</c>의 <c>FortObjectsSaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortObjectsSaveHooks
{
    /// <summary>고정 this 슬롯의 현재 첫 DWORD를 매 원본 위치에서 읽는다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>도달한 진단만 요구하며 복귀 뒤 현재 값으로 계속한다.</summary>
    public Action<FortObjectsSaveAssertion>? Assertion { get; set; }

    /// <summary>0044ce60: 현재 manager에서 Terr 이름을 검색한다.</summary>
    public Func<uint, string, uint>? Find { get; set; }

    /// <summary>0044d1b0: 없는 Terr·새 Temp·Chaff의 실제 생성·반납 수명이다.</summary>
    public Func<uint, string, uint>? Create { get; set; }

    /// <summary>0044ceb0: 현재 manager의 Temp를 writable 1로 선택하고 pos 0을 쓴다.</summary>
    public Func<uint, string, int, uint>? Select { get; set; }

    /// <summary>0044d160(manager, destination, source)의 정확한 순서다.</summary>
    public Action<uint, string, string>? Rename { get; set; }

    /// <summary>004be230(stream, clampedOwner, region)의 전체 영역 순회며 한 청크로 대체하지 않는다.</summary>
    public Action<uint, uint, int>? Territory { get; set; }

    /// <summary>004be350(stream)의 전체 지도 Chaff 순회다.</summary>
    public Action<uint>? Chaff { get; set; }

    /// <summary>같은 stream 16바이트·물리 버퍼의 동기 빌림이다. 검색·선택 효과를 추가하지 않는다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }
}

/// <summary>
/// 고정 this 슬롯의 20영역→조건부 Chaff를 원문 순서로 저장한다. 전체 snapshot·Notify·파일 쓰기·UI는 호출자의 경계다(cpppj <c>o/RawFortObjectsSave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public static class RawFortObjectsSave
{
    /// <summary>
    /// 선택되지 않은 하위는 요구하지 않고 도달한 미연결 경계만 명시 실패시킨다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Objects 저장 부모의 실제 경계 미연결이다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Objects 저장 부모 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 실제 비정렬 DWORD를 little endian으로 읽는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at)
    {
        if (at > bytes.Length || bytes.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Objects stream DWORD 물리 범위");
        }
        uint value = 0;
        // 해당 네 바이트만 합쳐 주변 필드를 읽지 않는다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 원본 16바이트 stream의 지정 DWORD만 쓴다.
    /// </summary>
    /// <param name="bytes">쓸 16바이트 필드다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int at, uint value)
    {
        if (at > bytes.Length || bytes.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Objects stream DWORD 쓰기 범위");
        }
        // 낮은 바이트부터 원본 폭을 보존한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// version은 진단 전에 캡처하며 진단 복귀 뒤의 현재 data·pos로 실제 바이트만 쓴다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="value">쓸 버전 하위 바이트다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    private static void Version(uint stream, byte value, FortObjectsSaveHooks hooks)
    {
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            if (unchecked((int)Read(fields, 12)) < unchecked((int)(Read(fields, 4) + 1U)))
            {
                Need(hooks.Assertion)(FortObjectsSaveAssertion.WriteCapacity);
            }
            // 진단이 바꾼 현재 필드를 실제 쓰기 주소로 결정한다
            uint position = Read(fields, 4);
            Span<byte> versionBuffer = Need(memory.Buffer)(Read(fields, 0));
            if (position >= (uint)versionBuffer.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(stream), position, "Objects version 출력 바이트 물리 범위");
            }
            versionBuffer[(int)position] = value;
            uint next = Read(fields, 4) + 1U;
            Put(fields, 4, next);
            if (unchecked((int)Read(fields, 8)) < unchecked((int)next))
            {
                Put(fields, 8, next);
            }
        });
    }

    /// <summary>
    /// Temp의 포인터·논리 end가 모두 비영일 때만 rename한다. pos나 논리 자료 길이 1 이상을 추가 조건으로 쓰지 않는다.
    /// </summary>
    /// <param name="stream">볼 stream 토큰이다.</param>
    /// <param name="hooks">실제 조회 경계다.</param>
    private static bool HasData(uint stream, FortObjectsSaveHooks hooks)
    {
        bool result = false;
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            result = Read(fields, 0) != 0 && Read(fields, 8) != 0;
        });
        return result;
    }

    /// <summary>
    /// 진단 뒤의 현재 manager와 낮은 version 바이트를 사용하며 owner는 고정 this 슬롯에서만 clamp한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="globals">같은 Template 선택·현재 버전·Fort 분기다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 저장소·쓰기 경계다.</param>
    public static void SaveFortObjects(int slot, FortObjectsSaveGlobals globals, FortObjectsSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Manager)(slot) == 0)
        {
            Need(hooks.Assertion)(FortObjectsSaveAssertion.MissingTemplate);
        }
        if (unchecked((uint)slot) != globals.TemplateOwner)
        {
            Need(hooks.Assertion)(FortObjectsSaveAssertion.NotMine);
        }
        Need(globals.SetVersion)(2);
        // 원본은 존재하지 않는 Terr를 먼저 생성한 뒤 각 영역마다 새 Temp를 만든다
        for (int region = 0; region < 20; region++)
        {
            string name = $"Terr{region:00}";
            if (Need(hooks.Find)(Need(hooks.Manager)(slot), name) == 0)
            {
                Need(hooks.Create)(Need(hooks.Manager)(slot), name);
            }
            uint stream = Need(hooks.Create)(Need(hooks.Manager)(slot), "Temp");
            Version(stream, (byte)Need(globals.GetVersion)(), hooks);
            Need(hooks.Territory)(stream, slot >= 1 && slot <= 8 ? (uint)slot : 0U, region);
            uint selected = Need(hooks.Select)(Need(hooks.Manager)(slot), "Temp", 1);
            if (HasData(selected, hooks))
            {
                Need(hooks.Rename)(Need(hooks.Manager)(slot), name, "Temp");
            }
        }
        if (Need(globals.Fort)() != 0)
        {
            uint stream = Need(hooks.Create)(Need(hooks.Manager)(slot), "Chaff");
            Version(stream, (byte)Need(globals.GetVersion)(), hooks);
            Need(hooks.Chaff)(stream);
        }
    }
}
