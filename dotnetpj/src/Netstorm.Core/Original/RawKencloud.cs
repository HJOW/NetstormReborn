// 원본 Kencloud의 파일 부모 00470bb0·생성 00471040·행 이동 00470b20을 같은 현재 물리·GameRandom에 복원한다.
// 새 난수·정렬·CRT로 대체하지 않는다. cpppj o/RawKencloud.cpp/.h 의 직역이다. (0.381, 2026-11-11)
namespace Netstorm.Core.Original;

/// <summary>
/// Kencloud 장치가 빌린 주소 물리에서 원문이 실패하는 접근 시점에만 던지는 예외다.
/// <see cref="Cause"/>는 원문 근거의 짧은 원인 이름("step-read"·"move-physical" 등)이며 fixture의 중단 원인과 같다.
/// cpppj의 <c>std::out_of_range("Kencloud …")</c>에 대응한다.
/// </summary>
/// <remarks>0.381에서 추가했다. 원문 몸체의 부분 효과(앞선 쓰기)는 던지기 전에 이미 남아 있다.</remarks>
public sealed class KencloudPhysicalException : Exception
{
    /// <summary>중단 원인 이름이다("Kencloud " 접두 제외).</summary>
    public string Cause { get; }

    /// <summary>원인 이름으로 만든다.</summary>
    /// <param name="cause">중단 원인 이름이다.</param>
    public KencloudPhysicalException(string cause) : base("Kencloud " + cause) => Cause = cause;
}

/// <summary>
/// 구름 전역을 현재 소유자의 실제 DWORD 물리(<see cref="State"/> 2054칸)와 옵션 8칸·고정 행표 16칸으로 빌려 쓰는 상자다.
/// 객체 자체가 난수나 캐시를 새로 소유하지 않으며 모든 읽기·쓰기는 같은 배열을 통과한다(cpppj <c>KencloudGlobals</c>의 참조 묶음).
/// 연속 전역 005697f0..0056b807의 DWORD 위치: 이전 행 dirty 0..511, end 512, lastRow 513, nextRow 514, cache 515, 현재 행 dirty 516..1027,
/// seeds 1028..2027, 이름·flags 2028..2053(allDirty 2051, partialDirty 2052, phase 2053).
/// </summary>
/// <remarks>0.381에서 추가했다. 옵션 순서는 cloudiness·cloudSpeed·cloudFury·cloudSpacer·cloudRecenter·seedCount·lowColor·highColor다(원문 00540a7c..90과 색 두 DWORD).</remarks>
public sealed class KencloudGlobals
{
    /// <summary>연속 전역의 DWORD 수다(8216바이트).</summary>
    public const int StateLength = 2054;

    /// <summary>옵션 DWORD 수다.</summary>
    public const int OptionLength = 8;

    /// <summary>고정 위상 행표의 칸 수다.</summary>
    public const int PhaseRowLength = 16;

    /// <summary>00569ff0 end의 위치다.</summary>
    public const int EndIndex = 512;

    /// <summary>00569ff4 lastRow의 위치다.</summary>
    public const int LastRowIndex = 513;

    /// <summary>00569ff8 nextRow의 위치다.</summary>
    public const int NextRowIndex = 514;

    /// <summary>00569ffc cache의 위치다.</summary>
    public const int CacheIndex = 515;

    /// <summary>현재 행 dirty 표(0056a000, 512 DWORD)의 시작 위치다.</summary>
    public const int DirtyRowsIndex = 516;

    /// <summary>seed 주소 표(0056a800, 1000 DWORD)의 시작 위치다.</summary>
    public const int SeedsIndex = 1028;

    /// <summary>0056b7fc 전체 표시 요청의 위치다.</summary>
    public const int AllDirtyIndex = 2051;

    /// <summary>0056b800 부분 표시 요청의 위치다.</summary>
    public const int PartialDirtyIndex = 2052;

    /// <summary>0056b804 위상의 위치다(하위 4비트로 진행한다).</summary>
    public const int PhaseIndex = 2053;

    /// <summary>cloudSpeed 옵션(00540a80)의 위치다.</summary>
    public const int SpeedOption = 1;

    /// <summary>cloudFury 옵션(00540a84)의 위치다.</summary>
    public const int FuryOption = 2;

    /// <summary>cloudSpacer 옵션(00540a88)의 위치다.</summary>
    public const int SpacerOption = 3;

    /// <summary>seedCount 옵션(00540a90)의 위치다.</summary>
    public const int SeedCountOption = 5;

    /// <summary>현재 low 색(00540a34) 옵션의 위치다. fallback 할당을 이 BYTE로 채운다.</summary>
    public const int LowColorOption = 6;

    /// <summary>같은 packed 연속 전역 2054 DWORD다. 인접 별칭을 보존하기 위해 사본을 만들지 않는다.</summary>
    public uint[] State { get; }

    /// <summary>옵션 8 DWORD다. 최초값은 호출자가 정한다.</summary>
    public uint[] Options { get; }

    /// <summary>보호 PE 00540a98의 고정 16행 순서다.</summary>
    public uint[] PhaseRows { get; }

    /// <summary>빌려 쓸 실제 배열 셋으로 만든다.</summary>
    /// <param name="state">연속 전역 2054 DWORD다.</param>
    /// <param name="options">옵션 8 DWORD다.</param>
    /// <param name="phaseRows">고정 위상 행표 16 DWORD다.</param>
    public KencloudGlobals(uint[] state, uint[] options, uint[] phaseRows)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(options);
        ArgumentNullException.ThrowIfNull(phaseRows);
        if (state.Length != StateLength || options.Length != OptionLength || phaseRows.Length != PhaseRowLength)
            throw new ArgumentException("Kencloud 전역 물리 크기 불일치");
        State = state;
        Options = options;
        PhaseRows = phaseRows;
    }

    /// <summary>00569ffc 현재 512×512 BYTE 할당의 주소 토큰이다.</summary>
    public uint Cache { get => State[CacheIndex]; set => State[CacheIndex] = value; }

    /// <summary>00569ff0이며 cache 변경 시 임의로 자동 동기화하지 않는다.</summary>
    public uint End { get => State[EndIndex]; set => State[EndIndex] = value; }

    /// <summary>00569ff8은 생성 당시 cache+512다.</summary>
    public uint NextRow { get => State[NextRowIndex]; set => State[NextRowIndex] = value; }

    /// <summary>00569ff4는 생성 당시 end-512다.</summary>
    public uint LastRow { get => State[LastRowIndex]; set => State[LastRowIndex] = value; }

    /// <summary>0056b804의 현재 위상이며 하위 4비트로 진행한다.</summary>
    public uint Phase { get => State[PhaseIndex]; set => State[PhaseIndex] = value; }

    /// <summary>0056b7fc의 전체 표시 요청이다.</summary>
    public uint AllDirty { get => State[AllDirtyIndex]; set => State[AllDirtyIndex] = value; }

    /// <summary>0056b800의 부분 표시 요청이다.</summary>
    public uint PartialDirty { get => State[PartialDirtyIndex]; set => State[PartialDirtyIndex] = value; }

    /// <summary>00540a88은 같은 GameRandom(256)+5를 저장한다.</summary>
    public uint Spacer { get => Options[SpacerOption]; set => Options[SpacerOption] = value; }

    /// <summary>00540a34의 현재 low BYTE로 fallback 할당을 채운다.</summary>
    public uint LowColor => Options[LowColorOption];

    /// <summary>00540a80 cloudSpeed이며 각 warm-up 반복의 재귀 복귀 뒤 캡처한다.</summary>
    public uint Speed => Options[SpeedOption];

    /// <summary>00540a84의 signed 값으로 10000을 나눈다. 0 나눗셈을 성공으로 바꾸지 않는다.</summary>
    public uint Fury => Options[FuryOption];

    /// <summary>00540a90은 첫 관문과 각 seed 뒤 signed 값을 다시 읽는다.</summary>
    public uint SeedCount => Options[SeedCountOption];

    /// <summary>0056a000의 실제 512 DWORD 행 표다. 물리 길이를 넘는 쓰기는 예외다.</summary>
    public Span<uint> DirtyRows => State.AsSpan(DirtyRowsIndex, 512);

    /// <summary>0056a800의 실제 주소 표다. 설정 의미상 상한을 새로 만들지 않는다.</summary>
    public Span<uint> Seeds => State.AsSpan(SeedsIndex, 1000);
}

/// <summary>
/// 파일 부모가 쓰는 현재 264바이트 stack 입력이다. 무정의 초기값을 임의 0으로 만들지 않고 호출자가 명시한다.
/// 원문 local+0 height(DWORD), local+4 width(DWORD), local+8 이름 복사 버퍼 256바이트(NUL 포함)다.
/// </summary>
/// <remarks>0.381에서 추가했다.</remarks>
public sealed class KencloudFileScratch
{
    /// <summary>원문 local+0의 현재 DWORD다. 디코더는 이 실제 참조를 쓴다.</summary>
    public int Height;

    /// <summary>원문 local+4의 현재 DWORD며 반환 뒤 width부터 비교한다.</summary>
    public int Width;

    /// <summary>원문 local+8의 실제 NUL 포함 이름 복사 버퍼다.</summary>
    public byte[] Path = new byte[256];
}

/// <summary>004410f0("CloudFileSpec",0,0,0)이 돌려주는 NUL 물리를 빌린다.</summary>
/// <param name="key">PathSpec 키(항상 "CloudFileSpec")다.</param>
/// <param name="first">첫째 인자(항상 0)다.</param>
/// <param name="second">둘째 인자(항상 0)다.</param>
/// <param name="third">셋째 인자(항상 0)다.</param>
public delegate ReadOnlyMemory<byte> KencloudPathSpec(string key, uint first, uint second, uint third);

/// <summary>004dae60(path,&amp;width,&amp;height)의 현재 필드와 반환 주소다.</summary>
/// <param name="path">현재 NUL 포함 이름 복사 버퍼다.</param>
/// <param name="width">현재 폭 출력이다.</param>
/// <param name="height">현재 높이 출력이다.</param>
public delegate uint KencloudDecode(byte[] path, ref int width, ref int height);

/// <summary>주소부터 남은 현재 물리를 빌린다. 순수 주소 조회이며 미등록 주소는 <see cref="ArgumentOutOfRangeException"/>이다.</summary>
/// <param name="address">조회할 주소 토큰이다.</param>
public delegate Span<byte> KencloudMemory(uint address);

/// <summary>
/// 파일 시스템·원본 GIF 디코더는 기존 구현에 합성하는 필수 경계다. 연결하지 않은 장치를 도달하면 <see cref="InvalidOperationException"/>이다.
/// </summary>
/// <remarks>0.381에서 추가했다.</remarks>
public sealed class KencloudFileHooks
{
    /// <summary>004410f0("CloudFileSpec",0,0,0)의 NUL 물리다.</summary>
    public KencloudPathSpec? PathSpec { get; set; }

    /// <summary>0041ab10은 같은 복사 버퍼를 받으며 비영일 때만 decode한다.</summary>
    public Func<byte[], uint>? Exists { get; set; }

    /// <summary>004dae60(path,&amp;width,&amp;height)의 현재 필드와 반환 주소다.</summary>
    public KencloudDecode? Decode { get; set; }

    /// <summary>치수가 512×512가 아니면 반환 주소 0도 004e4354에 그대로 전달한다.</summary>
    public Action<uint>? Release { get; set; }
}

/// <summary>
/// 메모리 소유권·파일 장치를 명시하며 실제 fill·memmove·난수 본체는 모듈이 실행한다.
/// </summary>
/// <remarks>0.381에서 추가했다.</remarks>
public sealed class KencloudHooks
{
    /// <summary>LoadKencloudFile의 실제 필수 합성 경계다. 도달하지 않으면 미연결을 허용한다.</summary>
    public Func<uint>? LoadFile { get; set; }

    /// <summary>operator_new(0x40000)의 현재 반환 주소다. 0을 가짜 성공으로 바꾸지 않는다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>주소부터 남은 현재 물리를 반환한다.</summary>
    public KencloudMemory? Memory { get; set; }

    /// <summary>실제 memmove 뒤의 선택 관측 경계(대상·원본·길이)다. 없어도 이동 본체는 실행한다.</summary>
    public Action<uint, uint, uint>? AfterMove { get; set; }

    /// <summary>같은 GameRandom.Next 뒤 limit·직전 상태·반환값 관측이다. 새 난수나 상태 동기화가 아니다.</summary>
    public Action<uint, uint, uint>? AfterRandom { get; set; }
}

/// <summary>
/// 원본 Kencloud의 파일 부모(00470bb0)·행 이동(00470b20)·생성(00471040)을 복원한다.
/// cpppj <c>o/RawKencloud.cpp</c>의 직역이며 원문 몸체의 앞선 부분 효과와 정수 폭(32비트 감김)·signed 비교를 그대로 보존한다.
/// </summary>
/// <remarks>0.381에서 추가했다(10.78 구름 L1 fixture <c>kencloud-v2-x86.tsv</c> 246행 재생).</remarks>
public static class RawKencloud
{
    /// <summary>원문의 512×512 할당 크기다.</summary>
    private const uint Bytes512Squared = 0x40000;

    /// <summary>구름 한 행의 바이트 수(512)다.</summary>
    private const uint RowBytes = 512;

    /// <summary>행 이동의 한 행당 DWORD 오프셋 증가량(989)이다.</summary>
    private const uint Stride = 0x3dd;

    /// <summary>16행 간격 묶음의 오프셋 증가량(989×16)이다.</summary>
    private const uint StrideBatch = 0x3dd0;

    /// <summary>
    /// 외부 소유자가 빌리는 주소의 물리 길이를 확인하고 원문의 해당 접근 시점에만 실패한다.
    /// </summary>
    /// <param name="hooks">메모리 조회 장치를 가진 묶음이다.</param>
    /// <param name="address">시작 주소 토큰이다.</param>
    /// <param name="length">필요한 길이다.</param>
    /// <param name="cause">실패 원인 이름이다.</param>
    /// <returns>주소부터 <paramref name="length"/>바이트의 현재 물리 뷰다.</returns>
    private static Span<byte> Bytes(KencloudHooks hooks, uint address, uint length, string cause)
    {
        Span<byte> bytes;
        try
        {
            bytes = Need(hooks.Memory)(address);
        }
        catch (ArgumentOutOfRangeException)
        {
            throw new KencloudPhysicalException(cause);
        }
        if ((uint)bytes.Length < length) throw new KencloudPhysicalException(cause);
        return bytes[..(int)length];
    }

    /// <summary>선택 경로에 도달한 필수 장치가 없으면 원문 미연결 오류로 거부한다.</summary>
    /// <typeparam name="T">장치 형식이다.</typeparam>
    /// <param name="device">필요한 장치다.</param>
    private static T Need<T>(T? device) where T : class =>
        device ?? throw new InvalidOperationException("Kencloud 선택된 실제 장치 미연결");

    /// <summary>fury의 signed 해석과 매회 IDIV를 보존한다. 음수는 그대로 음수 몫이며 0만 명시 예외다.</summary>
    /// <param name="globals">현재 전역이다.</param>
    private static int WarmCount(KencloudGlobals globals)
    {
        int divisor = unchecked((int)globals.Fury);
        if (divisor == 0) throw new DivideByZeroException("Kencloud divide-zero");
        return 10000 / divisor;
    }

    /// <summary>기존 실제 난수를 정확히 한 번 소비하고 선택 관측 뒤 반환값은 그대로 유지한다.</summary>
    /// <param name="random">같은 GameRandom 객체다.</param>
    /// <param name="limit">상한이다.</param>
    /// <param name="hooks">관측 장치를 가진 묶음이다.</param>
    private static uint Next(GameRandom random, uint limit, KencloudHooks hooks)
    {
        uint before = random.State();
        uint result = random.Next(limit);
        hooks.AfterRandom?.Invoke(limit, before, result);
        return result;
    }

    /// <summary>
    /// 00470bb0: 현재 256바이트 이름 복사 → exists → decode → 현재 치수 검사/free를 실행한다. 모든 scratch BYTE는 호출자의 입력이다.
    /// </summary>
    /// <param name="scratch">현재 264바이트 stack 입력이다. 중단해도 부분 쓰기는 남는다.</param>
    /// <param name="hooks">경로·존재·해독·반납 장치다.</param>
    /// <returns>512×512 이미지의 주소 토큰이며 실패면 0이다.</returns>
    public static uint LoadKencloudFile(KencloudFileScratch scratch, KencloudFileHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(scratch);
        ArgumentNullException.ThrowIfNull(hooks);
        ReadOnlySpan<byte> source = Need(hooks.PathSpec)("CloudFileSpec", 0, 0, 0).Span;
        // 전체 길이 선계산/문자열 사본 없이 원문처럼 NUL 포함한 BYTE씩 복사한다.
        for (int index = 0; ; index++)
        {
            if (index >= source.Length) throw new KencloudPhysicalException("file-path-read");
            byte value = source[index];
            if (index >= scratch.Path.Length) throw new KencloudPhysicalException("file-path-write");
            scratch.Path[index] = value;
            if (value == 0) break;
        }
        if (Need(hooks.Exists)(scratch.Path) != 0)
        {
            uint image = Need(hooks.Decode)(scratch.Path, ref scratch.Width, ref scratch.Height);
            if (scratch.Width == 512 && scratch.Height == 512) return image;
            Need(hooks.Release)(image);
        }
        return 0;
    }

    /// <summary>
    /// 00470b20: 구름 한 단계 행 이동. cache 첫 BYTE를 읽은 뒤 phase·end[-1]를 쓰고, 현재 위상 행 번호에서 16행 간격으로
    /// 989바이트 구간을 1바이트씩 왼쪽으로 민다(겹침은 실제 memmove). 난수를 소비하지 않는다.
    /// </summary>
    /// <param name="globals">빌린 구름 전역이다.</param>
    /// <param name="hooks">주소 물리 조회와 이동 뒤 관측 장치다.</param>
    public static void StepKencloud(KencloudGlobals globals, KencloudHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        uint previousPhase = globals.Phase; // 원문 첫 load이며 물리 read 실패면 phase는 아직 변하지 않는다.
        byte first = Bytes(hooks, globals.Cache, 1, "step-read")[0]; // cache 첫 BYTE가 phase 쓰기보다 앞이다.
        uint end = globals.End; // end도 phase 쓰기 앞에 현재 값을 캡처한다.
        uint phase = unchecked(previousPhase + 1U) & 15U; // 원문의 32비트 inc/and다.
        globals.Phase = phase;
        Bytes(hooks, unchecked(end - 1U), 1, "step-tail")[0] = first;
        int start = unchecked((int)globals.PhaseRows[(int)phase]); // 위상 쓰기 뒤 현재 행표를 읽는다.
        if (start >= 512) return;
        uint row = unchecked((uint)start); // 원문의 고정 dirty 포인터를 행 offset으로 표현한다.
        uint offset = unchecked(row * Stride); // imul의 low32다.
        // 각 CRT 호출 복귀 뒤 다음 행의 현재 cache/end를 다시 읽는다.
        do
        {
            uint destination = unchecked(globals.Cache + offset); // 현재 base를 매 행 조회한다.
            uint currentEnd = globals.End; // 현재 end와 unsigned 주소 비교한다.
            if (destination >= currentEnd) destination = unchecked(currentEnd - 1U);
            uint last = unchecked(destination + Stride); // DWORD 주소 덧셈은 원문처럼 감긴다.
            if (last >= currentEnd) last = unchecked(currentEnd - 1U);
            uint length = unchecked(last - destination); // CRT에 전달한 원문 DWORD 길이다.
            Span<byte> target = Bytes(hooks, destination, length, "move-physical"); // 길이 0도 물리 one-past를 허용하는 소유자에 조회한다.
            Span<byte> source = Bytes(hooks, unchecked(destination + 1U), length, "move-physical"); // 같은 버퍼의 겹침은 실제 memmove로 보존한다.
            if (length != 0) source.CopyTo(target); // Span.CopyTo는 겹친 구간에서 원본 값을 보존한다(memmove).
            hooks.AfterMove?.Invoke(destination, unchecked(destination + 1U), length);
            if (row >= (uint)globals.DirtyRows.Length) throw new KencloudPhysicalException("dirty-physical");
            globals.DirtyRows[(int)row] = 1;
            row = unchecked(row + 16U);
            offset = unchecked(offset + StrideBatch);
        } while (unchecked((int)row) < 512);
    }

    /// <summary>
    /// 00471040: 현재 캐시·파일·할당·fill·seed·재귀 warm-up을 실행한다. 받은 실제 GameRandom 참조만 소비한다.
    /// 캐시가 이미 있으면 dirty 플래그만 갱신하고 끝난다.
    /// </summary>
    /// <param name="globals">빌린 구름 전역이다.</param>
    /// <param name="random">같은 실제 게임 난수다.</param>
    /// <param name="hooks">파일·할당·물리·관측 장치다.</param>
    public static void BuildKencloud(KencloudGlobals globals, GameRandom random, KencloudHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Cache != 0) { globals.PartialDirty = 0; globals.AllDirty = 1; return; }
        uint image = Need(hooks.LoadFile)(); // 파일 부모의 반환 주소를 캡처하고 같은 전역에 쓴다.
        globals.Cache = image;
        bool generated = false; // 파일 실패 fallback에서만 warm-up을 실행하는 원문 EBX다.
        if (image == 0)
        {
            uint allocated = Need(hooks.Allocate)(Bytes512Squared); // 할당 callback 뒤 현재 lowColor를 읽는다.
            byte color = unchecked((byte)globals.LowColor); // 원문 color 읽기는 cache 대입보다 앞이다.
            globals.Cache = allocated;
            generated = true;
            Span<byte> bytes = Bytes(hooks, allocated, Bytes512Squared, "fill-physical"); // 실패 전 cache 대입은 이미 발생했다.
            bytes.Fill(color);
        }
        uint baseAddress = globals.Cache; // fallback rep 종료 뒤의 현재 base다.
        uint endAddress = unchecked(baseAddress + Bytes512Squared); // 같은 base에서 상·하부 행 뷰를 구성한다.
        globals.NextRow = unchecked(baseAddress + RowBytes);
        int initialCount = unchecked((int)globals.SeedCount); // 첫 signed 관문에 사용한다.
        globals.End = endAddress;
        globals.LastRow = unchecked(endAddress - RowBytes);
        if (initialCount > 0)
        {
            uint index = 0; // 원문 EDI는 0부터 시작하며 각 callback 뒤 현재 count와 비교한다.
            // 첫 양수 관문 뒤에는 현재 count가 줄어도 현재 한 seed의 두 난수와 쓰기를 먼저 완료한다.
            do
            {
                uint y = unchecked(Next(random, 510, hooks) + 1U); // 첫 호출의 반환값을 다음 호출 동안 캡처한다.
                uint x = Next(random, 512, hooks); // 같은 GameRandom 상태를 두 번째 소비한다.
                if (index >= (uint)globals.Seeds.Length) throw new KencloudPhysicalException("seed-physical");
                globals.Seeds[(int)index] = unchecked(globals.Cache + y * RowBytes + x);
                int count = unchecked((int)globals.SeedCount); // 원문은 저장 뒤 다시 읽고 EDI를 증가시킨다.
                index++;
                if (unchecked((int)index) >= count) break;
            } while (true);
        }
        globals.Spacer = unchecked(Next(random, 256, hooks) + 5U);
        if (generated && WarmCount(globals) > 0)
        {
            int iteration = 0; // 현재 signed 몫과 비교할 원문 EDI다.
            // fury는 각 batch 복귀 뒤 재조회하며 재귀 생성 뒤 현재 speed를 캡처한다.
            do
            {
                if (globals.Cache == 0) BuildKencloud(globals, random, hooks);
                uint remaining = globals.Speed; // 재귀 복귀 뒤의 현재 DWORD 한 번만 캡처한다.
                // step callback이 speed를 바꿔도 현재 batch의 남은 횟수는 변하지 않는다.
                while (remaining != 0) { StepKencloud(globals, hooks); remaining--; }
                int limit = WarmCount(globals); // 현재 fury 0이면 reset에 앞서 중단한다.
                iteration++;
                if (iteration >= limit) break;
            } while (true);
        }
        globals.PartialDirty = 0;
        globals.AllDirty = 1;
    }
}
