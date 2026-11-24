namespace Netstorm.Core.Original; // 원문 임시 고리 알고리즘과 실제 장치 결선을 분리한다.

// 원문 임시 문자열 고리의 같은 네 DWORD를 실제 본체와 관측에 공유한다.
public sealed class StaticStringGlobals
{
    /// <summary>예약 전체 크기다. 초기화 때 실제 할당 크기로 다시 읽는다.</summary>
    public uint Size;
    /// <summary>실제 할당의 원문 주소 토큰이다. 0이면 아직 초기화하지 않았다.</summary>
    public uint Base;
    /// <summary>다음 예약 시작의 원문 주소 토큰이다.</summary>
    public uint Current;
    /// <summary>마지막 예약 결과의 원문 주소 토큰이다.</summary>
    public uint Last;
}

// 할당과 순수 물리 대여 및 진단과 CRT 서식 장치다. 본체는 도달한 장치만 요구한다.
public sealed class StaticStringHooks
{
    /// <summary>실제 operator new이며 반환 0도 뒤 물리 접근에서 판정한다.</summary>
    public Func<uint, uint>? Allocate { get; set; }
    /// <summary>한 원문 접근의 같은 실제 물리 폭이다. 조회 자체의 상태 변경은 허용하지 않는다.</summary>
    public Func<uint, uint, Memory<byte>>? Memory { get; set; }
    /// <summary>이미 있는 base의 Staticstring.cpp 17 진단을 기록한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>004de490의 같은 목적과 서식 주소다. 실제 값인자 수명은 호출자가 소유한다.</summary>
    public Action<uint, uint>? Format { get; set; }
}

// cpppj/src/o/RawStaticStrings.h/.cpp의 공유 고리 외곽을 같은 네 DWORD에 실행한다.
public static class RawStaticStrings
{
    /// <summary>선택 위치에서만 실제 장치를 요구한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("임시 문자열의 선택된 실제 장치 미연결");

    /// <summary>한 DWORD와 BYTE 전체 폭을 확인한 뒤 해당 바이트만 작성한다.</summary>
    private static void Store(StaticStringHooks hooks, uint address, uint width, byte value)
    {
        // 매장마다 같은 물리 폭을 다시 대여한다.
        Memory<byte> bytes = Need(hooks.Memory)(address, width);
        if (bytes.Length < width) throw new ArgumentOutOfRangeException(nameof(address), "임시 문자열 원문 저장 물리 범위");
        for (int index = 0; index < width; index++) bytes.Span[index] = value;
    }

    /// <summary>004b92b0 새 할당 뒤 DWORD와 BYTE 0 채움과 첫 D5와 current+1을 수행한다. last는 쓰지 않는다. 0.985에서 추가됐다.</summary>
    /// <param name="size">고정할 전체 크기다.</param>
    /// <param name="globals">같은 네 DWORD 저장소다.</param>
    /// <param name="hooks">실제 할당과 물리와 진단 장치다.</param>
    public static void InitializeStaticStrings(uint size, StaticStringGlobals globals, StaticStringHooks hooks)
    {
        if (globals.Base != 0) Need(hooks.Assertion)(17);
        globals.Size = size;
        uint allocated = Need(hooks.Allocate)(size);
        uint count = globals.Size;
        globals.Base = allocated;
        uint destination = allocated;
        // rep stosd의 전체 DWORD 폭과 부분 효과를 유지한다.
        for (uint word = 0; word < (count >> 2); word++, destination += 4) Store(hooks, destination, 4, 0);
        // 마지막 나머지만 BYTE로 쓴다.
        for (uint rest = 0; rest < (count & 3u); rest++, destination++) Store(hooks, destination, 1, 0);
        uint current = globals.Base;
        globals.Current = current;
        Store(hooks, current, 1, 0xd5);
        globals.Current = globals.Current + 1u;
    }

    /// <summary>004b9320 최초 8192와 부호 없는 감김과 last와 current 끝 D5 순서를 보존한다. 0.985에서 추가됐다.</summary>
    /// <param name="size">예약할 크기다. 반환 뒤 같은 할당 안에서는 더 쓸 수 있다.</param>
    /// <param name="globals">같은 네 DWORD 저장소다.</param>
    /// <param name="hooks">실제 할당과 물리와 진단 장치다.</param>
    /// <returns>예약 결과의 원문 주소 토큰이다.</returns>
    public static uint ReserveStaticString(uint size, StaticStringGlobals globals, StaticStringHooks hooks)
    {
        uint currentBase = globals.Base;
        if (currentBase == 0) { InitializeStaticStrings(8192, globals, hooks); currentBase = globals.Base; }
        uint end = unchecked(currentBase + globals.Size);
        uint start = globals.Current;
        // 두 주소 합은 원문 DWORD로 감긴다.
        if (unchecked(start + size + 1u) >= end)
        {
            globals.Current = currentBase;
            Store(hooks, currentBase, 1, 0xd5);
            start = globals.Current + 1u;
        }

        globals.Last = start;
        globals.Current = unchecked(start + size);
        Store(hooks, globals.Current, 1, 0xd5);
        globals.Current = unchecked(globals.Current + 1u);
        return globals.Last;
    }

    /// <summary>004b9390 예약 결과와 서식 주소가 같으면 실제 CRT 호출도 생략한다. 0.985에서 추가됐다.</summary>
    /// <param name="size">예약할 크기다.</param>
    /// <param name="pattern">같은 고리 안 서식 주소 토큰이다.</param>
    /// <param name="globals">같은 네 DWORD 저장소다.</param>
    /// <param name="hooks">실제 할당과 물리와 CRT 장치다.</param>
    /// <returns>캡처한 반환 주소 토큰이다. CRT가 last와 current를 바꿔도 유지한다.</returns>
    public static uint FormatStaticString(uint size, uint pattern, StaticStringGlobals globals, StaticStringHooks hooks)
    {
        uint result = ReserveStaticString(size, globals, hooks);
        if (result != pattern) Need(hooks.Format)(result, pattern);
        return result;
    }
}
